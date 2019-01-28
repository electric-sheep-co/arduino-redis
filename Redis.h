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

    RedisReturnValue connect(const char* password = "");
    bool set(const char*, const char*);
    String get(const char*);
    int publish(const char*, const char*);
    void close();

  private:
    const char* addr;
    int port;
    WiFiClient conn;

    String checkError(String);
 };

#endif
