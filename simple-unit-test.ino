#define ARDUINO_REDIS_TEST 1

#include <Redis.h>
#include <ESP8266WiFi.h>

#define WIFI_SSID   ""
#define WIFI_PASS   ""
#define REDIS_HOST  ""
#define REDIS_PORT  6379
#define REDIS_AUTH  ""

void setup() {
  Serial.begin(115200);

  WiFi.begin(WIFI_SSID, WIFI_PASS);

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
  }

  Serial.print("Connected! IP is: ");
  Serial.println(WiFi.localIP());
  
  WiFiClient rc;
  if (!rc.connect(REDIS_HOST, REDIS_PORT)) {
    Serial.println("Failed to connect to Redis server!");
    return;
  }
  
  auto r = new Redis(rc);
  if (!strlen(REDIS_AUTH) || r->authenticate(REDIS_AUTH) == RedisSuccess) {
    auto res = r->runTests(true);
    Serial.printf("%d tests passed of %d total run.\n", res.passed, res.total);
  }
}

void loop() {}
