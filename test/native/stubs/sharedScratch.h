#pragma once

#include "ArduinoJson.h"

inline StaticJsonDocument<2048> &sharedJsonDocument() {
  static StaticJsonDocument<2048> document;
  return document;
}
