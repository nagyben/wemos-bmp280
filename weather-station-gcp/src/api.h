#ifndef API_H
#define API_H

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecureBearSSL.h>

#ifndef DEBUG_PRINT
  #define DEBUG_PRINT(x)
#endif
#ifndef DEBUG_PRINTLN
  #define DEBUG_PRINTLN(x)
#endif

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
    for (int i = 0; i < 3; i++) {
      digitalWrite(LED_BUILTIN, LOW);
      delay(750);
      digitalWrite(LED_BUILTIN, HIGH);
      delay(200);
    }
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

#endif