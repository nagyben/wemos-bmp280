
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecureBearSSL.h>
#include <LittleFS.h>
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_BME280.h>

#define DEBUG
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

const long DEEPSLEEP_TIME = 5 * 60 * 1e6; // microseconds
const int MAX_WIFI_CONNECT_TIME = 100 * 1e3; // milliseconds

void bmeSensorJson(DynamicJsonDocument &d);

IPAddress ip( 192, 168, 1, 132 );
IPAddress gateway( 192, 168, 1, 1 );
IPAddress subnet( 255, 255, 255, 0 );

void initWiFi(Config &config, int timeout = MAX_WIFI_CONNECT_TIME) {
  WiFi.forceSleepWake();
  yield(); // IMPORTANT!
  WiFi.persistent( false ); // Disable the WiFi persistence.  The ESP8266 will not load and save WiFi settings in the flash memory.
  WiFi.mode(WIFI_STA);
  WiFi.begin(config.ssid, config.password);
  DEBUG_PRINT("Connecting to WiFi ..");
  long start = millis();
  while (WiFi.status() != WL_CONNECTED) {
    DEBUG_PRINT('.');
    delay(500);
    if (millis() - start > timeout) {
      DEBUG_PRINTLN("WiFi failed to connect within power budget");
      DEBUG_PRINTLN("Deep sleeping...");
      ESP.deepSleep(DEEPSLEEP_TIME, WAKE_RF_DISABLED);
    }
  }
  DEBUG_PRINT("Connected, IP address: "); DEBUG_PRINTLN(WiFi.localIP());
}

void wifiDisconnect(void) {
    // Disconnecting wifi
    DEBUG_PRINT(F("Disconnecting client"));
    client->stop();

    DEBUG_PRINT(F(", wifi"));
    WiFi.disconnect();
    WiFi.mode(WIFI_OFF);
    delay(100);  // FIXME

    DEBUG_PRINTLN(F(", sleeping"));
    WiFi.forceSleepBegin();  // turn off ESP8266 RF
    delay(100);  // FIXME
}

void setup() {
  WiFi.forceSleepBegin();
  delay(1);
  long start = millis();
  // =================================================
  // Serial
  // =================================================
#ifdef DEBUG
  Serial.begin(115200);
  delay(500);
#endif

  DEBUG_PRINT("Code version "); DEBUG_PRINTLN(GIT_REV);

  // =================================================
  // Filesystem & load config
  // =================================================
  bool fsStatus = LittleFS.begin();
  if (!fsStatus) {
    DEBUG_PRINTLN("Could not mount filesystem!");
  }
  loadConfiguration(filename, config);
  String privateKey = loadPrivateKey();
  LittleFS.end();

  if (!isConfigValid(config)) {
    return;
  }

  // =================================================
  // Wifi Client
  // =================================================
  long preConnectTime = millis();
  initWiFi(config);
  long postConnectTime = millis();

  // =================================================
  // BME280
  // =================================================
  // DynamicJsonDocument data(128);
  // data["wifiConnecTime_ms"] = postConnectTime - preConnectTime;
  // data["Vcc"] = analogRead(BATTERY_VCC_PIN);
  // data["git_rev"] = GIT_REV;
  // bmeSensorJson(data);
  // String jsonData;
  // serializeJson(data, jsonData);
  // DEBUG_PRINTLN(jsonData);

  long preHttpCallTime = millis();
  client = new BearSSL::WiFiClientSecure;
  // client->setFingerPrint(fingerprint);
  // Or, if you happy to ignore the SSL certificate, then use the following line instead:
  client->setInsecure();
  String result = getGcpToken(*client, http, config, privateKey.c_str());
  DynamicJsonDocument gcpResponse(1024);
  // String postResult = postData(*client, http, config.url, jsonData.c_str());
  DEBUG_PRINTLN(result);
  // long postHttpCallTime = millis();

  // DEBUG_PRINT("Wifi connect time: "); DEBUG_PRINT(postConnectTime - preConnectTime); DEBUG_PRINTLN("ms");
  // DEBUG_PRINT("Http call time: "); DEBUG_PRINT(postHttpCallTime - preHttpCallTime); DEBUG_PRINTLN("ms");
  // DEBUG_PRINT("Total time:"); DEBUG_PRINT(postHttpCallTime - start); DEBUG_PRINTLN("ms");

  wifiDisconnect();
  DEBUG_PRINTLN(F("Deep sleeping..."));
  // WAKE_RF_DISABLED to keep the WiFi radio disabled when we wake up
  // ESP.deepSleep(DEEPSLEEP_TIME, WAKE_RF_DISABLED );
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

