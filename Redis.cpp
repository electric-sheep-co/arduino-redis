#include "Redis.h"
#include <map>
#include <vector>
#include <memory>
#include <functional>

#define CRLF F("\r\n")

class RedisRESPString : public String {
public:
    RedisRESPString(char c) : String(c) {}
    RedisRESPString(String& s) : String(s) {}
};

/* The lack of RTTI on Ardunio is unfortunate but understandable.
 * However, we're not going to let that stop us. So here's our very
 * basic RedisObject type system */

class RedisObject {
public:
    typedef enum {
        SimpleString = '+',
        Error = '-',
        Integer = ':',
        BulkString = '$',
        Array = '*'
    } Type;

    RedisObject(Type tc) : _type(tc) {}

    static std::shared_ptr<RedisObject> parseType(String);

    virtual explicit operator RedisRESPString() = 0;
    virtual operator String() { return operator RedisRESPString(); }

    Type type() const { return _type; } 

protected:
    Type _type;
};

class RedisSimpleString : public RedisObject {
public:
    RedisSimpleString(String d) : data(d), RedisObject(Type::SimpleString) {}

    virtual explicit operator RedisRESPString() override
    {
        RedisRESPString emitStr((char)_type);
        // Simple strings cannot contain CRLF, as they must terminate with CRLF
        // https://redis.io/topics/protocol#resp-simple-strings
        data.replace(CRLF, F(""));
        emitStr += data;
        emitStr += CRLF;
        return emitStr;
    }

    virtual operator String() override { return data; }

protected:
    String data;
};

class RedisBulkString : public RedisObject {
public:
    RedisBulkString(String d) : data(d), RedisObject(Type::BulkString) {}

    RedisBulkString(RedisRESPString rd) : RedisObject(Type::BulkString)
    {
        auto crlfIndex = rd.indexOf(CRLF);
        if (crlfIndex != -1) {
            auto expectLen = rd.substring(rd.charAt(0) == RedisObject::Type::BulkString ? 1 : 0, 
                    crlfIndex).toInt();
            auto bStrStart = crlfIndex + String(CRLF).length();
            data = rd.substring(bStrStart, expectLen + bStrStart);
        }
    }
    
    virtual explicit operator RedisRESPString() override
    {
        RedisRESPString emitStr((char)_type);
        emitStr += String(data.length());
        emitStr += CRLF;
        emitStr += data;
        emitStr += CRLF;
        return emitStr;
    }

    virtual operator String() override { return data; }

protected:
    String data;
};

class RedisArray : public RedisObject {
public:
    RedisArray() : RedisObject(Type::Array) {}
    RedisArray(String d) : RedisArray() {
        Serial.printf("TODO RedisArray()!\n%s\n", d.c_str());
    }

    void add(std::shared_ptr<RedisObject> param) 
    {
        vec.push_back(param);
    }

    virtual explicit operator RedisRESPString() override 
    {
        RedisRESPString emitStr((char)_type);
        emitStr += String(vec.size());
        emitStr += CRLF;
        for (auto rTypeInst : vec) {
            emitStr += (RedisRESPString)*rTypeInst;
        }
        return emitStr;
    }

protected:
    std::vector<std::shared_ptr<RedisObject>> vec;
};

class RedisInteger : public RedisSimpleString {
public:
    RedisInteger(String d)
        : RedisSimpleString(d)
    {
        _type = RedisObject::Type::Integer;
    }

    operator int() { return data.substring(1).toInt(); }
    operator bool() { return (bool)operator int(); }
};

class RedisError : public RedisSimpleString {
public:
    RedisError(String d) 
        : RedisSimpleString(d) 
    {
        _type = RedisObject::Type::Error;
    }
};

class RedisCommand : public RedisArray {
public:
    RedisCommand(String command) : RedisArray() {
        add(std::shared_ptr<RedisObject>(new RedisBulkString(command)));
    }

    RedisCommand(String command, std::vector<String> args)
        : RedisCommand(command)
    {
        for (auto arg : args) {
            add(std::shared_ptr<RedisObject>(new RedisBulkString(arg)));
        }
    }

    std::shared_ptr<RedisObject> issue(Client& cmdClient) 
    {
        if (!cmdClient.connected())
            return std::shared_ptr<RedisObject>(new RedisError("INTERNAL ERROR: Client is not connected"));

        cmdClient.print((RedisRESPString)*this);
        return RedisObject::parseType(cmdClient.readStringUntil('\0'));
    }

private:
    String _cmd;
};

typedef std::map<RedisObject::Type, std::function<RedisObject*(String)>> TypeParseMap;

static TypeParseMap g_TypeParseMap {
    { RedisObject::Type::SimpleString, [](String substr) { return new RedisSimpleString(substr); } },
    { RedisObject::Type::BulkString, [](String substr) { return new RedisBulkString((RedisRESPString)substr); } },
    { RedisObject::Type::Integer, [](String substr) { return new RedisInteger(substr); } },
    { RedisObject::Type::Array, [](String substr) { return new RedisArray(substr); } },
    { RedisObject::Type::Error, [](String substr) { return new RedisError(substr); } }
};

std::shared_ptr<RedisObject> RedisObject::parseType(String data)
{
    RedisObject *rv = nullptr;

    if (data.length()) {
        auto typeChar = (RedisObject::Type)data.charAt(0);
        auto substr = data.substring(1);

        if (g_TypeParseMap.find(typeChar) != g_TypeParseMap.end()) {
            return std::shared_ptr<RedisObject>(g_TypeParseMap[typeChar](substr));
        }

        return std::shared_ptr<RedisObject>(new RedisError("INTERNAL ERROR: " + substr));
    }

    return std::shared_ptr<RedisObject>(new RedisError("INTERNAL ERROR: zero-length data!"));
}

#pragma mark Redis class implemenation

RedisReturnValue Redis::connect(const char* password)
{
    if(conn.connect(addr, port)) 
    {
        conn.setTimeout(500);
        int passwordLength = strlen(password);
        if (passwordLength > 0)
        {
            return ((RedisRESPString)*RedisCommand("AUTH", 
                        std::vector<String>{password}).issue(conn))
                .indexOf("+OK") == -1 ? RedisAuthFailure : RedisSuccess;
        }
        return RedisSuccess;
    }
    return RedisConnectFailure;
}

bool Redis::set(const char* key, const char* value)
{
    return ((RedisRESPString)*RedisCommand("SET", 
                std::vector<String>{key, value})
            .issue(conn)).indexOf("+OK") != -1;
}

String Redis::get(const char* key) 
{
    return (String)*RedisCommand("GET", std::vector<String>{key}).issue(conn);
}

int Redis::publish(const char* channel, const char* message)
{
    auto reply = RedisCommand("PUBLISH", std::vector<String>{channel, message}).issue(conn);

    switch (reply->type()) {
        case RedisObject::Type::Error:
            return -1;
        case RedisObject::Type::Integer:
            return (int)(RedisInteger)*reply;
    }
}

bool Redis::expire(const char* key, int seconds)
{
    return (bool)((RedisInteger)*RedisCommand("EXPIRE", 
        std::vector<String>{key, String(seconds)}).issue(conn));
}

void Redis::close(void)
{
    conn.stop();
}
