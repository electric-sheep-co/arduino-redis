#include <Redis.h>

// this sketch will build for the ESP8266 or ESP32 platform
#ifdef HAL_ESP32_HAL_H_ // ESP32
#include <WiFiClient.h>
#include <WiFi.h>
#else
#ifdef CORE_ESP8266_FEATURES_H // ESP8266
#include <ESP8266WiFi.h>
#endif
#endif

#define WIFI_SSID       ""
#define WIFI_PASSWORD   ""

#define REDIS_ADDR      "0.0.0.0"
#define REDIS_PORT      6379
#define REDIS_PASSWORD  "password"

void setup() 
{
    Serial.begin(115200);
    Serial.println();

    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    Serial.print("Connecting to the WiFi");
    while (WiFi.status() != WL_CONNECTED) 
    {
        delay(250);
        Serial.print(".");
    }
    Serial.println();
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());

    WiFiClient redisConn;
    if (!redisConn.connect(REDIS_ADDR, REDIS_PORT))
    {
        Serial.println("Failed to connect to the Redis server!");
        return;
    }

    Redis redis(redisConn);
    auto connRet = redis.authenticate(REDIS_PASSWORD);
    if (connRet == RedisSuccess)
    {
        Serial.println("Connected to the Redis server!");
    } 
    else 
    {
        Serial.printf("Failed to authenticate to the Redis server! Errno: %d\n", (int)connRet);
        return;
    }

    redis.subscribe("foo");
    redis.subscribe("bar");

    redis.psubscribe("ctrl-*");

    Serial.println("Listening...");

    auto subRv = redis.startSubscribing(
      [=](Redis* redisInst, String channel, String msg) 
      {
        Serial.printf("Message on channel '%s': \"%s\"\n", channel.c_str(), msg.c_str());
      
        if (channel == "ctrl-close")
        {
          Serial.println("Got message on ctrl-close: ending!");
          redisInst->stopSubscribing();
        } else if (channel == "ctrl-add")
        {
          Serial.printf("Adding subscription to channel '%s'\n", msg.c_str());

          if (!redisInst->subscribe(msg.c_str()))
          {
            Serial.println("Failed to add subscription!");
          }
        } else if (channel == "ctrl-rm")
        {
          Serial.printf("Removing subscription to channel '%s'\n", msg.c_str());

          if (!redisInst->unsubscribe(msg.c_str()))
          {
            Serial.println("Failed to remove subscription!");
          }
        }
      },
      [=](Redis* redisInst, RedisMessageError err) 
      {
        Serial.printf("Subscription error! '%d'\n", err);
      }
    );

    if (subRv) 
    {
      Serial.printf("Subscription setup failure: %d\n", subRv);
    }

    Serial.println("Done!");
    redisConn.stop();
    Serial.print("Connection closed!");
}

void loop() {}