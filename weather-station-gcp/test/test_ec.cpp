#include <Arduino.h>
#include <unity.h>
#include <LittleFS.h>
#include "ec.h"
#include <helpers.h>

void test_get_pkey_from_pem_file(void) {
    String privateKey_content = R"(-----BEGIN EC PRIVATE KEY-----
MHcCAQEEIB3UR25L5vbYcIamyEVecRj+fiV7IUuUhVW888eANFD6oAoGCCqGSM49
AwEHoUQDQgAEJvmqqM9HCLKJZAQ+LXJTKAG6IGgClZCt9r2euJ1C73ajt9dTOEgX
43zUmhIgwIk3p41ahwEdQYjTHxL/lJWZXA==
-----END EC PRIVATE KEY-----)";

    bool fsStatus = LittleFS.begin();
    TEST_ASSERT_TRUE(fsStatus);

    File f = LittleFS.open("/test.pem", "w+");
    f.print(privateKey_content);
    f.close();


    unsigned char actualPrivateKey[32];
    readPrivateKeyFromFile("/test.pem", actualPrivateKey);


    const unsigned char expectedPrivateKey[] = {
        0x1d, 0xd4, 0x47, 0x6e, 0x4b, 0xe6, 0xf6, 0xd8, 0x70, 0x86, 0xa6, 0xc8, 0x45, 0x5e, 0x71,
        0x18, 0xfe, 0x7e, 0x25, 0x7b, 0x21, 0x4b, 0x94, 0x85, 0x55, 0xbc, 0xf3, 0xc7, 0x80, 0x34,
        0x50, 0xfa
    };

    Serial.println();
    Serial.print("expected: "); hexprint((const char*)expectedPrivateKey, sizeof(expectedPrivateKey));
    Serial.print("actual:   "); hexprint((const char*)actualPrivateKey, sizeof(actualPrivateKey));
    TEST_ASSERT_EQUAL_CHAR_ARRAY((const char*)expectedPrivateKey, (const char*)actualPrivateKey, sizeof(expectedPrivateKey));
}

void setup() {
    Serial.begin(115200);
    UNITY_BEGIN();
    RUN_TEST(test_get_pkey_from_pem_file);
    UNITY_END();
}

void loop() {}