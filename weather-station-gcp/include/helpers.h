#ifndef HELPERS_H
#define HELPERS_H

#include <Arduino.h>

#ifndef HEAP
#define HEAP                   \
  DEBUG_PRINT("free heap: "); \
  DEBUG_PRINTLN(ESP.getFreeHeap())
#endif
#ifndef STACK
#define STACK                   \
  DEBUG_PRINT("free stack: "); \
  DEBUG_PRINTLN(ESP.getFreeContStack())
#endif

void hexprint(const char *arr, size_t length)
{
  char buffer[4];
  for (unsigned int i = 0; i < length; i++)
  {
    sprintf(buffer, "%02x", arr[i]);
    DEBUG_PRINT(buffer);
    DEBUG_PRINT(" ");
  }
  DEBUG_PRINTLN();
}

inline void blink(int n){
  for(int i = 0; i < n; i++) {
    digitalWrite(LED_BUILTIN, LOW);
    delay(100);
    digitalWrite(LED_BUILTIN, HIGH);
    delay(100);
  }
}

#endif