#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecureBearSSL.h>
#include <LittleFS.h>
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_BME280.h>

// #define DEBUG
#ifdef DEBUG
  #define DEBUG_PRINT(x)  Serial.print (x)
  #define DEBUG_PRINTLN(x)  Serial.println (x)
#else
  #define DEBUG_PRINT(x)
  #define DEBUG_PRINTLN(x)
#endif

#include <config.h>
#include <api.h>

#define BME_VCC_PIN 3
#define BATTERY_VCC_PIN A0

//global configuration object
Config config;

// uploaded to ESP's filesystem separately
// if the file is not present, the code won't do anything
const char *filename = "config.json";

BearSSL::WiFiClientSecure *client;
HTTPClient http;

Adafruit_BME280 bme;

const long DEEPSLEEP_TIME =  5 * 1e6 /*microseconds*/;
const int MAX_WIFI_CONNECT_TIME = 100 * 1e3; // milliseconds

void bmeSensorJson(DynamicJsonDocument &d);

void setup() {
  WiFi.forceSleepBegin();
  long start = millis();
  // =================================================
  // Serial
  // =================================================
#ifdef DEBUG
  Serial.begin(115200);
#endif

  // =================================================
  // Filesystem & load config
  // =================================================
  bool fsStatus = LittleFS.begin();
  if (!fsStatus) {
    DEBUG_PRINTLN("Could not mount filesystem!");
  }
  loadConfiguration(filename, config);
  LittleFS.end();

  if (!isConfigValid(config)) {
    return;
  }

  // =================================================
  // Wifi Client
  // =================================================
  WiFi.forceSleepWake();
  WiFi.mode(WIFI_STA);
  client = new BearSSL::WiFiClientSecure;
  // client->setFingerPrint(fingerprint);
  // Or, if you happy to ignore the SSL certificate, then use the following line instead:
  client->setInsecure();
  long preConnectTime = millis();
  WiFi.begin(config.ssid, config.password);
  DEBUG_PRINT("Connecting to "); DEBUG_PRINT(config.ssid);
  pinMode(LED_BUILTIN, OUTPUT);
  while (WiFi.status() != WL_CONNECTED)
  {
    digitalWrite(LED_BUILTIN, LOW);
    delay(100);
    digitalWrite(LED_BUILTIN, HIGH);
    delay(300);
    DEBUG_PRINT(".");
    if (millis() - preConnectTime > MAX_WIFI_CONNECT_TIME) {
      DEBUG_PRINTLN("WiFi failed to connect within power budget");
      DEBUG_PRINTLN("Deep sleeping...");
      ESP.deepSleep(DEEPSLEEP_TIME);
    }
  }
  long postConnectTime = millis();
  DEBUG_PRINTLN();

  DEBUG_PRINT("Connected, IP address: ");
  DEBUG_PRINTLN(WiFi.localIP());


  // =================================================
  // BME280
  // =================================================
  DynamicJsonDocument data(128);
  data["wifiConnecTime_ms"] = postConnectTime - preConnectTime;
  data["Vcc"] = analogRead(BATTERY_VCC_PIN);
  bmeSensorJson(data);
  String jsonData;
  serializeJson(data, jsonData);

  DEBUG_PRINTLN(jsonData);
  long preHttpCallTime = millis();
  DEBUG_PRINTLN(postData(*client, http, config.url, jsonData.c_str()));
  long postHttpCallTime = millis();

  DEBUG_PRINT("Wifi connect time: "); DEBUG_PRINT(postConnectTime - preConnectTime); DEBUG_PRINTLN("ms");
  DEBUG_PRINT("Http call time: "); DEBUG_PRINT(postHttpCallTime - preHttpCallTime); DEBUG_PRINTLN("ms");
  DEBUG_PRINT("Total time:"); DEBUG_PRINT(postHttpCallTime - start); DEBUG_PRINTLN("ms");

  DEBUG_PRINTLN("Deep sleeping...");
  ESP.deepSleep(DEEPSLEEP_TIME);
}

void loop() {}

void bmeSensorJson(DynamicJsonDocument &data) {
  pinMode(BME_VCC_PIN, OUTPUT);
  digitalWrite(BME_VCC_PIN, HIGH);
  if (!bme.begin()) {
    DEBUG_PRINTLN("Could not initialize BMP280 - check wiring!");
  }
  bme.setSampling(Adafruit_BME280::MODE_FORCED,
                  Adafruit_BME280::SAMPLING_X1, // temp
                  Adafruit_BME280::SAMPLING_X1, // pressure
                  Adafruit_BME280::SAMPLING_X1, // humidity
                  Adafruit_BME280::FILTER_OFF
                  );
  delay(1000);
  bme.takeForcedMeasurement();
  data["temp_C"] = bme.readTemperature();
  data["pressure_Pa"] = bme.readPressure();
  data["humidity_%"] = bme.readHumidity();
  digitalWrite(BME_VCC_PIN, LOW);
}