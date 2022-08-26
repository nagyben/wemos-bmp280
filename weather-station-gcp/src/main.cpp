// #define DEBUG
#ifdef DEBUG
#define DEBUG_PRINT(x) Serial.print(x)
#define DEBUG_PRINTLN(x) Serial.println(x)
#define DEBUG_PRINTF(...) Serial.printf(__VA_ARGS__)
#else
#define DEBUG_PRINT(x)
#define DEBUG_PRINTLN(x)
#define DEBUG_PRINTF(...)
#endif

#ifndef LED_BUILTIN
#define LED_BUILTIN 13
#endif

#define BLINK digitalWrite(LED_BUILTIN, LOW); delay(100); digitalWrite(LED_BUILTIN, HIGH); delay(100);

#include <CloudIoTCore.h>
#include "esp8266_mqtt.h"
#include "config.h"
#include <helpers.h>
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_BME280.h>

#define DEEPSLEEP_TIME 5 * 60 * 1e6

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
const int TIMEOUT = 10 * 1e3; // milliseconds

void bmeSensorJson(DynamicJsonDocument &d);


void wifiDisconnect()
{
  DEBUG_PRINT(F("Disconnecting wifi..."));
  WiFi.disconnect();
  WiFi.mode(WIFI_OFF);
  delay(100); // FIXME

  DEBUG_PRINTLN(F("Forcing RF sleep..."));
  WiFi.forceSleepBegin(); // turn off ESP8266 RF
  delay(100);             // FIXME
}

inline void deepSleep() {
  DEBUG_PRINT(F("Deep sleeping for ")); DEBUG_PRINT(DEEPSLEEP_TIME / 1e6); DEBUG_PRINTLN(F(" seconds..."));
  wifiDisconnect();
  /*
   !!!
   ---> don't forget to connect D0 to RST!!
   !!!
  */
  ESP.deepSleep(DEEPSLEEP_TIME, WAKE_RF_DISABLED);
}

const char* connectionStatus(int);

void eventWiFi(WiFiEvent_t event) {
  switch(event) {
  case WIFI_EVENT_STAMODE_CONNECTED:
    DEBUG_PRINTF("[wifi] %d, Connected\n", event);
    break;

  case WIFI_EVENT_STAMODE_DISCONNECTED:
    DEBUG_PRINTF("[wifi] %d, Disconnected - Status %d, %s\n", event, WiFi.status(), connectionStatus(WiFi.status()));
    break;

  case WIFI_EVENT_STAMODE_AUTHMODE_CHANGE:
    DEBUG_PRINTF("[wifi] %d, AuthMode Change\n", event);
    break;

  case WIFI_EVENT_STAMODE_GOT_IP:
    DEBUG_PRINTF("[wifi] %d, Got IP\n", event);
    break;

  case WIFI_EVENT_STAMODE_DHCP_TIMEOUT:
    DEBUG_PRINTF("[wifi] %d, DHCP Timeout\n", event);
    break;

  case WIFI_EVENT_SOFTAPMODE_STACONNECTED:
    DEBUG_PRINTF("[ap] %d, Client Connected\n", event);
    break;

  case WIFI_EVENT_SOFTAPMODE_STADISCONNECTED:
    DEBUG_PRINTF("[ap] %d, Client Disconnected\n", event);
    break;

  case WIFI_EVENT_SOFTAPMODE_PROBEREQRECVED:
    DEBUG_PRINTF("[ap] %d, Probe Request Recieved\n", event);
    break;

  default:
    DEBUG_PRINTF("[GENERIC EVENT] %d\n", event);
  }
}

const char* connectionStatus(int status) {
  switch(status) {
  case WL_CONNECTED:
    return "Connected";

  case WL_NO_SSID_AVAIL:
    return "Network not availible";

  case WL_CONNECT_FAILED:
    return "Wrong password";

  case WL_IDLE_STATUS:
    return "Idle status";

  case WL_DISCONNECTED:
    return "Disconnected";

  default:
    return "Unknown";
  }
}

void initWiFi(Config &config) {
  WiFi.onEvent(eventWiFi);
  WiFi.forceSleepWake();
  yield();                // IMPORTANT!
  WiFi.persistent(false); // Disable the WiFi persistence.  The ESP8266 will not load and save WiFi settings in the flash memory.
  WiFi.mode(WIFI_STA);
  WiFi.config(IP, GW, MASK, DNS); // need this to speed up wifi connect
  DEBUG_PRINTLN(config.ssid);
  DEBUG_PRINTLN(config.password);
  WiFi.begin(config.ssid, config.password);
  DEBUG_PRINT(F("Connecting to WiFi with SSID: ")); DEBUG_PRINT(config.ssid);
  HEAP; STACK;
  while (WiFi.status() != WL_CONNECTED)
  {
    DEBUG_PRINT('.');
    BLINK;
    delay(100);
    // if (millis() > TIMEOUT)
    // {
    //   DEBUG_PRINTLN(F("WiFi failed to connect within power budget"));
    //   deepSleep();
    // }
  }

  DEBUG_PRINT(F("Connected, IP address: "));
  DEBUG_PRINTLN(WiFi.localIP());

}

void timeSync() {
  configTime(0, 0, ntp_primary, ntp_secondary);
  DEBUG_PRINTLN(F("Waiting on time sync..."));
  while (time(nullptr) < 1510644967)
  {
    delay(10);
    if (millis() > TIMEOUT)
    {
      DEBUG_PRINTLN(F("Time failed to sync within power budget"));
      deepSleep();
    }
  }
  DEBUG_PRINTLN(F("Time synchronised"));
}

void setup()
{
  WiFi.forceSleepBegin();
  delay(1);
  long start = millis();
  pinMode(LED_BUILTIN, OUTPUT);

#ifdef DEBUG
  Serial.begin(115200);
  delay(500);
#endif
  DEBUG_PRINT(F("Code version "));
  DEBUG_PRINTLN(GIT_REV);

  loadConfiguration("config.json", config);

  // ====================
  // Enable BME280 sensor
  // ====================
  DEBUG_PRINTLN(F("Enabling BME_VCC_PIN..."));
  pinMode(BME_VCC_PIN, OUTPUT);
  digitalWrite(BME_VCC_PIN, HIGH);

  // ===============
  // Connect to WiFi
  // ===============
  long preConnectTime = millis();
  initWiFi(config);
  long postConnectTime = millis();

  setupCloudIoT(config); // Creates globals for MQTT

  timeSync();

  if (!mqtt->loop())
  {
    mqtt->mqttConnect();
  }

  if (!mqttClient->connected()) {
    DEBUG_PRINTLN(F("mqtt client did not connect successfully - likely due to invalid JWT"));
    invalidateJWT();
    deepSleep();
  }

  long postMqttConnectTime = millis();

  delay(10); // <- fixes some issues with WiFi stability

  // =================================================
  // BME280
  // =================================================
  DynamicJsonDocument data(256);
  data["start_ms"] = start;
  data["preConnectTime_ms"] = preConnectTime;
  data["postConnectTime_ms"] = postConnectTime;
  data["postMqttConnectTime_ms"] = postMqttConnectTime;
  data["Vcc"] = analogRead(BATTERY_VCC_PIN);
  data["git_rev"] = GIT_REV;
#ifdef DEBUG
  data["debug"] = 1;
#else
  data["debug"] = 0;
#endif

  bmeSensorJson(data);

  data["postSensorRead_ms"] = millis();

  String jsonData;
  serializeJson(data, jsonData);
  DEBUG_PRINTLN(jsonData);

  publishTelemetry(jsonData);

  wifiDisconnect();

#ifdef DEBUG
  pinMode(LED_BUILTIN, INPUT_PULLUP); // disable LED during deepsleep
#endif
  deepSleep();
}

void bmeSensorJson(DynamicJsonDocument &data)
{
  if (!bme.begin())
  {
    DEBUG_PRINTLN(F("Could not initialize BMP280 - check wiring!"));
  }
  bme.setSampling(Adafruit_BME280::MODE_FORCED,
                  Adafruit_BME280::SAMPLING_X1, // temp
                  Adafruit_BME280::SAMPLING_X1, // pressure
                  Adafruit_BME280::SAMPLING_X1, // humidity
                  Adafruit_BME280::FILTER_OFF);
  delay(500);
  DEBUG_PRINTLN(F("Reading values from BME280..."));
  bme.takeForcedMeasurement();
  data["temp_C"] = bme.readTemperature();
  data["pressure_Pa"] = bme.readPressure();
  data["humidity_%"] = bme.readHumidity();
#ifdef DEBUG
  data["debug"] = true;
#endif

  // adding this line causes issues with serial comms
  // no idea why :joy:
  // digitalWrite(BME_VCC_PIN, LOW);
}

void loop() {}