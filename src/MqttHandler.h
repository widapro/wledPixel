#pragma once

#include "Globals.h"

void MQTTPublishHADiscovry(const String &zone, const String &device_type);
void MQTTPublishState();
void MQTTCallback(char *topic, byte *payload, int length);
void MQTTProtectScheduledState(bool powerChanged, bool intensityChanged);
boolean reconnect();
