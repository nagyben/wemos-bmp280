#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecureBearSSL.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include "config.h"

//global configuration object
Config config;

// uploaded to ESP's filesystem separately
// if the file is not present, the code won't do anything
const char *filename = "config.json";

// Loads the configuration from a file
void loadConfiguration(const char *filename, Config &config) {
  Serial.println("Loading config...");
  File file = LittleFS.open(filename, "r");

  // Allocate a temporary JsonDocument
  // Don't forget to change the capacity to match your requirements.
  // Use https://arduinojson.org/v6/assistant to compute the capacity.
  StaticJsonDocument<512> doc;

  // Deserialize the JSON document
  DeserializationError error = deserializeJson(doc, file);
  if (error)
    Serial.println(F("Failed to read file, using default configuration"));

  // Close the file (Curiously, File's destructor doesn't close the file)
  file.close();

  // Copy values from the JsonDocument to the Config
  String(doc["ssid"]).toCharArray(config.ssid, sizeof(config.ssid));
  String(doc["password"]).toCharArray(config.password, sizeof(config.password));
  String(doc["url"]).toCharArray(config.url, sizeof(config.url));

  if (isConfigValid(config)) {
    Serial.println("Config loaded!");
  } else {
    Serial.println("Config is invalid!");
  }
}

String callApi() {
  String payload = "disconnected";
  if (WiFi.status() == WL_CONNECTED) {
    std::unique_ptr<BearSSL::WiFiClientSecure>client(new BearSSL::WiFiClientSecure);

    // client->setFingerprint(fingerprint);
    // Or, if you happy to ignore the SSL certificate, then use the following line instead:
    client->setInsecure();

    // WiFiClient client;
    HTTPClient http;

    http.begin(*client, config.url);

    char buffer[100];
    sprintf(buffer, "Making request to %s", config.url);
    Serial.println(buffer);
    int httpStatusCode = http.GET();

    if (httpStatusCode > 0) {
        Serial.print("HTTP Response code: ");
        Serial.println(httpStatusCode);
        payload = http.getString();
      }
      else {
        Serial.print("Error code: ");
        Serial.println(httpStatusCode);
        payload = "error";
      }
    // Free resources
    http.end();
  }

  return payload;
}

void setup() {
  Serial.begin(115200);

  if (!LittleFS.begin()) {
    Serial.println("Could not mount filesystem!");
  }
  loadConfiguration(filename, config);
  LittleFS.end();

  if (isConfigValid(config)) {
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

    Serial.println(callApi());
  }
}

void loop() {
  if (isConfigValid(config)) {

  }
}