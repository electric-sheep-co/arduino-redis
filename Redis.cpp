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

class RedisType {
public:
    RedisType(char tc) : type_char(tc) {}

    static std::shared_ptr<RedisType> parseType(String);

    virtual operator String() = 0;

protected:
    char type_char;
};

class RedisSimpleString : public RedisType {
public:
    RedisSimpleString(String d) : data(d), RedisType('+') {}

    virtual operator String() override
    {
        String emitStr(type_char);
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

class RedisBulkString : public RedisType {
public:
    RedisBulkString(String d) : data(d), RedisType('$') {}
    
    virtual operator String() override
    {
        String emitStr(type_char);
        emitStr += String(data.length());
        emitStr += CRLF;
        emitStr += data;
        emitStr += CRLF;
        return emitStr;
    }

protected:
    String data;
};

class RedisArray : public RedisType {
public:
    RedisArray() : RedisType('*') {}

    void add(std::shared_ptr<RedisType> param) 
    {
        vec.push_back(param);
    }

    virtual operator String() override 
    {
        String emitStr(type_char);
        emitStr += String(vec.size());
        emitStr += CRLF;
        for (auto rTypeInst : vec) {
            emitStr += *rTypeInst;
        }
        return emitStr;
    }

protected:
    std::vector<std::shared_ptr<RedisType>> vec;
};

class RedisError : public RedisSimpleString {
public:
    RedisError(String d) 
        : RedisSimpleString(d) 
    {
        type_char = '-';
    }
};

class RedisCommand : public RedisArray {
public:
    RedisCommand(String command) : RedisArray() {
        add(std::shared_ptr<RedisType>(new RedisBulkString(command)));
    }

    RedisCommand(String command, std::vector<String> args)
        : RedisCommand(command)
    {
        for (auto arg : args) {
            add(std::shared_ptr<RedisType>(new RedisBulkString(arg)));
        }
    }

    std::shared_ptr<RedisType> issue(Client& cmdClient) 
    {
        if (!cmdClient.connected())
            return std::shared_ptr<RedisType>(new RedisError("not connected or some shit"));

        auto tAsString = (String)*this;
        Serial.printf("[DEBUG] issue:\n%s\n", tAsString.c_str());
        cmdClient.print(tAsString);
        return RedisType::parseType(cmdClient.readStringUntil('\0'));
    }

private:
    String _cmd;
};

std::shared_ptr<RedisType> RedisType::parseType(String data)
{
    RedisType *rv = nullptr;
   
    if (data.length()) {
        auto substr = data.substring(1);
        switch (data.charAt(0)) {
            case '+': 
                rv = new RedisSimpleString(substr);
                break;
            case '-': 
            default:
                rv = new RedisError(substr);
                break;
        }
    }

    return std::shared_ptr<RedisType>(rv);
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

int Redis::publish(const char* channel, const char* message)
{
    conn.println("*3");
    conn.println("$7");
    conn.println("PUBLISH");
    conn.print("$");
    conn.println(strlen(channel));
    conn.println(channel);
    conn.print("$");
    conn.println(strlen(message));
    conn.println(message);

    String resp = conn.readStringUntil('\0');
    if (checkError(resp) != "")
    {
        return -1;
    }
    return resp.substring(1, resp.indexOf("\r\n")).toInt();
}

void Redis::close(void)
{
    conn.stop();
}
