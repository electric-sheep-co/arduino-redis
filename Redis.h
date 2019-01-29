#ifndef REDIS_H
#define REDIS_H

#include <ESP8266WiFi.h>

typedef enum {
  RedisSuccess = 0,
  RedisConnectFailure = 1,
  RedisAuthFailure = 2,
} RedisReturnValue;

class Redis {
 public:
    Redis(const char* addr = "127.0.0.1", 
        int port = 6379) : 
	    addr(addr), port(port) {}
    ~Redis() { close(); }
    Redis(const Redis&) = delete;
    Redis& operator=(const Redis&) = delete;

    /**
     * Connect to the host specified in the constructor.
     * @param password The password to connect with (AUTH).
     * @returns RedisReturnValue detailing the result
     */
    RedisReturnValue connect(const char* password = "");

    /**
     * Set 'key' to 'value' (SET).
     * @note Current implementation only supports basic SET without behavioral
     * modification options added in Redis 2.6.12. To expire a set key, use the
     * expire() method below.     
     * @param key The key name to set
     * @param value The value to set for 'key'
     * @return 'true' if 'key' was set to 'value', false if error.
     */
    bool set(const char* key, const char* value);

    /**
     * Get 'key' (GET).
     * @param key The key name to retrieve.
     * @return The key's value as a string, empty if the key does not exist.
     */
    String get(const char* key);

    /**
     * Publish 'message' to 'channel' (PUBLISH).
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
     * @note A timestamp in the past will delete the ket immediately.
     * @param key The key name for which to set expire time.
     * @param timestamp The UNIX timestamp at which this key will expire.
     * @return 'true' if the expire time was set successfully, 'false' otherwise.
     */
    bool expire_at(const char* key, int timestamp) { return _expire_(key, timestamp, "EXPIREAT"); }

    /**
     * Persist 'key'.
     * @param key The key to persist (remove any expiry).
     * @return 'true' if the timeout was removed, 'false' if 'key' DNE or had no expiry.
     */
    bool persist(const char* key);

    /**
     * Close the underlying Client connection to Redis server
     */
    void close();

  private:
    const char* addr;
    int port;
    WiFiClient conn;

    bool _expire_(const char*, int, const char*);
 };

#endif
