
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <ArduinoOTA.h>
#include <Adafruit_GFX.h>
#include <Max72xxPanel.h>
#include "secrets.h"

const char* ssid = STASSID;
const char* password = STAPSK;

const int pinCS = D8; 
const int numberOfHorizontalDisplays = 8;
const int numberOfVerticalDisplays   = 1;

Max72xxPanel matrix = Max72xxPanel(pinCS, numberOfHorizontalDisplays, numberOfVerticalDisplays);

void setup() {
  // Init Matrix Panel
  matrix.setIntensity(0); // Use a value between 0 and 15 for brightness
  for (int i=0; i<8; i++) {
     matrix.setRotation(i,1);
  }
  matrix.fillScreen(LOW);
  matrix.write();

  matrix.setIntensity(8);  // set brightness 
  
  // Init Wifi
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    delay(5000);
    ESP.restart();
  }

  // NTP Init
  if (esp8266::coreVersionNumeric() >= 20700000) {
     configTime("CET-1CEST-2,M3.5.0/02,M10.5.0/03", "at.pool.ntp.org"); 
  } else {
     setenv("TZ", "CET-1CEST-2,M3.5.0/02,M10.5.0/03",1); 
     configTime(0, 0, "at.pool.ntp.org", "time.nist.gov");
  }  

  // Init OTA
  ArduinoOTA.begin();
}

void loop() {
  struct tm *timeinfo;

  // fetch actual time
  time_t now = time(nullptr);
  time(&now);
  timeinfo=localtime(&now);

  matrix.fillScreen(LOW); // clear LEDs

  // display date on LED panel
  matrix.drawChar(0,0, (timeinfo->tm_mday)/10?'0'+(timeinfo->tm_mday)/10:' ', HIGH,LOW,1);   // day of month, blank if zero
  matrix.drawChar(6,0, '0'+(timeinfo->tm_mday)%10, HIGH,LOW,1);                              // day of month
  matrix.drawChar(11,0,'.', HIGH,LOW,1);                                                     // .
  matrix.drawChar(17,0, '0'+(timeinfo->tm_mon+1)/10, HIGH,LOW,1);                            // month
  matrix.drawChar(23,0, '0'+(timeinfo->tm_mon+1)%10, HIGH,LOW,1);                            // month
  matrix.drawChar(28,0,'.', HIGH,LOW,1);      

  // display time
  matrix.drawChar(36,0, (timeinfo->tm_hour)/10?'0'+(timeinfo->tm_hour)/10:' ', HIGH,LOW,1);  // hour, blank if zero
  matrix.drawChar(42,0, '0'+(timeinfo->tm_hour)%10, HIGH,LOW,1);                             // hour  
  matrix.drawChar(47,0,timeinfo->tm_sec&1?':':' ', HIGH,LOW,1);                              // blinking :
  matrix.drawChar(52,0,'0'+(timeinfo->tm_min)/10, HIGH,LOW,1);                               // minute
  matrix.drawChar(58,0,'0'+(timeinfo->tm_min)%10, HIGH,LOW,1);                               // minute   

  matrix.write(); // Send bitmap to display
    
  // Over The Air Update
  ArduinoOTA.handle();
}
