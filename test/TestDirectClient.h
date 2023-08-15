#include <Arduino.h>
#include <Client.h>

#include <string>

class TestDirectClient : public Client
{
private:
    std::string toSend;

public:
    TestDirectClient(std::string RESPtoSend) : toSend(RESPtoSend) {}

    int connect(IPAddress ip, uint16_t port)
    {
        (void)ip;
        (void)port;
        return 42;
    }

    int connect(const char *host, uint16_t port)
    {
        (void)host;
        (void)port;
        return 42;
    }

    size_t write(uint8_t val)
    {
        (void)val;
        return 0;
    }

    size_t write(const uint8_t *buf, size_t size)
    {
        (void)buf;
        (void)size;
        return 0;
    }

    int available() { return toSend.size(); }

    int read()
    {
        auto retval = toSend.at(0);
        toSend = toSend.substr(1, std::string::npos);
        return retval;
    }

    int read(uint8_t *buf, size_t size)
    {
        if (size > toSend.size())
        {
            size = toSend.size();
        }

        ::memcpy(buf, toSend.c_str(), size);
        toSend = toSend.substr(size, std::string::npos);
        return size;
    }

    int peek()
    {
        return toSend.size() ? toSend.at(0) : -1;
    }

    void flush() {}

    void stop() {}

    uint8_t connected()
    {
        return operator bool();
    }

    virtual operator bool()
    {
        return 1;
    }
};