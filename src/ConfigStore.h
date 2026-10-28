#pragma once

#include "Globals.h"

// ─── Config Storage Functions
// ──────────────────────────────────────────────────

void saveVarsToConfFile(const char *groupName, uint8_t n);
inline void saveVarsToConfFile(const String &groupName, uint8_t n) {
  saveVarsToConfFile(groupName.c_str(), n);
}
void saveSchedulerStateToConfFile();
bool queueConfigSave(const char *groupName, uint8_t n = 0);
bool serviceDeferredConfigSave();
bool hasDeferredConfigSaves();
void readConfig(const char *groupName, uint8_t n);
void readAllConfig();
