#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <LittleFS.h>
#include <helpers.h>

#ifndef DEBUG_PRINT
  #define DEBUG_PRINT(x)
#endif
#ifndef DEBUG_PRINTLN
  #define DEBUG_PRINTLN(x)
#endif

#define GCP_PRIVATE_KEY_FILE "esp8266_sa.pem"

struct Config {
  char ssid[32];
  char password[16];
  char url[128];
  char saEmail[64];
};

inline bool isConfigValid(Config &config) {
  return !(
    (strcmp(config.ssid, "") == 0)
    | (strcmp(config.password, "") == 0)
    | (strcmp(config.url, "") == 0)
  );
}

// Loads the configuration from a file
inline void loadConfiguration(const char *filename, Config &config) {

  if (!LittleFS.begin()) {
    DEBUG_PRINTLN(F("Could not start LittleFS!"));
    return;
  }

  DEBUG_PRINT("Loading config: "); DEBUG_PRINTLN(filename);
  File file = LittleFS.open(filename, "r");

  // Allocate a temporary JsonDocument
  // Don't forget to change the capacity to match your requirements.
  // Use https://arduinojson.org/v6/assistant to compute the capacity.
  StaticJsonDocument<256> doc;

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
  String(doc["saEmail"]).toCharArray(config.saEmail, sizeof(config.saEmail));

  if (isConfigValid(config)) {
    DEBUG_PRINTLN(F("Config loaded!"));
  } else {
    DEBUG_PRINTLN(F("Config is invalid!"));
  }
}

inline String loadPrivateKey() {
  if (!LittleFS.begin()) {
    DEBUG_PRINTLN(F("Could not start LittleFS!"));
    return "";
  }
  DEBUG_PRINT(F("Reading private key: ")); DEBUG_PRINTLN(GCP_PRIVATE_KEY_FILE);
  File file = LittleFS.open(GCP_PRIVATE_KEY_FILE, "r");
  if (!file) {
    DEBUG_PRINT(F("Could not open ")); DEBUG_PRINTLN(GCP_PRIVATE_KEY_FILE);
  }

  return file.readString();
}

#endif