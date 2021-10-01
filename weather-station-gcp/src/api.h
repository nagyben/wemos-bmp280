#ifndef API_H
#define API_H

#include <Arduino.h>
#include <NTPClient.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecureBearSSL.h>
#include <WiFiUdp.h>
#include <base64.hpp>
#include <ArduinoJson.h>
#include "config.h"

#ifndef DEBUG_PRINT
  #define DEBUG_PRINT(x)
#endif
#ifndef DEBUG_PRINTLN
  #define DEBUG_PRINTLN(x)
#endif

#define RS256_JWT_HEADER "eyJhbGciOiJSUzI1NiIsInR5cCI6IkpXVCJ9"
#define GCP_AUTH_URL "https://www.googleapis.com/oauth2/v4/token"

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org");

unsigned long getTime() {
  timeClient.begin();
  unsigned long now;
  do {
    timeClient.update();
    now = timeClient.getEpochTime();
    DEBUG_PRINT(now); DEBUG_PRINT(F(" "));
    delay(500);
  } while (now < 100000);
  return now;
}

String postData(WiFiClient &client, HTTPClient &http, const char* url, const char* data)
{
  String payload = "";

  http.begin(client, url);
  http.addHeader("Content-Type", "application/json");

  char buffer[512];
  sprintf(buffer, "Making request to %s with data %s", url, data);
  DEBUG_PRINTLN(buffer);
  int httpStatusCode = http.POST(data);

  if (httpStatusCode > 0)
  {
    DEBUG_PRINT("HTTP Response code: ");
    DEBUG_PRINTLN(httpStatusCode);
    payload = http.getString();
  }
  else
  {
    DEBUG_PRINT("Error code: ");
    DEBUG_PRINTLN(httpStatusCode);
  }
  // Free resources
  http.end();

  return payload;
}

void signPayload_rs256_base64(const unsigned char* payload, const char* privateKey, unsigned char* rsa256Signature_base64);
void base64UrlEncode(unsigned char* input, uint16_t inputLength, unsigned char*);
void getJWT(const char* payload, uint16_t payloadLength, const char* privateKey, char* jwt);

inline void signPayload_rs256_base64(const unsigned char* payload, const char* privateKey, unsigned char* rsa256Signature_base64) {
  br_sha256_context* ctx = new br_sha256_context;
  unsigned char sha256[256/8]; // stores sha256 of payload
  unsigned char rsa256Signature[2048/8];
  BearSSL::PrivateKey *pKey;
  pKey = new BearSSL::PrivateKey(privateKey);

  br_rsa_pkcs1_sign rsa256Sign = br_rsa_pkcs1_sign_get_default();

  br_sha256_init(ctx);
  br_sha256_update(ctx, payload, strlen((char*)payload));
  br_sha256_out(ctx, sha256); // calculate sha256

  ESP.wdtDisable(); // disable watchdog for this part because it takes longer than the timeout

  // get signature of sha256 hashed payload
  DEBUG_PRINTLN(F("Generating RSA256 signature..."));
  rsa256Sign(BR_HASH_OID_SHA256, sha256, sizeof(sha256), pKey->getRSA(), rsa256Signature);

  ESP.wdtEnable(3000); // re-enable watchdog timer

  DEBUG_PRINTLN(F("Encoding signature into base64..."));
  // encode_base64(rsa256Signature, sizeof(rsa256Signature), rsa256Signature_base64);
  base64UrlEncode(rsa256Signature, sizeof(rsa256Signature), rsa256Signature_base64);
  DEBUG_PRINTLN((char*)rsa256Signature_base64);
}

inline void base64UrlEncode(unsigned char* input, uint16_t inputLength, unsigned char* output) {
  unsigned int b64Length = encode_base64(input, inputLength, output);
  for (int i = 0; i < b64Length; i++) {
    if (output[i] == '/') {
      output[i] = '_';
      continue;
    }
    if (output[i] == '+') {
      output[i] = '-';
      continue;
    }
    if (output[i] == '=') {
      output[i] = '\0';
      break;
    }
  }
}

inline void getJWT(const char* payload, uint16_t payloadLength, const char* privateKey, char* jwt) {
  strcpy(jwt, RS256_JWT_HEADER);
  strcat(jwt, ".");

  DEBUG_PRINT("payload: "); DEBUG_PRINTLN(payload);
  unsigned char encodedPayload[1024];
  base64UrlEncode((unsigned char*) payload, payloadLength, encodedPayload);
  strcat(jwt, (char*) encodedPayload);

  char rsa256Signature_base64[1024];
  signPayload_rs256_base64((unsigned char*) jwt, privateKey, (unsigned char*) rsa256Signature_base64);

  strcat(jwt, ".");
  strcat(jwt, rsa256Signature_base64);
}

String getGcpToken(WiFiClient &client, HTTPClient &http, Config &config, const char* privateKey) {
  // see https://cloud.google.com/functions/docs/securing/authenticating
  DEBUG_PRINTLN(F("Generating JWT token for GCP auth..."));
  const unsigned long curtime = getTime();
  DynamicJsonDocument payload(256);
  payload["target_audience"] = config.url;
  payload["iss"] = config.saEmail;
  payload["sub"] = config.saEmail;
  payload["iat"] = curtime;
  payload["exp"] = curtime + 3600;
  payload["aud"] = GCP_AUTH_URL;

  String jsonData;
  serializeJson(payload, jsonData);
  DEBUG_PRINTLN(jsonData.c_str());

  char jwt[1200];
  getJWT(jsonData.c_str(), jsonData.length(), privateKey, jwt);
  DEBUG_PRINT(F("JWT: ")); DEBUG_PRINTLN(jwt);

  char authHeader[1200];
  strcpy(authHeader, "Bearer ");
  strcat(authHeader, jwt);
  DEBUG_PRINT(F("authHeader: ")); DEBUG_PRINTLN(authHeader);

  http.begin(client, GCP_AUTH_URL);
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");
  http.addHeader("Authorization", authHeader);

  char body[1000];
  strcat(body, "grant_type=urn:ietf:params:oauth:grant-type:jwt-bearer&assertion=");
  strcat(body, jwt);
  char buffer[1000];
  sprintf(buffer, "Making request to %s with data %s", GCP_AUTH_URL, body);
  DEBUG_PRINTLN(buffer);
  int httpStatusCode = http.POST(body);

  String returnValue;
  if (httpStatusCode > 0)
  {
    DEBUG_PRINT(F("HTTP Response code: "));
    DEBUG_PRINTLN(httpStatusCode);
    returnValue = http.getString();
  }
  else
  {
    DEBUG_PRINT(F("Error code: "));
    DEBUG_PRINTLN(httpStatusCode);
    returnValue = http.errorToString(httpStatusCode);
  }
  // Free resources
  http.end();

  return returnValue;
}

#endif