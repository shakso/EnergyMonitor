#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <PubSubClient.h>
#include <ArduinoOTA.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include "Adafruit_LEDBackpack.h"

Adafruit_7segment electricMatrix = Adafruit_7segment();
Adafruit_7segment gasMatrix = Adafruit_7segment();

const char* ssid = "xxx";
const char* password = "xxx";

float oldGasReading=0;
int displayMode=0;

float gasReading=0;
float leccyReading=0;
float gasRate=0;
float leccyRate=0;

WiFiClient wclient;
PubSubClient client(wclient);

void setup() {
  
  Serial.begin(115200);
  
  pinMode(35, OUTPUT); // ELECTRIC
  pinMode(33, OUTPUT); // GAS

  pinMode(16, INPUT_PULLUP); // DAY BUTTON
  pinMode(18, INPUT_PULLUP); // RATE BUTTON

  WiFi.setHostname("electricframe");
  WiFi.begin(ssid, password);
  Serial.println("");
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  client.setServer("x.x.x.x", 1883);
  client.setBufferSize(4096);
  client.setCallback(callback);

  Wire.begin(37,39);
  electricMatrix.begin(0x70);
  gasMatrix.begin(0x71);

  ArduinoOTA.setHostname("electricframe");

  ArduinoOTA.onStart([]() {
    Serial.println("Start");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();
}


void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);

    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
      client.subscribe("glow/#");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

void callback(char* topic, byte* data, unsigned int length) {
  StaticJsonDocument<1024> jsonData;
  deserializeJson(jsonData, data, length);

  if (jsonData["dccsourced"]) {
    leccyReading = jsonData["dccsourced"]["total_electric"].as<float>();
    leccyRate = jsonData["dccsourced"]["electric_rate"].as<float>();
    
    if (displayMode == 0) {
      electricMatrix.print(leccyReading);
    } else {
      electricMatrix.print(leccyRate);
    }
    
    electricMatrix.writeDisplay();

    gasReading = jsonData["dccsourced"]["total_gas"].as<float>();
    gasRate = jsonData["dccsourced"]["gas_rate"].as<float>();
    
    if (displayMode == 0) {
      gasMatrix.print(gasReading);
    } else {
      gasMatrix.print(gasRate);
    }
    gasMatrix.writeDisplay();
  }

  if (jsonData["electricitymeter"]) {
    float leccy = 230*(jsonData["electricitymeter"]["power"]["value"].as<float>()/6);
    if (leccy > 230) {
      leccy=230;
    }
    analogWrite(35, leccy);
  }

  if (jsonData["gasmeter"]) { 
    if (jsonData["gasmeter"]["energy"]["import"]["day"].as<float>() != oldGasReading) {
      float gas = 255*((jsonData["gasmeter"]["energy"]["import"]["day"].as<float>()-oldGasReading)/6);
      if (gas > 255) {
        gas=255;
      }
      analogWrite(33, gas);
    }
    oldGasReading=jsonData["gasmeter"]["energy"]["import"]["day"].as<float>();
  }
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  int rateButton = digitalRead(18);
  int dayButton = digitalRead(16);

  if (dayButton == 0) {
    displayMode=0;
    gasMatrix.print(gasReading);
    electricMatrix.print(leccyReading);
    gasMatrix.writeDisplay();
    electricMatrix.writeDisplay();
  }

  if (rateButton == 0) {
    displayMode=1;
    gasMatrix.print(gasRate);
    electricMatrix.print(leccyRate);
    gasMatrix.writeDisplay();
    electricMatrix.writeDisplay();
  }
  
  ArduinoOTA.handle();
  yield();
}