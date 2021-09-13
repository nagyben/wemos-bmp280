#ifndef API_H
#define API_H

#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecureBearSSL.h>

String postData(WiFiClient &client, HTTPClient &http, const char* url, const char* data)
{
  String payload = "";

  http.begin(client, url);
  http.addHeader("content-type", "application/json");

  char buffer[512];
  sprintf(buffer, "Making request to %s with data %s", url, data);
  Serial.println(buffer);
  int httpStatusCode = http.POST(data);

  if (httpStatusCode > 0)
  {
    Serial.print("HTTP Response code: ");
    Serial.println(httpStatusCode);
    payload = http.getString();
  }
  else
  {
    Serial.print("Error code: ");
    Serial.println(httpStatusCode);
  }
  // Free resources
  http.end();

  return payload;
}

#endif