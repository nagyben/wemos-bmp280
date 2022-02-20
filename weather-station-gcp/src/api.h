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
// #define GCP_AUTH_URL "https://192.168.1.47:5000"

#define HEAP DEBUG_PRINTLN(ESP.getFreeHeap())
#define STACK DEBUG_PRINTLN(ESP.getFreeContStack())

unsigned long getTime();
void signPayload_rs256_base64(const unsigned char* payload, const char* privateKey, unsigned char* rsa256Signature_base64);
void base64UrlEncode(unsigned char* input, uint16_t inputLength, unsigned char*);
void getJWT(const char* payload, uint16_t payloadLength, const char* privateKey, char* jwt);
String postData(WiFiClient &client, HTTPClient &http, const char* url, const char* data, String &token);
String getGcpToken(WiFiClient &client, HTTPClient &http, Config &config, const char* privateKey);
String _unpackIdTokenFromGcpResponse(String response);

unsigned long getTime() {
  WiFiUDP ntpUDP;
  NTPClient timeClient(ntpUDP, "pool.ntp.org");
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

String postData(WiFiClient &client, HTTPClient &http, const char* url, const char* data, String &token)
{
  String payload = "";

  http.begin(client, url);
  http.addHeader("Content-Type", "application/json");
  http.addHeader("Authorization", "Bearer " + token);

  int httpStatusCode = http.POST(data);

  if (httpStatusCode > 0)
  {
    DEBUG_PRINT(F("HTTP Response code: "));
    DEBUG_PRINTLN(httpStatusCode);
    payload = http.getString();
  }
  else
  {
    DEBUG_PRINT(F("Error code: "));
    DEBUG_PRINTLN(httpStatusCode);
  }
  // Free resources
  http.end();

  return payload;
}

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

  DEBUG_PRINT(F("payload: ")); DEBUG_PRINTLN(payload);
  auto encodedPayload = std::make_unique<unsigned char[]>(1024);
  base64UrlEncode((unsigned char*) payload, payloadLength, encodedPayload.get());
  strcat(jwt, (char*) encodedPayload.get());

  auto rsa256Signature_base64 = std::make_unique<unsigned char[]>(1024);
  signPayload_rs256_base64((unsigned char*) jwt, privateKey, (unsigned char*) rsa256Signature_base64.get());

  strcat(jwt, ".");
  strcat(jwt, (char*) rsa256Signature_base64.get());
}

String getGcpToken(WiFiClient &client, HTTPClient &http, Config &config, const char* privateKey) {
  // see https://cloud.google.com/functions/docs/securing/authenticating
  DEBUG_PRINTLN(F("Generating JWT token for GCP auth..."));
  const unsigned long curtime = getTime();
  DynamicJsonDocument payload(512);
  payload["target_audience"] = config.url;
  payload["iss"] = config.saEmail;
  payload["sub"] = config.saEmail;
  payload["iat"] = curtime;
  payload["exp"] = curtime + 3600;
  payload["aud"] = GCP_AUTH_URL;

  String jsonData;
  serializeJson(payload, jsonData);
  DEBUG_PRINTLN(jsonData);

  auto jwt = std::make_unique<char[]>(1024);
  getJWT(jsonData.c_str(), jsonData.length(), privateKey, jwt.get());
  DEBUG_PRINT(strlen(jwt.get())); DEBUG_PRINT(F(" JWT: ")); DEBUG_PRINTLN(jwt.get());

  auto authHeader = std::make_unique<char[]>(1024);
  strcpy(authHeader.get(), "Bearer ");
  strcat(authHeader.get(), jwt.get());
  DEBUG_PRINT(strlen(authHeader.get())); DEBUG_PRINT(F(" authHeader: ")); DEBUG_PRINTLN(authHeader.get());

  http.setReuse(false);
  http.begin(client, GCP_AUTH_URL);
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");
  http.addHeader("Authorization", authHeader.get());

  auto body = std::make_unique<char[]>(1024);
  strcat(body.get(), "grant_type=urn:ietf:params:oauth:grant-type:jwt-bearer&assertion=");
  strcat(body.get(), jwt.get());
  DEBUG_PRINT(strlen(authHeader.get())); DEBUG_PRINTLN(body.get());
  int httpStatusCode = http.POST(body.get());

  String returnValue;
  if (httpStatusCode > 0)
  {
    DEBUG_PRINT(F("HTTP Response code: "));
    DEBUG_PRINTLN(httpStatusCode);
    returnValue = _unpackIdTokenFromGcpResponse(http.getString());
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

String _unpackIdTokenFromGcpResponse(String response) {
  return response.substring(13, response.length()-2);
}

#endif