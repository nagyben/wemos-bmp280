#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecureBearSSL.h>
#include <LittleFS.h>
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_BME280.h>

#include <config.h>
#include <api.h>

//global configuration object
Config config;

// uploaded to ESP's filesystem separately
// if the file is not present, the code won't do anything
const char *filename = "config.json";

BearSSL::WiFiClientSecure *client;
HTTPClient http;

Adafruit_BME280 bme;

String bmeSensorJson();

void setup() {
  client = new BearSSL::WiFiClientSecure;
  // client->setFingerprint(fingerprint);
  // Or, if you happy to ignore the SSL certificate, then use the following line instead:
  client->setInsecure();

  Serial.begin(115200);

  if (!LittleFS.begin()) {
    Serial.println("Could not mount filesystem!");
  }
  loadConfiguration(filename, config);
  LittleFS.end();

  if (!isConfigValid(config)) {
    return;
  }

  WiFi.begin(config.ssid, config.password);
  char buffer[100];
  sprintf(buffer, "Connecting to %s using password %s...", config.ssid, config.password);
  Serial.print(buffer);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println();

  Serial.print("Connected, IP address: ");
  Serial.println(WiFi.localIP());


  if (!bme.begin()) {
    Serial.println("Could not initialize BMP280 - check wiring!");
  }

  String jsonData = bmeSensorJson();

  Serial.println(jsonData);

  Serial.println(postData(*client, http, config.url, jsonData.c_str()));

}

void loop() {
  if (!isConfigValid(config)) {
    return;
  }

  if (WiFi.status() != WL_CONNECTED) {
    delay(500);
    return;
  }

  String jsonData = bmeSensorJson();

  Serial.println(jsonData);

  delay(1000);
}

String bmeSensorJson() {
  DynamicJsonDocument data(128);

  data["temp_C"] = bme.readTemperature();
  data["pressure_Pa"] = bme.readPressure();
  data["humidity_%"] = bme.readHumidity();
  String jsonData;
  serializeJson(data, jsonData);

  return jsonData;
}