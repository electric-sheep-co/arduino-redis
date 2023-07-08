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
#define STREAMS_GROUP_1 "group-1"
#define STREAMS_GROUP_2 "group-2"
#define STREAMS_CONSUMER_1 "consumer-1"
#define STREAMS_CONSUMER_2 "consumer-2"

void print_vector(std::vector<String> output)
{
  if(output.size() > 0) {
    for(uint i = 0; i < output.size(); i++) {
      Serial.println(output.at(i));
    }
  } else {
    Serial.println("Error: No info!");
  }
}

void test_write(Redis *redis)
{
  char charBuf[60];
  char charField[5] = "name";
  char charValues[4][7] = {"Sarah", "John", "Ginger", "Kyle"};
  char charId[20] = "";

  // XADD
  sprintf(charBuf, "XADD %s %s %s %s", STREAMS_KEY, "*", charField,
    charValues[0]);
  Serial.println(charBuf);
  String strId = redis->xadd(STREAMS_KEY, "*", charField, charValues[0]);
  Serial.println(strId);

  int len = strId.length() + 1;
  strId.toCharArray(charId, len);

  for(uint i = 1; i < 4; i++)
  {
    sprintf(charBuf, "XADD %s %s %s %s", STREAMS_KEY, "*", charField,
      charValues[i]);
    Serial.println(charBuf);
    Serial.println(redis->xadd(STREAMS_KEY, "*", charField, charValues[i]));
  }

  // XLEN
  Serial.print("XLEN ");
  Serial.println(STREAMS_KEY);
  Serial.println(redis->xlen(STREAMS_KEY));

  // XREAD
  sprintf(charBuf, "XREAD COUNT 2 BLOCK 100 STREAMS %s %s", STREAMS_KEY, charId);
  Serial.println(charBuf);
  print_vector(redis->xread(2, 100, STREAMS_KEY, charId));

  // XDEL
  sprintf(charBuf, "XDEL %s %s", STREAMS_KEY, charId);
  Serial.println(charBuf);
  Serial.println(redis->xdel(STREAMS_KEY, charId));

  // XTRIM
  sprintf(charBuf, "XTRIM %s %s %c %d", STREAMS_KEY, "MAXLEN",
    char(XtrimCompareExact), 2);
  Serial.println(charBuf);
  Serial.println(redis->xtrim(STREAMS_KEY, "MAXLEN", XtrimCompareExact, 2, 0));

  // XLEN
  Serial.print("XLEN ");
  Serial.println(STREAMS_KEY);
  Serial.println(redis->xlen(STREAMS_KEY));

  // XRANGE
  sprintf(charBuf, "XRANGE %s - +", STREAMS_KEY);
  Serial.println(charBuf);
  print_vector(redis->xrange(STREAMS_KEY, "-", "+", 0));

  // XREVRANGE
  sprintf(charBuf, "XREVRANGE %s + -", STREAMS_KEY);
  Serial.println(charBuf);
  print_vector(redis->xrevrange(STREAMS_KEY, "+", "-", 0));
}

void test_xgroup(Redis *redis)
{
  char charBuf[60];

  // XGROUP CREATE
  sprintf(charBuf, "XGROUP CREATE %s %s $ MKSTREAM", STREAMS_KEY, STREAMS_GROUP_1);
  Serial.println(charBuf);
  Serial.println(redis->xgroup_create(STREAMS_KEY, STREAMS_GROUP_1, "$", true));

  // XGROUP CREATECONSUMER
  sprintf(charBuf, "XGROUP CREATECONSUMER %s %s %s", STREAMS_KEY, STREAMS_GROUP_1,
    STREAMS_CONSUMER_1);
  Serial.println(charBuf);
  Serial.println(redis->xgroup_createconsumer(STREAMS_KEY, STREAMS_GROUP_1,
    STREAMS_CONSUMER_1));

  // XINFO CONSUMERS
  sprintf(charBuf, "XINFO CONSUMERS %s %s", STREAMS_KEY, STREAMS_GROUP_1);
  Serial.println(charBuf);
  print_vector(redis->xinfo_consumers(STREAMS_KEY, STREAMS_GROUP_1));

  // XINFO GROUPS
  sprintf(charBuf, "XINFO GROUPS %s", STREAMS_KEY);
  Serial.println(charBuf);
  print_vector(redis->xinfo_groups(STREAMS_KEY));

  // XINFO STREAM
  sprintf(charBuf, "XINFO STREAM %s FULL 1", STREAMS_KEY);
  Serial.println(charBuf);
  print_vector(redis->xinfo_stream(STREAMS_KEY, true, 1));
}

void test_xclaim(Redis *redis)
{
  char charBuf[100];
  char charId[24];
  std::vector<String> pending;

  // XGROUP CREATE
  sprintf(charBuf, "XGROUP CREATE %s %s 0-0", STREAMS_KEY, STREAMS_GROUP_2);
  Serial.println(charBuf);
  Serial.println(redis->xgroup_create(STREAMS_KEY, STREAMS_GROUP_2, "0-0", true));

  // XREADGROUP
  sprintf(charBuf, "XREADGROUP GROUP %s %s COUNT 1 STREAMS %s >", STREAMS_GROUP_2,
    STREAMS_CONSUMER_2, STREAMS_KEY);
  Serial.println(charBuf);
  print_vector(redis->xreadgroup(STREAMS_GROUP_2, STREAMS_CONSUMER_2, 1, 0,
      false, STREAMS_KEY, ">"));

  // XPENDING
  sprintf(charBuf, "XPENDING %s %s - + 1", STREAMS_KEY, STREAMS_GROUP_2);
  Serial.println(charBuf);
  pending = redis->xpending(STREAMS_KEY, STREAMS_GROUP_2, 0, "-", "+", 1, NULL);
  print_vector(pending);

  sprintf(charBuf, "XPENDING %s %s", STREAMS_KEY, STREAMS_GROUP_2);
  Serial.println(charBuf);
  pending = redis->xpending(STREAMS_KEY, STREAMS_GROUP_2, 0, NULL, NULL, 0, NULL);
  print_vector(pending);

  // XCLAIM
  String pendingId = pending[2].substring(3,22);
  pendingId.toCharArray(charId, pendingId.length() + 1);
  sprintf(charBuf, "XCLAIM %s %s %s 10 %s", STREAMS_KEY, STREAMS_GROUP_2,
      STREAMS_CONSUMER_2, charId);
  Serial.println(charBuf);
  print_vector(redis->xclaim(STREAMS_KEY, STREAMS_GROUP_2, STREAMS_CONSUMER_2,
      10, charId, 0, 0, 0, true, true, NULL));

  // XAUTOCLAIM
  sprintf(charBuf, "XAUTOCLAIM %s %s %s 100 0-0 COUNT 1", STREAMS_KEY,
      STREAMS_GROUP_2, STREAMS_CONSUMER_2);
  Serial.println(charBuf);
  print_vector(redis->xautoclaim(STREAMS_KEY, STREAMS_GROUP_2, STREAMS_CONSUMER_2,
      100, "0-0", 1, false));

  // XACK
  sprintf(charBuf, "XACK %s %s %s", STREAMS_KEY, STREAMS_GROUP_2, charId);
  Serial.println(charBuf);
  Serial.println(redis->xack(STREAMS_KEY, STREAMS_GROUP_2, charId));
}

void test_xgroup_destroy(Redis *redis)
{
  char charBuf[60];

  // XGROUP DESTROY
  sprintf(charBuf, "XGROUP DESTROY %s %s ", STREAMS_KEY, STREAMS_GROUP_1);
  Serial.println(charBuf);
  Serial.println(redis->xgroup_destroy(STREAMS_KEY, STREAMS_GROUP_1));
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

  test_xclaim(&redis);

  test_xgroup_destroy(&redis);

  // close connection
  redisConn.stop();
  Serial.print("Connection closed!");
}

void loop()
{
}
