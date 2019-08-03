#include "RedisInternal.h"
#include <map>
#include <limits.h>

#if ARDUINO_REDIS_SERIAL_TRACE && 0
void pbytes(uint8_t* bytes, ssize_t len, const char* header)
{
    Serial.println();
    const uint _break = 0x20;
    Serial.printf("[%d bytes] %s\n", len, header ? header : "");
    for (int i = 0; i < len; i++)
    {
        if (!(i%_break)) Serial.printf("[H] %08x> ", i);
        Serial.printf("%02x ", *(bytes + i));
        if (!((i+1)%_break)) Serial.printf("\n");
    }
    Serial.println();
    for (int i = 0; i < len; i++)
    {
        if (!(i%_break)) Serial.printf("[C] %08x> ", i);
        Serial.printf("% 2c ", *(bytes + i));
        if (!((i+1)%_break)) Serial.printf("\n");
    }
    Serial.println();
    Serial.println();
}
#else
#define pbytes(x,y,z)
#endif

void RedisObject::init(Client& client)
{
    data = client.readStringUntil('\r');
    pbytes((uint8_t*)data.c_str(), data.length(), "RedisObject::init()::readStringUntil");
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

void RedisBulkString::init(Client& client)
{
    auto dLen = data.toInt();

    // "Null Bulk String" -- https://redis.io/topics/protocol#resp-bulk-strings
    if (dLen == -1) {
        data = (const char*)nullptr;
        return;
    }

    auto charBuf = new char[dLen + 1];
    bzero(charBuf, dLen + 1);

    auto readB = client.readBytes(charBuf, dLen);
    pbytes((uint8_t*)charBuf, readB, "RedisBulkString::init()::readBytes");
    if (readB != dLen) {
        Serial.printf("ERROR! Bad read (%ld ?= %ld)\n", (long)readB, (long)dLen);
        exit(-1);
    }

    data = String(charBuf);
    delete [] charBuf;
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

void RedisArray::init(Client& client)
{
    for (int i = 0; i < data.toInt(); i++)
        add(RedisObject::parseType(client));
}

RedisArray::operator std::vector<String>() const
{
    std::vector<String> rv;
    for (auto ro : vec)
        rv.push_back((String)*ro.get());
    return rv;
}

String RedisArray::RESP()
{
    String emitStr((char)_type);
    emitStr += String(vec.size());
    emitStr += CRLF;
    for (auto rTypeInst : vec) {
        emitStr += rTypeInst->RESP();
    }
    return emitStr;
}

std::shared_ptr<RedisObject> RedisCommand::issue(Client& cmdClient) 
{
    if (!cmdClient.connected())
        return std::shared_ptr<RedisObject>(new RedisInternalError("Client is not connected"));

    auto cmdRespStr = RESP();
    sprint("----- CMD ----\n%s---- /CMD ----\n", cmdRespStr.c_str());
    cmdClient.print(cmdRespStr);
    auto ret = RedisObject::parseType(cmdClient);
    if (ret && ret->type() == RedisObject::Type::InternalError)
        _err = (String)*ret;
    return ret;
}

template <>
int RedisCommand::issue_typed<int>(Client& cmdClient)
{
    auto cmdRet = issue(cmdClient);
    if (!cmdRet)
        return INT_MAX - 0x0f;
    if (cmdRet->type() != RedisObject::Type::Integer)
        return INT_MAX - 0xf0;
    return (int)*((RedisInteger*)cmdRet.get());
}

template <>
bool RedisCommand::issue_typed<bool>(Client& cmdClient)
{
    auto cmdRet = issue(cmdClient);
    if (cmdRet && cmdRet->type() == RedisObject::Type::Integer)
        return (bool)*((RedisInteger*)cmdRet.get());
    return false;
}

template <>
String RedisCommand::issue_typed<String>(Client& cmdClient)
{
    return (String)*issue(cmdClient);
}

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
        while (!client.available());

#if ARDUINO_REDIS_SERIAL_TRACE
        int readB = 0;
#endif
        RedisObject::Type typeChar = RedisObject::Type::NoType;
        do {
            typeChar = (RedisObject::Type)client.read();
#if ARDUINO_REDIS_SERIAL_TRACE
            ++readB;
#endif
        } while (typeChar == -1 || typeChar == '\r' || typeChar == '\n');

#if ARDUINO_REDIS_SERIAL_TRACE
        sprint("Parsed type character '%c' (after %d reads) (0x%x, %dd)\n", typeChar, readB, typeChar, (int)typeChar);
#endif

        if (g_TypeParseMap.find(typeChar) != g_TypeParseMap.end()) {
            auto retVal = g_TypeParseMap[typeChar](client);

            if (!retVal || retVal->type() == RedisObject::Type::Error) {
                String err = retVal ? (String)*retVal : "(nil)";
                return std::shared_ptr<RedisObject>(new RedisInternalError(err));
            }

            return std::shared_ptr<RedisObject>(retVal);
        }

        return std::shared_ptr<RedisObject>(new RedisInternalError("Unable to find type: " + typeChar));
    }

    return std::shared_ptr<RedisObject>(new RedisInternalError("Not connected"));
}

