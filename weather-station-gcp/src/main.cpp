#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecureBearSSL.h>
#include <LittleFS.h>
#include <ArduinoJson.h>

struct Config {
  char ssid[64];
  char password[64];
  char url[128];
};

const char *filename = "config.json";
Config config;                         // <- global configuration object

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

  // Copy values from the JsonDocument to the Config
  strlcpy(config.password,                  // <- destination
          doc["password"] | "",             // <- source
          sizeof(config.password));         // <- destination's capacity

  strlcpy(config.ssid,                  // <- destination
          doc["ssid"] | "",             // <- source
          sizeof(config.ssid));         // <- destination's capacity

  strlcpy(config.url,                  // <- destination
          doc["url"] | "",             // <- source
          sizeof(config.url));         // <- destination's capacity

  // Close the file (Curiously, File's destructor doesn't close the file)
  file.close();
  Serial.println("Config loaded!");
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

    int httpStatusCode = http.GET();

    if (httpStatusCode > 0) {
        Serial.print("HTTP Response code: ");
        Serial.println(httpStatusCode);
        String payload = http.getString();
        Serial.println(payload);
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

void loop() {
  // put your main code here, to run repeatedly:
}