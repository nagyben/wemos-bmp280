#include <Arduino.h>
#include <unity.h>
#include "config.h"

void test_config_valid(void) {
    Config config;
    String("ssid").toCharArray(config.ssid, sizeof(config.ssid));
    String("password").toCharArray(config.password, sizeof(config.password));
    String("url").toCharArray(config.url, sizeof(config.url));

    TEST_ASSERT_TRUE(isConfigValid(config));
}

void test_config_invalid(void) {
    Config config;
    String("ssid").toCharArray(config.ssid, sizeof(config.ssid));
    String("").toCharArray(config.password, sizeof(config.password));
    String("url").toCharArray(config.url, sizeof(config.url));

    TEST_ASSERT_FALSE(isConfigValid(config));
}

int main(int argc, char **argv) {
    UNITY_BEGIN();
    RUN_TEST(test_config_valid);
    RUN_TEST(test_config_invalid);
    UNITY_END();

    return 0;
}