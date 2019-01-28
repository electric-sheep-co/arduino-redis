#include "Redis.h"

/**
 * Check if the response is an error.
 * @return Return the error message or "".
 */
String Redis::checkError(String resp)
{
    if (resp.startsWith("-"))
    {
        return resp.substring(1, resp.indexOf("\r\n"));
    }
    return "";
}

#include <vector>
#include <memory>

#define CRLF F("\r\n")

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

    virtual operator String() = 0;

    Type type() const { return _type; } 

protected:
    Type _type;
};

class RedisSimpleString : public RedisObject {
public:
    RedisSimpleString(String d) : data(d), RedisObject(Type::SimpleString) {}

    virtual operator String() override
    {
        String emitStr((char)_type);
        // Simple strings cannot contain CRLF, as they must terminate with CRLF
        // https://redis.io/topics/protocol#resp-simple-strings
        data.replace(CRLF, F(""));
        emitStr += data;
        emitStr += CRLF;
        return emitStr;
    }

protected:
    String data;
};

class RedisBulkString : public RedisObject {
public:
    RedisBulkString(String d) : data(d), RedisObject(Type::BulkString) {}
    
    virtual operator String() override
    {
        String emitStr((char)_type);
        emitStr += String(data.length());
        emitStr += CRLF;
        emitStr += data;
        emitStr += CRLF;
        return emitStr;
    }

protected:
    String data;
};

class RedisArray : public RedisObject {
public:
    RedisArray() : RedisObject(Type::Array) {}

    void add(std::shared_ptr<RedisObject> param) 
    {
        vec.push_back(param);
    }

    virtual operator String() override 
    {
        String emitStr((char)_type);
        emitStr += String(vec.size());
        emitStr += CRLF;
        for (auto rTypeInst : vec) {
            emitStr += *rTypeInst;
        }
        return emitStr;
    }

protected:
    std::vector<std::shared_ptr<RedisObject>> vec;
};

class RedisError : public RedisSimpleString {
public:
    RedisError(String d) 
        : RedisSimpleString(d) 
    {
        _type = RedisObject::Type::Error;
    }
};

class RedisInteger : public RedisSimpleString {
public:
    RedisInteger(String d)
        : RedisSimpleString(d)
    {
        _type = RedisObject::Type::Integer;
    }

    operator int()
    {
        return data.substring(1).toInt();
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
            return std::shared_ptr<RedisObject>(new RedisError("not connected or some shit"));

        auto tAsString = (String)*this;
        Serial.printf("[DEBUG] issue:\n%s\n", tAsString.c_str());
        cmdClient.print(tAsString);
        return RedisObject::parseType(cmdClient.readStringUntil('\0'));
    }

private:
    String _cmd;
};

std::shared_ptr<RedisObject> RedisObject::parseType(String data)
{
    RedisObject *rv = nullptr;
   
    if (data.length()) {
        auto substr = data.substring(1);
        Serial.printf("[DEBUG] parsing type from string:\n%s\n", data.c_str());
        switch (data.charAt(0)) {
            case RedisObject::Type::SimpleString:
                rv = new RedisSimpleString(substr);
                break;
            case RedisObject::Type::Integer:
                rv = new RedisInteger(substr);
                break;
            case RedisObject::Type::Error:
            default:
                rv = new RedisError(substr);
                break;
        }
    }

    Serial.printf("[DEBUG] parsed object of type '%c', substr='%s'\n", (char)rv->type(), ((String)*rv).c_str());
    return std::shared_ptr<RedisObject>(rv);
}

RedisReturnValue Redis::connect(const char* password)
{
    if(conn.connect(addr, port)) 
    {
        int passwordLength = strlen(password);
        if (passwordLength > 0)
        {
            return ((String)*RedisCommand("AUTH", 
                        std::vector<String>{password}).issue(conn))
                .indexOf("+OK") == -1 ? RedisAuthFailure : RedisSuccess;
        }
        return RedisSuccess;
    }
    return RedisConnectFailure;
}

bool Redis::set(const char* key, const char* value)
{
    return ((String)*RedisCommand("SET", 
                std::vector<String>{key, value})
            .issue(conn)).indexOf("+OK") != -1;
}

String Redis::get(const char* key) 
{
    conn.println("*2");
    conn.println("$3");
    conn.println("GET");
    conn.print("$");
    conn.println(strlen(key));
    conn.println(key);

    String resp = conn.readStringUntil('\0');
    String error = checkError(resp);
    if (error != "")
    {
        return error;
    }
    if (resp.startsWith("$-1"))
    {
        return "";
    }
    int start = resp.indexOf("\r\n");
    int length = resp.substring(1, start).toInt();
    return resp.substring(start + 2, start + length + 2);
}

#include <typeinfo>

int Redis::publish(const char* channel, const char* message)
{
    auto reply = RedisCommand("PUBLISH", std::vector<String>{channel, message}).issue(conn);

    switch (reply->type()) {
        case RedisObject::Type::Error:
            Serial.printf("[DEBUG] PUBLISH ERROR: \n%s\n", ((String)*reply).c_str());
            return -1;
        case RedisObject::Type::Integer:
            return (RedisInteger)*reply;
    }
}

void Redis::close(void)
{
    conn.stop();
}
