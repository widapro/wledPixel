#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>

// ESP8266 background jobs are serialized by main.cpp. Reusing one fixed arena
// avoids several persistent DynamicJsonDocument allocations and heap churn.
StaticJsonDocument<2048> &sharedJsonDocument();
char *sharedJsonBuffer();
size_t sharedJsonBufferCapacity();
