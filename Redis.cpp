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

#define TRCMD(t, c, ...) return RedisCommand(c, ArgList{__VA_ARGS__}).issue_typed<t>(conn)

String Redis::get(const char* key) 
{
    TRCMD(String, "GET", key);
}

bool Redis::del(const char* key)
{
    TRCMD(bool, "DEL", key);
}

int Redis::publish(const char* channel, const char* message)
{
    TRCMD(int, "PUBLISH", channel, message);
}

bool Redis::_expire_(const char* key, int arg, const char* cmd_var)
{
    TRCMD(bool, cmd_var, key, String(arg));
}

bool Redis::persist(const char* key)
{
    TRCMD(bool, "PERSIST", key);
}

int Redis::_ttl_(const char* key, const char* cmd_var)
{
    TRCMD(int, cmd_var, key);
}

bool Redis::_hset_(const char* key, const char* field, const char* value, const char* cmd_var)
{
    TRCMD(int, cmd_var, key, field, value);
}

String Redis::hget(const char* key, const char* field) 
{ 
    TRCMD(String, "HGET", key, field); 
}

bool Redis::hdel(const char* key, const char* field) 
{ 
    TRCMD(bool, "HDEL", key, field); 
}

int Redis::hlen(const char* key) 
{ 
    TRCMD(int, "HLEN", key); 
}

int Redis::hstrlen(const char* key, const char* field)
{
    TRCMD(int, "HSTRLEN", key, field);
}
