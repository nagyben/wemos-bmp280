#ifndef HELPERS_H
#define HELPERS_H

#include <Arduino.h>

#define HEAP Serial.print("free heap: "); Serial.println(ESP.getFreeHeap())
#define STACK Serial.print("free stack: "); Serial.println(ESP.getFreeContStack())

void hexprint(const char* arr, size_t length) {
    char buffer[4];
    for (unsigned int i = 0; i < length; i++){
        sprintf(buffer, "%02x", arr[i]);
        Serial.print(buffer); Serial.print(" ");
    }
    Serial.println();
}
#endif