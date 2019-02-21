#include <limits.h>
#include "RedisInternal.h"

void RedisObject::init(Client& client)
{
    data = client.readStringUntil('\r');
    Serial.printf("RedisObject::init has data '%s'\n", data.c_str());
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
    RedisObject::init(client);

    auto dLen = data.toInt();
    auto charBuf = new char[dLen + 1];
    bzero(charBuf, dLen + 1);

    auto crlfCstr = String(CRLF).c_str();
    auto readB = client.readBytes(charBuf, dLen);
    if (readB != dLen) {//|| client.find(crlfCstr, 2)) {
        (void)client.find(crlfCstr, 2);
        Serial.printf("ERROR! Bad read (%d ?= %d)\n", readB, dLen);
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
    Serial.printf("ARRAY INIT!! I HAVE %d ELEMENTS TO GET!\n", data.toInt());
    for (int i = 0; i < data.toInt(); i++) {
        Serial.printf("Adding %d...\n", i);
        add(RedisObject::parseType(client));
        auto last = vec.back();
        if (last->type() == RedisObject::Type::BulkString) {
            Serial.printf("%d> '%s'\n", i, ((String)*last.get()).c_str());
        }
    }
    //auto numElements = client.readString();
    //Serial.printf("ARRAY HAS '%s' ELEMENTS!\n", numElements.c_str());
    //auto bs1 = new RedisBulkString(client);
    //Serial.printf("BULKY! '%s'\n", ((String)*bs1).c_str());
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
#if ARDUINO_REDIS_SERIAL_TRACE
    Serial.printf("----- CMD ----\n%s---- /CMD ----\n", cmdRespStr.c_str());
#endif
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
        while (!client.available()) {
            delay(1);
        }

        auto typeChar = (RedisObject::Type)client.read();

#if ARDUINO_REDIS_SERIAL_TRACE
        Serial.printf("Parsed type character '%c' (0x%x)\n", typeChar, typeChar);
#endif

        if (g_TypeParseMap.find(typeChar) != g_TypeParseMap.end()) {
            auto retVal = g_TypeParseMap[typeChar](client);

            if (!retVal || retVal->type() == RedisObject::Type::Error) {
                String err = retVal ? (String)*retVal : "(nil)";
                return std::shared_ptr<RedisObject>(new RedisInternalError(err));
            }

            Serial.println("GOOD NEW OBJ!");
            return std::shared_ptr<RedisObject>(retVal);
        }

        return std::shared_ptr<RedisObject>(new RedisInternalError("Unable to find type: " + typeChar));
    }

    return std::shared_ptr<RedisObject>(new RedisInternalError("Not connected"));
}

