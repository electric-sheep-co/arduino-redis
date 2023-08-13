#include <Arduino.h>
#include <Client.h>

#include <climits>
#include <sstream>

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>

class TestRawClient : public Client
{
public:
  TestRawClient() {}

  int connect(IPAddress ip, uint16_t port)
  {
    (void)ip;
    (void)port;
    return -1;
  }

  int connect(const char *host, uint16_t port)
  {
    struct addrinfo hints, *res;
    int status;
    std::stringstream ss;
    ss << port;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    if ((status = getaddrinfo(host, ss.str().c_str(), &hints, &res)) != 0)
    {
      fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
      return 0;
    }

    sock_fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);

    if (sock_fd < 0)
    {
      fprintf(stderr, "socket\n");
      return 0;
    }

    if (::connect(sock_fd, res->ai_addr, res->ai_addrlen) == -1)
    {
      perror("connect");
      return 0;
    }

    return 1;
  };

  size_t write(uint8_t val)
  {
    return write(&val, sizeof(val));
  }

  size_t write(const uint8_t *buf, size_t size)
  {
    if (sock_fd < 0)
    {
      return 0;
    }

    return ::send(sock_fd, buf, size, 0);
  }

  // RedisObject::parseTypeNonBlocking() calls this but does not
  // use the return value, so *for now* this is OK...
  int available() { return 1; }

  int read()
  {
    uint8_t buf;

    if (::recv(sock_fd, &buf, sizeof(buf), 0) > 0)
    {
      return buf;
    }

    return -1;
  }

  int read(uint8_t *buf, size_t size)
  {
    return ::recv(sock_fd, buf, size, 0);
  }

  int peek() { return 0; }

  void flush() {}

  void stop()
  {
    ::close(sock_fd);
  }

  uint8_t connected()
  {
    return operator bool();
  }

  virtual operator bool()
  {
    return (sock_fd != -1);
  }

private:
  int sock_fd = -1;
};
