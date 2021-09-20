#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>
#include <ArduinoJson.h>

#ifndef DEBUG_PRINT
  #define DEBUG_PRINT(x)
#endif
#ifndef DEBUG_PRINTLN
  #define DEBUG_PRINTLN(x)
#endif

struct Config {
  char ssid[128];
  char password[128];
  char url[128];
};

bool isConfigValid(Config &config) {
  return !(
    (strcmp(config.ssid, "") == 0)
    | (strcmp(config.password, "") == 0)
    | (strcmp(config.url, "") == 0)
  );
}

// Loads the configuration from a file
void loadConfiguration(const char *filename, Config &config) {
  DEBUG_PRINTLN("Loading config...");
  File file = LittleFS.open(filename, "r");

  // Allocate a temporary JsonDocument
  // Don't forget to change the capacity to match your requirements.
  // Use https://arduinojson.org/v6/assistant to compute the capacity.
  StaticJsonDocument<512> doc;

  // Deserialize the JSON document
  DeserializationError error = deserializeJson(doc, file);
  if (error)
    DEBUG_PRINTLN(F("Failed to read file, using default configuration"));

  // Close the file (Curiously, File's destructor doesn't close the file)
  file.close();

  // Copy values from the JsonDocument to the Config
  String(doc["ssid"]).toCharArray(config.ssid, sizeof(config.ssid));
  String(doc["password"]).toCharArray(config.password, sizeof(config.password));
  String(doc["url"]).toCharArray(config.url, sizeof(config.url));

  if (isConfigValid(config)) {
    DEBUG_PRINTLN("Config loaded!");
  } else {
    DEBUG_PRINTLN("Config is invalid!");
  }
}

#endif