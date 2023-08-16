#include <Arduino.h>
#include <Client.h>

#include <Redis.h>
#include <RedisInternal.h>

#include <AUnitVerbose.h>

#include "../../../ArduinoRedisTestBase.h"
#include "../../../IntegrationTestBase.h"
#include "../PubSubCommon.h"

#include <sstream>

using namespace aunit;

ArduinoRedisTestCommonSetupAndLoop;

testF(IntegrationTests, publisher)
{
    randomSeed(time(NULL));
    auto sub_key = ChannelName(gKeyPrefix);
    std::stringstream ss;
    ss << sub_key.c_str() << ":" << std::hex << random(pow(2, 31), pow(2, 60));

    r->publish(sub_key.c_str(), ss.str().c_str());
    assertEqual(r->subscribe(ss.str().c_str()), true);
    auto subRet = r->startSubscribing(
        [](Redis *rconn, String chan, String message)
        {
            (void)chan;
            auto cur_time = time(NULL);
            auto message_time = std::atoi(message.c_str());
            rconn->stopSubscribing();

            if (message_time < cur_time - 1 || message_time > cur_time + 1)
            {
                printf("Time match failure! %ld vs %d\n", cur_time, message_time);
                exit(-1); // can't use asserts here...? this "works"
            }
        });

    assertEqual(subRet, RedisSubscribeSuccess);
}