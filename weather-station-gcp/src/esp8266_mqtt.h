/******************************************************************************
 * Copyright 2018 Google
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *****************************************************************************/
// This file contains static methods for API requests using Wifi / MQTT

#include <ESP8266WiFi.h>
#include "FS.h"
#include "rtc_memory.h"

// You need to set certificates to All SSL cyphers and you may need to
// increase memory settings in Arduino/cores/esp8266/StackThunk.cpp:
//   https://github.com/esp8266/Arduino/issues/6811
#include "WiFiClientSecureBearSSL.h"
#include <time.h>

#include <MQTT.h>

#include <CloudIoTCore.h>
#include <CloudIoTCoreMqtt.h>
#include "ciotc_config.h" // Wifi configuration here
#include "ec.h"
#include "config.h"

#define GCP_PRIVATE_KEY "/ec_private.pem"

typedef struct {
  char jwt[256];
  time_t jwtExp = 0;
} MyJWT;

// !!REPLACEME!!
// The MQTT callback function for commands and configuration updates
// Place your message handler code here.
void messageReceivedAdvanced(MQTTClient *client, char topic[], char bytes[], int length)
{
  return;
  Serial.printf("Incoming Topic: %s", topic);
  if (length > 0)// On message
  {
    Serial.printf("\n\r   Data: %s", bytes);
  }
  Serial.println();
}
///////////////////////////////

// Initialize WiFi and MQTT for this board
static MQTTClient *mqttClient;
static BearSSL::WiFiClientSecure netClient;
static BearSSL::X509List certList;
static CloudIoTCoreDevice device(project_id, location, registry_id, device_id);
CloudIoTCoreMqtt *mqtt;

///////////////////////////////
// Helpers specific to this board
///////////////////////////////
String getDefaultSensor()
{
  return "Wifi: " + String(WiFi.RSSI()) + "db";
}

String getJwt()
{
  // Disable software watchdog as these operations can take a while.
  ESP.wdtDisable();
  time_t iat = time(nullptr);

  DEBUG_PRINTLN(F("Loading JWT from RTC"));
  RtcMemory r;
  bool jwtExistsInRtc = r.begin();
  MyJWT* myjwt = r.getData<MyJWT>();
  String jwt;
  if (!jwtExistsInRtc) {
    DEBUG_PRINTLN(F("No JWT found in memory"));
    DEBUG_PRINTLN(F("Creating JWT"));
    jwt = device.createJWT(iat, jwt_exp_secs);
    myjwt->jwtExp = iat + jwt_exp_secs;
    strcpy(myjwt->jwt, jwt.c_str());
    DEBUG_PRINT(F("Saving JWT to RTC memory: ")); DEBUG_PRINT(myjwt->jwt); DEBUG_PRINT(F(" expiry: ")); DEBUG_PRINTLN(myjwt->jwtExp);
    r.save();
  } else {
    DEBUG_PRINTLN(F("JWT found in memory"));
    DEBUG_PRINT(F("cutime ")); DEBUG_PRINT(iat); DEBUG_PRINT(F("\tjwt_expiry ")); DEBUG_PRINTLN(myjwt->jwtExp);
    if (myjwt->jwtExp < iat) {
      DEBUG_PRINTLN(F("JWT expired! Creating new one..."));
      jwt = device.createJWT(iat, jwt_exp_secs);
      myjwt->jwtExp = iat + jwt_exp_secs;
      strcpy(myjwt->jwt, jwt.c_str());
      DEBUG_PRINT(F("Saving JWT to RTC memory: ")); DEBUG_PRINT(myjwt->jwt); DEBUG_PRINT(F(" expiry: ")); DEBUG_PRINTLN(myjwt->jwtExp);
      r.save();
    }
    jwt = String(myjwt->jwt);
    DEBUG_PRINTLN(jwt);
  }
  ESP.wdtEnable(0);
  return jwt;
}

void invalidateJWT() {
  DEBUG_PRINTLN(F("Invalidating JWT..."));
  RtcMemory r;
  r.begin();
  MyJWT* myjwt = r.getData<MyJWT>();
  myjwt->jwtExp = 0;
  r.save();
  DEBUG_PRINTLN(F("JWT invalidated"));
}

static void readDerCert(const char *filename) {
  File ca = LittleFS.open(filename, "r");
  if (ca)
  {
    size_t size = ca.size();
    uint8_t cert[size];
    ca.read(cert, size);
    certList.append(cert, size);
    ca.close();

    Serial.print("Success to open ca file ");
  }
  else
  {
    Serial.print("Failed to open ca file ");
  }
  Serial.println(filename);
}

static void setupCertAndPrivateKey()
{
  // Set CA cert on wifi client
  // If using a static (pem) cert, uncomment in ciotc_config.h:
  // certList.append(primary_ca);
  // certList.append(backup_ca);
  // netClient.setTrustAnchors(&certList);

  unsigned char private_key[32];
  readPrivateKeyFromFile(GCP_PRIVATE_KEY, private_key);

  device.setPrivateKey(private_key);
  // return;

  // If using the (preferred) method with the cert and private key in /data (SPIFFS)
  // To get the private key run
  // openssl ec -in <private-key.pem> -outform DER -out private-key.der

  if (!LittleFS.begin())
  {
    Serial.println("Failed to mount file system");
    return;
  }

  readDerCert("/gtsltsr.crt"); // primary_ca.pem
  readDerCert("/GSR4.crt"); // backup_ca.pem
  netClient.setTrustAnchors(&certList);
}

///////////////////////////////
// Orchestrates various methods from preceeding code.
///////////////////////////////
bool publishTelemetry(const String &data)
{
  Serial.print("Outcoming: ");
  Serial.println(data);
  return mqtt->publishTelemetry(data);
}

bool publishTelemetry(const char *data, int length)
{
  return mqtt->publishTelemetry(data, length);
}

// TODO: fix globals
void setupCloudIoT(Config &config)
{
  // ESP8266 WiFi secure initialization and device private key
  setupCertAndPrivateKey();

  device.setJwtExpSecs(60 * 60 * 24);

  mqttClient = new MQTTClient(512);
  mqttClient->setOptions(180, true, 1000); // keepAlive, cleanSession, timeout
  mqtt = new CloudIoTCoreMqtt(mqttClient, &netClient, &device);
  mqtt->setUseLts(true);
  mqtt->startMQTTAdvanced(); // Opens connection using advanced callback
}