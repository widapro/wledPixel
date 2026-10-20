#include "ConfigStore.h"
#include "Scheduler.h"
#include "ConfigStringStorage.h"
#include <cstdio>
#include <cstring>

namespace {
constexpr uint8_t kDeferredSaveCapacity = 16;

enum DeferredGroup : uint8_t {
  DG_INVALID = 0,
  DG_SYSTEM,
  DG_DISPLAY,
  DG_MQTT,
  DG_CLOCK,
  DG_OWM,
  DG_HA,
  DG_DS18,
  DG_INTENSITY,
  DG_ZONE,
  DG_PROGRESS,
  DG_STOCK,
  DG_CRON_GLOBAL,
  DG_CRON_SETTINGS,
};

struct DeferredConfigSave {
  DeferredGroup group;
  uint8_t index;
};

DeferredConfigSave deferredSaves[kDeferredSaveCapacity];
uint8_t deferredSaveCount = 0;

DeferredGroup deferredGroupFromName(const char *name) {
  if (!name) return DG_INVALID;
  if (strcmp(name, "systemSettings") == 0) return DG_SYSTEM;
  if (strcmp(name, "displaySettings") == 0) return DG_DISPLAY;
  if (strcmp(name, "mqttSettings") == 0) return DG_MQTT;
  if (strcmp(name, "wallClockSett") == 0) return DG_CLOCK;
  if (strcmp(name, "owmSettings") == 0) return DG_OWM;
  if (strcmp(name, "haSettings") == 0) return DG_HA;
  if (strcmp(name, "ds18b20Settings") == 0) return DG_DS18;
  if (strcmp(name, "intensity") == 0) return DG_INTENSITY;
  if (strcmp(name, "zoneSettings") == 0) return DG_ZONE;
  if (strcmp(name, "pbSettings") == 0) return DG_PROGRESS;
  if (strcmp(name, "stockSettings") == 0) return DG_STOCK;
  if (strcmp(name, "cronGlobal") == 0) return DG_CRON_GLOBAL;
  if (strcmp(name, "cronSettings") == 0) return DG_CRON_SETTINGS;
  return DG_INVALID;
}

const char *deferredGroupName(DeferredGroup group) {
  switch (group) {
  case DG_SYSTEM: return "systemSettings";
  case DG_DISPLAY: return "displaySettings";
  case DG_MQTT: return "mqttSettings";
  case DG_CLOCK: return "wallClockSett";
  case DG_OWM: return "owmSettings";
  case DG_HA: return "haSettings";
  case DG_DS18: return "ds18b20Settings";
  case DG_INTENSITY: return "intensity";
  case DG_ZONE: return "zoneSettings";
  case DG_PROGRESS: return "pbSettings";
  case DG_STOCK: return "stockSettings";
  case DG_CRON_GLOBAL: return "cronGlobal";
  case DG_CRON_SETTINGS: return "cronSettings";
  default: return "";
  }
}
} // namespace

bool queueConfigSave(const char *groupName, uint8_t n) {
  DeferredGroup group = deferredGroupFromName(groupName);
  if (group == DG_INVALID)
    return false;
  for (uint8_t i = 0; i < deferredSaveCount; i++) {
    if (deferredSaves[i].index == n && deferredSaves[i].group == group)
      return true;
  }
  if (deferredSaveCount >= kDeferredSaveCapacity) {
    Serial.printf_P(PSTR("\n[Config] Deferred save queue full: %s/%u"), groupName, n);
    return false;
  }
  DeferredConfigSave &entry = deferredSaves[deferredSaveCount++];
  entry.group = group;
  entry.index = n;
  return true;
}

bool serviceDeferredConfigSave() {
  if (deferredSaveCount == 0)
    return false;
  SchedulerWorkGuard work;
  DeferredConfigSave entry = deferredSaves[0];
  for (uint8_t i = 1; i < deferredSaveCount; i++)
    deferredSaves[i - 1] = deferredSaves[i];
  deferredSaveCount--;
  const char *groupName = deferredGroupName(entry.group);
  Serial.printf_P(PSTR("\n[Config] Deferred save: %s/%u"), groupName, entry.index);
  if (entry.group == DG_CRON_GLOBAL)
    saveSchedulerEnabledToConfFile();
  else
    saveVarsToConfFile(groupName, entry.index);
  return true;
}

bool hasDeferredConfigSaves() { return deferredSaveCount > 0; }

namespace {
constexpr uint8_t kZoneCapacity = 4;

typedef struct __attribute__((packed)) {
  bool     enabled;
  uint8_t  minute;
  uint8_t  hour;
  uint8_t  dayOfWeek;
  uint32_t dayOfMonth;
  uint8_t  power;
  uint8_t  intensity;
  uint8_t  zoneCount;
  uint8_t  pbEnable;
  uint8_t  zoneBegin[4];
  uint8_t  zoneEnd[4];
  uint8_t  zoneWorkMode[4];
  uint8_t  zoneOwmDisplay[4];
  uint8_t  zoneClockFmt[4];
  uint8_t  zoneFont[4];
  char     zoneHaSensorId[4][32];
  char     zoneHaPostfix[4][6];
  char     zoneMqttTopic[4][32];
  char     name[12];
} LegacyScheduleProfile;

void buildZoneKey(char *out, size_t outSize, uint8_t zone, const char *suffix) {
  snprintf_P(out, outSize, PSTR("zone%u%s"), zone, suffix);
}

// These missing-key defaults match every zone's initial values in Globals.cpp.
// Reclaim them before any new writes, including on the first boot after OTA.
void reclaimZoneDefaultStrings(uint8_t n) {
  Preferences storage;
  if (!storage.begin("zoneSettings", false)) return;
  struct Setting { const char *suffix; const String *value; const char *fallback; };
  const Setting settings[] = {
      {"mqttPosfix", &zones[n].mqttPostfix, ""},
      {"HaSensorId", &zones[n].haSensorId, ""},
      {"HaSensorPf", &zones[n].haSensorPostfix, ""},
      {"Ds18b20Pf", &zones[n].ds18b20Postfix, ""},
      {"CdPrefix", &zones[n].countdownPrefix, ""},
      {"CdSuffix", &zones[n].countdownSuffix, ""},
      {"CdTarget", &countdownState[n].targetStr, ""},
      {"StockSym", &zones[n].stockSymbols, ""},
      {"StockPre", &zones[n].stockPrefix, ""},
      {"StockPst", &zones[n].stockPostfix, ""},
      {"WorkMode", &zones[n].workMode, "manualInput"},
      {"ScrolAlign", &zones[n].scrollAlign, "PA_CENTER"},
      {"ScrolEfIn", &zones[n].scrollEffectIn, "PA_SCROLL_DOWN"},
      {"ScrolEfOut", &zones[n].scrollEffectOut, "PA_NO_EFFECT"},
      {"Font", &zones[n].font, "wledFont_cyrillic"},
      {"ClDispForm", &zones[n].clockDisplayFormat, "HHMM"},
      {"OwmWhDisp", &zones[n].owmWhatToDisplay, "owmTemperature"},
      {"CdFormat", &zones[n].countdownFormat, "DDHHMMSS"},
      {"CdFinish", &zones[n].countdownFinish, "blinkForever"},
      {"StockFmt", &zones[n].stockDisplayFormat, "namePrice"}};
  for (const auto &setting : settings) {
    if (*setting.value != setting.fallback) continue;
    char key[24]; buildZoneKey(key, sizeof(key), n, setting.suffix);
    if (storage.isKey(key) && storage.getString(key, "") == setting.fallback)
      storage.remove(key);
  }
}

uint8_t normalizedZoneCount(uint8_t raw) {
  if (raw < 1)
    return 1;
  if (raw > kZoneCapacity)
    return kZoneCapacity;
  return raw;
}

void migrateLegacyScheduleProfile(const LegacyScheduleProfile &oldProfile,
                                  ScheduleProfile &profile) {
  profile.enabled = oldProfile.enabled;
  profile.minute = oldProfile.minute;
  profile.hour = oldProfile.hour;
  profile.dayOfWeek = oldProfile.dayOfWeek;
  profile.dayOfMonth = oldProfile.dayOfMonth;
  profile.power = oldProfile.power;
  profile.intensity = oldProfile.intensity;
  profile.zoneCount = oldProfile.zoneCount;
  profile.pbEnable = oldProfile.pbEnable;
  memcpy(profile.zoneBegin, oldProfile.zoneBegin, sizeof(profile.zoneBegin));
  memcpy(profile.zoneEnd, oldProfile.zoneEnd, sizeof(profile.zoneEnd));
  memcpy(profile.zoneWorkMode, oldProfile.zoneWorkMode,
         sizeof(profile.zoneWorkMode));
  memcpy(profile.zoneOwmDisplay, oldProfile.zoneOwmDisplay,
         sizeof(profile.zoneOwmDisplay));
  memcpy(profile.zoneClockFmt, oldProfile.zoneClockFmt,
         sizeof(profile.zoneClockFmt));
  memcpy(profile.zoneFont, oldProfile.zoneFont, sizeof(profile.zoneFont));
  for (uint8_t z = 0; z < kZoneCapacity; z++) {
    memset(profile.zoneHaSensorId[z], 0, sizeof(profile.zoneHaSensorId[z]));
    memcpy(profile.zoneHaSensorId[z], oldProfile.zoneHaSensorId[z],
           sizeof(oldProfile.zoneHaSensorId[z]));
    if ((uint8_t)oldProfile.zoneHaSensorId[z][0] != SCHED_STR_NO_CHANGE)
      profile.zoneHaSensorId[z][sizeof(oldProfile.zoneHaSensorId[z]) - 1] =
          '\0';
  }
  memcpy(profile.zoneHaPostfix, oldProfile.zoneHaPostfix,
         sizeof(profile.zoneHaPostfix));
  memcpy(profile.zoneMqttTopic, oldProfile.zoneMqttTopic,
         sizeof(profile.zoneMqttTopic));
  memcpy(profile.name, oldProfile.name, sizeof(profile.name));
  profile.name[sizeof(profile.name) - 1] = '\0';
}

} // namespace

// ─── Save Config
// ───────────────────────────────────────────────────────────────
void saveVarsToConfFile(const char *groupName, uint8_t n) {
  if (!groupName)
    return;
  preferences.begin(groupName, false);

  if (strcmp(groupName, "systemSettings") == 0) {
    preferences.putString("deviceName", shortMACaddr);
    preferences.putBool("disablServMess", disableServiceMessages);
    preferences.putBool("displayFlip", displayFlip);
  }

  if (strcmp(groupName, "displaySettings") == 0) {
    preferences.putUChar("zoneNumbers", normalizedZoneCount(zoneNumbers));
    preferences.putUChar("zones0Begin", zones[0].begin);
    preferences.putUChar("zones0End", zones[0].end);
    preferences.putUChar("zones1Begin", zones[1].begin);
    preferences.putUChar("zones1End", zones[1].end);
    preferences.putUChar("zones2Begin", zones[2].begin);
    preferences.putUChar("zones2End", zones[2].end);
    preferences.putUChar("zones3Begin", zones[3].begin);
    preferences.putUChar("zones3End", zones[3].end);
  }

  if (strcmp(groupName, "zoneSettings") == 0) {
    if (n >= kZoneCapacity) {
      preferences.end();
      return;
    }

    char key[24];
    reclaimZoneDefaultStrings(n);
    const char *optionalKeys[] = {"mqttPosfix", "HaSensorId", "HaSensorPf",
        "Ds18b20Pf", "CdPrefix", "CdSuffix", "CdTarget", "StockSym", "StockPre", "StockPst"};
    const String *optionalValues[] = {&zones[n].mqttPostfix, &zones[n].haSensorId,
        &zones[n].haSensorPostfix, &zones[n].ds18b20Postfix, &zones[n].countdownPrefix,
        &zones[n].countdownSuffix, &countdownState[n].targetStr, &zones[n].stockSymbols,
        &zones[n].stockPrefix, &zones[n].stockPostfix};
    for (uint8_t i = 0; i < 10; ++i) {
      if (optionalValues[i]->length()) continue;
      buildZoneKey(key, sizeof(key), n, optionalKeys[i]);
      storeOptionalConfigString(preferences, key, *optionalValues[i]);
    }
    buildZoneKey(key, sizeof(key), n, "WorkMode");
    storeDefaultConfigString(preferences, key, zones[n].workMode, "manualInput");
    buildZoneKey(key, sizeof(key), n, "ScrolSpeed");
    preferences.putUChar(key, zones[n].scrollSpeed);
    buildZoneKey(key, sizeof(key), n, "ScrolPause");
    preferences.putUChar(key, zones[n].scrollPause);
    buildZoneKey(key, sizeof(key), n, "ScrolAlign");
    storeDefaultConfigString(preferences, key, zones[n].scrollAlign, "PA_CENTER");
    buildZoneKey(key, sizeof(key), n, "ScrolEfIn");
    storeDefaultConfigString(preferences, key, zones[n].scrollEffectIn, "PA_SCROLL_DOWN");
    buildZoneKey(key, sizeof(key), n, "ScrolEfOut");
    storeDefaultConfigString(preferences, key, zones[n].scrollEffectOut, "PA_NO_EFFECT");
    buildZoneKey(key, sizeof(key), n, "Font");
    storeDefaultConfigString(preferences, key, zones[n].font, "wledFont_cyrillic");
    buildZoneKey(key, sizeof(key), n, "Charspac");
    preferences.putUChar(key, zones[n].charspacing);
    buildZoneKey(key, sizeof(key), n, "mqttTexTop");
    preferences.putString(key, MQTTZones[n].message);
    buildZoneKey(key, sizeof(key), n, "mqttPosfix");
    storeOptionalConfigString(preferences, key, zones[n].mqttPostfix);
    buildZoneKey(key, sizeof(key), n, "ClDispForm");
    storeDefaultConfigString(preferences, key, zones[n].clockDisplayFormat, "HHMM");
    buildZoneKey(key, sizeof(key), n, "OwmWhDisp");
    storeDefaultConfigString(preferences, key, zones[n].owmWhatToDisplay, "owmTemperature");
    buildZoneKey(key, sizeof(key), n, "HaSensorId");
    storeOptionalConfigString(preferences, key, zones[n].haSensorId);
    buildZoneKey(key, sizeof(key), n, "HaSensorPf");
    storeOptionalConfigString(preferences, key, zones[n].haSensorPostfix);
    buildZoneKey(key, sizeof(key), n, "Ds18b20Pf");
    storeOptionalConfigString(preferences, key, zones[n].ds18b20Postfix);
    buildZoneKey(key, sizeof(key), n, "WoprUpdInt");
    preferences.putUShort(key, woprZones[n].updateInterval);
    buildZoneKey(key, sizeof(key), n, "ScrolInf");
    preferences.putBool(key, zones[n].scrollInfinite);
    // Countdown settings
    buildZoneKey(key, sizeof(key), n, "CdFormat");
    storeDefaultConfigString(preferences, key, zones[n].countdownFormat, "DDHHMMSS");
    buildZoneKey(key, sizeof(key), n, "CdFinish");
    storeDefaultConfigString(preferences, key, zones[n].countdownFinish, "blinkForever");
    buildZoneKey(key, sizeof(key), n, "CdUnits");
    preferences.putBool(key, zones[n].countdownShowUnits);
    buildZoneKey(key, sizeof(key), n, "CdPrefix");
    storeOptionalConfigString(preferences, key, zones[n].countdownPrefix);
    buildZoneKey(key, sizeof(key), n, "CdSuffix");
    storeOptionalConfigString(preferences, key, zones[n].countdownSuffix);
    buildZoneKey(key, sizeof(key), n, "CdTarget");
    storeOptionalConfigString(preferences, key, countdownState[n].targetStr);
    // Stock ticker settings
    buildZoneKey(key, sizeof(key), n, "StockSym");
    storeOptionalConfigString(preferences, key, zones[n].stockSymbols);
    buildZoneKey(key, sizeof(key), n, "StockFmt");
    storeDefaultConfigString(preferences, key, zones[n].stockDisplayFormat, "namePrice");
    buildZoneKey(key, sizeof(key), n, "StockPre");
    storeOptionalConfigString(preferences, key, zones[n].stockPrefix);
    buildZoneKey(key, sizeof(key), n, "StockPst");
    storeOptionalConfigString(preferences, key, zones[n].stockPostfix);
    buildZoneKey(key, sizeof(key), n, "StockArr");
    preferences.putBool(key, zones[n].stockShowArrows);
  }

  if (strcmp(groupName, "mqttSettings") == 0) {
    preferences.putBool("mqttEnable", mqttEnable);
    preferences.putString("mqttServerAddr", mqttServerAddress);
    preferences.putUShort("mqttServerPort", mqttServerPort);
    preferences.putString("mqttUsername", mqttUsername);
    preferences.putString("mqttPassword", mqttPassword);
  }

  if (strcmp(groupName, "wallClockSett") == 0) {
    preferences.putFloat("ntpTz", ntpTimeZone);
    preferences.putBool("disableDotBlink", disableDotsBlink);
    preferences.putUShort("ntpUpdateInt", ntpUpdateInterval);
    preferences.putString("ntpServer", ntpServer);
  }

  if (strcmp(groupName, "owmSettings") == 0) {
    preferences.putString("owmApiToken", owmApiToken);
    preferences.putString("owmUnitsFormat", owmUnitsFormat);
    preferences.putUShort("owmUpdateInt", owmUpdateInterval);
    preferences.putString("owmCity", owmCity);
  }

  if (strcmp(groupName, "haSettings") == 0) {
    preferences.putString("haAddr", haAddr);
    preferences.putString("haApiToken", haApiToken);
    preferences.putString("haApiHttpType", haApiHttpType);
    preferences.putUShort("haApiPort", haApiPort);
    preferences.putUShort("haUpdateInt", haUpdateInterval);
  }

  if (strcmp(groupName, "ds18b20Settings") == 0) {
    preferences.putBool("ds18b20Enable", ds18b20Enable);
    preferences.putUShort("ds18b20UpdInt", ds18b20UpdateInterval);
    preferences.putString("ds18b20UnitFr", ds18b20UnitsFormat);
  }

  if (strcmp(groupName, "stockSettings") == 0) {
    preferences.putString("stApiToken", stockApiToken);
    preferences.putUShort("stUpdateInt", stockUpdateInterval);
  }

  if (strcmp(groupName, "pbSettings") == 0) {
    // Reclaim unset live strings before appending any other settings. Existing
    // ESP32 NVS partitions can otherwise be full of empty/default records.
    const char *optionalKeys[] = {"pb%uSrcId", "pb%uCdSrcI", "pb%uCdVal",
                                  "pb%uDSrcI", "pb%uDR1V", "pb%uDR2V"};
    for (uint8_t i = 0; i < kZoneCapacity; ++i) {
      const String *values[] = {&progressBars[i].dataSourceId,
                               &progressBars[i].conditionSourceId,
                               &progressBars[i].conditionValue,
                               &progressBars[i].dynSourceId,
                               &progressBars[i].dynRule1Value,
                               &progressBars[i].dynRule2Value};
      for (uint8_t field = 0; field < 6; ++field) {
        if (values[field]->length() != 0) continue;
        char key[24];
        snprintf(key, sizeof(key), optionalKeys[field], i);
        if (!storeOptionalConfigString(preferences, key, *values[field]))
          Serial.printf_P(PSTR("\n[Config] Failed to clear optional PB key %s"), key);
      }
    }
    for (uint8_t i = 0; i < kZoneCapacity; i++) {
      char key[24];
      snprintf_P(key, sizeof(key), PSTR("pb%uEnable"), i);
      preferences.putBool(key, progressBars[i].enabled);
      snprintf_P(key, sizeof(key), PSTR("pb%uSrcType"), i);
      preferences.putString(key, progressBars[i].dataSourceType);
      snprintf_P(key, sizeof(key), PSTR("pb%uSrcId"), i);
      storeOptionalConfigString(preferences, key, progressBars[i].dataSourceId);
      snprintf_P(key, sizeof(key), PSTR("pb%uMin"), i);
      preferences.putFloat(key, progressBars[i].minValue);
      snprintf_P(key, sizeof(key), PSTR("pb%uMax"), i);
      preferences.putFloat(key, progressBars[i].maxValue);
      snprintf_P(key, sizeof(key), PSTR("pb%uCondEn"), i);
      preferences.putBool(key, progressBars[i].conditionEnabled);
      snprintf_P(key, sizeof(key), PSTR("pb%uCdSrcT"), i);
      preferences.putString(key, progressBars[i].conditionSourceType);
      snprintf_P(key, sizeof(key), PSTR("pb%uCdSrcI"), i);
      storeOptionalConfigString(preferences, key, progressBars[i].conditionSourceId);
      snprintf_P(key, sizeof(key), PSTR("pb%uCdVal"), i);
      storeOptionalConfigString(preferences, key, progressBars[i].conditionValue);
      snprintf_P(key, sizeof(key), PSTR("pb%uDynMode"), i);
      preferences.putBool(key, progressBars[i].dynamicMode);
      snprintf_P(key, sizeof(key), PSTR("pb%uDynAnim"), i);
      preferences.putUChar(key, progressBars[i].dynamicAnimation);
      snprintf_P(key, sizeof(key), PSTR("pb%uDynSpd"), i);
      preferences.putUShort(key, progressBars[i].dynamicSpeedMs);
      snprintf_P(key, sizeof(key), PSTR("pb%uDSrcT"), i);
      preferences.putString(key, progressBars[i].dynSourceType);
      snprintf_P(key, sizeof(key), PSTR("pb%uDSrcI"), i);
      storeOptionalConfigString(preferences, key, progressBars[i].dynSourceId);
      snprintf_P(key, sizeof(key), PSTR("pb%uDR1V"), i);
      storeOptionalConfigString(preferences, key, progressBars[i].dynRule1Value);
      snprintf_P(key, sizeof(key), PSTR("pb%uDR2V"), i);
      storeOptionalConfigString(preferences, key, progressBars[i].dynRule2Value);
      snprintf_P(key, sizeof(key), PSTR("pb%uDR2A"), i);
      preferences.putUChar(key, progressBars[i].dynRule2Animation);
      snprintf_P(key, sizeof(key), PSTR("pb%uDR2S"), i);
      preferences.putUShort(key, progressBars[i].dynRule2SpeedMs);
    }
  }

  if (strcmp(groupName, "cronSettings") == 0) {
    preferences.putBool("schEnable", schedulerEnabled);
    preferences.end();
#if !defined(ESP8266)
    // ESP32: save from in-memory array
    for (uint8_t i = 0; i < MAX_PROFILES; i++) {
      char ns[8];
      snprintf_P(ns, sizeof(ns), PSTR("cronP%u"), i);
      preferences.begin(ns, false);
      preferences.putBytes("data", &scheduleProfiles[i],
                           sizeof(ScheduleProfile));
      preferences.end();
    }
#endif
    // ESP8266: profiles are saved individually via schedulerSaveProfile()
    return; // Already called preferences.end()
  }

  if (strcmp(groupName, "intensity") == 0) {
    preferences.putUChar("intensity", intensity);
  }

  preferences.end();
}

void saveSchedulerEnabledToConfFile() {
  preferences.begin("cronSettings", false);
  preferences.putBool("schEnable", schedulerEnabled);
  preferences.end();
}

// ─── Read Config
// ───────────────────────────────────────────────────────────────
void readConfig(const char *groupName, uint8_t n) {
  if (!groupName)
    return;
  preferences.begin(groupName, true);

  if (strcmp(groupName, "systemSettings") == 0) {
    shortMACaddr = preferences.getString("deviceName", shortMACaddr);
    disableServiceMessages =
        preferences.getBool("disablServMess", disableServiceMessages);
    displayFlip = preferences.getBool("displayFlip", false);
  }

  if (strcmp(groupName, "displaySettings") == 0) {
    zoneNumbers =
        normalizedZoneCount(preferences.getUChar("zoneNumbers", zoneNumbers));
    zones[0].begin = preferences.getUChar("zones0Begin", zones[0].begin);
    zones[0].end = preferences.getUChar("zones0End", zones[0].end);
    zones[1].begin = preferences.getUChar("zones1Begin", zones[1].begin);
    zones[1].end = preferences.getUChar("zones1End", zones[1].end);
    zones[2].begin = preferences.getUChar("zones2Begin", zones[2].begin);
    zones[2].end = preferences.getUChar("zones2End", zones[2].end);
    zones[3].begin = preferences.getUChar("zones3Begin", zones[3].begin);
    zones[3].end = preferences.getUChar("zones3End", zones[3].end);
  }

  if (strcmp(groupName, "zoneSettings") == 0) {
    if (n >= kZoneCapacity) {
      preferences.end();
      return;
    }

    char key[24];
    buildZoneKey(key, sizeof(key), n, "WorkMode");
    zones[n].workMode = preferences.getString(key, zones[n].workMode);
    buildZoneKey(key, sizeof(key), n, "ScrolSpeed");
    zones[n].scrollSpeed = preferences.getUChar(key, zones[n].scrollSpeed);
    buildZoneKey(key, sizeof(key), n, "ScrolPause");
    zones[n].scrollPause = preferences.getUChar(key, zones[n].scrollPause);
    buildZoneKey(key, sizeof(key), n, "ScrolAlign");
    zones[n].scrollAlign = preferences.getString(key, zones[n].scrollAlign);
    buildZoneKey(key, sizeof(key), n, "ScrolEfIn");
    zones[n].scrollEffectIn =
        preferences.getString(key, zones[n].scrollEffectIn);
    buildZoneKey(key, sizeof(key), n, "ScrolEfOut");
    zones[n].scrollEffectOut =
        preferences.getString(key, zones[n].scrollEffectOut);
    buildZoneKey(key, sizeof(key), n, "Font");
    zones[n].font = preferences.getString(key, zones[n].font);
    buildZoneKey(key, sizeof(key), n, "Charspac");
    zones[n].charspacing = preferences.getUChar(key, zones[n].charspacing);
    buildZoneKey(key, sizeof(key), n, "mqttTexTop");
    MQTTZones[n].message = preferences.getString(key, MQTTZones[n].message);
    buildZoneKey(key, sizeof(key), n, "mqttPosfix");
    zones[n].mqttPostfix = preferences.getString(key, zones[n].mqttPostfix);
    buildZoneKey(key, sizeof(key), n, "ClDispForm");
    zones[n].clockDisplayFormat =
        preferences.getString(key, zones[n].clockDisplayFormat);
    buildZoneKey(key, sizeof(key), n, "OwmWhDisp");
    zones[n].owmWhatToDisplay =
        preferences.getString(key, zones[n].owmWhatToDisplay);
    buildZoneKey(key, sizeof(key), n, "ScrolInf");
    zones[n].scrollInfinite = preferences.getBool(key, zones[n].scrollInfinite);
    buildZoneKey(key, sizeof(key), n, "HaSensorId");
    zones[n].haSensorId = preferences.getString(key, zones[n].haSensorId);
    buildZoneKey(key, sizeof(key), n, "HaSensorPf");
    zones[n].haSensorPostfix =
        preferences.getString(key, zones[n].haSensorPostfix);
    buildZoneKey(key, sizeof(key), n, "Ds18b20Pf");
    zones[n].ds18b20Postfix =
        preferences.getString(key, zones[n].ds18b20Postfix);
    buildZoneKey(key, sizeof(key), n, "WoprUpdInt");
    woprZones[n].updateInterval =
        preferences.getUShort(key, woprZones[n].updateInterval);
    // Countdown settings
    buildZoneKey(key, sizeof(key), n, "CdFormat");
    zones[n].countdownFormat = preferences.getString(key, zones[n].countdownFormat);
    buildZoneKey(key, sizeof(key), n, "CdFinish");
    zones[n].countdownFinish = preferences.getString(key, zones[n].countdownFinish);
    buildZoneKey(key, sizeof(key), n, "CdUnits");
    zones[n].countdownShowUnits = preferences.getBool(key, zones[n].countdownShowUnits);
    buildZoneKey(key, sizeof(key), n, "CdPrefix");
    zones[n].countdownPrefix = preferences.getString(key, zones[n].countdownPrefix);
    buildZoneKey(key, sizeof(key), n, "CdSuffix");
    zones[n].countdownSuffix = preferences.getString(key, zones[n].countdownSuffix);
    buildZoneKey(key, sizeof(key), n, "CdTarget");
    String savedTarget = preferences.getString(key, "");
    if (savedTarget.length() > 0) {
      parseCountdownTarget(savedTarget, n, timeClient.getEpochTime(),
                           ntpTimeZone);
    } else {
      countdownState[n] = CountdownState{};
    }
    // Stock ticker settings
    buildZoneKey(key, sizeof(key), n, "StockSym");
    zones[n].stockSymbols = preferences.getString(key, zones[n].stockSymbols);
    buildZoneKey(key, sizeof(key), n, "StockFmt");
    zones[n].stockDisplayFormat =
        preferences.getString(key, zones[n].stockDisplayFormat);
    buildZoneKey(key, sizeof(key), n, "StockPre");
    zones[n].stockPrefix = preferences.getString(key, zones[n].stockPrefix);
    buildZoneKey(key, sizeof(key), n, "StockPst");
    zones[n].stockPostfix = preferences.getString(key, zones[n].stockPostfix);
    buildZoneKey(key, sizeof(key), n, "StockArr");
    zones[n].stockShowArrows =
        preferences.getBool(key, zones[n].stockShowArrows);
  }

  if (strcmp(groupName, "mqttSettings") == 0) {
    mqttEnable = preferences.getBool("mqttEnable", mqttEnable);
    mqttServerAddress =
        preferences.getString("mqttServerAddr", mqttServerAddress);
    mqttServerPort = preferences.getUShort("mqttServerPort", mqttServerPort);
    mqttUsername = preferences.getString("mqttUsername", mqttUsername);
    mqttPassword = preferences.getString("mqttPassword", mqttPassword);
    Serial.print(F("readConfig: Loaded mqttPassword: "));
    Serial.println(mqttPassword.length() > 0 ? "EXISTS" : "EMPTY");
  }

  if (strcmp(groupName, "wallClockSett") == 0) {
    ntpTimeZone = preferences.getFloat("ntpTz", ntpTimeZone);
    disableDotsBlink = preferences.getBool("disableDotBlink", disableDotsBlink);
    ntpUpdateInterval =
        preferences.getUShort("ntpUpdateInt", ntpUpdateInterval);
    ntpServer = preferences.getString("ntpServer", ntpServer);
  }

  if (strcmp(groupName, "owmSettings") == 0) {
    owmApiToken = preferences.getString("owmApiToken", owmApiToken);
    owmUnitsFormat = preferences.getString("owmUnitsFormat", owmUnitsFormat);
    owmUpdateInterval =
        preferences.getUShort("owmUpdateInt", owmUpdateInterval);
    owmCity = preferences.getString("owmCity", owmCity);
  }

  if (strcmp(groupName, "haSettings") == 0) {
    haAddr = preferences.getString("haAddr", haAddr);
    haApiToken = preferences.getString("haApiToken", haApiToken);
    haApiHttpType = preferences.getString("haApiHttpType", haApiHttpType);
    haApiPort = preferences.getUShort("haApiPort", haApiPort);
    haUpdateInterval = preferences.getUShort("haUpdateInt", haUpdateInterval);
  }

  if (strcmp(groupName, "ds18b20Settings") == 0) {
    ds18b20Enable = preferences.getBool("ds18b20Enable", ds18b20Enable);
    ds18b20UpdateInterval =
        preferences.getUShort("ds18b20UpdInt", ds18b20UpdateInterval);
    ds18b20UnitsFormat =
        preferences.getString("ds18b20UnitFr", ds18b20UnitsFormat);
  }

  if (strcmp(groupName, "intensity") == 0) {
    intensity = preferences.getUChar("intensity", intensity);
  }

  if (strcmp(groupName, "stockSettings") == 0) {
    stockApiToken = preferences.getString("stApiToken", stockApiToken);
    stockUpdateInterval =
        preferences.getUShort("stUpdateInt", stockUpdateInterval);
  }

  if (strcmp(groupName, "pbSettings") == 0) {
    for (uint8_t i = 0; i < kZoneCapacity; i++) {
      char key[24];
      snprintf_P(key, sizeof(key), PSTR("pb%uEnable"), i);
      progressBars[i].enabled = preferences.getBool(key, progressBars[i].enabled);
      snprintf_P(key, sizeof(key), PSTR("pb%uSrcType"), i);
      progressBars[i].dataSourceType =
          preferences.getString(key, progressBars[i].dataSourceType);
      snprintf_P(key, sizeof(key), PSTR("pb%uSrcId"), i);
      progressBars[i].dataSourceId =
          preferences.getString(key, progressBars[i].dataSourceId);
      snprintf_P(key, sizeof(key), PSTR("pb%uMin"), i);
      progressBars[i].minValue =
          preferences.getFloat(key, progressBars[i].minValue);
      snprintf_P(key, sizeof(key), PSTR("pb%uMax"), i);
      progressBars[i].maxValue =
          preferences.getFloat(key, progressBars[i].maxValue);
      snprintf_P(key, sizeof(key), PSTR("pb%uCondEn"), i);
      progressBars[i].conditionEnabled =
          preferences.getBool(key, progressBars[i].conditionEnabled);
      snprintf_P(key, sizeof(key), PSTR("pb%uCdSrcT"), i);
      progressBars[i].conditionSourceType =
          preferences.getString(key, progressBars[i].conditionSourceType);
      snprintf_P(key, sizeof(key), PSTR("pb%uCdSrcI"), i);
      progressBars[i].conditionSourceId =
          preferences.getString(key, progressBars[i].conditionSourceId);
      snprintf_P(key, sizeof(key), PSTR("pb%uCdVal"), i);
      progressBars[i].conditionValue =
          preferences.getString(key, progressBars[i].conditionValue);
      snprintf_P(key, sizeof(key), PSTR("pb%uDynMode"), i);
      progressBars[i].dynamicMode =
          preferences.getBool(key, progressBars[i].dynamicMode);
      snprintf_P(key, sizeof(key), PSTR("pb%uDynAnim"), i);
      progressBars[i].dynamicAnimation =
          preferences.getUChar(key, progressBars[i].dynamicAnimation);
      snprintf_P(key, sizeof(key), PSTR("pb%uDynSpd"), i);
      progressBars[i].dynamicSpeedMs =
          preferences.getUShort(key, progressBars[i].dynamicSpeedMs);
      snprintf_P(key, sizeof(key), PSTR("pb%uDSrcT"), i);
      progressBars[i].dynSourceType =
          preferences.getString(key, progressBars[i].dynSourceType);
      snprintf_P(key, sizeof(key), PSTR("pb%uDSrcI"), i);
      progressBars[i].dynSourceId =
          preferences.getString(key, progressBars[i].dynSourceId);
      snprintf_P(key, sizeof(key), PSTR("pb%uDR1V"), i);
      progressBars[i].dynRule1Value =
          preferences.getString(key, progressBars[i].dynRule1Value);
      snprintf_P(key, sizeof(key), PSTR("pb%uDR2V"), i);
      progressBars[i].dynRule2Value =
          preferences.getString(key, progressBars[i].dynRule2Value);
      snprintf_P(key, sizeof(key), PSTR("pb%uDR2A"), i);
      progressBars[i].dynRule2Animation =
          preferences.getUChar(key, progressBars[i].dynRule2Animation);
      snprintf_P(key, sizeof(key), PSTR("pb%uDR2S"), i);
      progressBars[i].dynRule2SpeedMs =
          preferences.getUShort(key, progressBars[i].dynRule2SpeedMs);
      // Reset runtime state
      progressBars[i].currentValue = 0.0;
      progressBars[i].conditionMet = !progressBars[i].conditionEnabled;
      progressBars[i].barActive = false;
      progressBars[i].lastDataUpdate = 0;
      progressBars[i].lastCondUpdate = 0;
      progressBars[i].lastCondReceivedValue = "";
      progressBars[i].dynCurrentSensorValue = "";
      progressBars[i].lastDynSrcUpdate = 0;
      progressBars[i].dynPhase = 0;
      progressBars[i].dynPosition = 0;
      progressBars[i].dynLastStep = 0;
      progressBars[i].dynActiveAnim = 0;
    }
  }

  if (strcmp(groupName, "cronSettings") == 0) {
    schedulerEnabled = preferences.getBool("schEnable", false);
    preferences.end();
    // Read each profile from its own namespace
    for (uint8_t i = 0; i < MAX_PROFILES; i++) {
      char ns[8];
      snprintf_P(ns, sizeof(ns), PSTR("cronP%u"), i);
      preferences.begin(ns, true);

      // ESP8266 must not place a ~1.5KB profile on its small setup stack.
#if defined(ESP8266)
      ScheduleProfile &tmpProf = schedulerSharedProfileBuffer();
#else
      ScheduleProfile tmpProf;
#endif
      schedulerInitProfileForEdit(i, tmpProf);
      bool loaded = false;

#if defined(ESP32)
      size_t len = preferences.getBytesLength("data");
#else
      uint8_t probe;
      size_t len = preferences.getBytes("data", &probe, 0);
#endif
      if (schedulerProfileBank(i) != 0) {
        loaded = schedulerLoadStoredProfile(i, tmpProf);
      } else if (len == sizeof(ScheduleProfile)) {
        preferences.getBytes("data", &tmpProf, sizeof(ScheduleProfile));
        loaded = true;
      } else if (len == sizeof(ScheduleProfileV2)) {
        preferences.getBytes("data", &tmpProf, sizeof(ScheduleProfileV2));
        schedulerMigrateProfileV2InPlace(tmpProf);
        loaded = true;
      } else if (len == sizeof(LegacyScheduleProfile)) {
        LegacyScheduleProfile lp;
        preferences.getBytes("data", &lp, sizeof(lp));
        migrateLegacyScheduleProfile(lp, tmpProf);
        loaded = true;
      } else if (len == 0) {
        // Migration: try old single-namespace format (sp0, sp1, etc.)
        preferences.end();
        preferences.begin("cronSettings", true);
        char oldKey[8];
        snprintf_P(oldKey, sizeof(oldKey), PSTR("sp%u"), i);
#if defined(ESP32)
        size_t oldLen = preferences.getBytesLength(oldKey);
#else
        size_t oldLen = preferences.getBytes(oldKey, &probe, 0);
#endif
        if (oldLen == sizeof(ScheduleProfile)) {
          preferences.getBytes(oldKey, &tmpProf, sizeof(ScheduleProfile));
          loaded = true;
        } else if (oldLen == sizeof(ScheduleProfileV2)) {
          preferences.getBytes(oldKey, &tmpProf, sizeof(ScheduleProfileV2));
          schedulerMigrateProfileV2InPlace(tmpProf);
          loaded = true;
        } else if (oldLen == sizeof(LegacyScheduleProfile)) {
          LegacyScheduleProfile lp;
          preferences.getBytes(oldKey, &lp, sizeof(lp));
          migrateLegacyScheduleProfile(lp, tmpProf);
          loaded = true;
        }
        preferences.end();
        // Re-save migrated profile to new namespace
        if (loaded) {
          sanitizeScheduleProfile(tmpProf);
          schedulerSaveProfile(i, tmpProf);
        }
        // Skip the normal end/store below since we already handled it
        goto nextProfile;
      }
      preferences.end();

      if (loaded) {
        sanitizeScheduleProfile(tmpProf);
#if defined(ESP8266)
        // The earliest format is not supported by on-demand loads: migrate it
        // once now, preserving no-change defaults for newly introduced fields.
        if (schedulerProfileBank(i) == 0 && len == sizeof(LegacyScheduleProfile))
          schedulerSaveProfile(i, tmpProf);
        else
          schedulerSyncMeta(i, tmpProf);
#else
        // ESP32: store in the in-memory array
        scheduleProfiles[i] = tmpProf;
#endif
      }
      nextProfile:;
      yield();
    }
    return; // Already called preferences.end()
  }

  preferences.end();
}

// ─── Read All Config
// ───────────────────────────────────────────────────────────
void readAllConfig() {
  readConfig("systemSettings", 99);
  readConfig("displaySettings", 99);
  for (uint8_t n = 0; n < normalizedZoneCount(zoneNumbers); n++) {
    readConfig("zoneSettings", n);
  }
  readConfig("mqttSettings", 99);
  readConfig("wallClockSett", 99);
  readConfig("owmSettings", 99);
  readConfig("haSettings", 99);
  readConfig("ds18b20Settings", 99);
  readConfig("stockSettings", 99);
  readConfig("pbSettings", 99);
  readConfig("intensity", 99);
  schedulerInitDefaults();  // fill defaults before loading
  readConfig("cronSettings", 99);
#if defined(ESP32)
  for (uint8_t n = 0; n < normalizedZoneCount(zoneNumbers); ++n)
    reclaimZoneDefaultStrings(n);
#endif
}
