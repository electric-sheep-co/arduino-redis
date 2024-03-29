#include <Arduino.h>
#include <Client.h>

#include <Redis.h>
#include <RedisInternal.h>

#include <AUnitVerbose.h>

#include <map>

#include "../ArduinoRedisTestBase.h"
#include "../IntegrationTestBase.h"

using namespace aunit;

ArduinoRedisTestCommonSetupAndLoop;

testF(IntegrationTests, set)
{
  assertEqual(r->set(prefixKeyCStr("set"), "!"), true);
}

testF(IntegrationTests, setget)
{
  defineKey("setget");

  assertEqual(r->set(key, "!"), true);
  assertEqual(r->get(key), String("!"));
}

testF(IntegrationTests, expire)
{
  defineKey("expire");

  assertEqual(r->set(key, "E"), true);
  assertEqual(r->expire(key, 5), true);
  assertMore(r->ttl(key), 0);
}

testF(IntegrationTests, expire_at)
{
  defineKey("expire_at");

  assertEqual(r->set(key, "E"), true);
  assertEqual(r->expire_at(key, 0), true);
  assertEqual(Redis::isNilReturn(r->get(key)), true);
}

testF(IntegrationTests, pexpire_and_pttl)
{
  defineKey("pexpire");

  assertEqual(r->set(key, "PE"), true);
  assertEqual(r->pexpire(key, 5000), true);
  assertMore(r->pttl(key), 0);
}

testF(IntegrationTests, pexpire_at)
{
  defineKey("pexpire_at");

  assertEqual(r->set(key, "PEA"), true);
  assertEqual(r->pexpire_at(key, 0), true);
  assertEqual(Redis::isNilReturn(r->get(key)), true);
}

testF(IntegrationTests, ttl)
{
  defineKey("ttl");

  assertEqual(r->set(key, "T"), true);
  assertEqual(r->expire(key, 100), true);
  assertMore(r->ttl(key), 99);
}

testF(IntegrationTests, pttl)
{
  defineKey("pttl");

  assertEqual(r->set(key, "PT"), true);
  assertEqual(r->pexpire(key, 1000), true);
  // allows for <250ms latency between pexpire & pttl calls
  assertMore(r->pttl(key), 750);
}

testF(IntegrationTests, wait_for_expiry)
{
  defineKey("wait_for_expiry");

  assertEqual(r->set(key, "WFE"), true);
  assertEqual(r->expire(key, 1), true);
  delay(1250); // again, allow <250ms
  assertEqual(r->ttl(key), -2);
}

testF(IntegrationTests, wait_for_expiry_ms)
{
  defineKey("wait_for_expiry_ms");

  assertEqual(r->set(key, "PWFE"), true);
  assertEqual(r->pexpire(key, 1000), true);
  delay(1250); // again, allow <250ms
  assertEqual(r->ttl(key), -2);
}

testF(IntegrationTests, hset_hexists)
{
  defineKey("hset");

  assertEqual(r->hset(key, "TF", "H!"), true);
  assertEqual(r->hexists(key, "TF"), true);
  assertEqual(r->hdel(key, "TF"), true);
  assertEqual(r->hexists(key, "TF"), false);
}

testF(IntegrationTests, hsetget)
{
  defineKey("hsetget");

  assertEqual(r->hset(key, "TF", "HH"), true);
  assertEqual(r->hget(key, "TF"), "HH");
}

testF(IntegrationTests, hlen)
{
  defineKey("hlen");

  for (int i = 0; i < 10; i++)
  {
    auto fv = String(i);
    r->hset(key, String("field-" + fv).c_str(), fv.c_str());
  }

  assertEqual(r->hlen(key), 10);
}

testF(IntegrationTests, hlen2)
{
  defineKey("hlen2");

  auto lim = random(64) + 64;

  for (auto i = 0; i < lim; i++)
  {
    r->hset(key, (String("hlen_test__") + String(i)).c_str(), String(i).c_str());
  }

  assertEqual(r->hlen(key), (int)lim);
}

testF(IntegrationTests, hstrlen)
{
  defineKey("hstrlen");

  r->hset(key, "hsr", key);
  assertEqual(r->hstrlen(key, "hsr"), (int)strlen(key));
}

testF(IntegrationTests, hdel)
{
  defineKey("hdel");

  assertEqual(r->hset(key, "delete_me", key), true);
  assertEqual(r->hdel(key, "delete_me"), true);
  assertEqual(Redis::isNilReturn(r->hget(key, "delete_me")), true);
}

testF(IntegrationTests, append)
{
  assertEqual(r->append(prefixKeyCStr("append"), "foo"), 3);
}

testF(IntegrationTests, exists)
{
  defineKey("exists");

  assertEqual(r->set(key, key), true);
  assertEqual(r->exists(key), true);
}

testF(IntegrationTests, lpush)
{
  defineKey("lpush");

  auto pushRes = r->lpush(key, key);
  assertEqual(pushRes, 1);
  assertEqual(r->llen(key), 1);
  assertEqual(r->lindex(key, pushRes - 1), String(key));
}

testF(IntegrationTests, rpush)
{
  defineKey("rpush");

  auto pushRes = r->rpush(key, key);
  assertEqual(pushRes, 1);
  assertEqual(r->llen(key), 1);
  assertEqual(r->lindex(key, pushRes - 1), String(key));
}

testF(IntegrationTests, lrem)
{
  defineKey("lrem");

  auto pushRes = r->lpush(key, key);
  assertEqual(pushRes, 1);
  assertEqual(r->llen(key), 1);
  assertEqual(r->lindex(key, pushRes - 1), String(key));
  assertEqual(r->lrem(key, 1, key), 1);
}

testF(IntegrationTests, lrem2)
{
  defineKey("lrem2");
  assertEqual(r->rpush(key, "foo"), 1);
  assertEqual(r->rpush(key, "foo"), 2);
  assertEqual(r->rpush(key, "foo"), 3);
  assertEqual(r->rpush(key, "bar"), 4);
  assertEqual(r->rpush(key, "foo"), 5);
  assertEqual(r->rpush(key, "bar"), 6);
  assertEqual(r->rpush(key, "baz"), 7);
  assertEqual(r->llen(key), 7);

  assertEqual(r->lrem(key, 1, "foo"), "foo");
  assertEqual(r->llen(key), 6);
  assertEqual(r->lrem(key, 2, "foo"), "foo");
  assertEqual(r->llen(key), 4);

  assertEqual(r->lrem(key, 1, "baz"), "baz");
  assertEqual(r->llen(key), 3);
  assertEqual(r->lrem(key, 1, "baz"), 0 /* this needs to be fixed!! Issue #74 */);
  assertEqual(r->llen(key), 3);
  
  assertEqual(r->lrem(key, 0, "bar"), "bar");
  assertEqual(r->llen(key), 1);
  assertEqual(r->lrem(key, 0, "bar"), 0 /* this needs to be fixed!! Issue #74 */);
  assertEqual(r->llen(key), 1);

  assertEqual(r->lpos(key, "foo"), 0);
}

testF(IntegrationTests, lpop)
{
  defineKey("lpop");

  assertEqual(r->lpush(key, key), 1);
  assertEqual(r->llen(key), 1);
  assertEqual(r->lpop(key), String(key));
  assertEqual(r->llen(key), 0);
}

testF(IntegrationTests, lset)
{
  defineKey("lset");

  assertEqual(r->lpush(key, "foo"), 1);
  assertEqual(r->lset(key, 0, key), true);
  assertEqual(r->lindex(key, 0), String(key));
}

testF(IntegrationTests, ltrim)
{
  defineKey("ltrim");

  assertEqual(r->lpush(key, "bar"), 1);
  assertEqual(r->lpush(key, "bar"), 2);
  assertEqual(r->ltrim(key, -1, 0), true);
  assertEqual(r->llen(key), 0);
}

testF(IntegrationTests, lindex)
{
  defineKey("lindex");
  assertEqual(r->rpush(key, "foo"), 1);
  assertEqual(r->rpush(key, "bar"), 2);
  assertEqual(r->lindex(key, 0), "foo");
  assertEqual(r->lindex(key, 1), "bar");
  assertEqual(Redis::isNilReturn(r->lindex(key, 2)), true);
}

testF(IntegrationTests, lpos)
{
  defineKey("lpos");
  assertEqual(r->rpush(key, "foo"), 1);
  assertEqual(r->rpush(key, "bar"), 2);
  assertEqual(r->lpos(key, "foo"), 0);
  assertEqual(r->lpos(key, "bar"), 1);
  assertEqual(r->lpos(key, "baz"), 2147483407 /* this needs to be fixed!! Issue #74 */);
}

testF(IntegrationTests, rpop)
{
  defineKey("rpop");

  assertEqual(r->lpush(key, key), 1);
  assertEqual(r->llen(key), 1);
  assertEqual(r->rpop(key), String(key));
  assertEqual(r->llen(key), 0);
}

testF(IntegrationTests, hgetnil)
{
  auto nothing = r->hget(prefixKeyCStr("hgetnil"), "doesNotExist");
  assertEqual(nothing, String("(nil)"));
  assertEqual(Redis::isNilReturn(nothing), true);
}

testF(IntegrationTests, lindexnil)
{
  auto nothing = r->lindex(prefixKeyCStr("lindexnil"), 0);
  assertEqual(nothing, String("(nil)"));
  assertEqual(Redis::isNilReturn(nothing), true);
}

testF(IntegrationTests, op_vec_string_issue67)
{
  defineKey("op_vec_string_issue67");

  assertEqual(r->rpush(key, "1"), 1);
  assertEqual(r->rpush(key, "2"), 2);
  assertEqual(r->rpush(key, "3"), 3);

  auto list = r->lrange(key, 0, -1);
  assertEqual((int)list.size(), 3);

  assertEqual(list[0], "1");
  assertEqual(list[1], "2");
  assertEqual(list[2], "3");
}

testF(IntegrationTests, del)
{
  defineKey("del");
  assertEqual(r->set(key, "del"), true);
  assertEqual(r->del(key), true);
  assertEqual(r->del(key), false);
}

testF(IntegrationTests, persist)
{
  defineKey("persist");
  assertEqual(r->set(key, "persist"), true);
  assertEqual(r->expire(key, 2), true);
  delay(1000);
  assertEqual(r->persist(key), true);
  delay(2000);
  assertEqual(r->get(key), "persist");
}

testF(IntegrationTests, hsetnx)
{
  defineKey("hsetnx");
  assertEqual(r->hsetnx(key, "hsetnx", key), true);
  assertEqual(r->hsetnx(key, "hsetnx", key), false);
}

testF(IntegrationTests, xadd)
{
  int len = r->xlen("mystream");
  auto id = r->xadd("mystream", "*", "name", "Taryssa");
  assertEqual(r->xlen("mystream"), len + 1);
}

testF(IntegrationTests, xdel)
{
  int len = r->xlen("mystream");
  auto id = r->xadd("mystream", "*", "name", "Serena");
  assertEqual(r->xdel("mystream", String(id).c_str()), 1);
  assertEqual(r->xlen("mystream"), len);
}

testF(IntegrationTests, xread)
{
  auto id1 = r->xadd("mystream", "*", "name", "Jesse");
  auto id2 = r->xadd("mystream", "*", "name", "James");
  std::vector<String> result = r->xread(0, 0, "mystream", String(id1).c_str());
  assertEqual(result[0], "mystream");
  assertEqual(result[1], String(id2));
}

testF(IntegrationTests, xtrim)
{
  int len = r->xlen("mystream");
  auto id1 = r->xadd("mystream", "*", "name", "Dani");
  auto id2 = r->xadd("mystream", "*", "name", "Grace");
  assertEqual(r->xtrim("mystream", "MAXLEN", XtrimCompareExact, 2, 0), len);

  std::vector<String> info_stream = r->xinfo_stream("mystream", false, 0);
  assertEqual(info_stream[0], "length");
  assertEqual(info_stream[1], 2);
}

testF(IntegrationTests, xrange)
{
  auto id1 = r->xadd("mystream", "*", "name", "Serena");
  auto id2 = r->xadd("mystream", "*", "name", "Allison");
  std::vector<String> result = r->xrange("mystream", String(id1).c_str(),
                                        String(id2).c_str(), 0);
  assertEqual(result[0], String(id1));
  assertEqual(result[2], "Serena");
  assertEqual(result[3], String(id2));
  assertEqual(result[5], "Allison");
}

testF(IntegrationTests, xrevrange)
{
  auto id1 = r->xadd("mystream", "*", "name", "Serena");
  auto id2 = r->xadd("mystream", "*", "name", "Allison");
  std::vector<String> result = r->xrevrange("mystream", String(id2).c_str(),
                                        String(id1).c_str(), 0);
  assertEqual(result[0], String(id2));
  assertEqual(result[2], "Allison");
  assertEqual(result[3], String(id1));
  assertEqual(result[5], "Serena");
}

testF(IntegrationTests, xgroup)
{
  assertEqual(r->xgroup_create("stream", "group1", "$", true), true);
  assertEqual(r->xgroup_createconsumer("stream", "group1", "consumer1"), 1);

  std::vector<String> info_consumers = r->xinfo_consumers("stream", "group1");
  assertEqual(info_consumers[0], "name");
  assertEqual(info_consumers[1], "consumer1");

  std::vector<String> info_groups = r->xinfo_groups("stream");
  assertEqual(info_groups[0], "name");
  assertEqual(info_groups[1], "group1");
  assertEqual(info_groups[2], "consumers");
  assertEqual(info_groups[3], 1);

  std::vector<String> info_stream = r->xinfo_stream("stream", false, 0);
  assertEqual(info_stream[14], "groups");
  assertEqual(info_stream[15], 1);

  assertEqual(r->xgroup_destroy("stream", "group1"), 1);
}

testF(IntegrationTests, xpending_xclaim)
{
  auto id1 = r->xadd("stream", "*", "name", "Serena");
  auto id2 = r->xadd("stream", "*", "name", "Allison");
  auto id3 = r->xadd("stream", "*", "name", "Kate");
  assertEqual(r->xgroup_create("stream", "group1", "$", true), true);
  assertEqual(r->xgroup_createconsumer("stream", "group1", "consumer1"), 1);
  assertEqual(r->xgroup_create("stream", "group2", "0-0", true), true);

  std::vector<String> result = r->xreadgroup("group2", "consumer2", 3, 0,
                                      false, "stream", ">");
  assertEqual(result[0], "stream");
  delay(100);

  std::vector<String> xpending = r->xpending("stream", "group2", 0, NULL,
                                              NULL, 0, NULL);
  assertEqual((int)(xpending.size()), 5);
  assertEqual(xpending[0], 1);
  assertEqual(xpending[1], String(id1).c_str());
  assertEqual(xpending[2], String(id3).c_str());
  assertEqual(xpending[3], "consumer2");
  assertEqual(xpending[4], 3);

  std::vector<String> xclaim = r->xclaim("stream", "group2", "consumer2", 25,
                                          String(id1).c_str(), 0, 0, 0, true,
                                          true, NULL);
  assertEqual(xclaim[0], String(id1).c_str());

  assertEqual(r->xack("stream", "group2", String(id1).c_str()), 1);

  std::vector<String> xpending2 = r->xpending("stream", "group2", 0, NULL,
                                              NULL, 0, NULL);
  assertEqual((int)(xpending2.size()), 5);
  assertEqual(xpending2[0], 1);
  assertEqual(xpending2[1], String(id2).c_str());
  assertEqual(xpending2[2], String(id3).c_str());
  assertEqual(xpending2[3], "consumer2");
  assertEqual(xpending2[4], 2);

  std::vector<String> xautoclaim = r->xautoclaim("stream", "group2",
                                                  "consumer2", 25, "0-0", 2,
                                                  false);
  assertEqual((int)(xautoclaim.size()), 7);
  assertEqual(xautoclaim[0], "0-0");
  assertEqual(xautoclaim[1], String(id2).c_str());
  assertEqual(xautoclaim[2], "name");
  assertEqual(xautoclaim[3], "Allison");
  assertEqual(xautoclaim[4], String(id3).c_str());
  assertEqual(xautoclaim[5], "name");
  assertEqual(xautoclaim[6], "Kate");

  assertEqual(r->xgroup_destroy("stream", "group1"), 1);
  assertEqual(r->xgroup_destroy("stream", "group2"), 1);
}
