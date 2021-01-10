#include <Redis.h>
#include <ESP8266WiFi.h>

#include "creds.h"

typedef struct {
    int total;
    int passed;
} TestResults;

typedef bool (*TestFunc)(Redis*, const char*);

/* tests are not expected to pass if retainData = true */
TestResults runTests(Redis *redis, String prefix, bool retainData = false);

void setup() {
  Serial.begin(115200);
  delay(2000);
  Serial.printf("\n\n\n");
  Serial.println("Arduino-Redis tests starting...");

  WiFi.begin(WIFI_SSID, WIFI_PASS);

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
  }

  Serial.print("Connected to network, IP is: ");
  Serial.println(WiFi.localIP());
  
  WiFiClient rc;
  if (!rc.connect(REDIS_HOST, REDIS_PORT)) {
    Serial.println("Failed to connect to Redis server!");
    return;
  }
  
  auto r = new Redis(rc);
  if (!strlen(REDIS_AUTH) || r->authenticate(REDIS_AUTH) == RedisSuccess) {
    Serial.printf("Connection is%s authenticated\n", strlen(REDIS_AUTH) ? "" : " NOT");
    
    randomSeed(analogRead(0));
    auto keyPrefix = String("__arduino_redis__test") + ":" + String(random(INT_MAX));
    auto res = runTests(r, keyPrefix);

    Serial.printf("\n\n%s (%d passed / %d total)\n", 
      (res.passed == res.total ? "SUCCESS": "FAILURE"), res.passed, res.total);
  }
}

void loop() {}

#include <map>

TestResults runTests(Redis *redis, String prefix, bool retainData) 
{
    /* These are NOT executed in definition order! */
    std::map<String, TestFunc> g_Tests 
    {
        { "set", [=](Redis *r, const char* k) { 
            return r->set(k, "!"); 
        } },
        { "setget", [=](Redis *r, const char* k) {
            return r->set(k, "!") == 1 && r->get(k) == "!"; 
        } },
        { "expire", [=](Redis *r, const char* k) { 
            return r->set(k, "E") && r->expire(k, 5) && r->ttl(k) > 0;
        } },
        { "expire_at", [=](Redis *r, const char* k) { 
            return r->set(k, "E") && r->expire_at(k, 0) && r->get(k) == NULL;
        } },
        { "pexpire", [=](Redis *r, const char* k) {
            return r->set(k, "PE") && r->pexpire(k, 5000) && r->pttl(k) > 0;
        } },
        { "pexpire_at", [=](Redis *r, const char* k) { 
            return r->set(k, "E") && r->pexpire_at(k, 0) && r->get(k) == NULL;
        } },
        { "persist", [=](Redis *r, const char* k) { 
            return r->set(k, "E") && r->expire(k, 5) && r->persist(k) && r->ttl(k) == -1;
        } },
        { "ttl", [=](Redis *r, const char* k) { 
            return r->set(k, "E") && r->expire(k, 100) && r->ttl(k) > 99;
        } },
        { "pttl", [=](Redis *r, const char* k) { 
            return r->set(k, "E") && r->pexpire(k, 1000) && r->pttl(k) > 750; // allows up to 250ms latency between pexpire & pttl calls
        } },
        { "dlyexp", [=](Redis *r, const char* k) {
            auto sr = r->set(k, "DE");
            r->expire(k, 10); 
            delay(4000);
            auto t = r->ttl(k);
            return sr && t > 5 && t < 7;
        } },
        { "hset", [=](Redis *r, const char* k) { 
            return r->hset(k, "TF", "H!") && r->hexists(k, "TF"); 
        } },
        { "hsetget", [=](Redis *r, const char* k) { 
            return r->hset(k, "TF", "HH") && r->hget(k, "TF") == "HH"; 
        } },
        { "hlen", [=](Redis *r, const char* k) {
            for (int i = 0; i < 10; i++) {
                auto fv = String(i);
                r->hset(k, String("field-" + fv).c_str(), fv.c_str());
            }

            return r->hlen(k) == 10;
        } },
        { "hstrlen", [=](Redis *r, const char* k) { 
            return r->hset(k, "hsr", k) && r->hstrlen(k, "hsr") == strlen(k); 
        } },
        { "hdel", [=](Redis *r, const char* k) { 
            return r->hset(k, "delete_me", k) && r->hdel(k, "delete_me") && r->hget(k, "delete_me") == NULL; 
        } },
        { "hlen", [=](Redis *r, const char* k) {
          auto lim = random(64) + 64;
          auto key = k + String(":hlen");
          
          for (auto i = 0; i < lim; i++) {
            r->hset(key.c_str(), (String("hlen_test__") + String(i)).c_str(), String(i).c_str());
          }

          auto verif = r->hlen(key.c_str());

          // the generic cleanup doesn't take care of these
          for (auto i = 0; i < lim; i++) {
            r->hdel(key.c_str(), (String("hlen_test__") + String(i)).c_str());
          }
          
          return verif == lim;
        } },
        { "append", [=](Redis *r, const char* k) { 
            return r->append(k, "foo") == 3; 
        } },
        { "exists", [=](Redis *r, const char* k) { 
            return r->set(k, k) && r->exists(k); 
        } }
    };
    
    Serial.printf("\nRunning tests (key prefix: \"%s\"):\n", prefix.c_str());

    auto pFunc = [&prefix](const String& n) { return String(prefix + "." + n); };

    int total = 0, pass = 0;
    for (auto& kv : g_Tests)
    {
        auto tName = kv.first;
        auto tRes = kv.second(redis, pFunc(tName).c_str());
        total++;
        pass += tRes ? 1 : 0;
        Serial.printf("  %s %s\n", tRes ? "✔" : "X", tName.c_str());
    }

    if (!retainData) {
      Serial.printf("\nCleaning up (set retainData=true to skip)...\n");
        for (auto& kv : g_Tests) {
            auto rmed = redis->del(pFunc(kv.first).c_str());
            Serial.printf("  %s %s\n", rmed ? "✔" : "X", kv.first.c_str());
        }
    }

    return { .total = total, .passed = pass };
}
