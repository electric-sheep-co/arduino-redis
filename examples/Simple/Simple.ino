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

#define WIFI_SSID "SSID"
#define WIFI_PASSWORD "PASSWORD"

#define REDIS_ADDR "127.0.0.1"
#define REDIS_PORT 6379
#define REDIS_PASSWORD ""

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

    Serial.print("SET foo bar: ");
    if (redis.set("foo", "bar"))
    {
        Serial.println("ok!");
    }
    else
    {
        Serial.println("err!");
    }

    Serial.print("GET foo: ");
    Serial.println(redis.get("foo"));

    redisConn.stop();
    Serial.print("Connection closed!");
}

void loop()
{
}
