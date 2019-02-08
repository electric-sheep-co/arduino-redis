#include "Redis.h"
#include "RedisInternal.h"

#pragma mark Redis class implemenation

RedisReturnValue Redis::authenticate(const char* password)
{
    if(conn.connected())
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
    return RedisCommand("GET", ArgList{key}).issue_scalar<String>(conn);
}

int Redis::publish(const char* channel, const char* message)
{
    return RedisCommand("PUBLISH", ArgList{channel, message}).issue_scalar<int>(conn);
}

bool Redis::_expire_(const char* key, int arg, const char* cmd_var)
{
    return RedisCommand(cmd_var, ArgList{key, String(arg)}).issue_scalar<bool>(conn);
}

bool Redis::persist(const char* key)
{
    return RedisCommand("PERSIST", ArgList{key}).issue_scalar<bool>(conn);
}

int Redis::_ttl_(const char* key, const char* cmd_var)
{
    return RedisCommand(cmd_var, ArgList{key}).issue_scalar<int>(conn);
}
