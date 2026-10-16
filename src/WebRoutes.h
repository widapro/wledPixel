#pragma once

#include "Globals.h"

// Register all web-server routes (called from setup())
void setupWebRoutes();

// Execute an uploaded ESP8266 restore after the HTTP response has drained.
bool webServicePendingRestore();
