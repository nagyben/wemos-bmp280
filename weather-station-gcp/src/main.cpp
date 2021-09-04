#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecureBearSSL.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include <config.h>
#include <api.h>

//global configuration object
Config config;

// uploaded to ESP's filesystem separately
// if the file is not present, the code won't do anything
const char *filename = "config.json";

BearSSL::WiFiClientSecure *client;
HTTPClient http;

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

  Serial.println(callApi(*client, http, config.url));
}

void loop() {
  if (!isConfigValid(config)) {
    return;
  }

  if (WiFi.status() != WL_CONNECTED) {
    delay(500);
    return;
  }
}