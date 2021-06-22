#include <Redis.h>
#include <ESP8266WiFi.h>

#include <map>

#include "creds.h"

#define SUBSCRIBE_TESTS 0
#define SUBSCRIBE_TESTS_ONLY 0
#define BOOTSTRAP_PUB_DELAY_SECS 30

// tests are not expected to pass if RETAIN_DATA is set;
// it is useful for debugging the tests only
#define RETAIN_DATA 0

typedef struct
{
  int total;
  int passed;
} TestResults;

typedef bool (*TestFunc)(Redis *, const char *);

TestResults runTests(Redis *redis, String prefix);

const String gKeyPrefix = String("__arduino_redis__test");

/* Tests are NOT executed in definition order! */
std::map<String, TestFunc> g_Tests{
    {"set", [=](Redis *r, const char *k) {
       return r->set(k, "!");
     }},
    {"setget", [=](Redis *r, const char *k) {
       return r->set(k, "!") == 1 && r->get(k) == "!";
     }},
    {"expire", [=](Redis *r, const char *k) {
       return r->set(k, "E") && r->expire(k, 5) && r->ttl(k) > 0;
     }},
    {"expire_at", [=](Redis *r, const char *k) {
       return r->set(k, "E") && r->expire_at(k, 0) && r->get(k) == NULL;
     }},
    {"pexpire", [=](Redis *r, const char *k) {
       return r->set(k, "PE") && r->pexpire(k, 5000) && r->pttl(k) > 0;
     }},
    {"pexpire_at", [=](Redis *r, const char *k) {
       return r->set(k, "PEA") && r->pexpire_at(k, 0) && r->get(k) == NULL;
     }},
    {"persist", [=](Redis *r, const char *k) {
       return r->set(k, "P") && r->expire(k, 5) && r->persist(k) && r->ttl(k) == -1;
     }},
    {"ttl", [=](Redis *r, const char *k) {
       return r->set(k, "T") && r->expire(k, 100) && r->ttl(k) > 99;
     }},
    {"pttl", [=](Redis *r, const char *k) {
       return r->set(k, "PT") && r->pexpire(k, 1000) && r->pttl(k) > 750; // allows for <250ms latency between pexpire & pttl calls
     }},
    {"wait-for-expiry", [=](Redis *r, const char *k) {
       auto sr = r->set(k, "WFE");
       r->expire(k, 1);
       delay(1250); // again, allow <250ms
       auto t = r->ttl(k);
       return sr && t == -2;
     }},
    {"wait-for-expiry-ms", [=](Redis *r, const char *k) {
       auto sr = r->set(k, "PWFE");
       r->pexpire(k, 1000);
       delay(1250); // again, allow <250ms
       auto t = r->ttl(k);
       return sr && t == -2;
     }},
    {"hset", [=](Redis *r, const char *k) {
       return r->hset(k, "TF", "H!") && r->hexists(k, "TF");
     }},
    {"hsetget", [=](Redis *r, const char *k) {
       return r->hset(k, "TF", "HH") && r->hget(k, "TF") == "HH";
     }},
    {"hlen", [=](Redis *r, const char *k) {
       for (int i = 0; i < 10; i++)
       {
         auto fv = String(i);
         r->hset(k, String("field-" + fv).c_str(), fv.c_str());
       }

       return r->hlen(k) == 10;
     }},
    {"hstrlen", [=](Redis *r, const char *k) {
       return r->hset(k, "hsr", k) && r->hstrlen(k, "hsr") == (int)strlen(k);
     }},
    {"hdel", [=](Redis *r, const char *k) {
       return r->hset(k, "delete_me", k) && r->hdel(k, "delete_me") && r->hget(k, "delete_me") == NULL;
     }},
    {"hlen", [=](Redis *r, const char *k) {
       auto lim = random(64) + 64;
       auto key = k + String(":hlen");

       for (auto i = 0; i < lim; i++)
       {
         r->hset(key.c_str(), (String("hlen_test__") + String(i)).c_str(), String(i).c_str());
       }

       auto verif = r->hlen(key.c_str());

       // the generic cleanup doesn't take care of these
       for (auto i = 0; i < lim; i++)
       {
         r->hdel(key.c_str(), (String("hlen_test__") + String(i)).c_str());
       }

       return verif == lim;
     }},
    {"append", [=](Redis *r, const char *k) {
       return r->append(k, "foo") == 3;
     }},
    {"exists", [=](Redis *r, const char *k) {
       return r->set(k, k) && r->exists(k);
     }},
    {"lpush", [=](Redis *r, const char *k) {
       auto pushRes = r->lpush(k, k);
       return pushRes == 1 && r->llen(k) == 1 && String(k) == r->lindex(k, pushRes - 1);
     }},
    {"rpush", [=](Redis *r, const char *k) {
       auto pushRes = r->rpush(k, k);
       return pushRes == 1 && r->llen(k) == 1 && String(k) == r->lindex(k, pushRes - 1);
     }},
    {"lrem", [=](Redis *r, const char *k) {
       auto pushRes = r->lpush(k, k);
       return pushRes == 1 && r->llen(k) == 1 && String(k) == r->lindex(k, pushRes - 1) && r->lrem(k, 1, k) == 1;
     }},
    {"lpop", [=](Redis *r, const char *k) {
       return r->lpush(k, k) == 1 && r->llen(k) == 1 && r->lpop(k) == String(k) && r->llen(k) == 0;
     }},
    {"lset", [=](Redis *r, const char *k) {
       return r->lset(k, 0, k) && r->lindex(k, 0) == String(k);
     }},
    {"ltrim", [=](Redis *r, const char *k) {
       return r->lpush(k, "bar") == 1 && r->lpush(k, "bar") && r->ltrim(k, 0, 0) && r->llen(k) == 0;
     }},
    {"rpop", [=](Redis *r, const char *k) {
       return r->lpush(k, k) == 1 && r->llen(k) == 1 && r->rpop(k) == String(k) && r->llen(k) == 0;
     }}};

std::map<String, TestFunc> g_SubscribeTests{
    {"subscribe-simple", [](Redis *r, const char *k) {
       auto bsChan = String(gKeyPrefix + ":bootstrap");
       auto ackStr = String(k) + ":" + String(random(INT_MAX));

       Serial.printf("Publishing test channel name to \"%s\" in %d seconds...\n", bsChan.c_str(), BOOTSTRAP_PUB_DELAY_SECS);
       delay(BOOTSTRAP_PUB_DELAY_SECS * 1000);
       r->publish(bsChan.c_str(), k);
       Serial.printf("You must publish \"%s\" back to the published channel to complete the test\n", ackStr.c_str());

       r->setTestContext(ackStr.c_str());
       r->subscribe(k);

       auto subRet = r->startSubscribing(
           [](Redis *rconn, String chan, String message) {
             auto success = message == String((char *)rconn->getTestContext());

             if (!success)
             {
               Serial.printf("Message RX'ed but was not properly formed!\n");
             }

             rconn->setTestContext((const void *)success);
             rconn->stopSubscribing();
           },
           [](Redis *rconn, RedisMessageError err) {
             Serial.printf("!! subscribe error: %d\n", err);
           });

       return subRet == RedisSubscribeSuccess && r->getTestContext() == (const void *)1;
     }}};

void setup()
{
  Serial.begin(115200);
  Serial.flush();
  delay(2000);
  Serial.printf("\n\n\n");
  Serial.println("Arduino-Redis tests starting...");

  WiFi.begin(WIFI_SSID, WIFI_PASS);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(1000);
  }

  Serial.print("Connected to network, IP is: ");
  Serial.println(WiFi.localIP());

  WiFiClient rc;
  if (!rc.connect(REDIS_HOST, REDIS_PORT))
  {
    Serial.println("Failed to connect to Redis server!");
    return;
  }

  auto r = new Redis(rc);
  if (!strlen(REDIS_AUTH) || r->authenticate(REDIS_AUTH) == RedisSuccess)
  {
    Serial.printf("Connection is%s authenticated\n", strlen(REDIS_AUTH) ? "" : " NOT");

    Serial.printf("\n%s", r->info("server").c_str());

    randomSeed(analogRead(0));
    auto keyPrefix = gKeyPrefix + ":" + String(random(INT_MAX));

    if (!SUBSCRIBE_TESTS || !SUBSCRIBE_TESTS_ONLY)
    {
      Serial.printf("\nRunning tests (key prefix: \"%s\"):\n", keyPrefix.c_str());
      auto res = runTests(r, keyPrefix, g_Tests);

      Serial.printf("\n\n%s (%d passed / %d total)\n",
                    (res.passed == res.total ? "SUCCESS" : "FAILURE"), res.passed, res.total);
    }
    else
    {
      Serial.println("SUBSCRIBE_TESTS_ONLY is set; skipping all others!");
    }

    if (SUBSCRIBE_TESTS)
    {
      Serial.printf("\nRunning SUBSCRIBE tests:\n\n");
      auto res = runTests(r, keyPrefix, g_SubscribeTests);

      Serial.printf("\n\n%s (%d passed / %d total)\n",
                    (res.passed == res.total ? "SUCCESS" : "FAILURE"), res.passed, res.total);
    }
  }
}

void loop() {}

TestResults runTests(Redis *redis, String prefix, std::map<String, TestFunc> &tests)
{
  auto pFunc = [&prefix](const String &n) { return String(prefix + "." + n); };

  int total = 0, pass = 0;
  for (auto &kv : tests)
  {
    auto tName = kv.first;
    auto tRes = kv.second(redis, pFunc(tName).c_str());
    total++;
    pass += tRes ? 1 : 0;
    Serial.printf("  %s %s\n", tRes ? "✔" : "X", tName.c_str());
    Serial.flush();
  }

  if (!RETAIN_DATA)
  {
    Serial.printf("\nCleaning up (set RETAIN_DATA=1 to skip)... ");
    Serial.flush();

    for (auto &kv : tests)
    {
      (void)redis->del(pFunc(kv.first).c_str());
    }

    Serial.println("done!");
    Serial.flush();
  }

  return {.total = total, .passed = pass};
}
