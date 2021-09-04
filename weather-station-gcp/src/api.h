#ifndef API_H
#define API_H

#include <Arduino.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecureBearSSL.h>
#include <config.h>

String callApi(WiFiClient &client, HTTPClient &http, char * url)
{
  String payload = "disconnected";

  http.begin(client, url);

  char buffer[100];
  sprintf(buffer, "Making request to %s", url);
  Serial.println(buffer);
  int httpStatusCode = http.GET();

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
    payload = "error";
  }
  // Free resources
  http.end();

  return payload;
}

#endif