#include <M5StickC.h> 
#include <PubSubClient.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

#include "M5StickC.h"
#include <OneWire.h>
#include <DallasTemperature.h>

WiFiClient espClient;
PubSubClient client(espClient);
const char* ssid           = "x";
const char* password       = "x";
const char* mqtt_server    = "192.168.2.137";
const char* mqtt_user      = "x";
const char* mqtt_password  = "x";
void setupWifi();
void callback(char* topic, byte* payload, unsigned int length);
void reConnect();

#define ONE_WIRE_BUS 32
OneWire oneWire(ONE_WIRE_BUS); 
DallasTemperature sensors(&oneWire);

unsigned long previousMillis = 0;
#define DISPLAY_UPDATE_INTERVAL   1000  // in ms

unsigned long displayOffMillis = 0;
bool displayOff = true;
#define DISPLAY_ON_INTERVAL       5000

void setup() {
  Serial.begin(115200);

  setupWifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

  M5.begin();

  turnDisplayOff();
  
  M5.Lcd.setRotation(1);
  M5.Lcd.setTextSize(2);
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setTextColor(WHITE);

  ArduinoOTA
    .onStart([]() {
      String type;
      if (ArduinoOTA.getCommand() == U_FLASH)
        type = "sketch";
      else // U_SPIFFS
        type = "filesystem";

      // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
      Serial.println("Start updating " + type);
    })
    .onEnd([]() {
      Serial.println("\nEnd");
    })
    .onProgress([](unsigned int progress, unsigned int total) {
      Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    })
    .onError([](ota_error_t error) {
      Serial.printf("Error[%u]: ", error);
      if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
      else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
      else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
      else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
      else if (error == OTA_END_ERROR) Serial.println("End Failed");
    });

  ArduinoOTA.begin();
}

void turnDisplayOff(){
  M5.Axp.SetLDO2(false);
  M5.Axp.SetLDO3(false);
  displayOff = true;
}

void turnDisplayOn(){
  M5.Axp.SetLDO2(true);
  M5.Axp.SetLDO3(true);
  displayOff = false;
}

void loop(){ 
  unsigned long currentMillis = millis();

  M5.update();
  
  if (!client.connected()) {
    reConnect();
  }
  client.loop();

  if (M5.BtnA.wasPressed()){
    displayOffMillis = millis() + DISPLAY_ON_INTERVAL;
    if (displayOff){
      turnDisplayOn();
    }
  }

  if(millis() > displayOffMillis){
    if(!displayOff){
      turnDisplayOff();
    }
  }
  
  if (currentMillis - previousMillis >= DISPLAY_UPDATE_INTERVAL) {
    previousMillis = currentMillis;

    M5.Lcd.setCursor(0, 0);    
  
    sensors.requestTemperatures(); 
    float temp1 = sensors.getTempCByIndex(0);
    float temp2 = sensors.getTempCByIndex(1);
    bool temp1_avail = temp1 > -30.0; 
    bool temp2_avail = temp2 > -30.0;
    
    String text1 = temp1_avail ? String(temp1, 2) : "---"; // + "C";
    String text2 = temp2_avail ? String(temp2, 2) : "---"; // + "C";
  
    M5.Lcd.fillScreen(BLACK);
    M5.Lcd.setCursor(0, 0);    
    M5.Lcd.print("1: ");
    M5.Lcd.println(text1);
    M5.Lcd.print("2: ");
    M5.Lcd.println(text2);

  
    if (temp1_avail)
    {
      client.publish("thermometer/chn0", text1.c_str());
    }

    if (temp2_avail)
    {
      client.publish("thermometer/chn1", text2.c_str());
    }
  }

  ArduinoOTA.handle();
}

void setupWifi() {
    delay(10);
    M5.Lcd.printf("Connecting to %s", ssid);
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        M5.Lcd.print(".");
    }
    M5.Lcd.printf("\nSuccess\n");
}

void callback(char* topic, byte* payload, unsigned int length) {
    M5.Lcd.print("Message arrived [");
    M5.Lcd.print(topic);
    M5.Lcd.print("] ");
    for (int i = 0; i < length; i++) {
        M5.Lcd.print((char)payload[i]);
    }
    M5.Lcd.println();
}

void reConnect() {
    while (!client.connected()) {
        Serial.print("Attempting MQTT connection...");
        String clientId = "M5Stack-";
        clientId += String(random(0xffff), HEX);
        if (client.connect(clientId.c_str()), mqtt_user, mqtt_password) {
            Serial.printf("\nSuccess\n");
        } else {
            Serial.print("failed, rc=");
            Serial.print(client.state());
            Serial.println("try again in 5 seconds");
            delay(5000);
        }
    }
}
