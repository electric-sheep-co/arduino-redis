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
#define REDIS_PASSWORD "mypassword"

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

    // XADD
    char charBuf[60];
    char charKey[9] = "mystream";
    char charField[5] = "name";
    char charValue0[6] = "Sarah";
    char charValue1[5] = "John";
    char charId[20] = "";

    sprintf(charBuf, "XADD %s %s %s %s", charKey, "*", charField, charValue0);
    Serial.println(charBuf);
    String strId = redis.xadd(charKey, "*", charField, charValue0);
    Serial.println(strId);

    int len = strId.length() + 1;
    char bufferId[len];
    strId.toCharArray(charId, len);

    sprintf(charBuf, "XADD %s %s %s %s", charKey, "*", charField, charValue1);
    Serial.println(charBuf);
    Serial.println(redis.xadd(charKey, "*", charField, charValue1));

    // XLEN
    Serial.print("XLEN ");
    Serial.println(charKey);
    Serial.println(redis.xlen(charKey));

    // XDEL
    sprintf(charBuf, "XDEL %s %s", charKey, charId);
    Serial.println(charBuf);
    Serial.println(redis.xdel(charKey, charId));
  
    // close connection
    redisConn.stop();
    Serial.print("Connection closed!");
}

void loop()
{
}
