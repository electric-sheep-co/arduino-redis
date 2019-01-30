#include "Redis.h"
#include <map>
#include <vector>
#include <memory>
#include <functional>

#define CRLF F("\r\n")

/* The lack of RTTI on Ardunio is unfortunate but understandable.
 * However, we're not going to let that stop us. So here's our very
 * basic RedisObject type system */

class RedisObject {
public:
    typedef enum {
        NoType = ' ',
        SimpleString = '+',
        Error = '-',
        Integer = ':',
        BulkString = '$',
        Array = '*',
        InternalError = '!'
    } Type;

    RedisObject() {}
    RedisObject(Type tc) : _type(tc) {}
    RedisObject(Type tc, Client& c) : _type(tc) { init(c); }
    
    ~RedisObject() {}

    static std::shared_ptr<RedisObject> parseType(Client&);

    virtual void init(Client& client)
    {
        data = client.readStringUntil('\r');
        client.read(); // discard '\n' 
    }

    virtual String RESP() = 0;
    virtual operator String() { return data; }

    Type type() const { return _type; } 

protected:
    String data;
    Type _type = Type::NoType;
};

class RedisSimpleString : public RedisObject {
public:
    RedisSimpleString(Client& c) : RedisObject(Type::SimpleString, c) {}

    virtual String RESP() override
    {
        String emitStr((char)_type);
        // Simple strings cannot contain CRLF, as they must terminate with CRLF
        // https://redis.io/topics/protocol#resp-simple-strings
        data.replace(CRLF, F(""));
        emitStr += data;
        emitStr += CRLF;
        return emitStr;
    }
};

class RedisBulkString : public RedisObject {
public:
    RedisBulkString(Client& c) : RedisObject(Type::BulkString) { init(c); }
    RedisBulkString(String& s) : RedisObject(Type::BulkString) { data = s; }

    virtual void init(Client& client) override
    {
        RedisObject::init(client);

        auto dLen = String(data).toInt();
        auto charBuf = new char[dLen + 1];
        bzero(charBuf, dLen + 1);

        if (client.readBytes(charBuf, dLen) != dLen) {
            Serial.printf("ERROR! Bad read\n");
            exit(-1);
        }

        data = String(charBuf);
        delete [] charBuf;
    }
   
    virtual String RESP() override
    {
        String emitStr((char)_type);
        emitStr += String(data.length());
        emitStr += CRLF;
        emitStr += data;
        emitStr += CRLF;
        return emitStr;
    }
};

class RedisArray : public RedisObject {
public:
    RedisArray() : RedisObject(Type::Array) {}
    RedisArray(Client& c) : RedisObject(Type::Array, c) {}

    void add(std::shared_ptr<RedisObject> param) 
    {
        vec.push_back(param);
    }

    virtual String RESP() override 
    {
        String emitStr((char)_type);
        emitStr += String(vec.size());
        emitStr += CRLF;
        for (auto rTypeInst : vec) {
            emitStr += rTypeInst->RESP();
        }
        return emitStr;
    }

protected:
    std::vector<std::shared_ptr<RedisObject>> vec;
};

class RedisInteger : public RedisSimpleString {
public:
    RedisInteger(Client& c) : RedisSimpleString(c) { _type = Type::Integer; }
    operator int() { return data.toInt(); }
    operator bool() { return (bool)operator int(); }
};

class RedisError : public RedisSimpleString {
public:
    RedisError(Client& c) : RedisSimpleString(c) { _type = Type::Error; }
};

class RedisInternalError : public RedisObject
{
public:
    RedisInternalError(String err) : RedisObject(Type::InternalError) { data = err; }
    RedisInternalError(const char* err) : RedisInternalError(String(err)) {}

    virtual String RESP() override
    {
        return "INTERNAL ERROR: " + data;
    }
};

typedef std::vector<String> ArgList;

#define ARDUINO_REDIS_SERIAL_TRACE 1

class RedisCommand : public RedisArray {
public:
    RedisCommand(String command) : RedisArray() {
        add(std::shared_ptr<RedisObject>(new RedisBulkString(command)));
    }

    RedisCommand(String command, ArgList args)
        : RedisCommand(command)
    {
        for (auto arg : args) {
            add(std::shared_ptr<RedisObject>(new RedisBulkString(arg)));
        }
    }

    std::shared_ptr<RedisObject> issue(Client& cmdClient) 
    {
        if (!cmdClient.connected())
            return std::shared_ptr<RedisObject>(new RedisInternalError("Client is not connected"));

        auto cmdRespStr = RESP();
#if ARDUINO_REDIS_SERIAL_TRACE
        Serial.printf("----- CMD ----\n%s---- /CMD ----\n", cmdRespStr.c_str());
#endif
        cmdClient.print(cmdRespStr);
        return RedisObject::parseType(cmdClient);
    }

private:
    String _cmd;
};

typedef std::map<RedisObject::Type, std::function<RedisObject*(Client&)>> TypeParseMap;

static TypeParseMap g_TypeParseMap {
    { RedisObject::Type::SimpleString, [](Client& c) { return new RedisSimpleString(c); } },
    { RedisObject::Type::BulkString, [](Client& c) { return new RedisBulkString(c); } },
    { RedisObject::Type::Integer, [](Client& c) { return new RedisInteger(c); } },
    { RedisObject::Type::Array, [](Client& c) { return new RedisArray(c); } },
    { RedisObject::Type::Error, [](Client& c) { return new RedisError(c); } }
};

std::shared_ptr<RedisObject> RedisObject::parseType(Client& client)
{
    if (client.connected()) {
        while (!client.available()) {
            delay(1);
        }

        auto typeChar = (RedisObject::Type)client.read();

#if ARDUINO_REDIS_SERIAL_TRACE
        Serial.printf("Parsed type character '%c'\n", typeChar);
#endif

        if (g_TypeParseMap.find(typeChar) != g_TypeParseMap.end()) {
            return std::shared_ptr<RedisObject>(g_TypeParseMap[typeChar](client));
        }

        return std::shared_ptr<RedisObject>(new RedisInternalError("Unable to find type: " + typeChar));
    }
    return std::shared_ptr<RedisObject>(new RedisInternalError("Not connected"));
}

#pragma mark Redis class implemenation

RedisReturnValue Redis::connect(const char* password)
{
    if(conn.connect(addr, port)) 
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
    return RedisConnectFailure;
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
