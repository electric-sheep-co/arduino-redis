#include <Arduino.h>
#include <Client.h>

#include <Redis.h>
#include <RedisInternal.h>

#include <AUnitVerbose.h>

#include "../../../ArduinoRedisTestBase.h"
#include "../../../IntegrationTestBase.h"
#include "../PubSubCommon.h"

using namespace aunit;

ArduinoRedisTestCommonSetupAndLoop;

testF(IntegrationTests, subscriber)
{
    auto sub_key = ChannelName(gKeyPrefix);
    assertEqual(r->subscribe(sub_key.c_str()), true);
    auto subRet = r->startSubscribing(
        [](Redis *rconn, String chan, String message)
        {
            (void)chan;
            rconn->stopSubscribing();
            delay(1000);

            auto send_client = NewConnection();
            if (!send_client.second.get())
            {
                return;
            }

            auto cur_time = time(NULL);
            std::stringstream ss;
            ss << cur_time;
            send_client.second->publish(message.c_str(), ss.str().c_str());
        });

    assertEqual(subRet, RedisSubscribeSuccess);
}