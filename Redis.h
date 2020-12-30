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
 *  @authors Mark Lynch <mark.christinat@gmail.com>
 */

#ifndef REDIS_H
#define REDIS_H

#include "Arduino.h"
#include "Client.h"

#include <vector>

typedef enum {
  RedisSuccess = 0,
  RedisNotConnectedFailure = 1,
  RedisAuthFailure = 2,
} RedisReturnValue;

typedef enum {
  RedisSubscribeBadCallback = -255,
  RedisSubscribeSetupFailure,
  RedisSubscribeServerDisconnected,
  RedisSubscribeOtherError,
  RedisSubscribeSuccess = 0
} RedisSubscribeResult;

typedef enum {
  RedisMessageBadResponseType = -255,
  RedisMessageTruncatedResponse,
  RedisMessageUnknownType,
} RedisMessageError;

/** Redis-for-Arduino client interface.
 *
 *  The sole constructor takes a reference to any instance
 *  of a Client subclass, allowing it to be used with any library
 *  that abstracts the client bytestream in this way (such as
 *  the built-in Ethernet class or the WiFiClient for devices 
 *  such as the ESP8266).
 */
class Redis {
public:
    /** Called upon successful receipt of a pub/sub `message` on subscribed `channel` */
    typedef void (*RedisMsgCallback)(Redis*, String channel, String message);
    /** Called upon an error in the receipt of a pub/sub message */
    typedef void (*RedisMsgErrorCallback)(Redis*, RedisMessageError);

    /**
     * Create a Redis connection using Client reference `client`.
     * @param client A Client instance representing the connection to a Redis server.
     * @returns An initialized Redis client using `client` to communicate with the server.
     */
    Redis(Client& client) : conn(client) {}

    ~Redis() {}
    Redis(const Redis&) = delete;
    Redis& operator=(const Redis&) = delete;
    Redis(const Redis&&) = delete;
    Redis& operator=(const Redis&&) = delete;

    /**
     * Authenticate with the given password.
     * @param password The password with which to authenticate.
     * @returns RedisReturnValue detailing the result
     */
    RedisReturnValue authenticate(const char* password);

    /**
     * Set `key` to `value`.
     * @note Current implementation only supports basic SET without behavioral
     * modification options added in Redis 2.6.12. To expire a set key, use the
     * expire() method below.     
     * @param key The key name to set
     * @param value The value to set for `key`
     * @return `true` if `key` was set to `value`, false if error.
     */
    bool set(const char* key, const char* value);

    /**
     * Get `key`.
     * @param key The key name to retrieve.
     * @return The key`s value as a string, null if the key does not exist.
     */
    String get(const char* key);

    /**
     * Delete `key`.
     * @param key
     * @return `true` if `key` was removed.
     */
    bool del(const char* key);

    /**
     * Determine if `key` exists.
     * @param key
     * @return `true` if `key` exists.
     */
    bool exists(const char* key);

    /**
     * Appends `value` to `key`.
     * @param key
     * @param value
     * @return The length of the string after the append operation.
     */
    int append(const char* key, const char* value);

    /**
     * Publish `message` to `channel`.
     * @param channel The channel on which to publish the message.
     * @param message The message to be published to the channel.
     * @return The number of subscribers to the published message.
     */
    int publish(const char* channel, const char* message);

    /**
     * Expire a `key` in `seconds`.
     * @param key The key name for which to set expire time.
     * @param seconds The number of seconds (from "now") at which this key will expire.
     * @return `true` if the expire time was set successfully, `false` otherwise.
     */
    bool expire(const char* key, int seconds) { return _expire_(key, seconds, "EXPIRE"); }

    /**
     * Expire a `key` at UNIX timestamp `timestamp` (seconds since January 1, 1970).
     * @note A timestamp in the past will delete the key immediately.
     * @param key The key name for which to set expire time.
     * @param timestamp The UNIX timestamp at which this key will expire.
     * @return `true` if the expire time was set successfully, `false` otherwise.
     */
    bool expire_at(const char* key, int timestamp) { return _expire_(key, timestamp, "EXPIREAT"); }

    /**
     * Expire a `key` in `milliseconds`.
     * @param key The key name for which to set expire time.
     * @param milliseconds The number of milliseconds (from "now") at which this key will expire.
     * @return `true` if the expire time was set successfully, `false` otherwise.
     */
    bool pexpire(const char* key, int ms) { return _expire_(key, ms, "PEXPIRE"); }

    /**
     * Expire a `key` at UNIX timestamp `timestamp` (milliseconds since January 1, 1970).
     * @note A timestamp in the past will delete the key immediately.
     * @param key The key name for which to set expire time.
     * @param timestamp The UNIX timestamp at which this key will expire.
     * @return `true` if the expire time was set successfully, `false` otherwise.
     */
    bool pexpire_at(const char* key, int timestamp) { return _expire_(key, timestamp, "PEXPIREAT"); }

    /**
     * Persist `key` (remove any expiry).
     * @param key The key to persist.
     * @return `true` if the timeout was removed, `false` if `key` DNE or had no expiry.
     */
    bool persist(const char* key);

    /**
     * Query remaining time-to-live (time-until-expiry) for `key`.
     * @param key The query whose TTL to query.
     * @return The key's TTL in seconds, or a negative value signaling error:
     *   -1 if the key exists but has no associated expire, -2 if the key DNE.
     */
    int ttl(const char* key) { return _ttl_(key, "TTL"); }

    /**
     * Query remaining time-to-live (time-until-expiry) for `key`.
     * @param key The query whose TTL to query.
     * @return The key`s TTL in milliseconds, or a negative value signaling error:
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
     * @return The field`s value.
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

    /**
     * Determine if `field` exists in hash at `key`.
     * @param key
     * @param field
     * @return `true` if `key` exists.
     */
    bool hexists(const char* key, const char* field);

    /**
     * Returns the specified elements of the list stored at `key`.
     * @param start Zero-based starting index (can be negative to indicate end-of-list offset).
     * @param end Zero-based ending index.
     * @return The list of elements, as a vector of Strings; or an empty vector if error/DNE.
     */
    std::vector<String> lrange(const char* key, int start, int stop);

    /**
     * Sets up a subscription for messages published to `channel`. May be called in any mode & from message handlers.
     */
    bool subscribe(const char* channel) { return _subscribe(SubscribeSpec{false, String(channel)}); }

    /**
     * Sets up a subscription for messages published to any channels matching `pattern`. May be called in any mode & from message handlers.
     */
    bool psubscribe(const char* pattern) { return _subscribe(SubscribeSpec{true, String(pattern)}); }

    /**
     * Removes a subscription for `channelOrPattern`. May be called from message handlers.
     */
    bool unsubscribe(const char* channelOrPattern);

    /**
     * Enters subscription mode and subscribes to all channels/patterns setup via `subscribe()`/`psubscribe()`.
     * On success, this call will *block* until stopSubscribing() is called (meaning `loop()` will never be called!), and only *then* will return `RedisSubscribeSuccess`.
     * On failure, this call will return immediately with a return value indicated the failure mode.
     * Calling `stopSubscribing()` will force this method to exit on the *next* recieved message.
     * @param messageCallback The function to be called on each successful message receipt.
     * @param errorCallback The function to be called if message receipt processing produces an error. Call `stopSubscribing()` on
     * the passed-in instance to end all further message processing.
     */
    RedisSubscribeResult startSubscribing(RedisMsgCallback messageCallback, RedisMsgErrorCallback errorCallback = nullptr);

    /**
     * Stops message processing on receipt of next message. Can be called from message handlers.
     */
    void stopSubscribing() { subLoopRun = false; }

private:
    typedef struct {
        bool pattern;
        String spec;
    } SubscribeSpec;

    bool _subscribe(SubscribeSpec spec);

    Client& conn;
    std::vector<SubscribeSpec> subSpec;
    bool subscriberMode = false;
    bool subLoopRun = false;

    bool _expire_(const char*, int, const char*);
    int _ttl_(const char*, const char*);
    bool _hset_(const char*, const char*, const char*, const char*);
};

#endif
