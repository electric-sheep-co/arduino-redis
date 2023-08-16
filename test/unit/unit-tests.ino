#include <Arduino.h>
#include <Client.h>

#include <Redis.h>
#include <RedisInternal.h>

#include <AUnitVerbose.h>

#include "../TestDirectClient.h"

using namespace aunit;

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

// taken directly from the "Nested arrays" portion of
// https://redis.io/docs/reference/protocol-spec/#resp-arrays
// it encodes a two-element Array consisting of an Array that 
// contains three Integers (1, 2, 3) and an array of a Simple 
// String and an Error
const std::string nested_array_vector = "*2\r\n*3\r\n:1\r\n:2\r\n:3\r\n*2\r\n+Hello\r\n-World\r\n";

test(UnitTests, nested_array)
{
  TestDirectClient client(nested_array_vector);
  auto parsed = RedisObject::parseTypeNonBlocking(client);
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
  assertEqual(((String)*(RedisObject*)first_ptrs[0].get()).c_str(), "1");
  assertEqual(((String)*(RedisObject*)first_ptrs[1].get()).c_str(), "2");
  assertEqual(((String)*(RedisObject*)first_ptrs[2].get()).c_str(), "3");

  // second, an array of a Simple String and an Error
  auto second = ptrs[1];
  assertNotEqual(second.get(), nullptr);
  assertEqual(second->type(), RedisObject::Type::Array);
  std::vector<std::shared_ptr<RedisObject>> second_ptrs = *(RedisArray *)second.get();
  assertEqual(second_ptrs.size(), (size_t)2);
  assertEqual(second_ptrs[0]->type(), RedisObject::Type::SimpleString);
  assertEqual(second_ptrs[1]->type(), RedisObject::Type::Error);
  assertEqual(((String)*(RedisObject*)second_ptrs[0].get()).c_str(), "Hello");
  assertEqual(((String)*(RedisObject*)second_ptrs[1].get()).c_str(), "World");
}

test(UnitTests, nested_array_as_strings)
{
  TestDirectClient client(nested_array_vector);
  auto parsed = RedisObject::parseTypeNonBlocking(client);
  assertEqual(parsed->type(), RedisObject::Type::Array);

  std::vector<String> as_strings = *(RedisArray *)parsed.get();
  assertEqual(as_strings.size(), (size_t)5);
  
  auto expected = std::vector<String>{"1", "2", "3", "Hello", "World"};
  for (std::vector<String>::size_type i = 0; i < as_strings.size(); i++) {
    assertEqual(as_strings[i].c_str(), expected[i].c_str());
  }
}