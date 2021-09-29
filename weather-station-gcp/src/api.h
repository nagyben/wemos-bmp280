#ifndef API_H
#define API_H

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecureBearSSL.h>
#include <base64.hpp>

#ifndef DEBUG_PRINT
  #define DEBUG_PRINT(x)
#endif
#ifndef DEBUG_PRINTLN
  #define DEBUG_PRINTLN(x)
#endif

#define RS256_JWT_HEADER "eyJhbGciOiJSUzI1NiIsInR5cCI6IkpXVCJ9"

String postData(WiFiClient &client, HTTPClient &http, const char* url, const char* data)
{
  String payload = "";

  http.begin(client, url);
  http.addHeader("content-type", "application/json");

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

void signPayload_rs256_base64(const unsigned char* payload, const char* privateKey, unsigned char* rsa256Signature_base64) {
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
  DEBUG_PRINTLN("Generating RSA256 signature...");
  rsa256Sign(BR_HASH_OID_SHA256, sha256, sizeof(sha256), pKey->getRSA(), rsa256Signature);

  ESP.wdtEnable(3000); // re-enable watchdog timer

  DEBUG_PRINTLN("Encoding signature into base64...");
  // encode_base64(rsa256Signature, sizeof(rsa256Signature), rsa256Signature_base64);
  base64UrlEncode(rsa256Signature, sizeof(rsa256Signature), rsa256Signature_base64);
}

void base64UrlEncode(unsigned char* input, uint16_t inputLength, unsigned char* output) {
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

void getJWT(const char* payload, uint16_t payloadLength, const char* privateKey, char* jwt) {
  strcpy(jwt, RS256_JWT_HEADER);
  strcat(jwt, ".");

  // String payloadString(payload);
  // payloadString.replace(" ", "");

  unsigned char encodedPayload[1024];
  // encode_base64((unsigned char*) payloadString.c_str(), payloadString.length(), encodedPayload);
  base64UrlEncode((unsigned char*) payload, payloadLength-1, encodedPayload);
  strcat(jwt, (char*) encodedPayload);

  char rsa256Signature_base64[1024];
  signPayload_rs256_base64((unsigned char*) jwt, privateKey, (unsigned char*) rsa256Signature_base64);

  strcat(jwt, ".");
  strcat(jwt, rsa256Signature_base64);
}

#endif