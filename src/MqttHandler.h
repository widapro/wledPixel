#pragma once

#include "Globals.h"

// Forward declarations for functions from other modules
void saveVarsToConfFile(String groupName, uint8_t n);

void MQTTPublishHADiscovry(String zone, String device_type);
void MQTTPublishState();
void MQTTCallback(char *topic, byte *payload, int length);
boolean reconnect();
