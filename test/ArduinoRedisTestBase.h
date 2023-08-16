#define ArduinoRedisTestCommonSetupAndLoop                               \
    void setup()                                                         \
    {                                                                    \
        const char *include = std::getenv("ARDUINO_REDIS_TEST_INCLUDE"); \
                                                                         \
        if (!include)                                                    \
        {                                                                \
            include = "*";                                               \
        }                                                                \
                                                                         \
        TestRunner::include(include);                                    \
    }                                                                    \
                                                                         \
    void loop()                                                          \
    {                                                                    \
        TestRunner::run();                                               \
    }
