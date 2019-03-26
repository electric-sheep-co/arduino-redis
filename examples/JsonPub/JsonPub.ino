#include <Redis.h>
#include <ArduinoJson.h>

#define WIFI_SSID       "Safado2GH"
#define WIFI_PASSWORD   "Summer2017"

#define REDIS_ADDR      "redis-17772.c60.us-west-1-2.ec2.cloud.redislabs.com"
#define REDIS_PORT      17772
#define REDIS_PASSWORD  "qsBmsA51hnjNkfpQBIDE8gq6dJeTBqFc"

#define POST_FREQUENCY  60

// which analog input we read & include in our post
#define ANALOG_INPUT    A0

// this sketch will build for the ESP8266 or ESP32 platform
#ifdef HAL_ESP32_HAL_H_ // ESP32
#include <WiFiClient.h>
#include <WiFi.h>
#else
#ifdef CORE_ESP8266_FEATURES_H // ESP8266
#include <ESP8266WiFi.h>
#endif
#endif

WiFiClient redisConn;
Redis* gRedis = nullptr;

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

    if (!redisConn.connect(REDIS_ADDR, REDIS_PORT))
    {
        Serial.println("Failed to connect to the Redis server!");
        return;
    }

    gRedis = new Redis(redisConn);
    auto connRet = gRedis->authenticate(REDIS_PASSWORD);
    if (connRet == RedisSuccess)
    {
        Serial.printf("Connected to the Redis server at %s!\n", REDIS_ADDR);
    } 
    else 
    {
        Serial.printf("Failed to authenticate to the Redis server! Errno: %d\n", (int)connRet);
        return;
    }
}

StaticJsonDocument<2048> doc;
unsigned long lastPost = 0;

void loop() {
    auto startTime = millis();
    if (!lastPost || startTime - lastPost > POST_FREQUENCY * 1000) 
    {
        
        doc["analog"] = analogRead(ANALOG_INPUT);
        doc["wifi-rssi"] = WiFi.RSSI();
        doc["uptime-ms"] = startTime;

        String jsonStr;
        serializeJson(doc, Serial);
        Serial.printf("Sending JSON payload:\n\t'%s'\n", jsonStr.c_str());

        auto listeners = gRedis->publish("arduino-redis:jsonpub", jsonStr.c_str());
        Serial.printf("Published to %d listeners\n", listeners);

        doc.clear();
        lastPost = millis();
    }
}
