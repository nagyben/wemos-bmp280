#ifndef EC_H
#define EC_H

#include <BearSSLHelpers.h>
#include <LittleFS.h>
#include <base64.hpp>
#include <helpers.h>

#ifndef DEBUG_PRINT
  #define DEBUG_PRINT(x)
#endif
#ifndef DEBUG_PRINTLN
  #define DEBUG_PRINTLN(x)
#endif

void readPrivateKeyFromFile(const char* fileName, unsigned char* pkeyBuffer) {
    if (!LittleFS.begin()) {
      DEBUG_PRINTLN(F("Could not start LittleFS!"));
      return;
    }

    DEBUG_PRINT(F("Reading private key file: ")); DEBUG_PRINTLN(fileName);
    File f = LittleFS.open(fileName, "r");

    String contents = f.readString();
    f.close();

    BearSSL::PrivateKey pkey(contents.c_str());
    strncpy((char *)pkeyBuffer, (const char *)pkey.getEC()->x, pkey.getEC()->xlen);
}
#endif
