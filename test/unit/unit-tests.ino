#include <Arduino.h>
#include <Client.h>

#include <Redis.h>
#include <RedisInternal.h>

#include <AUnitVerbose.h>

#include "../ArduinoRedisTestBase.h"
#include "../TestDirectClient.h"

using namespace aunit;

ArduinoRedisTestCommonSetupAndLoop;

// creates a local TestDirectClient named `__client` and a local
// std::shared_ptr<RedisObject> named `parsed`
// defined as a macro so that it can take advantage of AUnit functions
// only available in test-function scope.
#define parseRESP2String(RESPtoParse)                        \
  TestDirectClient __client(RESPtoParse);                    \
  auto parsed = RedisObject::parseTypeNonBlocking(__client); \
  assertNotEqual(parsed.get(), nullptr);

// taken directly from the "Nested arrays" portion of
// https://redis.io/docs/reference/protocol-spec/#resp-arrays
// it encodes a two-element Array consisting of an Array that
// contains three Integers (1, 2, 3) and an array of a Simple
// String and an Error
const std::string nested_array_vector = "*2\r\n*3\r\n:1\r\n:2\r\n:3\r\n*2\r\n+Hello\r\n-World\r\n";

test(UnitTests, nested_array)
{
  parseRESP2String(nested_array_vector);
  assertEqual(parsed->type(), RedisObject::Type::Array);

  std::vector<std::shared_ptr<RedisObject>> ptrs = *(RedisArray *)parsed.get();
  // a two-element Array
  assertEqual(ptrs.size(), (size_t)2);

  // first, an Array with three Integers (1, 2, 3)
  auto first = ptrs[0];
  assertNotEqual(first.get(), nullptr);
  assertEqual(first->type(), RedisObject::Type::Array);
  std::vector<std::shared_ptr<RedisObject>> first_ptrs = *(RedisArray *)first.get();
  assertEqual(first_ptrs.size(), (size_t)3);
  assertEqual(first_ptrs[0]->type(), RedisObject::Type::Integer);
  assertEqual(first_ptrs[1]->type(), RedisObject::Type::Integer);
  assertEqual(first_ptrs[2]->type(), RedisObject::Type::Integer);
  assertEqual(((String) * (RedisObject *)first_ptrs[0].get()).c_str(), "1");
  assertEqual(((String) * (RedisObject *)first_ptrs[1].get()).c_str(), "2");
  assertEqual(((String) * (RedisObject *)first_ptrs[2].get()).c_str(), "3");

  // second, an array of a Simple String and an Error
  auto second = ptrs[1];
  assertNotEqual(second.get(), nullptr);
  assertEqual(second->type(), RedisObject::Type::Array);
  std::vector<std::shared_ptr<RedisObject>> second_ptrs = *(RedisArray *)second.get();
  assertEqual(second_ptrs.size(), (size_t)2);
  assertEqual(second_ptrs[0]->type(), RedisObject::Type::SimpleString);
  assertEqual(second_ptrs[1]->type(), RedisObject::Type::Error);
  assertEqual(((String) * (RedisObject *)second_ptrs[0].get()).c_str(), "Hello");
  assertEqual(((String) * (RedisObject *)second_ptrs[1].get()).c_str(), "World");
}

test(UnitTests, nested_array_as_strings)
{
  parseRESP2String(nested_array_vector);
  assertEqual(parsed->type(), RedisObject::Type::Array);

  std::vector<String> as_strings = *(RedisArray *)parsed.get();
  assertEqual(as_strings.size(), (size_t)5);

  auto expected = std::vector<String>{"1", "2", "3", "Hello", "World"};
  for (std::vector<String>::size_type i = 0; i < as_strings.size(); i++)
  {
    assertEqual(as_strings[i].c_str(), expected[i].c_str());
  }
}

test(UnitTests, empty_array)
{
  parseRESP2String("*0\r\n");
  assertEqual(parsed->type(), RedisObject::Type::Array);
  assertEqual(dynamic_cast<RedisArray *>(parsed.get())->operator std::vector<String>().size(), (size_t)0);
}

test(UnitTests, mixed_types_array)
{
  parseRESP2String("*3\r\n:1\r\n+OK\r\n$5\r\nhello\r\n");
  auto vec = dynamic_cast<RedisArray *>(parsed.get())->operator std::vector<std::shared_ptr<RedisObject>>();
  assertEqual(vec.size(), (size_t)3);

  assertEqual(vec[0]->type(), RedisObject::Type::Integer);
  assertEqual(dynamic_cast<RedisInteger *>(vec[0].get())->operator int(), 1);

  assertEqual(vec[1]->type(), RedisObject::Type::SimpleString);
  assertEqual(dynamic_cast<RedisSimpleString *>(vec[1].get())->operator String(), String("OK"));

  assertEqual(vec[2]->type(), RedisObject::Type::BulkString);
  assertEqual(dynamic_cast<RedisBulkString *>(vec[2].get())->operator String(), String("hello"));
}

test(UnitTests, null_array)
{
  parseRESP2String("*-1\r\n");
  assertEqual(parsed->type(), RedisObject::Type::Array);
  auto asArray = dynamic_cast<RedisArray *>(parsed.get());
  auto vec = asArray->operator std::vector<std::shared_ptr<RedisObject>>();
  assertEqual(asArray->isNilReturn(), true);
}

test(UnitTests, array_with_null_bulkstring)
{
  parseRESP2String("*3\r\n$5\r\nhello\r\n$-1\r\n$5\r\nworld\r\n");
  assertEqual(parsed->type(), RedisObject::Type::Array);
  auto vec = dynamic_cast<RedisArray *>(parsed.get())->operator std::vector<std::shared_ptr<RedisObject>>();
  assertEqual(vec.size(), (size_t)3);

  assertEqual(vec[0]->type(), RedisObject::Type::BulkString);
  assertEqual(dynamic_cast<RedisBulkString *>(vec[0].get())->operator String(), String("hello"));

  assertEqual(vec[1]->type(), RedisObject::Type::BulkString);
  assertEqual(Redis::isNilReturn(dynamic_cast<RedisBulkString *>(vec[1].get())->operator String()), true);

  assertEqual(vec[2]->type(), RedisObject::Type::BulkString);
  assertEqual(dynamic_cast<RedisBulkString *>(vec[2].get())->operator String(), String("world"));
}

test(UnitTests, simple_string_ok)
{
  parseRESP2String("+OK\r\n");
  assertEqual(parsed->type(), RedisObject::Type::SimpleString);
  assertEqual(parsed->operator String().c_str(), "OK");
}

test(UnitTests, simple_error)
{
  parseRESP2String("-Error message\r\n");
  assertEqual(parsed->type(), RedisObject::Type::Error);
  assertEqual(parsed->operator String().c_str(), "Error message");
}

test(UnitTests, integers)
{
  std::vector<std::pair<String, int>> test_vectors{
      std::make_pair(":0\r\n", 0),
      std::make_pair(":-0\r\n", 0),
      std::make_pair(":+0\r\n", 0),
      std::make_pair(":1000\r\n", 1000),
      std::make_pair(":-1000\r\n", -1000),
      std::make_pair(":+1000\r\n", 1000),
      std::make_pair(":2147483647\r\n", 2147483647),
      std::make_pair(":2147483648\r\n", -2147483648), // wrap around
      std::make_pair(":-2147483648\r\n", -2147483648),
      std::make_pair(":-2147483649\r\n", 2147483647), // wrap around
  };

  for (const auto &test_vec : test_vectors)
  {
    parseRESP2String(test_vec.first.c_str());
    assertEqual(parsed->type(), RedisObject::Type::Integer);
    assertEqual(dynamic_cast<RedisInteger *>(parsed.get())->operator int(), test_vec.second);
  }
}

test(UnitTests, bulk_strings)
{
  std::vector<std::pair<String, String>> test_vectors{
      std::make_pair("$5\r\nhello\r\n", "hello"),
      std::make_pair("$0\r\n\r\n", ""),
      std::make_pair("$3\r\n\x01\x02\x03\r\n", "\x01\x02\x03")};

  for (const auto &test_vec : test_vectors)
  {
    parseRESP2String(test_vec.first.c_str());
    assertEqual(parsed->type(), RedisObject::Type::BulkString);
    assertEqual(parsed->operator String(), test_vec.second);
  }
}

test(UnitTests, null_bulk_string)
{
  parseRESP2String("$-1\r\n");
  assertEqual(parsed->type(), RedisObject::Type::BulkString);
  assertEqual(Redis::isNilReturn(parsed->operator String()), true);
}

test(UnitTests, non_RESP_data)
{
  std::vector<String> vectors{
      "Foo bar baz",
      "Hello world!",
      "\x01\x02\x03",
      "2d9c9124-ca30-4d5b-b1ba-b757d08cdb03"};

  for (const auto &fuzz_vec : vectors)
  {
    parseRESP2String(fuzz_vec.c_str());
    assertEqual(parsed->type(), RedisObject::Type::InternalError);
  }
}