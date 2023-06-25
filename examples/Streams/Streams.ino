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

#define STREAMS_KEY "mystream"
#define STREAMS_GROUP "mygroup"
#define STREAMS_CONSUMER "myconsumer"

void test_write(Redis *redis)
{
  char charBuf[60];
  char charField[5] = "name";
  char charValue0[6] = "Sarah";
  char charValue1[5] = "John";
  char charId[20] = "";

  // XADD
  sprintf(charBuf, "XADD %s %s %s %s", STREAMS_KEY, "*", charField, charValue0);
  Serial.println(charBuf);
  String strId = redis->xadd(STREAMS_KEY, "*", charField, charValue0);
  Serial.println(strId);

  int len = strId.length() + 1;
  char bufferId[len];
  strId.toCharArray(charId, len);

  sprintf(charBuf, "XADD %s %s %s %s", STREAMS_KEY, "*", charField, charValue1);
  Serial.println(charBuf);
  Serial.println(redis->xadd(STREAMS_KEY, "*", charField, charValue1));

  // XLEN
  Serial.print("XLEN ");
  Serial.println(STREAMS_KEY);
  Serial.println(redis->xlen(STREAMS_KEY));

  // XDEL
  sprintf(charBuf, "XDEL %s %s", STREAMS_KEY, charId);
  Serial.println(charBuf);
  Serial.println(redis->xdel(STREAMS_KEY, charId));

  // XLEN
  Serial.print("XLEN ");
  Serial.println(STREAMS_KEY);
  Serial.println(redis->xlen(STREAMS_KEY));
}

void test_xgroup(Redis *redis)
{
  char charBuf[60];

  // XGROUP CREATE
  sprintf(charBuf, "XGROUP CREATE %s %s $ MKSTREAM", STREAMS_KEY, STREAMS_GROUP);
  Serial.println(charBuf);
  Serial.println(redis->xgroup_create(STREAMS_KEY, STREAMS_GROUP, "$", true));

  // XGROUP CREATECONSUMER
  sprintf(charBuf, "XGROUP CREATECONSUMER %s %s %s", STREAMS_KEY, STREAMS_GROUP, STREAMS_CONSUMER);
  Serial.println(charBuf);
  Serial.println(redis->xgroup_createconsumer(STREAMS_KEY, STREAMS_GROUP, STREAMS_CONSUMER));

  // XGROUP DESTROY
  sprintf(charBuf, "XGROUP DESTROY %s %s ", STREAMS_KEY, STREAMS_GROUP);
  Serial.println(charBuf);
  Serial.println(redis->xgroup_destroy(STREAMS_KEY, STREAMS_GROUP));
}


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

  test_write(&redis);

  test_xgroup(&redis);

  // close connection
  redisConn.stop();
  Serial.print("Connection closed!");
}

void loop()
{
}
