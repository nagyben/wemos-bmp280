#ifndef HELPERS_H
#define HELPERS_H

#include <Arduino.h>

#ifndef HEAP
#define HEAP                   \
  Serial.print("free heap: "); \
  Serial.println(ESP.getFreeHeap())
#endif
#ifndef STACK
#define STACK                   \
  Serial.print("free stack: "); \
  Serial.println(ESP.getFreeContStack())
#endif

void hexprint(const char *arr, size_t length)
{
  char buffer[4];
  for (unsigned int i = 0; i < length; i++)
  {
    sprintf(buffer, "%02x", arr[i]);
    Serial.print(buffer);
    Serial.print(" ");
  }
  Serial.println();
}

void blink(int n){
  for(int i = 0; i < n; i++) {
    digitalWrite(LED_BUILTIN, LOW);
    delay(100);
    digitalWrite(LED_BUILTIN, HIGH);
    delay(100);
  }
}

#endif