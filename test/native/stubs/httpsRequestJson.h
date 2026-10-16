#pragma once

#include "Arduino.h"
#include "ArduinoJson.h"

inline bool httpsRequestToDoc(const String &, int, const String &,
                              const String &, bool, JsonDocument &) {
  return false;
}
