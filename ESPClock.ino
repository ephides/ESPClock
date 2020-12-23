
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <ArduinoOTA.h>
#include <Adafruit_GFX.h>
#include <Max72xxPanel.h>
#include <DHTesp.h>
#include <PubSubClient.h>
#include "secrets.h"

const char* ssid = STASSID;
const char* password = STAPSK;

const char* mqttserver = MQTTSERVER;

const int pinCS = D8; 
const int numberOfHorizontalDisplays = 8;
const int numberOfVerticalDisplays   = 1;

Max72xxPanel matrix = Max72xxPanel(pinCS, numberOfHorizontalDisplays, numberOfVerticalDisplays);

#define DHTPIN            D2                // Pin which is connected to the DHT sensor.
#define DHTTYPE           DHTesp::DHT22     // DHT 22 (AM2302)

float temperature;
float humidity;
long dhttimer;

DHTesp dht;

WiFiClient espClient;
PubSubClient MQTTclient(espClient);

long mqtttimer;
char MQTTclientName[32];
long mqttlastsent=0;
const long mqttinterval=60000l;

IPAddress ip;

void setup() {
  // Init Matrix Panel
  matrix.setIntensity(0); // Use a value between 0 and 15 for brightness
  for (int i=0; i<8; i++) {
     matrix.setRotation(i,1);
  }
  matrix.fillScreen(LOW);
  matrix.write();

  matrix.setIntensity(analogRead(A0)/64);  // set brightness 

  matrix.drawChar(2,0, 'W', HIGH,LOW,1); // W
  matrix.drawChar(8,0, '-', HIGH,LOW,1); // W-  
  matrix.drawChar(14,0,'L', HIGH,LOW,1); // W-L
  matrix.drawChar(20,0,'A', HIGH,LOW,1); // W-LA
  matrix.drawChar(26,0,'N', HIGH,LOW,1); // W-LAN
  matrix.write(); // Send bitmap to display
  
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

  // Display message on OTA update
  ArduinoOTA.onStart([]() {    
    matrix.fillScreen(LOW);
    matrix.drawChar(2,0, 'U', HIGH,LOW,1); 
    matrix.drawChar(8,0, 'p', HIGH,LOW,1);
    matrix.drawChar(14,0,'d', HIGH,LOW,1);
    matrix.drawChar(20,0,'a', HIGH,LOW,1);
    matrix.drawChar(26,0,'t', HIGH,LOW,1);
    matrix.drawChar(32,0,'e', HIGH,LOW,1);  // Update
    matrix.setIntensity(analogRead(A0)/64);    
    matrix.write(); // Send bitmap to display
  });

  // Use dots to indicate the progress of the OTA update
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    matrix.setIntensity(analogRead(A0)/64);
    if (progress*100/total>25) {
      matrix.drawChar(38,0,'.', HIGH,LOW,1);
      matrix.write();
    }
    if (progress*100/total>50) {
      matrix.drawChar(44,0,'.', HIGH,LOW,1);
      matrix.write();
    }
    if (progress*100/total>75) {
      matrix.drawChar(50,0,'.', HIGH,LOW,1);
      matrix.write();
    }
  });
  // Init OTA
  ArduinoOTA.begin();

  // Setup MQTT
  sprintf(MQTTclientName, "esp8266-%06X", ESP.getChipId());
  MQTTclient.setServer(MQTTSERVER, 1883);
  mqtttimer=0l;

  // Setup DHT Sensor
  dht.setup(DHTPIN,DHTTYPE);
  dhttimer=millis()+dht.getMinimumSamplingPeriod();
}

void loop() {
  struct tm *timeinfo;
  int temp;
  
  char topic[64];
  char payload[64];

  // get DHT readings
  if (dhttimer<millis()) {
     temperature = dht.getTemperature();
     humidity = dht.getHumidity();
     dhttimer=millis()+dht.getMinimumSamplingPeriod();
  }

  // fetch actual time
  time_t now = time(nullptr);
  time(&now);
  timeinfo=localtime(&now);

  matrix.fillScreen(LOW); // clear LEDs

  if (timeinfo->tm_year==70) {
    
     matrix.drawChar(2,0, 'T', HIGH,LOW,1); 
     matrix.drawChar(8,0, 'I', HIGH,LOW,1); 
     matrix.drawChar(14,0,'M', HIGH,LOW,1); 
     matrix.drawChar(20,0,'E', HIGH,LOW,1);
    
  } else {

     if (timeinfo->tm_sec&4) {
        // display date on LED panel
        matrix.drawChar(0,0, (timeinfo->tm_mday)/10?'0'+(timeinfo->tm_mday)/10:' ', HIGH,LOW,1);   // day of month, blank if zero
        matrix.drawChar(6,0, '0'+(timeinfo->tm_mday)%10, HIGH,LOW,1);                              // day of month
        matrix.drawChar(11,0,'.', HIGH,LOW,1);                                                     // .
        matrix.drawChar(17,0, '0'+(timeinfo->tm_mon+1)/10, HIGH,LOW,1);                            // month
        matrix.drawChar(23,0, '0'+(timeinfo->tm_mon+1)%10, HIGH,LOW,1);                            // month
        matrix.drawChar(28,0,'.', HIGH,LOW,1);      
     } else {
        // display temperature on LED panel
        matrix.drawChar(29,0,'C',HIGH,LOW,1);
        matrix.drawChar(23,-2,'o',HIGH,LOW,1);
        temp = (int)(temperature*10.0);
        matrix.drawChar(17,0,'0'+(temp%10)?'0'+(temp%10):' ',HIGH,LOW,1);
        temp = temp/10;
        matrix.drawChar(11,0,'.',HIGH,LOW,1);     
        matrix.drawChar(6,0,'0'+(temp%10),HIGH,LOW,1);
        temp = temp/10;
        matrix.drawChar(0,0,temp%10?'0'+(temp%10):' ',HIGH,LOW,1);       
     }

     // display time
     matrix.drawChar(36,0, (timeinfo->tm_hour)/10?'0'+(timeinfo->tm_hour)/10:' ', HIGH,LOW,1);  // hour, blank if zero
     matrix.drawChar(42,0, '0'+(timeinfo->tm_hour)%10, HIGH,LOW,1);                             // hour  
     matrix.drawChar(47,0,timeinfo->tm_sec&1?':':' ', HIGH,LOW,1);                              // blinking :
     matrix.drawChar(52,0,'0'+(timeinfo->tm_min)/10, HIGH,LOW,1);                               // minute
     matrix.drawChar(58,0,'0'+(timeinfo->tm_min)%10, HIGH,LOW,1);                               // minute   

  }

  matrix.setIntensity(analogRead(A0)/64);
  matrix.write(); // Send bitmap to display

  // MQTT
  if (!MQTTclient.connected()) {
     if (mqtttimer<millis()) {
        mqtttimer=millis()+5000;
        MQTTclient.connect(MQTTclientName);
     }
  } else {
     MQTTclient.loop();
     // Publish sensor readings
     if (mqttlastsent+mqttinterval<millis()) {
        sprintf(topic,"%s/sensors/temperature",MQTTclientName);
        sprintf(payload,"%.2f",temperature);
        MQTTclient.publish(topic, payload);
        sprintf(topic,"%s/sensors/humidity",MQTTclientName);
        sprintf(payload,"%.2f",humidity);
        MQTTclient.publish(topic, payload);
        sprintf(topic,"%s/sensors/illumination",MQTTclientName);
        sprintf(payload,"%u",analogRead(A0));
        MQTTclient.publish(topic, payload);        
        sprintf(topic,"%s/sensors/headindex",MQTTclientName);
        sprintf(payload,"%.2f",dht.computeHeatIndex(temperature, humidity, 0));
        MQTTclient.publish(topic, payload);
        sprintf(topic,"%s/sensors/dewpoint",MQTTclientName);
        sprintf(payload,"%.2f",dht.computeDewPoint(temperature, humidity, 0));
        MQTTclient.publish(topic, payload);        
        // infos about the system
        sprintf(topic,"%s/network/rssi",MQTTclientName);
        sprintf(payload,"%i",WiFi.RSSI());
        MQTTclient.publish(topic, payload);
        sprintf(topic,"%s/network/localip",MQTTclientName);
        ip = WiFi.localIP();
        sprintf(payload,"%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
        MQTTclient.publish(topic, payload);
        mqttlastsent=millis();
     }
  }
    
  // Over The Air Update
  ArduinoOTA.handle();
}
