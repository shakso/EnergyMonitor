#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <PubSubClient.h>
#include <ArduinoOTA.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <TM1637Display.h>
#include <Adafruit_NeoPixel.h>

Adafruit_NeoPixel strip = Adafruit_NeoPixel(60, 25, NEO_GRB + NEO_KHZ800);
TM1637Display avg_12hr(33, 32);
TM1637Display current_wholesale(19,23);

const char* ssid = "xxx";
const char* password = "xxx";

WiFiClient wclient;
PubSubClient client(wclient);

void setup() {
  
  Serial.begin(115200);
  WiFi.setHostname("octoclock");
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

  ArduinoOTA.setHostname("octoclock");

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

  strip.begin();
  strip.setBrightness(40);
  strip.show();
}


void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    String clientId = "OCTOCLOCK-";
    clientId += String(random(0xffff), HEX);
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
      client.subscribe("octoclock");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

void callback(char* topic, byte* data, unsigned int length) {
  StaticJsonDocument<4096> jsonData;
  DeserializationError error = deserializeJson(jsonData, data, length);
  
  if (error) {
    Serial.println(error.c_str()); 
    return;
  }

  avg_12hr.setBrightness(0x0f);
  avg_12hr.showNumberDec(jsonData["avg_12hr"].as<float>(), false);

  current_wholesale.setBrightness(0x0f);
  current_wholesale.showNumberDec(jsonData["current_wholesale"].as<float>(), false);
  
  int i = 0;

  for(JsonVariant rgbArray : jsonData["ws_vals"].as<JsonArray>()) {
    strip.setPixelColor(i, strip.Color(rgbArray.as<JsonArray>()[0].as<int>(),rgbArray.as<JsonArray>()[1].as<int>(),rgbArray.as<JsonArray>()[2].as<int>()));
    i = i + 1;
  }
  strip.show();
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  ArduinoOTA.handle();
  yield();
}
