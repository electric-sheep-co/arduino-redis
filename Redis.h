#ifndef REDIS_H
#define REDIS_H

#include <ESP8266WiFi.h>

class Redis {
  private:
    const char  *addr;
    int         port;
    int         timeout;
    bool        noDelay;
    WiFiClient  conn;
    String      checkError(String);
  public:
    Redis(void);
    Redis(const char *, int);
    bool    begin(void);
    bool    begin(const char *);
    bool    set(const char *, const char *);
    bool    setNoDelay(bool val);
    bool    setTimeout(long val);
    void    setAddr(const char *addr);
    void    setPort(int port);
    String  get(const char *);
    int     publish(const char *, const char *);
    //bool    subscribe(char *);
    //int     available(void);
    //String  read(void);
    void    close(void);
};

#endif
