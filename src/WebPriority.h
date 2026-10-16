#pragma once

#include <Arduino.h>

void noteWebActivity();
bool beginTrackedWebRequest(bool apiRequest);
void endTrackedWebRequest(bool apiRequest);
bool webPriorityIsActive(unsigned long now);
bool serviceWebPriorityMode(unsigned long now);
bool webPriorityHasActiveRequests();
uint8_t webPriorityActiveRequestCount();
uint8_t webPriorityActiveApiCount();
unsigned long webPriorityLastActivity();

// Gives a foreground outbound HTTP(S) request a temporary network-memory
// budget. Incoming web/API requests still have priority and may abort it.
bool beginExternalNetworkRequest(bool tls);
void endExternalNetworkRequest();
