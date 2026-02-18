#pragma once

#include "Globals.h"

// ─── Config Storage Functions
// ──────────────────────────────────────────────────

String processor(const String &var);
void saveVarsToConfFile(String groupName, uint8_t n);
void readConfig(String groupName, uint8_t n);
void readAllConfig();
