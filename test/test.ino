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
  Serial.println("Connecting...");

  WiFi.begin(WIFI_SSID, WIFI_PASS);

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
  }

  Serial.print("Connected! IP is: ");
  Serial.println(WiFi.localIP());
  
  WiFiClient rc;
  if (!rc.connect(REDIS_HOST, REDIS_PORT)) {
    Serial.println("Failed to connect to Redis server!");
    return;
  }
  
  auto r = new Redis(rc);
  if (!strlen(REDIS_AUTH) || r->authenticate(REDIS_AUTH) == RedisSuccess) {
    auto res = runTests(r, "__arduino_redis__test");
    Serial.printf("%d tests passed of %d total run.\n", res.passed, res.total);
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
            return r->set(k, "E") && r->expire(k, 5) && r->ttl(k) > 4;
        } },
        { "pexpire", [=](Redis *r, const char* k) {
            return r->set(k, "PE") && r->pexpire(k, 5000) && r->pttl(k) > 4500;
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
        { "append", [=](Redis *r, const char* k) { 
            return r->append(k, "foo") == 3; 
        } },
        { "exists", [=](Redis *r, const char* k) { 
            return r->set(k, k) && r->exists(k); 
        } }
    };
    
    Serial.printf("Redis::run_tests(\"%s\") starting...\n", prefix.c_str());

    auto pFunc = [&prefix](const String& n) { return String(prefix + "." + n); };

    int total = 0, pass = 0;
    for (auto& kv : g_Tests)
    {
        auto tName = kv.first;
        auto tRes = kv.second(redis, pFunc(tName).c_str());
        total++;
        pass += tRes ? 1 : 0;
        Serial.printf("\tTest \"%s\":\t%s\n", tName.c_str(), tRes ? "pass" : "FAIL");
    }

    Serial.printf("Result: %d passed / %d total\n", pass, total);

    if (!retainData) {
        for (auto& kv : g_Tests) {
            (void)redis->del(pFunc(kv.first).c_str());
        }
    }

    return { .total = total, .passed = pass };
}
