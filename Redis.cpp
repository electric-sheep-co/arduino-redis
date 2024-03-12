#include "Redis.h"
#include "RedisInternal.h"

RedisReturnValue Redis::authenticate(const char *client, const char *password)
{
  if (conn.connected())
  {
    int clientLength = strlen(client);
    int passwordLength = strlen(password);
    if (passwordLength > 0 && clientLength > 0)
    {
      auto cmdRet = RedisCommand("AUTH", ArgList{ client, password }).issue(conn);
      return cmdRet->type() == RedisObject::Type::SimpleString && (String)*cmdRet == "OK"
                 ? RedisSuccess
                 : RedisAuthFailure;
    }

    return RedisSuccess;
  }

  return RedisNotConnectedFailure;
}

RedisReturnValue Redis::authenticate(const char *password)
{
  return authenticate("default", password);
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

  if (rv->type() == RedisObject::Type::InternalError)
  {
    std::vector<String> r = std::vector<String>();
    String error_message = (String)(((RedisInternalError *)rv.get())->RESP());
    r.push_back(error_message);
    return r;
  }
  else
  {
    return rv->type() == RedisObject::Type::Array
               ? (std::vector<String>)*((RedisArray *)rv.get())
               : std::vector<String>();
  }
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

bool Redis::tsadd(const char *key, long timestamp, const int value)
{
  if (timestamp < 0)
  {
    TRCMD_EXPECTOK("TS.ADD", key, "*", String(value));
  }
  else
  {
    TRCMD_EXPECTOK("TS.ADD", key, String(timestamp) + "000", String(value));
  }
}

int Redis::xack(const char *key, const char *group, const char *id)
{
  TRCMD(int, "XACK", key, group, id);
}

String Redis::xadd(const char *key, const char *id, const char *field,
                   const char *value)
{
  TRCMD(String, "XADD", key, id, field, value);
}

std::vector<String> Redis::xautoclaim(const char *key, const char *group,
                                      const char *consumer, unsigned int min_idle_time, const char *start,
                                      unsigned int count, bool justid)
{
  ArgList argList = ArgList{key, group, consumer, String(min_idle_time), start};

  if (count > 0)
  {
    argList.push_back("COUNT");
    argList.push_back(String(count));
  }

  if (justid == true)
  {
    argList.push_back("JUSTID");
  }

  auto rv = RedisCommand("XAUTOCLAIM", argList).issue(conn);

  if (rv->type() == RedisObject::Type::InternalError)
  {
    std::vector<String> r = std::vector<String>();
    String error_message = (String)(((RedisInternalError *)rv.get())->RESP());
    r.push_back(error_message);
    return r;
  }
  else
  {
    return rv->type() == RedisObject::Type::Array
               ? (std::vector<String>)*((RedisArray *)rv.get())
               : std::vector<String>();
  }
}

std::vector<String> Redis::xclaim(const char *key, const char *group,
                                  const char *consumer, unsigned int min_idle_time, const char *id,
                                  unsigned int idle_ms, unsigned int time_ms, unsigned int retrycount,
                                  bool force, bool justid, const char *lastid)
{
  ArgList argList = ArgList{key, group, consumer, String(min_idle_time), id};

  if (idle_ms > 0)
  {
    argList.push_back("IDLE");
    argList.push_back(String(idle_ms));
  }

  if (time_ms > 0)
  {
    argList.push_back("TIME");
    argList.push_back(String(time_ms));
  }

  if (retrycount > 0)
  {
    argList.push_back("RETRYCOUNT");
    argList.push_back(String(retrycount));
  }

  if (force == true)
  {
    argList.push_back("FORCE");
  }

  if (justid == true)
  {
    argList.push_back("JUSTID");
  }

  if (lastid != NULL && strlen(lastid) > 0)
  {
    argList.push_back("LASTID");
    argList.push_back(lastid);
  }

  auto rv = RedisCommand("XCLAIM", argList).issue(conn);

  if (rv->type() == RedisObject::Type::InternalError)
  {
    std::vector<String> r = std::vector<String>();
    String error_message = (String)(((RedisInternalError *)rv.get())->RESP());
    r.push_back(error_message);
    return r;
  }
  else
  {
    return rv->type() == RedisObject::Type::Array
               ? (std::vector<String>)*((RedisArray *)rv.get())
               : std::vector<String>();
  }
}

int Redis::xdel(const char *key, const char *id)
{
  TRCMD(int, "XDEL", key, id);
}

bool Redis::xgroup_create(const char *key, const char *group, const char *id,
                          bool mkstream)
{
  if (mkstream)
  {
    TRCMD_EXPECTOK("XGROUP", "CREATE", key, group, id, "MKSTREAM");
  }
  else
  {
    TRCMD_EXPECTOK("XGROUP", "CREATE", key, group, id);
  }
}

int Redis::xgroup_createconsumer(const char *key, const char *group,
                                 const char *consumer)
{
  TRCMD(int, "XGROUP", "CREATECONSUMER", key, group, consumer);
}

int Redis::xgroup_delconsumer(const char *key, const char *group,
                              const char *consumer)
{
  TRCMD(int, "XGROUP", "DELCONSUMER", key, group, consumer);
}

int Redis::xgroup_destroy(const char *key, const char *group)
{
  TRCMD(int, "XGROUP", "DESTROY", key, group);
}

bool Redis::xgroup_setid(const char *key, const char *group, const char *id)
{
  TRCMD_EXPECTOK("XGROUP", "SETID", key, group, id);
}

std::vector<String> Redis::xinfo_consumers(const char *key, const char *group)
{
  auto rv = RedisCommand("XINFO", ArgList{"CONSUMERS", key, group}).issue(conn);

  if (rv->type() == RedisObject::Type::InternalError)
  {
    std::vector<String> r = std::vector<String>();
    String error_message = (String)(((RedisInternalError *)rv.get())->RESP());
    r.push_back(error_message);
    return r;
  }
  else
  {
    return rv->type() == RedisObject::Type::Array
               ? (std::vector<String>)*((RedisArray *)rv.get())
               : std::vector<String>();
  }
}

std::vector<String> Redis::xinfo_groups(const char *key)
{
  auto rv = RedisCommand("XINFO", ArgList{"GROUPS", key}).issue(conn);

  if (rv->type() == RedisObject::Type::InternalError)
  {
    std::vector<String> r = std::vector<String>();
    String error_message = (String)(((RedisInternalError *)rv.get())->RESP());
    r.push_back(error_message);
    return r;
  }
  else
  {
    return rv->type() == RedisObject::Type::Array
               ? (std::vector<String>)*((RedisArray *)rv.get())
               : std::vector<String>();
  }
}

std::vector<String> Redis::xinfo_stream(const char *key, bool full,
                                        unsigned int count)
{
  ArgList argList = ArgList{"STREAM", key};

  if (full == true)
  {
    argList.push_back("FULL");

    if (count > 0)
    {
      argList.push_back("COUNT");
      argList.push_back(String(count));
    }
  }

  auto rv = RedisCommand("XINFO", argList).issue(conn);

  if (rv->type() == RedisObject::Type::InternalError)
  {
    std::vector<String> r = std::vector<String>();
    String error_message = (String)(((RedisInternalError *)rv.get())->RESP());
    r.push_back(error_message);
    return r;
  }
  else
  {
    return rv->type() == RedisObject::Type::Array
               ? (std::vector<String>)*((RedisArray *)rv.get())
               : std::vector<String>();
  }
}

int Redis::xlen(const char *key)
{
  TRCMD(int, "XLEN", key);
}

std::vector<String> Redis::xpending(const char *key, const char *group,
                                    unsigned int min_idle_time, const char *start, const char *end,
                                    unsigned int count, const char *consumer)
{
  ArgList argList = ArgList{key, group};

  if (min_idle_time > 0)
  {
    argList.push_back("IDLE");
    argList.push_back(String(min_idle_time));
  }

  if (start != NULL && strlen(start) > 0 && end != NULL && strlen(end) > 0)
  {
    argList.push_back(start);
    argList.push_back(end);
    argList.push_back(String(count));
  }

  if (consumer != NULL && strlen(consumer) > 0)
  {
    argList.push_back(consumer);
  }

  auto rv = RedisCommand("XPENDING", argList).issue(conn);

  if (rv->type() == RedisObject::Type::InternalError)
  {
    std::vector<String> r = std::vector<String>();
    String error_message = (String)(((RedisInternalError *)rv.get())->RESP());
    r.push_back(error_message);
    return r;
  }
  else
  {
    return rv->type() == RedisObject::Type::Array
               ? (std::vector<String>)*((RedisArray *)rv.get())
               : std::vector<String>();
  }
}

std::vector<String> Redis::xrange(const char *key, const char *start,
                                  const char *end, unsigned int count)
{
  ArgList argList = ArgList{key, start, end};

  if (count > 0)
  {
    argList.push_back("COUNT");
    argList.push_back(String(count));
  }

  auto rv = RedisCommand("XRANGE", argList).issue(conn);

  if (rv->type() == RedisObject::Type::InternalError)
  {
    std::vector<String> r = std::vector<String>();
    String error_message = (String)(((RedisInternalError *)rv.get())->RESP());
    r.push_back(error_message);
    return r;
  }
  else
  {
    return rv->type() == RedisObject::Type::Array
               ? (std::vector<String>)*((RedisArray *)rv.get())
               : std::vector<String>();
  }
}

std::vector<String> Redis::xread(unsigned int count, unsigned int block,
                                 const char *key, const char *id)
{
  ArgList argList = std::vector<String>();

  if (count > 0)
  {
    argList.push_back("COUNT");
    argList.push_back(String(count));
  }

  if (block > 0)
  {
    argList.push_back("BLOCK");
    argList.push_back(String(block));
  }

  argList.push_back("STREAMS");
  argList.push_back(key);
  argList.push_back(id);

  auto rv = RedisCommand("XREAD", argList).issue(conn);

  if (rv->type() == RedisObject::Type::InternalError)
  {
    std::vector<String> r = std::vector<String>();
    String error_message = (String)(((RedisInternalError *)rv.get())->RESP());
    r.push_back(error_message);
    return r;
  }
  else
  {
    return rv->type() == RedisObject::Type::Array
               ? (std::vector<String>)*((RedisArray *)rv.get())
               : std::vector<String>();
  }
}

std::vector<String> Redis::xreadgroup(const char *group, const char *consumer,
                                      unsigned int count, unsigned int block_ms, bool noack, const char *key,
                                      const char *id)
{
  ArgList argList = ArgList{"GROUP", group, consumer};

  if (count > 0)
  {
    argList.push_back("COUNT");
    argList.push_back(String(count));
  }

  if (block_ms > 0)
  {
    argList.push_back("BLOCK");
    argList.push_back(String(block_ms));
  }

  if (noack == true)
  {
    argList.push_back("NOACK");
  }

  if (key != NULL && strlen(key) > 0)
  {
    argList.push_back("STREAMS");
    argList.push_back(key);
  }

  argList.push_back(id);

  auto rv = RedisCommand("XREADGROUP", argList).issue(conn);

  if (rv->type() == RedisObject::Type::InternalError)
  {
    std::vector<String> r = std::vector<String>();
    String error_message = (String)(((RedisInternalError *)rv.get())->RESP());
    r.push_back(error_message);
    return r;
  }
  else
  {
    return rv->type() == RedisObject::Type::Array
               ? (std::vector<String>)*((RedisArray *)rv.get())
               : std::vector<String>();
  }
}

std::vector<String> Redis::xrevrange(const char *key, const char *end,
                                     const char *start, unsigned int count)
{
  ArgList argList = ArgList{key, end, start};

  if (count > 0)
  {
    argList.push_back("COUNT");
    argList.push_back(String(count));
  }

  auto rv = RedisCommand("XREVRANGE", argList).issue(conn);

  if (rv->type() == RedisObject::Type::InternalError)
  {
    std::vector<String> r = std::vector<String>();
    String error_message = (String)(((RedisInternalError *)rv.get())->RESP());
    r.push_back(error_message);
    return r;
  }
  else
  {
    return rv->type() == RedisObject::Type::Array
               ? (std::vector<String>)*((RedisArray *)rv.get())
               : std::vector<String>();
  }
}

int Redis::xtrim(const char *key, const char *strategy, XtrimCompareType compare,
                 const int threshold, const int count)
{
  if (compare == XtrimCompareAtLeast)
  {
    if (count > 0)
    {
      TRCMD(int, "XTRIM", key, strategy, String(char(compare)),
            String(threshold), "LIMIT", String(count));
    }
    else
    {
      TRCMD(int, "XTRIM", key, strategy, String(threshold));
    }
  }
  else
  {
    TRCMD(int, "XTRIM", key, strategy, String(char(compare)),
          String(threshold));
  }
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

RedisSubscribeResult Redis::startSubscribingNonBlocking(RedisMsgCallback messageCallback, LoopCallback loopCallback, RedisMsgErrorCallback errCallback)
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

  auto emitErr = [=](RedisMessageError errCode) -> void
  {
    if (errCallback)
    {
      errCallback(this, errCode);
    }
  };

  subLoopRun = true;
  while (subLoopRun)
  {
    loopCallback();
    auto msg = RedisObject::parseTypeNonBlocking(conn);
    if (msg == nullptr)
    {
      continue;
    }

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

RedisSubscribeResult Redis::startSubscribing(RedisMsgCallback messageCallback, RedisMsgErrorCallback errCallback)
{
  return startSubscribingNonBlocking(
      messageCallback, [] {}, errCallback);
}

bool Redis::isErrorReturn(std::vector<String> &returnVec)
{
  return ((returnVec[0].c_str())[0] == '-');
}
