/** @mainpage Arduino-Redis
 *
 *  A Redis library for Arduino.
 *
 *  Primary interface documentation: @ref Redis
 *
 *  Internal documentation for library developers: @ref RedisObject
 *
 *  @authors Ryan Joseph (Electric Sheep) <ryan@electricsheep.co>
 *  @authors Remi Caumette
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

  private:
    Client& conn;

    bool _expire_(const char*, int, const char*);
    int _ttl_(const char*, const char*);
 };

#endif
