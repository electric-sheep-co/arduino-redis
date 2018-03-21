#ifndef REDIS_H
#define REDIS_H

#include <ESP8266WiFi.h>

class Redis {
  private:
    const char* addr;
    int         port;
    WiFiClient  conn;
    String      checkError(String);
  public:
    Redis(const char *, int);
    bool    begin(void);
    bool    begin(const char *);
    bool    set(const char *, const char *);
    String  get(const char *);
    int     publish(const char *, const char *);
    //bool    subscribe(char *);
    //int     available(void);
    //String  read(void);
    void    close(void);
};

#endif
