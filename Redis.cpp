#include "Redis.h"
#include "RedisInternal.h"

#pragma mark Redis class implemenation

RedisReturnValue Redis::connect(const char* password)
{
    if(!conn.connected())
    {
        int passwordLength = strlen(password);
        if (passwordLength > 0)
        {
            auto cmdRet = RedisCommand("AUTH", ArgList{password}).issue(conn);
            return cmdRet->type() == RedisObject::Type::SimpleString && (String)*cmdRet == "OK" 
                ? RedisSuccess : RedisAuthFailure;
        }

        return RedisSuccess;
    }

    return RedisNotConnectedFailure;
}

bool Redis::set(const char* key, const char* value)
{
    return ((String)*RedisCommand("SET", ArgList{key, value}).issue(conn)).indexOf("OK") != -1;
}

String Redis::get(const char* key) 
{
    return (String)*RedisCommand("GET", ArgList{key}).issue(conn);
}

int Redis::publish(const char* channel, const char* message)
{
    auto reply = RedisCommand("PUBLISH", ArgList{channel, message}).issue(conn);

    switch (reply->type()) {
        case RedisObject::Type::Error:
            return -1;
        case RedisObject::Type::Integer:
            return (int)*((RedisInteger*)reply.get());
    }
}

bool Redis::_expire_(const char* key, int arg, const char* cmd_var)
{
    return (bool)*(RedisInteger*)RedisCommand(cmd_var, ArgList{key, String(arg)}).issue(conn).get();
}

bool Redis::persist(const char* key)
{
    return (bool)*(RedisInteger*)RedisCommand("PERSIST", ArgList{key}).issue(conn).get();
}

int Redis::_ttl_(const char* key, const char* cmd_var)
{
    return (int)*(RedisInteger*)RedisCommand(cmd_var, ArgList{key}).issue(conn).get();
}

void Redis::close(void)
{
    conn.stop();
}
