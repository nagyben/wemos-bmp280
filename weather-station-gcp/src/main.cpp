// #define DEBUG
#ifdef DEBUG
#define DEBUG_PRINT(x) Serial.print(x)
#define DEBUG_PRINTLN(x) Serial.println(x)
#define BLINK(x) blink(x)
#else
#define DEBUG_PRINT(x)
#define DEBUG_PRINTLN(x)
#define BLINK(x)
#endif

#include <CloudIoTCore.h>
#include "esp8266_mqtt.h"
#include "config.h"
#include <helpers.h>
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_BME280.h>

#ifndef LED_BUILTIN
#define LED_BUILTIN 13
#endif

#define DEEPSLEEP_TIME 5 * 1e6

#define BME_VCC_PIN 3
#define BATTERY_VCC_PIN A0

Adafruit_BME280 bme;

Config config;

IPAddress IP(192, 168, 1, 132);
IPAddress GW(192, 168, 1, 1);
IPAddress MASK(255, 255, 255, 0);
IPAddress DNS(8, 8, 8, 8);
const int CHANNEL = 1; // wifi channel
const uint8_t MAC[6] = {0xB0, 0x6E, 0xBF, 0x7C, 0x0F, 0x68};
const int MAX_WIFI_CONNECT_TIME = 10 * 1e3; // milliseconds

void bmeSensorJson(DynamicJsonDocument &d);

void initWiFi(Config &config, int timeout = MAX_WIFI_CONNECT_TIME)
{
  WiFi.forceSleepWake();
  yield();                // IMPORTANT!
  WiFi.persistent(false); // Disable the WiFi persistence.  The ESP8266 will not load and save WiFi settings in the flash memory.
  WiFi.mode(WIFI_STA);
  WiFi.config(IP, GW, MASK, DNS); // need this to speed up wifi connect
  WiFi.begin(config.ssid, config.password, CHANNEL, MAC, true);
  DEBUG_PRINT(F("Connecting to WiFi .."));
  long start = millis();
  while (WiFi.status() != WL_CONNECTED)
  {
    DEBUG_PRINT('.');
    BLINK(1);
    delay(100);
    if (millis() - start > timeout)
    {
      DEBUG_PRINTLN(F("WiFi failed to connect within power budget"));
      DEBUG_PRINTLN(F("Deep sleeping..."));
      ESP.deepSleep(DEEPSLEEP_TIME, WAKE_RF_DISABLED);
    }
  }

  DEBUG_PRINT(F("Connected, IP address: "));
  DEBUG_PRINTLN(WiFi.localIP());

  configTime(0, 0, ntp_primary, ntp_secondary);
  Serial.println("Waiting on time sync...");
  while (time(nullptr) < 1510644967)
  {
    delay(10);
  }
}

void wifiDisconnect(void)
{
  DEBUG_PRINT(F(", wifi"));
  WiFi.disconnect();
  WiFi.mode(WIFI_OFF);
  delay(100); // FIXME

  DEBUG_PRINTLN(F(", sleeping"));
  WiFi.forceSleepBegin(); // turn off ESP8266 RF
  delay(100);             // FIXME
}

void setup()
{
  WiFi.forceSleepBegin();
  delay(1);
  long start = millis();

#ifdef DEBUG
  pinMode(LED_BUILTIN, OUTPUT);
  Serial.begin(115200);
  delay(500);
#endif
  DEBUG_PRINT(F("Code version "));
  DEBUG_PRINTLN(GIT_REV);

  loadConfiguration("config.json", config);

  long preConnectTime = millis();
  initWiFi(config);
  long postConnectTime = millis();

  setupCloudIoT(config); // Creates globals for MQTT
  pinMode(LED_BUILTIN, OUTPUT);

  if (!mqtt->loop())
  {
    mqtt->mqttConnect();
  }

  long postMqttConnectTime = millis();

  delay(10); // <- fixes some issues with WiFi stability

  // =================================================
  // BME280
  // =================================================
  DynamicJsonDocument data(192);
  data["start_ms"] = start;
  data["preConnectTime_ms"] = preConnectTime;
  data["postConnectTime_ms"] = postConnectTime;
  data["postMqttConnectTime_ms"] = postMqttConnectTime;
  data["Vcc"] = analogRead(BATTERY_VCC_PIN);
  data["git_rev"] = GIT_REV;
  bmeSensorJson(data);
  String jsonData;
  serializeJson(data, jsonData);
  DEBUG_PRINTLN(jsonData);

  publishTelemetry(jsonData);

  wifiDisconnect();

#ifdef DEBUG
  pinMode(LED_BUILTIN, INPUT_PULLUP); // disable LED during deepsleep
#endif
  DEBUG_PRINTLN(F("Deep sleeping..."));
  // WAKE_RF_DISABLED to keep the WiFi radio disabled when we wake up
  ESP.deepSleep(DEEPSLEEP_TIME); // don't forget to connect D0 to RST
}

void bmeSensorJson(DynamicJsonDocument &data)
{
  pinMode(BME_VCC_PIN, OUTPUT);
  digitalWrite(BME_VCC_PIN, HIGH);
  delay(500);
  if (!bme.begin())
  {
    DEBUG_PRINTLN("Could not initialize BMP280 - check wiring!");
  }
  bme.setSampling(Adafruit_BME280::MODE_FORCED,
                  Adafruit_BME280::SAMPLING_X1, // temp
                  Adafruit_BME280::SAMPLING_X1, // pressure
                  Adafruit_BME280::SAMPLING_X1, // humidity
                  Adafruit_BME280::FILTER_OFF);
  delay(1000);
  bme.takeForcedMeasurement();
  data["temp_C"] = bme.readTemperature();
  data["pressure_Pa"] = bme.readPressure();
  data["humidity_%"] = bme.readHumidity();
  digitalWrite(BME_VCC_PIN, LOW);
}

void loop() {}