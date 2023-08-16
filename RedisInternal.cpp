#include "RedisInternal.h"
#include <map>
#include <limits.h>
#include <memory>

void RedisObject::init(Client &client)
{
    data = client.readStringUntil('\r');
    client.read(); // discard '\n'
}

String RedisSimpleString::RESP()
{
    String emitStr((char)_type);
    // Simple strings cannot contain CRLF, as they must terminate with CRLF
    // https://redis.io/topics/protocol#resp-simple-strings
    data.replace(CRLF, F(""));
    emitStr += data;
    emitStr += CRLF;
    return emitStr;
}

void RedisBulkString::init(Client &client)
{
    auto dLen = data.toInt();

    // "Null Bulk String" -- https://redis.io/topics/protocol#resp-bulk-strings
    if (dLen == -1)
    {
        data = (const char *)nullptr;
        return;
    }

    auto charBuf = new char[dLen + 1];
    bzero(charBuf, dLen + 1);

    auto readB = client.readBytes(charBuf, dLen);
    if ((int)readB != dLen)
    {
        Serial.printf("ERROR! Bad read (%ld ?= %ld)\n", (long)readB, (long)dLen);
        exit(-1);
    }

    data = String(charBuf);
    delete[] charBuf;
}

String RedisBulkString::RESP()
{
    String emitStr((char)_type);
    emitStr += String(data.length());
    emitStr += CRLF;
    emitStr += data;
    emitStr += CRLF;
    return emitStr;
}

void RedisArray::init(Client &client)
{
    for (int i = 0; i < data.toInt(); i++)
        add(RedisObject::parseType(client));
}

RedisArray::operator std::vector<std::shared_ptr<RedisObject>>() const
{
    return vec;
}

RedisArray::operator std::vector<String>() const
{
    std::vector<String> rv;
    for (auto ro : vec)
    {
        if (ro->type() == RedisObject::Type::Array)
        {
            for (auto append_inner : ((RedisArray*)ro.get())->operator std::vector<String>())
            {
                rv.push_back(append_inner);
            }
        }
        else
        {
            rv.push_back((String)*ro.get());
        }
    }
    return rv;
}

String RedisArray::RESP()
{
    String emitStr((char)_type);
    emitStr += String(vec.size());
    emitStr += CRLF;
    for (auto rTypeInst : vec)
    {
        emitStr += rTypeInst->RESP();
    }
    return emitStr;
}

std::shared_ptr<RedisObject> RedisCommand::issue(Client &cmdClient)
{
    if (!cmdClient.connected())
        return std::shared_ptr<RedisObject>(new RedisInternalError(RedisInternalError::Disconnected));

    auto cmdRespStr = RESP();
    cmdClient.print(cmdRespStr);
    auto ret = RedisObject::parseType(cmdClient);
    if (ret && ret->type() == RedisObject::Type::InternalError)
        _err = (String)*ret;
    return ret;
}

template <>
int RedisCommand::issue_typed<int>(Client &cmdClient)
{
    auto cmdRet = issue(cmdClient);
    if (!cmdRet)
        return INT_MAX - 0x0f;
    if (cmdRet->type() != RedisObject::Type::Integer)
        return INT_MAX - 0xf0;
    return (int)*((RedisInteger *)cmdRet.get());
}

template <>
bool RedisCommand::issue_typed<bool>(Client &cmdClient)
{
    auto cmdRet = issue(cmdClient);
    if (cmdRet && cmdRet->type() == RedisObject::Type::Integer)
        return (bool)*((RedisInteger *)cmdRet.get());
    return false;
}

template <>
String RedisCommand::issue_typed<String>(Client &cmdClient)
{
    return (String)*issue(cmdClient);
}

typedef std::map<RedisObject::Type, std::function<RedisObject *(Client &)>> TypeParseMap;

static TypeParseMap g_TypeParseMap{
    {RedisObject::Type::SimpleString, [](Client &c)
     { return new RedisSimpleString(c); }},
    {RedisObject::Type::BulkString, [](Client &c)
     { return new RedisBulkString(c); }},
    {RedisObject::Type::Integer, [](Client &c)
     { return new RedisInteger(c); }},
    {RedisObject::Type::Array, [](Client &c)
     { return new RedisArray(c); }},
    {RedisObject::Type::Error, [](Client &c)
     { return new RedisError(c); }}};

std::shared_ptr<RedisObject> RedisObject::parseTypeNonBlocking(Client &client)
{
    if (client.connected() && !client.available()) {
        return nullptr;
    }

    RedisObject::Type typeChar = RedisObject::Type::NoType;
    if (!client.connected())
    {
        return std::shared_ptr<RedisObject>(new RedisInternalError(RedisInternalError::Disconnected));
    }

    typeChar = (RedisObject::Type)client.read();
    if (typeChar == -1 || typeChar == '\r' || typeChar == '\n') {
        return nullptr;
    };

    if (g_TypeParseMap.find(typeChar) != g_TypeParseMap.end())
    {
        auto retVal = g_TypeParseMap[typeChar](client);

        if (!retVal)
        {
            return std::shared_ptr<RedisObject>(new RedisInternalError(RedisInternalError::UnknownError, "(nil)"));
        }

        return std::shared_ptr<RedisObject>(retVal);
    }

    return std::shared_ptr<RedisObject>(new RedisInternalError(RedisInternalError::UnknownType, String(typeChar)));
}

std::shared_ptr<RedisObject> RedisObject::parseType(Client &client)
{
    std::shared_ptr<RedisObject> type = nullptr;
    while (type==nullptr) {
        type = parseTypeNonBlocking(client);
    }
    return type;
}
