#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <ArduinoOTA.h>

#ifndef STASSID
#define STASSID "AP-Name"
#define STAPSK  "Password"
#endif

const char* ssid = STASSID;
const char* password = STAPSK;

void setup() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    delay(5000);
    ESP.restart();
  }
  ArduinoOTA.begin();
}

void loop() {
  ArduinoOTA.handle();
}
