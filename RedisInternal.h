#ifndef REDIS_INTERNAL_H
#define REDIS_INTERNAL_H

#include "Arduino.h"
#include "Client.h"
#include <vector>
#include <memory>
#include <functional>

#define CRLF F("\r\n")

typedef std::vector<String> ArgList;

/** A basic object model for the Redis serialization protocol (RESP):
 *      https://redis.io/topics/protocol
 */

/* The lack of RTTI on Ardunio is unfortunate but completely understandable. 
 * However, we're not going to let that stop us, so RedisObject implements a basic 
 * but functional type system for Redis objects. 
 */
class RedisObject {
public:
    /** Denote a basic Redis type, with NoType and InternalError defined specifically for this API */
    typedef enum {
        NoType = '\0',
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
    
    virtual ~RedisObject() {}

    static std::shared_ptr<RedisObject> parseType(Client&);

    /** Initialize a RedisObject instance from the bytestream represented by 'client'.
     *  Only does very basic (e.g. SimpleString-style) parsing of the object from
     *  the byte stream. Concrete subclasses are expected to override this to provide
     *  class-specific parsing & initialization logic.
     */
    virtual void init(Client& client);

    /** Produce the Redis serialization protocol (RESP) representation. Must be overridden. */
    virtual String RESP() = 0;

    /** Produce a human-readable String representation. 
     *  Base implementation only returns the type character, so should be overriden. */
    virtual operator String() { return data; }

    Type type() const { return _type; } 

protected:
    String data;
    Type _type = Type::NoType;
};

/** A Simple String: https://redis.io/topics/protocol#resp-simple-strings */
class RedisSimpleString : public RedisObject {
public:
    RedisSimpleString(Client& c) : RedisObject(Type::SimpleString, c) {}
    ~RedisSimpleString() override {}

    virtual String RESP() override;
};

/** A Bulk String: https://redis.io/topics/protocol#resp-bulk-strings */
class RedisBulkString : public RedisObject {
public:
    RedisBulkString(Client& c) : RedisObject(Type::BulkString, c) { init(c); }
    RedisBulkString(String& s) : RedisObject(Type::BulkString) { data = s; }
    ~RedisBulkString() override {}

    virtual void init(Client& client) override;
   
    virtual String RESP() override;
};

/** An Array: https://redis.io/topics/protocol#resp-arrays */
class RedisArray : public RedisObject {
public:
    RedisArray() : RedisObject(Type::Array) {}
    RedisArray(Client& c) : RedisObject(Type::Array, c) { init(c); }
    ~RedisArray() override { vec.empty(); }

    void add(std::shared_ptr<RedisObject> param) { vec.push_back(param); }

    operator std::vector<String>() const;

    virtual void init(Client& client) override;

    virtual String RESP() override;

protected:
    std::vector<std::shared_ptr<RedisObject>> vec;
};

/** An Integer: https://redis.io/topics/protocol#resp-integers */
class RedisInteger : public RedisSimpleString {
public:
    RedisInteger(Client& c) : RedisSimpleString(c) { _type = Type::Integer; }
    ~RedisInteger() override {}

    operator int() { return data.toInt(); }
    operator bool() { return (bool)operator int(); }
};

/** An Error: https://redis.io/topics/protocol#resp-errors */
class RedisError : public RedisSimpleString {
public:
    RedisError(Client& c) : RedisSimpleString(c) { _type = Type::Error; }
    ~RedisError() override {}
};

/** An internal API error. Call RESP() to get the error string. */
class RedisInternalError : public RedisObject
{
public:
    typedef enum {
      UnknownError = -254,
      UnknownType,
      Disconnected,
      NoError = 0
    } RedisInternalErrorCode;

    RedisInternalError(RedisInternalErrorCode c) : RedisObject(Type::InternalError), _code(c) {}
    RedisInternalError(RedisInternalErrorCode c, String es) : RedisInternalError(c) { data = es; }
    ~RedisInternalError() override {}

    RedisInternalErrorCode code() { return _code; }
    void setErrorString(String es) { data = es; }

    virtual String RESP() override { return "INTERNAL ERROR " + String(_code) + (data ? ": " + data : ""); }

protected:
  RedisInternalErrorCode _code;
};

/** A Command (a specialized Array subclass): https://redis.io/topics/protocol#sending-commands-to-a-redis-server */
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

    ~RedisCommand() override {}

    /** Issue the command on the bytestream represented by `cmdClient`.
     *  @param cmdClient The client object representing the bytestream connection to a Redis server.
     *  @return A shared pointer of a "RedisObject" representing a concrete subclass instantiated as
     *  a result of parsing the Redis server return value. Check RedisObject::type() to determine concrete type.
     */
    std::shared_ptr<RedisObject> issue(Client& cmdClient);

    template <typename T>
    T issue_typed(Client& cmdClient);

private:
    String _err;
};

#endif // REDIS_INTERNAL_H
