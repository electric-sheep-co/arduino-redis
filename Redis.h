/** @mainpage Arduino-Redis
 *
 *  A Redis library for Arduino.
 *
 *  Primary interface documentation: @ref Redis
 *
 *  Project page on GitHub: https://github.com/electric-sheep-co/arduino-redis
 *
 *  Available in the Arduino IDE Library Manager (search for "redis")
 *
 *  Latest release always available at: https://github.com/electric-sheep-co/arduino-redis/releases/latest
 *
 *  Internal documentation for library developers: @ref RedisObject
 *
 *  @authors Ryan Joseph (Electric Sheep) <ryan@electricsheep.co>
 *  @authors Remi Caumette
 *  @authors Robert Oostenveld
 */

#ifndef REDIS_H
#define REDIS_H

#include "Arduino.h"
#include "Client.h"

typedef enum {
  RedisSuccess = 0,
  RedisNotConnectedFailure = 1,
  RedisAuthFailure = 2,
} RedisReturnValue;

/** Redis-for-Arduino client interface.
 *
 *  This is the primary (and currently sole) user-consumable interface
 *  class.
 *
 *  It has a constructor that takes a reference to any instance
 *  of a Client subclass, allowing it to be used with any communication
 *  framework that abstracts the client bytestream in this way (such as
 *  the builtin Ethernet class as well as WiFiClient for devices 
 *  such as the ESP8266).
 */
class Redis {
public:
    /**
     * Create a Redis connection using Client instance 'c'.
     * @param client A Client instance representing the connection to a Redis server.
     * @returns An initialized but unconnected (see connect()) Redis instance
     */
    Redis(Client& client) : conn(client) {}

    ~Redis() {}
    Redis(const Redis&) = delete;
    Redis& operator=(const Redis&) = delete;

    /**
     * Authenticate with the given password.
     * @param password The password with which to authenticate.
     * @returns RedisReturnValue detailing the result
     */
    RedisReturnValue authenticate(const char* password);

    /**
     * Set 'key' to 'value'.
     * @note Current implementation only supports basic SET without behavioral
     * modification options added in Redis 2.6.12. To expire a set key, use the
     * expire() method below.     
     * @param key The key name to set
     * @param value The value to set for 'key'
     * @return 'true' if 'key' was set to 'value', false if error.
     */
    bool set(const char* key, const char* value);

    /**
     * Get 'key'.
     * @param key The key name to retrieve.
     * @return The key's value as a string, empty if the key does not exist.
     */
    String get(const char* key);

    /**
     * Delete `key`.
     * @param key
     * @return `true` if `key` was removed.
     */
    bool del(const char* key);

    /**
     * Publish 'message' to 'channel'.
     * @param channel The channel on which to publish the message.
     * @param message The message to be published to the channel.
     * @return The number of subscribers to the published message.
     */
    int publish(const char* channel, const char* message);

    /**
     * Expire a 'key' in 'seconds'.
     * @param key The key name for which to set expire time.
     * @param seconds The number of seconds (from "now") at which this key will expire.
     * @return 'true' if the expire time was set successfully, 'false' otherwise.
     */
    bool expire(const char* key, int seconds) { return _expire_(key, seconds, "EXPIRE"); }

    /**
     * Expire a 'key' at UNIX timestamp 'timestamp' (seconds since January 1, 1970).
     * @note A timestamp in the past will delete the key immediately.
     * @param key The key name for which to set expire time.
     * @param timestamp The UNIX timestamp at which this key will expire.
     * @return 'true' if the expire time was set successfully, 'false' otherwise.
     */
    bool expire_at(const char* key, int timestamp) { return _expire_(key, timestamp, "EXPIREAT"); }

    /**
     * Expire a 'key' in 'milliseconds'.
     * @param key The key name for which to set expire time.
     * @param milliseconds The number of milliseconds (from "now") at which this key will expire.
     * @return 'true' if the expire time was set successfully, 'false' otherwise.
     */
    bool pexpire(const char* key, int ms) { return _expire_(key, ms, "PEXPIRE"); }

    /**
     * Expire a 'key' at UNIX timestamp 'timestamp' (milliseconds since January 1, 1970).
     * @note A timestamp in the past will delete the key immediately.
     * @param key The key name for which to set expire time.
     * @param timestamp The UNIX timestamp at which this key will expire.
     * @return 'true' if the expire time was set successfully, 'false' otherwise.
     */
    bool pexpire_at(const char* key, int timestamp) { return _expire_(key, timestamp, "PEXPIREAT"); }

    /**
     * Persist 'key' (remove any expiry).
     * @param key The key to persist.
     * @return 'true' if the timeout was removed, 'false' if 'key' DNE or had no expiry.
     */
    bool persist(const char* key);

    /**
     * Query remaining time-to-live (time-until-expiry) for 'key'.
     * @param key The query whose TTL to query.
     * @return The key's TTL in seconds, or a negative value signaling error:
     *   -1 if the key exists but has no associated expire, -2 if the key DNE.
     */
    int ttl(const char* key) { return _ttl_(key, "TTL"); }

    /**
     * Query remaining time-to-live (time-until-expiry) for 'key'.
     * @param key The query whose TTL to query.
     * @return The key's TTL in milliseconds, or a negative value signaling error:
     *   -1 if the key exists but has no associated expire, -2 if the key DNE.
     */
    int pttl(const char* key) { return _ttl_(key, "PTTL"); }

    /**
     * Set `field` in hash at `key` to `value`.
     * @param key
     * @param field
     * @param value
     * @return `true` if set, `false` if was updated
     */
    bool hset(const char* key, const char* field, const char* value) { return _hset_(key, field, value, "HSET"); }

    /**
     * Set `field` in hash at `key` to `value` i.f.f. `field` does not yet exist.
     * @param key
     * @param field
     * @param value
     * @return `true` if set, `false` if `field` already existed
     */
     bool hsetnx(const char* key, const char* field, const char* value) { return _hset_(key, field, value, "HSETNX"); }

    /**
     * Gets `field` stored in hash at `key`.
     * @param key
     * @param field
     * @return The field's value.
     */
    String hget(const char* key, const char* field);
    /**
     * Delete the `field` stored in hash at `key`.
     * @param key
     * @param field
     * @return `true` if deleted
     */
    bool hdel(const char* key, const char* field);

    /**
     * Gets the number of fields stored in hash at `key`.
     * @param key
     * @return The number of fields.
     */
    int hlen(const char* key);

    /**
     * Gets the length of the string at `field` in hash named `key`.
     * @param key
     * @param field
     * @return The field's string length value.
     */
    int hstrlen(const char* key, const char* field);

#if ARDUINO_REDIS_TEST
    typedef struct {
        int total;
        int passed;
    } TestResults;

    TestResults runTests(bool logToSerial = false, 
        String prefix = "__com.arduino-redis.test", 
        bool retainData = false);
#endif // ARDUINO_REDIS_TEST

private:
    Client& conn;

    bool _expire_(const char*, int, const char*);
    int _ttl_(const char*, const char*);
    bool _hset_(const char*, const char*, const char*, const char*);
};

#if ARDUINO_REDIS_TEST
#include <map>

/* tests are not expected to pass if retainData = true */
Redis::TestResults Redis::runTests(bool logToSerial, String prefix, bool retainData) 
{
    /* These are NOT executed in definition order! */
    std::map<String, std::function<bool(const char*)>> g_Tests 
    {
        { "set", [this](const char* k) { return set(k, "!"); } },
        { "setget", [this](const char* k) { return set(k, "!") == 1 && get(k) == "!"; } },
        { "expire", [this](const char* k) { 
            return set(k, "E") && expire(k, 5) && ttl(k) > 4;
        } },
        { "pexpire", [this](const char* k) {
            return set(k, "PE") && pexpire(k, 5000) && pttl(k) > 4500;
        } },
        { "dlyexp", [this](const char* k) {
            auto sr = set(k, "DE");
            expire(k, 10); 
            delay(4000);
            auto t = ttl(k);
            return sr && t > 5 && t < 7;
        } },
        { "hset", [this](const char* k) { return hset(k, "TEST-FIELD", "H!"); } },
        { "hsetget", [this](const char* k) { 
            return hset(k, "TF", "HH") && hget(k, "TF") == "HH"; 
        } },
        { "hlen", [this](const char* k) {
            for (int i = 0; i < 10; i++) {
                auto fv = String(i);
                hset(k, String("field-" + fv).c_str(), fv.c_str());
            }
            return hlen(k) == 10;
        } },
        { "hstrlen", [this](const char* k) { 
            return hset(k, "hsr", k) && hstrlen(k, "hsr") == strlen(k); 
        } }
    };

    if (logToSerial) {
        Serial.begin(115200);
        Serial.printf("Redis::run_tests(\"%s\") starting...\n", prefix.c_str());
    }

    auto pFunc = [&prefix](const String& n) { return String(prefix + "." + n); };

    int total = 0, pass = 0;
    for (auto& kv : g_Tests)
    {
        auto tName = kv.first;
        auto tFunc = kv.second;
        auto pName = pFunc(tName);
        auto tRes = tFunc(pName.c_str());
        if (logToSerial)
            Serial.printf("\tTest \"%s\":\t%s\n", tName.c_str(), tRes ? "pass" : "FAIL");
        total++;
        pass += tRes ? 1 : 0;
    }

    if (logToSerial)
        Serial.printf("Result: %d passed / %d total\n", pass, total);

    if (!retainData)
        for (auto& kv : g_Tests)
            (void)del(pFunc(kv.first).c_str());

    return { .total = total, .passed = pass };
}
#endif

#endif
