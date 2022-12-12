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

#define WIFI_SSID ""
#define WIFI_PASSWORD ""

#define REDIS_ADDR "0.0.0.0"
#define REDIS_PORT 6379
#define REDIS_PASSWORD "password"

#define MAX_BACKOFF 300000 // 5 minutes

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

    auto backoffCounter = -1;
    auto resetBackoffCounter = [&]() { backoffCounter = 0; };

    resetBackoffCounter();
    while (subscriberLoop(resetBackoffCounter))
    {
        auto curDelay = min((1000 * (int)pow(2, backoffCounter)), MAX_BACKOFF);

        if (curDelay != MAX_BACKOFF)
        {
            ++backoffCounter;
        }

        Serial.printf("Waiting %lds to reconnect...\n", curDelay / 1000);
        delay(curDelay);
    }

    Serial.printf("Done!\n");
}

// returning 'true' indicates the failure was retryable; false is fatal
bool subscriberLoop(std::function<void(void)> resetBackoffCounter)
{
    WiFiClient redisConn;
    if (!redisConn.connect(REDIS_ADDR, REDIS_PORT))
    {
        Serial.println("Failed to connect to the Redis server!");
        return true;
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
        return false;
    }

    redis.subscribe("foo");
    redis.subscribe("bar");

    redis.psubscribe("ctrl-*");

    Serial.println("Listening...");
    resetBackoffCounter();

    auto subRv = redis.startSubscribingNonBlocking(
        [=](Redis *redisInst, String channel, String msg) {
            Serial.printf("Message on channel '%s': \"%s\"\n", channel.c_str(), msg.c_str());

            if (channel == "ctrl-close")
            {
                Serial.println("Got message on ctrl-close: ending!");
                redisInst->stopSubscribing();
            }
            else if (channel == "ctrl-add")
            {
                Serial.printf("Adding subscription to channel '%s'\n", msg.c_str());

                if (!redisInst->subscribe(msg.c_str()))
                {
                    Serial.println("Failed to add subscription!");
                }
            }
            else if (channel == "ctrl-rm")
            {
                Serial.printf("Removing subscription to channel '%s'\n", msg.c_str());

                if (!redisInst->unsubscribe(msg.c_str()))
                {
                    Serial.println("Failed to remove subscription!");
                }
            }
        },
        [=]() {
            loop();
        },
        [=](Redis *redisInst, RedisMessageError err) {
            Serial.printf("Subscription error! '%d'\n", err);
        });

    redisConn.stop();
    Serial.printf("Connection closed! (%d)\n", subRv);
    return subRv == RedisSubscribeServerDisconnected; // server disconnect is retryable, everything else is fatal
}

void loop() {
    delay(1000);
    Serial.println("loop");
}