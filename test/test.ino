#include <Arduino.h>
#include <Client.h>

#include <Redis.h>

#include <AUnitVerbose.h>

#include <map>
#include <memory>

#include "TestRawClient.h"

using namespace aunit;

const String gKeyPrefix = String("__arduino_redis__test");

void setup()
{
  const char *include = std::getenv("ARDUINO_REDIS_TEST_INCLUDE");

  if (!include)
  {
    include = "*";
  }

  TestRunner::include(include);
}

void loop()
{
  TestRunner::run();
}

class IntegrationTests : public TestOnce
{
protected:
  void setup() override
  {
    TestOnce::setup();

    const char *r_host = std::getenv("ARDUINO_REDIS_TEST_HOST");
    if (!r_host)
    {
      r_host = "localhost";
    }

    const char *r_port = std::getenv("ARDUINO_REDIS_TEST_PORT");
    if (!r_port)
    {
      r_port = "6379";
    }

    auto r_auth = std::getenv("ARDUINO_REDIS_TEST_AUTH");

    assertNotEqual(client.connect(r_host, std::atoi(r_port)), 0);
    r = std::make_shared<Redis>(client);

    if (r_auth)
    {
      auto auth_ret = r->authenticate(r_auth);
      assertNotEqual(auth_ret, RedisReturnValue::RedisAuthFailure);
    }
  }

  // should really keep track of all keys created during testing and delete them UGH
  // this works well until then:
  // $ redis-cli -a $REDIS_PWD --raw keys "__arduino_redis__test*" 2> /dev/null | xargs -I{} redis-cli -a $REDIS_PWD del {}

  TestRawClient client;
  std::shared_ptr<Redis> r;
};

#define prefixKey(k) (String(gKeyPrefix + "." + k))
#define prefixKeyCStr(k) (String(gKeyPrefix + "." + k).c_str())

#define defineKey(KEY)        \
  auto __ks = prefixKey(KEY); \
  auto k = __ks.c_str();

// Any testF(IntegrationTests, ...) defined will automatically have scope access to `r`, the redis client

testF(IntegrationTests, set)
{
  assertEqual(r->set(prefixKeyCStr("set"), "!"), true);
}

testF(IntegrationTests, setget)
{
  defineKey("setget");

  assertEqual(r->set(k, "!"), true);
  assertEqual(r->get(k), String("!"));
}

testF(IntegrationTests, expire)
{
  defineKey("expire");

  assertEqual(r->set(k, "E"), true);
  assertEqual(r->expire(k, 5), true);
  assertMore(r->ttl(k), 0);
}

testF(IntegrationTests, expire_at)
{
  defineKey("expire_at");

  assertEqual(r->set(k, "E"), true);
  assertEqual(r->expire_at(k, 0), true);
  assertEqual(r->isNilReturn(r->get(k)), true);
}

testF(IntegrationTests, pexpire)
{
  defineKey("pexpire");

  assertEqual(r->set(k, "PE"), true);
  assertEqual(r->pexpire(k, 5000), true);
  assertMore(r->pttl(k), 0);
}

testF(IntegrationTests, pexpire_at)
{
  defineKey("pexpire_at");

  assertEqual(r->set(k, "PEA"), true);
  assertEqual(r->pexpire_at(k, 0), true);
  assertEqual(r->isNilReturn(r->get(k)), true);
}

testF(IntegrationTests, ttl)
{
  defineKey("ttl");

  assertEqual(r->set(k, "T"), true);
  assertEqual(r->expire(k, 100), true);
  assertMore(r->ttl(k), 99);
}

testF(IntegrationTests, pttl)
{
  defineKey("pttl");

  assertEqual(r->set(k, "PT"), true);
  assertEqual(r->pexpire(k, 1000), true);
  // allows for <250ms latency between pexpire & pttl calls
  assertMore(r->pttl(k), 750);
}

testF(IntegrationTests, wait_for_expiry)
{
  defineKey("wait_for_expiry");

  assertEqual(r->set(k, "WFE"), true);
  assertEqual(r->expire(k, 1), true);
  delay(1250); // again, allow <250ms
  assertEqual(r->ttl(k), -2);
}

testF(IntegrationTests, wait_for_expiry_ms)
{
  defineKey("wait_for_expiry_ms");

  assertEqual(r->set(k, "PWFE"), true);
  assertEqual(r->pexpire(k, 1000), true);
  delay(1250); // again, allow <250ms
  assertEqual(r->ttl(k), -2);
}

testF(IntegrationTests, hset)
{
  defineKey("hset");

  // it would be great to be able to assert hset* return values but unless
  // the keys are fully cleaned up (or testing is always done against a fresh redis instance)
  // it's possible this will be an update (returns false)
  // reallllly just need to keep track of and clean up keys... UGH
  r->hset(k, "TF", "H!");
  assertEqual(r->hexists(k, "TF"), true);
}

testF(IntegrationTests, hsetget)
{
  defineKey("hsetget");

  r->hset(k, "TF", "HH");
  assertEqual(r->hget(k, "TF"), "HH");
}

testF(IntegrationTests, hlen)
{
  defineKey("hlen");

  for (int i = 0; i < 10; i++)
  {
    auto fv = String(i);
    r->hset(k, String("field-" + fv).c_str(), fv.c_str());
  }

  assertEqual(r->hlen(k), 10);
}

testF(IntegrationTests, hlen2)
{
  defineKey("hlen2");

  auto lim = random(64) + 64;
  auto key = k + String(":hlen");

  for (auto i = 0; i < lim; i++)
  {
    r->hset(key.c_str(), (String("hlen_test__") + String(i)).c_str(), String(i).c_str());
  }

  assertEqual(r->hlen(key.c_str()), (int)lim);
}

testF(IntegrationTests, hstrlen)
{
  defineKey("hstrlen");

  r->hset(k, "hsr", k);
  assertEqual(r->hstrlen(k, "hsr"), (int)strlen(k));
}

testF(IntegrationTests, hdel)
{
  defineKey("hdel");

  assertEqual(r->hset(k, "delete_me", k), true);
  assertEqual(r->hdel(k, "delete_me"), true);
  assertEqual(r->isNilReturn(r->hget(k, "delete_me")), true);
}

testF(IntegrationTests, append)
{
  assertEqual(r->append(prefixKeyCStr("append"), "foo"), 3);
}

testF(IntegrationTests, exists)
{
  defineKey("exists");

  assertEqual(r->set(k, k), true);
  assertEqual(r->exists(k), true);
}

testF(IntegrationTests, lpush)
{
  defineKey("lpush");

  auto pushRes = r->lpush(k, k);
  assertEqual(pushRes, 1);
  assertEqual(r->llen(k), 1);
  assertEqual(r->lindex(k, pushRes - 1), String(k));
}

testF(IntegrationTests, rpush)
{
  defineKey("rpush");

  auto pushRes = r->rpush(k, k);
  assertEqual(pushRes, 1);
  assertEqual(r->llen(k), 1);
  assertEqual(r->lindex(k, pushRes - 1), String(k));
}

testF(IntegrationTests, lrem)
{
  defineKey("lrem");

  auto pushRes = r->lpush(k, k);
  assertEqual(pushRes, 1);
  assertEqual(r->llen(k), 1);
  assertEqual(r->lindex(k, pushRes - 1), String(k));
  assertEqual(r->lrem(k, 1, k), 1);
}

testF(IntegrationTests, lpop)
{
  defineKey("lpop");

  assertEqual(r->lpush(k, k), 1);
  assertEqual(r->llen(k), 1);
  assertEqual(r->lpop(k), String(k));
  assertEqual(r->llen(k), 0);
}

testF(IntegrationTests, lset)
{
  defineKey("lset");

  assertEqual(r->lpush(k, "foo"), 1);
  assertEqual(r->lset(k, 0, k), true);
  assertEqual(r->lindex(k, 0), String(k));
}

testF(IntegrationTests, ltrim)
{
  defineKey("ltrim");

  assertEqual(r->lpush(k, "bar"), 1);
  assertEqual(r->lpush(k, "bar"), 2);
  assertEqual(r->ltrim(k, -1, 0), true);
  assertEqual(r->llen(k), 0);
}

testF(IntegrationTests, rpop)
{
  defineKey("rpop");

  assertEqual(r->lpush(k, k), 1);
  assertEqual(r->llen(k), 1);
  assertEqual(r->rpop(k), String(k));
  assertEqual(r->llen(k), 0);
}

testF(IntegrationTests, hgetnil)
{
  auto nothing = r->hget(prefixKeyCStr("hgetnil"), "doesNotExist");
  assertEqual(nothing, String("(nil)"));
  assertEqual(r->isNilReturn(nothing), true);
}

testF(IntegrationTests, lindexnil)
{
  auto nothing = r->lindex(prefixKeyCStr("lindexnil"), 0);
  assertEqual(nothing, String("(nil)"));
  assertEqual(r->isNilReturn(nothing), true);
}

/* TODO: re-factor this to something that can be automated!!

#define SUBSCRIBE_TESTS 0
#define SUBSCRIBE_TESTS_ONLY 0
#define BOOTSTRAP_PUB_DELAY_SECS 30
std::map<String, TestFunc> g_SubscribeTests{
    {"subscribe-simple", [](Redis *r, const char *k)
     {
       auto bsChan = String(gKeyPrefix + ":bootstrap");
       auto ackStr = String(k) + ":" + String(random(INT_MAX));

       Serial.printf("Publishing test channel name to \"%s\" in %d seconds...\n", bsChan.c_str(), BOOTSTRAP_PUB_DELAY_SECS);
       delay(BOOTSTRAP_PUB_DELAY_SECS * 1000);
       r->publish(bsChan.c_str(), k);
       Serial.printf("You must publish \"%s\" back to the published channel to complete the test\n", ackStr.c_str());

       r->setTestContext(ackStr.c_str());
       r->subscribe(k);

       auto subRet = r->startSubscribing(
           [](Redis *rconn, String chan, String message)
           {
             auto success = message == String((char *)rconn->getTestContext());

             if (!success)
             {
               Serial.printf("Message RX'ed but was not properly formed!\n");
             }

             rconn->setTestContext((const void *)success);
             rconn->stopSubscribing();
           },
           [](Redis *rconn, RedisMessageError err)
           {
             Serial.printf("!! subscribe error: %d\n", err);
           });

       return subRet == RedisSubscribeSuccess && r->getTestContext() == (const void *)1;
     }}};
*/