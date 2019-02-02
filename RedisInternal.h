#ifndef REDIS_INTERNAL_H
#define REDIS_INTERNAL_H

#include "Arduino.h"
#include "Client.h"
#include <map>
#include <vector>
#include <memory>
#include <functional>

#define CRLF F("\r\n")
#define ARDUINO_REDIS_SERIAL_TRACE 0

typedef std::vector<String> ArgList;

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

    virtual void init(Client& client);

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

    virtual String RESP() override;
};

class RedisBulkString : public RedisObject {
public:
    RedisBulkString(Client& c) : RedisObject(Type::BulkString) { init(c); }
    RedisBulkString(String& s) : RedisObject(Type::BulkString) { data = s; }

    virtual void init(Client& client) override;
   
    virtual String RESP() override;
};

class RedisArray : public RedisObject {
public:
    RedisArray() : RedisObject(Type::Array) {}
    RedisArray(Client& c) : RedisObject(Type::Array, c) {}

    void add(std::shared_ptr<RedisObject> param) { vec.push_back(param); }

    virtual String RESP() override;

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

    virtual String RESP() override { return "INTERNAL ERROR: " + data; }
};

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

    std::shared_ptr<RedisObject> issue(Client& cmdClient);

private:
    String _cmd;
};

#endif // REDIS_INTERNAL_H
