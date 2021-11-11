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

/** The return value from from `Redis::authenticate()` */
typedef enum
{
  RedisSuccess = 0,
  /// Authenticate attempted before the connection has been established.
  RedisNotConnectedFailure = 1,
  /// The authentication credentials used are not valid.
  RedisAuthFailure = 2,
} RedisReturnValue;

/** The return value from `Redis::startSubscribing()`. See `examples/Subscribe.ino` for usage demo. */
typedef enum
{
  /// One of the callback parameters given is invalid.
  RedisSubscribeBadCallback = -255,
  /// Setting up for subscription mode failed.
  RedisSubscribeSetupFailure,
  /// The remote end disconnected, retry may be available.
  RedisSubscribeServerDisconnected,
  /// An unknown error occurred.
  RedisSubscribeOtherError,
  RedisSubscribeSuccess = 0
} RedisSubscribeResult;

/** A value of this type will be passed as the second argument ot `Redis::RedisMsgErrorCallback`, if called */
typedef enum
{
  /// The underlying Redis type detected in the message is not of the type expected.
  RedisMessageBadResponseType = -255,
  /// The message response was truncated early.
  RedisMessageTruncatedResponse,
  /// An unknown error occurred.
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
class Redis
{
public:
  /** Called upon successful receipt of a pub/sub `message` on subscribed `channel` */
  typedef void (*RedisMsgCallback)(Redis *, String channel, String message);
  /** Called upon an error in the receipt of a pub/sub message */
  typedef void (*RedisMsgErrorCallback)(Redis *, RedisMessageError);

  /**
     * Create a Redis connection using Client reference `client`.
     * @param client A Client instance representing the connection to a Redis server.
     * @returns An initialized Redis client using `client` to communicate with the server.
     */
  Redis(Client &client) : conn(client) {}

  ~Redis() {}
  Redis(const Redis &) = delete;
  Redis &operator=(const Redis &) = delete;
  Redis(const Redis &&) = delete;
  Redis &operator=(const Redis &&) = delete;

  /**
     * Authenticate with the given password.
     * @param password The password with which to authenticate.
     * @returns RedisReturnValue detailing the result
     */
  RedisReturnValue authenticate(const char *password);

  /**
     * Set `key` to `value`.
     * @note Current implementation only supports basic SET without behavioral
     * modification options added in Redis 2.6.12. To expire a set key, use the
     * expire() method below.     
     * @param key The key name to set
     * @param value The value to set for `key`
     * @return `true` if `key` was set to `value`, false if error.
     */
  bool set(const char *key, const char *value);

  /**
     * Get `key`.
     * @param key The key name to retrieve.
     * @return The key`s value as a string, null if the key does not exist.
     */
  String get(const char *key);

  /**
     * Delete `key`.
     * @param key
     * @return `true` if `key` was removed.
     */
  bool del(const char *key);

  /**
     * Determine if `key` exists.
     * @param key
     * @return `true` if `key` exists.
     */
  bool exists(const char *key);

  /**
     * Appends `value` to `key`.
     * @param key
     * @param value
     * @return The length of the string after the append operation.
     */
  int append(const char *key, const char *value);

  /**
     * Publish `message` to `channel`.
     * @param channel The channel on which to publish the message.
     * @param message The message to be published to the channel.
     * @return The number of subscribers to the published message.
     */
  int publish(const char *channel, const char *message);

  /**
     * Expire a `key` in `seconds`.
     * @param key The key name for which to set expire time.
     * @param seconds The number of seconds (from "now") at which this key will expire.
     * @return `true` if the expire time was set successfully, `false` otherwise.
     */
  bool expire(const char *key, int seconds) { return _expire_(key, seconds, "EXPIRE"); }

  /**
     * Expire a `key` at UNIX timestamp `timestamp` (seconds since January 1, 1970).
     * @note A timestamp in the past will delete the key immediately.
     * @param key The key name for which to set expire time.
     * @param timestamp The UNIX timestamp at which this key will expire.
     * @return `true` if the expire time was set successfully, `false` otherwise.
     */
  bool expire_at(const char *key, int timestamp) { return _expire_(key, timestamp, "EXPIREAT"); }

  /**
     * Expire a `key` in `milliseconds`.
     * @param key The key name for which to set expire time.
     * @param milliseconds The number of milliseconds (from "now") at which this key will expire.
     * @return `true` if the expire time was set successfully, `false` otherwise.
     */
  bool pexpire(const char *key, int ms) { return _expire_(key, ms, "PEXPIRE"); }

  /**
     * Expire a `key` at UNIX timestamp `timestamp` (milliseconds since January 1, 1970).
     * @note A timestamp in the past will delete the key immediately.
     * @param key The key name for which to set expire time.
     * @param timestamp The UNIX timestamp at which this key will expire.
     * @return `true` if the expire time was set successfully, `false` otherwise.
     */
  bool pexpire_at(const char *key, int timestamp) { return _expire_(key, timestamp, "PEXPIREAT"); }

  /**
     * Persist `key` (remove any expiry).
     * @param key The key to persist.
     * @return `true` if the timeout was removed, `false` if `key` DNE or had no expiry.
     */
  bool persist(const char *key);

  /**
     * Query remaining time-to-live (time-until-expiry) for `key`.
     * @param key The query whose TTL to query.
     * @return The key`s TTL in milliseconds, or a negative value signaling error:
     *   -1 if the key exists but has no associated expire, -2 if the key DNE.
     */
  int pttl(const char *key) { return _ttl_(key, "PTTL"); }

  /**
     * Query remaining time-to-live (time-until-expiry) for `key`.
     * @param key The query whose TTL to query.
     * @return The key's TTL in seconds, or a negative value signaling error:
     *   -1 if the key exists but has no associated expire, -2 if the key DNE.
     */
  int ttl(const char *key) { return _ttl_(key, "TTL"); }

  /**
     * Set `field` in hash at `key` to `value`.
     * @param key
     * @param field
     * @param value
     * @return `true` if set, `false` if was updated
     */
  bool hset(const char *key, const char *field, const char *value) { return _hset_(key, field, value, "HSET"); }

  /**
     * Set `field` in hash at `key` to `value` i.f.f. `field` does not yet exist.
     * @param key
     * @param field
     * @param value
     * @return `true` if set, `false` if `field` already existed
     */
  bool hsetnx(const char *key, const char *field, const char *value) { return _hset_(key, field, value, "HSETNX"); }

  /**
     * Gets `field` stored in hash at `key`.
     * @param key
     * @param field
     * @return The field`s value.
     */
  String hget(const char *key, const char *field);

  /**
     * Delete the `field` stored in hash at `key`.
     * @param key
     * @param field
     * @return `true` if deleted
     */
  bool hdel(const char *key, const char *field);

  /**
     * Gets the number of fields stored in hash at `key`.
     * @param key
     * @return The number of fields.
     */
  int hlen(const char *key);

  /**
     * Gets the length of the string at `field` in hash named `key`.
     * @param key
     * @param field
     * @return The field's string length value.
     */
  int hstrlen(const char *key, const char *field);

  /**
     * Determine if `field` exists in hash at `key`.
     * @param key
     * @param field
     * @return `true` if `key` exists.
     */
  bool hexists(const char *key, const char *field);

  /**
     * Returns the element of the list stored at `index`.
     * @param key
     * @param end Zero-based element index.
     * @return The element's contents as a String.
     */
  String lindex(const char *key, int index);

  /** Returns the length of the list stored at `key`. 
     *  If `key` does not exist, it is interpreted as an empty list and 0 is returned.
     *  An error is returned when the value stored at `key` is not a list.
     *  @param key
     *  @return The length of the list at `key`.
     */
  int llen(const char *key);

  /** Removes and returns the first element of the list stored at `key`.
     *  @param key
     *  @return The value of the first element, or nil when key does not exist.
     */
  String lpop(const char *key);

  /** Returns the index of the first matched `element` when scanning from head to tail
     *  @param key
     *  @param element
     *  @return The index of the first element, or nil when key does not exist.
     */
  int lpos(const char *key, const char *element);

  /** Insert the specified `value` at the head of the list stored at `key` (or 'LPUSHX' semantics if `exclusive` is true)
     *  @param key
     *  @param value
     *  @param exclusive If set, issues 'LPUSHX' instead of 'LPUSH' which pushes `value` only if `key` already exists and holds a list. In contrary to LPUSH, no operation will be performed when key does not yet exist.
     *  @return The length of the list after the push operations.
     */
  int lpush(const char *key, const char *value, bool exclusive = false);

  /**
     * Returns the specified elements of the list stored at `key`.
     * @param key
     * @param start Zero-based starting index (can be negative to indicate end-of-list offset).
     * @param end Zero-based ending index.
     * @return The list of elements, as a vector of Strings; or an empty vector if error/DNE.
     */
  std::vector<String> lrange(const char *key, int start, int stop);

  /** Removes the first `count` occurrences of elements equal to `element` from the list stored at `key`.
     *  @param key
     *  @param count if less than zero: removes elements moving from head to tail; if greater than zero, removes from tail to head. if zero, removes all.
     *  @param element
     *  @return The value of the first element, or nil when key does not exist.
     */
  int lrem(const char *key, int count, const char *element);

  /** Sets the list element at `index` to `element`. 
     *  @param key
     *  @param index
     *  @param element
     *  @return success or failure
     */
  bool lset(const char *key, int index, const char *element);

  /** Trim an existing list so that it will contain only the specified range of elements specified. Both `start` and `stop` are zero-based indexes.
     *  @param key
     *  @param start
     *  @param stop
     *  @return success or failure
     */
  bool ltrim(const char *key, int start, int stop);

  /** Removes and returns the last element of the list stored at `key`.
     *  @param key
     *  @return The value of the last element, or nil when key does not exist.
     */
  String rpop(const char *key);

  /** Insert the specified `value` at the tail of the list stored at `key` (or 'RPUSHX' semantics if `exclusive` is true)
     *  @param key
     *  @param value
     *  @param exclusive If set, issues 'RPUSHX' instead of 'RPUSH' which pushes `value` only if `key` already exists and holds a list. In contrary to RPUSH, no operation will be performed when key does not yet exist.
     *  @return The length of the list after the push operations.
     */
  int rpush(const char *key, const char *value, bool exclusive = false);

  /**
     * Sets up a subscription for messages published to `channel`. May be called in any mode & from message handlers.
     */
  bool subscribe(const char *channel) { return _subscribe(SubscribeSpec{false, String(channel)}); }

  /**
     * Sets up a subscription for messages published to any channels matching `pattern`. May be called in any mode & from message handlers.
     */
  bool psubscribe(const char *pattern) { return _subscribe(SubscribeSpec{true, String(pattern)}); }

  /**
     * Removes a subscription for `channelOrPattern`. May be called from message handlers.
     */
  bool unsubscribe(const char *channelOrPattern);

  bool tsadd(const char *key, unsigned long timestamp, const int value);
  /**
     * Enters subscription mode and subscribes to all channels/patterns setup via `subscribe()`/`psubscribe()`.
     * On success, this call will *block* until stopSubscribing() is called (meaning `loop()` will never be called!), and only *then* will return `RedisSubscribeSuccess`.
     * On remote disconnect, this call will end with the return value `RedisSubscribeServerDisconnected`, which is generally non-fatal.
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

  String info(const char *section);

  // The following are (obstensibly) for library testing purposes only
  void setTestContext(const void *context) { _test_context = context; }
  const void *getTestContext() { return _test_context; }

private:
  typedef struct
  {
    bool pattern;
    String spec;
  } SubscribeSpec;

  bool _subscribe(SubscribeSpec spec);

  Client &conn;
  std::vector<SubscribeSpec> subSpec;
  bool subscriberMode = false;
  bool subLoopRun = false;

  bool _expire_(const char *, int, const char *);
  int _ttl_(const char *, const char *);
  bool _hset_(const char *, const char *, const char *, const char *);

  const void *_test_context;
};

#endif
