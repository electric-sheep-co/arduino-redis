#include <Redis.h>
#include <RedisInternal.h>

#include <AUnitVerbose.h>

#include <memory>

#include "./TestRawClient.h"

const String gKeyPrefix = String("__arduino_redis__test");

#define prefixKey(k) (String(gKeyPrefix + "." + k))
#define prefixKeyCStr(k) (String(gKeyPrefix + "." + k).c_str())

#define defineKey(KEY)          \
    auto __ks = prefixKey(KEY); \
    auto key = __ks.c_str();

std::pair<std::shared_ptr<TestRawClient>, std::shared_ptr<Redis>> NewConnection()
{
    std::pair<std::shared_ptr<TestRawClient>, std::shared_ptr<Redis>> ret_val = std::make_pair(nullptr, nullptr);

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

    auto client = std::make_shared<TestRawClient>();
    if (client->connect(r_host, std::atoi(r_port)) == 0)
    {
        return ret_val;
    }

    Client &c_ref = *client.get();
    auto r = std::make_shared<Redis>(c_ref);
    if (r_auth)
    {
        auto auth_ret = r->authenticate(r_auth);
        if (auth_ret == RedisReturnValue::RedisAuthFailure)
        {
            return ret_val;
        }
    }

    return std::make_pair(client, r);
}

// Any testF(IntegrationTests, ...) defined will automatically have scope access to `r`, the redis client
class IntegrationTests : public aunit::TestOnce
{
protected:
    void setup() override
    {
        aunit::TestOnce::setup();
        auto conn_ret = NewConnection();
        assertNotEqual(conn_ret.first.get(), nullptr);
        assertNotEqual(conn_ret.second.get(), nullptr);
        client = std::move(conn_ret.first);
        r = std::move(conn_ret.second);
    }

    void teardown() override
    {
        auto keys = RedisCommand("KEYS", ArgList{String(gKeyPrefix + "*").c_str()}).issue(*client);

        if (keys->type() == RedisObject::Type::Array)
        {
            std::vector<String> as_strings = *dynamic_cast<RedisArray *>(keys.get());
            for (const auto &key : as_strings)
            {
                r->del(key.c_str());
            }
        }

        aunit::TestOnce::teardown();
    }

    std::shared_ptr<TestRawClient> client;
    std::shared_ptr<Redis> r;
};