#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

struct Config {
  char ssid[128];
  char password[128];
  char url[128];
};

bool isConfigValid(Config &config) {
  return !(
    strcmp(config.ssid, "") == 0
    | strcmp(config.password, "") == 0
    | strcmp(config.url, "") == 0
  );
}

#endif