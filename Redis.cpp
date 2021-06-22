#include "Redis.h"
#include "RedisInternal.h"

RedisReturnValue Redis::authenticate(const char *password)
{
    if (conn.connected())
    {
        int passwordLength = strlen(password);
        if (passwordLength > 0)
        {
            auto cmdRet = RedisCommand("AUTH", ArgList{password}).issue(conn);
            return cmdRet->type() == RedisObject::Type::SimpleString && (String)*cmdRet == "OK"
                       ? RedisSuccess
                       : RedisAuthFailure;
        }

        return RedisSuccess;
    }

    return RedisNotConnectedFailure;
}

#define TRCMD(t, c, ...) return RedisCommand(c, ArgList{__VA_ARGS__}).issue_typed<t>(conn)

#define TRCMD_EXPECTOK(c, ...) return (bool)(((String)*RedisCommand(c, ArgList{__VA_ARGS__}).issue(conn)).indexOf("OK") != -1)

bool Redis::set(const char *key, const char *value)
{
    TRCMD_EXPECTOK("SET", key, value);
}

String Redis::get(const char *key)
{
    TRCMD(String, "GET", key);
}

bool Redis::del(const char *key)
{
    TRCMD(bool, "DEL", key);
}

int Redis::append(const char *key, const char *value)
{
    TRCMD(int, "APPEND", key, value);
}

int Redis::publish(const char *channel, const char *message)
{
    TRCMD(int, "PUBLISH", channel, message);
}

bool Redis::exists(const char *key)
{
    TRCMD(bool, "EXISTS", key);
}

bool Redis::_expire_(const char *key, int arg, const char *cmd_var)
{
    TRCMD(bool, cmd_var, key, String(arg));
}

bool Redis::persist(const char *key)
{
    TRCMD(bool, "PERSIST", key);
}

int Redis::_ttl_(const char *key, const char *cmd_var)
{
    TRCMD(int, cmd_var, key);
}

bool Redis::_hset_(const char *key, const char *field, const char *value, const char *cmd_var)
{
    TRCMD(int, cmd_var, key, field, value);
}

String Redis::hget(const char *key, const char *field)
{
    TRCMD(String, "HGET", key, field);
}

bool Redis::hdel(const char *key, const char *field)
{
    TRCMD(bool, "HDEL", key, field);
}

int Redis::hlen(const char *key)
{
    TRCMD(int, "HLEN", key);
}

int Redis::hstrlen(const char *key, const char *field)
{
    TRCMD(int, "HSTRLEN", key, field);
}

bool Redis::hexists(const char *key, const char *field)
{
    TRCMD(bool, "HEXISTS", key, field);
}

std::vector<String> Redis::lrange(const char *key, int start, int stop)
{
    auto rv = RedisCommand("LRANGE", ArgList{key, String(start), String(stop)}).issue(conn);
    return rv->type() == RedisObject::Type::Array
               ? (std::vector<String>)*((RedisArray *)rv.get())
               : std::vector<String>();
}

String Redis::lindex(const char *key, int index)
{
    TRCMD(String, "LINDEX", key, String(index));
}

int Redis::llen(const char *key)
{
    TRCMD(int, "LLEN", key);
}

String Redis::lpop(const char *key)
{
    TRCMD(String, "LPOP", key);
}

int Redis::lpos(const char *key, const char *element)
{
    TRCMD(int, "LPOS", key, element);
}

int Redis::lpush(const char *key, const char *value, bool exclusive)
{
    TRCMD(int, (exclusive ? "LPUSHX" : "LPUSH"), key, value);
}

int Redis::lrem(const char *key, int count, const char *element)
{
    TRCMD(int, "LREM", key, String(count), element);
}

bool Redis::lset(const char *key, int index, const char *element)
{
    TRCMD_EXPECTOK("LSET", key, String(index), element);
}

bool Redis::ltrim(const char *key, int start, int stop)
{
    TRCMD_EXPECTOK("LTRIM", key, String(start), String(stop));
}

String Redis::info(const char *section)
{
    TRCMD(String, "INFO", (section ? section : ""));
}

String Redis::rpop(const char *key)
{
    TRCMD(String, "RPOP", key);
}

int Redis::rpush(const char *key, const char *value, bool exclusive)
{
    TRCMD(int, (exclusive ? "RPUSHX" : "RPUSH"), key, value);
}

bool Redis::_subscribe(SubscribeSpec spec)
{
    if (!subscriberMode)
    {
        subSpec.push_back(spec);
        return true;
    }

    const char *cmdName = spec.pattern ? "PSUBSCRIBE" : "SUBSCRIBE";
    auto rv = RedisCommand(cmdName, ArgList{spec.spec}).issue(conn);
    return rv->type() == RedisObject::Type::Array;
}

bool Redis::unsubscribe(const char *channelOrPattern)
{
    auto rv = RedisCommand("UNSUBSCRIBE", ArgList{channelOrPattern}).issue(conn);

    if (rv->type() == RedisObject::Type::Array)
    {
        auto vec = (std::vector<String>)*((RedisArray *)rv.get());
        return vec.size() == 3 && vec[1] == String(channelOrPattern);
    }

    return false;
}

RedisSubscribeResult Redis::startSubscribing(RedisMsgCallback messageCallback, RedisMsgErrorCallback errCallback)
{
    if (!messageCallback)
    {
        return RedisSubscribeBadCallback;
    }

    bool success = true;
    subscriberMode = true;
    if (subSpec.size())
    {
        for (auto spec : subSpec)
        {
            success = _subscribe(spec) && success;
        }
    }

    if (!success)
    {
        return RedisSubscribeSetupFailure;
    }

    auto emitErr = [=](RedisMessageError errCode) -> void {
        if (errCallback)
        {
            errCallback(this, errCode);
        }
    };

    subLoopRun = true;
    while (subLoopRun)
    {
        auto msg = RedisObject::parseType(conn);

        if (msg->type() == RedisObject::Type::InternalError)
        {
            auto errPtr = (RedisInternalError *)msg.get();

            if (errPtr->code() == RedisInternalError::Disconnected)
            {
                return RedisSubscribeServerDisconnected;
            }

            return RedisSubscribeOtherError;
        }

        if (msg->type() != RedisObject::Type::Array)
        {
            emitErr(RedisMessageBadResponseType);
            continue;
        }

        auto msgVec = (std::vector<String>)*((RedisArray *)msg.get());

        if (msgVec.size() < 3)
        {
            emitErr(RedisMessageTruncatedResponse);
            continue;
        }

        if (msgVec[0] != "message" && msgVec[0] != "pmessage")
        {
            emitErr(RedisMessageUnknownType);
            continue;
        }

        // pmessage payloads have an extra paramter at index 1 that specifies the matched pattern; we ignore it here
        auto pMsgAdd = msgVec[0] == "pmessage" ? 1 : 0;
        messageCallback(this, msgVec[1 + pMsgAdd], msgVec[2 + pMsgAdd]);
    }

    return RedisSubscribeSuccess;
}