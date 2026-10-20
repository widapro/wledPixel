#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>

// ESP8266 background jobs are serialized by main.cpp. Reusing one fixed arena
// avoids several persistent DynamicJsonDocument allocations and heap churn.
StaticJsonDocument<2048> &sharedJsonDocument();
// May return nullptr if the ESP8266 secondary heap cannot supply the buffer.
char *sharedJsonBuffer();
size_t sharedJsonBufferCapacity();
