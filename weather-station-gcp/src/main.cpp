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

// #define DEBUG
#ifdef DEBUG
  #define print(x)  Serial.print (x)
  #define println(x)  Serial.println (x)
#else
  #define print(x)
  #define println(x)
#endif

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

const long DEEPSLEEP_TIME = 5 /*minutes*/ * 60 /*seconds*/ * 1e6 /*microseconds*/;
const int MAX_WIFI_CONNECT_TIME = 10 * 1e3; // milliseconds

void bmeSensorJson(DynamicJsonDocument &d);

void setup() {
  long start = millis();
  WiFi.forceSleepBegin();
  yield();
  // =================================================
  // Serial
  // =================================================
#ifdef DEBUG
  Serial.begin(115200);
#endif

  // =================================================
  // Filesystem & load config
  // =================================================
  if (!LittleFS.begin()) {
    println("Could not mount filesystem!");
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
  // client->setFingerprint(fingerprint);
  // Or, if you happy to ignore the SSL certificate, then use the following line instead:
  client->setInsecure();
  long preConnectTime = millis();
  WiFi.begin(config.ssid, config.password);
  print("Connecting to "); print(config.ssid);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    print(".");
    if (millis() - preConnectTime > MAX_WIFI_CONNECT_TIME) {
      println("WiFi failed to connect within power budget");
      println("Deep sleeping...");
      ESP.deepSleep(DEEPSLEEP_TIME);
    }
  }
  long postConnectTime = millis();
  println();

  print("Connected, IP address: ");
  println(WiFi.localIP());


  // =================================================
  // BME280
  // =================================================
  DynamicJsonDocument data(128);
  data["wifiConnecTime_ms"] = postConnectTime - preConnectTime;
  data["Vcc"] = analogRead(BATTERY_VCC_PIN);
  bmeSensorJson(data);
  String jsonData;
  serializeJson(data, jsonData);

  println(jsonData);
  long preHttpCallTime = millis();
  println(postData(*client, http, config.url, jsonData.c_str()));
  long postHttpCallTime = millis();

  print("Wifi connect time: "); print(postConnectTime - preConnectTime); println("ms");
  print("Http call time: "); print(postHttpCallTime - preHttpCallTime); println("ms");
  print("Total time:"); print(postHttpCallTime - start); println("ms");

  digitalWrite(BME_VCC_PIN, LOW);
  println("Deep sleeping...");
  ESP.deepSleep(DEEPSLEEP_TIME);
}

void loop() {}

void bmeSensorJson(DynamicJsonDocument &data) {
  pinMode(BME_VCC_PIN, OUTPUT);
  digitalWrite(BME_VCC_PIN, HIGH);
  if (!bme.begin()) {
    println("Could not initialize BMP280 - check wiring!");
  }
  bme.setSampling(Adafruit_BME280::MODE_FORCED,
                  Adafruit_BME280::SAMPLING_X1, // temp
                  Adafruit_BME280::SAMPLING_X1, // pressure
                  Adafruit_BME280::SAMPLING_X1, // humidity
                  Adafruit_BME280::FILTER_OFF
                  );
  delay(1000);
  bme.takeForcedMeasurement();
  digitalWrite(BME_VCC_PIN, LOW);
  data["temp_C"] = bme.readTemperature();
  data["pressure_Pa"] = bme.readPressure();
  data["humidity_%"] = bme.readHumidity();
}