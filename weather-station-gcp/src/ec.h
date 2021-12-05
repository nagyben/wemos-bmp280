#ifndef EC_H
#define EC_H

#include <BearSSLHelpers.h>
#include <LittleFS.h>
#include <base64.hpp>
#include <helpers.h>

void readPrivateKeyFromFile(const char* fileName, unsigned char* pkeyBuffer) {
    File f = LittleFS.open(fileName, "r");

    String contents = f.readString();
    f.close();

    BearSSL::PrivateKey pkey(contents.c_str());

    const auto ec = pkey.getEC();
    strncpy((char *)pkeyBuffer, (const char *)ec->x, 32);
}
#endif
