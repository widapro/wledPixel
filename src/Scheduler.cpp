#include "Scheduler.h"
#include "Globals.h"
#include "ConfigStore.h"
#include "DisplayUtils.h"
#include "MqttHandler.h"
#include "SchedulerLogic.h"
#include "ProfileStorage.h"
#include <cstddef>
#include <atomic>

static_assert(sizeof(ScheduleProfileV2) == 1359,
              "ScheduleProfileV2 must match the previous flash format");

// ─── Variables ──────────────────────────────────────────────────────────────

bool     schedulerEnabled = false;
int8_t   lastActiveProfile = -1;
uint32_t profileLastRun[MAX_PROFILES] = {0};

static ProfileStorage profileStorage;
#if defined(ESP32)
bool ProfileRecordStore::mounted_ = false;
#endif
static ScheduleProfile sharedProfileBuffer;
static uint32_t profileBufferGeneration = 0;
static bool pendingProfileSave = false;
static bool storageWriteFailed = false;
static uint8_t pendingProfileIndex = 0;
static uint32_t suppressScheduleMinute = UINT32_MAX;
#if defined(ESP8266)
ScheduleProfileMeta scheduleMeta[MAX_PROFILES];
#else
ScheduleProfile scheduleProfiles[MAX_PROFILES];
#endif

static bool schedHasStringValue(const char *value) {
  return value && (uint8_t)value[0] != SCHED_STR_NO_CHANGE;
}

static bool pendingProfileApply = false;
static uint8_t pendingProfileApplyIndex = 0;
static ScheduleProgressBar sharedProgressBarBuffer;
static bool pendingProgressBarSave = false;
static uint8_t pendingProgressBarProfile = 0;
static uint8_t pendingProgressBarZone = 0;

static void copyProgressBarString(char *dest, size_t size,
                                  const String &value) {
  if (!dest || size == 0)
    return;
  strncpy(dest, value.c_str(), size - 1);
  dest[size - 1] = '\0';
}

static void captureProgressBar(uint8_t zone, ScheduleProgressBar &out) {
  memset(&out, 0, sizeof(out));
  out.version = 1;
  copyProgressBarString(out.dataSourceType, sizeof(out.dataSourceType),
                        progressBars[zone].dataSourceType);
  copyProgressBarString(out.dataSourceId, sizeof(out.dataSourceId),
                        progressBars[zone].dataSourceId);
  out.minValue = progressBars[zone].minValue;
  out.maxValue = progressBars[zone].maxValue;
  out.conditionEnabled = progressBars[zone].conditionEnabled;
  copyProgressBarString(out.conditionSourceType,
                        sizeof(out.conditionSourceType),
                        progressBars[zone].conditionSourceType);
  copyProgressBarString(out.conditionSourceId, sizeof(out.conditionSourceId),
                        progressBars[zone].conditionSourceId);
  copyProgressBarString(out.conditionValue, sizeof(out.conditionValue),
                        progressBars[zone].conditionValue);
  out.dynamicMode = progressBars[zone].dynamicMode;
  out.dynamicAnimation = progressBars[zone].dynamicAnimation;
  out.dynamicSpeedMs = progressBars[zone].dynamicSpeedMs;
  copyProgressBarString(out.dynSourceType, sizeof(out.dynSourceType),
                        progressBars[zone].dynSourceType);
  copyProgressBarString(out.dynSourceId, sizeof(out.dynSourceId),
                        progressBars[zone].dynSourceId);
  copyProgressBarString(out.dynRule1Value, sizeof(out.dynRule1Value),
                        progressBars[zone].dynRule1Value);
  copyProgressBarString(out.dynRule2Value, sizeof(out.dynRule2Value),
                        progressBars[zone].dynRule2Value);
  out.dynRule2Animation = progressBars[zone].dynRule2Animation;
  out.dynRule2SpeedMs = progressBars[zone].dynRule2SpeedMs;
}

static void applyProgressBar(uint8_t zone, const ScheduleProgressBar &in) {
  progressBars[zone].dataSourceType = in.dataSourceType;
  progressBars[zone].dataSourceId = in.dataSourceId;
  progressBars[zone].minValue = in.minValue;
  progressBars[zone].maxValue = in.maxValue;
  progressBars[zone].conditionEnabled = in.conditionEnabled;
  progressBars[zone].conditionSourceType = in.conditionSourceType;
  progressBars[zone].conditionSourceId = in.conditionSourceId;
  progressBars[zone].conditionValue = in.conditionValue;
  progressBars[zone].dynamicMode = in.dynamicMode;
  progressBars[zone].dynamicAnimation = in.dynamicAnimation;
  progressBars[zone].dynamicSpeedMs = in.dynamicSpeedMs;
  progressBars[zone].dynSourceType = in.dynSourceType;
  progressBars[zone].dynSourceId = in.dynSourceId;
  progressBars[zone].dynRule1Value = in.dynRule1Value;
  progressBars[zone].dynRule2Value = in.dynRule2Value;
  progressBars[zone].dynRule2Animation = in.dynRule2Animation;
  progressBars[zone].dynRule2SpeedMs = in.dynRule2SpeedMs;

  // Runtime values never belong to a profile. Force fresh HA/MQTT data and
  // restart the animation state after applying stored settings.
  progressBars[zone].currentValue = 0.0f;
  progressBars[zone].conditionMet = !progressBars[zone].conditionEnabled;
  progressBars[zone].barActive = false;
  progressBars[zone].lastDataUpdate = 0;
  progressBars[zone].lastCondUpdate = 0;
  progressBars[zone].lastCondReceivedValue = "";
  progressBars[zone].dynCurrentSensorValue = "";
  progressBars[zone].lastDynSrcUpdate = 0;
  progressBars[zone].dynPhase = 0;
  progressBars[zone].dynPosition = 0;
  progressBars[zone].dynLastStep = 0;
  progressBars[zone].dynActiveAnim = 0;
  progressBars[zone].dynExtra = 0;
  progressBars[zone].dynDirection = 0;
}

ScheduleProgressBar &schedulerSharedProgressBarBuffer() {
  return sharedProgressBarBuffer;
}

bool schedulerLoadProgressBar(uint8_t profileIdx, uint8_t zone,
                              ScheduleProgressBar &out) {
  if (profileIdx >= MAX_PROFILES || zone >= 4)
    return false;
  bool loaded = profileStorage.loadBar(profileIdx, zone, out) == 1;
  if (loaded) {
      out.dataSourceType[sizeof(out.dataSourceType) - 1] = '\0';
      out.dataSourceId[sizeof(out.dataSourceId) - 1] = '\0';
      out.conditionSourceType[sizeof(out.conditionSourceType) - 1] = '\0';
      out.conditionSourceId[sizeof(out.conditionSourceId) - 1] = '\0';
      out.conditionValue[sizeof(out.conditionValue) - 1] = '\0';
      out.dynSourceType[sizeof(out.dynSourceType) - 1] = '\0';
      out.dynSourceId[sizeof(out.dynSourceId) - 1] = '\0';
      out.dynRule1Value[sizeof(out.dynRule1Value) - 1] = '\0';
      out.dynRule2Value[sizeof(out.dynRule2Value) - 1] = '\0';
  }
  if (!loaded)
    captureProgressBar(zone, out);
  return loaded;
}

bool schedulerSaveProgressBar(uint8_t profileIdx, uint8_t zone,
                            const ScheduleProgressBar &in) {
  if (profileIdx >= MAX_PROFILES || zone >= 4)
    return false;
  return profileStorage.stageBar(profileIdx, zone, &in);
}

bool schedulerClearProgressBar(uint8_t idx, uint8_t zone) {
  return profileStorage.stageBar(idx, zone, nullptr);
}
uint8_t schedulerProfileBank(uint8_t idx) { return profileStorage.bank(idx); }
void schedulerAbandonStaging(uint8_t idx) { profileStorage.abandon(idx); }

bool schedulerStageProgressBarSave(uint8_t profileIdx, uint8_t zone) {
  if (profileIdx >= MAX_PROFILES || zone >= 4 || pendingProgressBarSave)
    return false;
  storageWriteFailed = false;
  pendingProgressBarProfile = profileIdx;
  pendingProgressBarZone = zone;
  pendingProgressBarSave = true;
  return true;
}

bool schedulerServicePendingProgressBarSave() {
  if (!pendingProgressBarSave)
    return false;
  SchedulerWorkGuard work;
  storageWriteFailed = !schedulerSaveProgressBar(pendingProgressBarProfile, pendingProgressBarZone,
                  sharedProgressBarBuffer);
  pendingProgressBarSave = false;
  return true;
}

bool schedulerHasPendingProgressBarSave() {
  return pendingProgressBarSave;
}

// ─── Init a single profile to defaults ──────────────────────────────────────

// ─── Sync metadata from a full profile ──────────────────────────────────────

void schedulerSyncMeta(uint8_t idx, const ScheduleProfile &p) {
  if (idx >= MAX_PROFILES) return;
#if defined(ESP8266)
  scheduleMeta[idx].enabled    = p.enabled;
  scheduleMeta[idx].minute     = p.minute;
  scheduleMeta[idx].hour       = p.hour;
  scheduleMeta[idx].dayOfWeek  = p.dayOfWeek;
  scheduleMeta[idx].dayOfMonth = p.dayOfMonth;
  memcpy(scheduleMeta[idx].name, p.name, sizeof(scheduleMeta[idx].name));
#else
  (void)p; // ESP32: full profile already in scheduleProfiles[]
#endif
}

// ─── Load profile from NVS ──────────────────────────────────────────────────

bool schedulerLoadProfile(uint8_t idx, ScheduleProfile &out) {
  if (idx >= MAX_PROFILES) return false;
  if (&out == &sharedProfileBuffer)
    ++profileBufferGeneration;
#if defined(ESP8266)
  initProfileDefaults(out, idx);
  return profileStorage.load(idx, out);
#else
  out = scheduleProfiles[idx];
  return true;
#endif
}

// ─── Save profile to NVS ────────────────────────────────────────────────────

bool schedulerSaveProfile(uint8_t idx, const ScheduleProfile &p) {
  if (idx >= MAX_PROFILES || !schedulerProfileValid(p, MAX_DEVICES)) return false;
  if (!profileStorage.commit(idx, p)) return false;
#if defined(ESP8266)
  schedulerSyncMeta(idx, p);
#else
  scheduleProfiles[idx] = p;
#endif
  Serial.printf("\n[SCHED] Profile %u saved: %u bytes", idx,
                static_cast<unsigned>(sizeof(p)));
  return true;
}

ScheduleProfile &schedulerSharedProfileBuffer() {
  return sharedProfileBuffer;
}

uint32_t schedulerProfileBufferGeneration() { return profileBufferGeneration; }

bool schedulerStageProfileSave(uint8_t idx) {
  if (idx >= MAX_PROFILES || pendingProfileSave)
    return false;
  storageWriteFailed = false;
  pendingProfileIndex = idx;
  pendingProfileSave = true;
  return true;
}

bool schedulerServicePendingProfileSave() {
  if (!pendingProfileSave)
    return false;
  SchedulerWorkGuard work;
  bool saved =
      schedulerSaveProfile(pendingProfileIndex, sharedProfileBuffer);
  storageWriteFailed = !saved;
  if (saved && ntpSyncedOnce) {
    // Editing a schedule must not execute it halfway through the current
    // minute. It becomes eligible at the next minute boundary.
    suppressScheduleMinute = timeClient.getEpochTime() / 60UL;
  }
  pendingProfileSave = false;
  schedulerEndEdit();
  return true;
}

bool schedulerHasPendingProfileSave() {
  return pendingProfileSave;
}

bool schedulerStageProfileApply(uint8_t idx) {
  if (idx >= MAX_PROFILES || schedulerEditActive() || pendingProfileApply ||
      schedulerHasPendingProfileSave() ||
      pendingProgressBarSave)
    return false;
  pendingProfileApplyIndex = idx;
  pendingProfileApply = true;
  return true;
}

bool schedulerServicePendingProfileApply() {
  if (!pendingProfileApply)
    return false;
  uint8_t idx = pendingProfileApplyIndex;
  pendingProfileApply = false;
  Serial.printf("\n[SCHED] Manual apply queued profile %u\n", idx);
  schedulerApplyProfile(idx);
  return true;
}

bool schedulerHasPendingProfileApply() { return pendingProfileApply; }

// ─── Init Defaults ──────────────────────────────────────────────────────────

void schedulerInitDefaults() {
  for (uint8_t i = 0; i < MAX_PROFILES; i++) {
#if defined(ESP8266)
    scheduleMeta[i].enabled    = false;
    scheduleMeta[i].minute     = SCHED_NO_CHANGE_U8;
    scheduleMeta[i].hour       = SCHED_NO_CHANGE_U8;
    scheduleMeta[i].dayOfWeek  = 0x7F;
    scheduleMeta[i].dayOfMonth = SCHED_NO_CHANGE_U32;
    snprintf(scheduleMeta[i].name, sizeof(scheduleMeta[i].name),
             "Profile %u", i + 1);
#else
    initProfileDefaults(scheduleProfiles[i], i);
#endif
  }
}

// ─── Enum ↔ String Helpers ──────────────────────────────────────────────────

String cronWorkModeToString(uint8_t mode) {
  switch (mode) {
    case CWRK_MANUAL:    return F("manualInput");
    case CWRK_CLOCK:     return F("wallClock");
    case CWRK_OWM:       return F("owmWeather");
    case CWRK_HA:        return F("haClient");
    case CWRK_MQTT:      return F("mqttClient");
    case CWRK_DS18:      return F("intTempSensor");
    case CWRK_WOPR:      return F("wopr");
    case CWRK_COUNTDOWN: return F("countdown");
    case CWRK_STOCK:     return F("stockTicker");
    default:             return String();
  }
}

String cronOwmDisplayToString(uint8_t val) {
  switch (val) {
    case COWM_TEMPERATURE: return F("owmTemperature");
    case COWM_ICON:        return F("owmWeatherIcon");
    case COWM_HUMIDITY:    return F("owmHumidity");
    case COWM_PRESSURE:    return F("owmPressure");
    case COWM_WIND:        return F("owmWindSpeed");
    case COWM_DESCRIPTION: return F("owmDescription");
    default:               return String();
  }
}

String cronClockFmtToString(uint8_t val) {
  switch (val) {
    case CCLK_HHMM:       return F("HHMM");
    case CCLK_HHMMSS:     return F("HHMMSS");
    case CCLK_HH:         return F("HH");
    case CCLK_MM:         return F("MM");
    case CCLK_DDMMYYYY:   return F("ddmmyyyy");
    case CCLK_DDMM:       return F("ddmm");
    case CCLK_DDMMAA:     return F("ddmmaa");
    case CCLK_AA:         return F("aa");
    case CCLK_DDMMAAHHMM: return F("ddmmaahhmm");
    case CCLK_HHMM12:     return F("HHMM12");
    case CCLK_HHMM12AP:   return F("HHMM12AP");
    default:              return String();
  }
}

String cronFontToString(uint8_t val) {
  switch (val) {
    case CFNT_DEFAULT:  return F("default");
    case CFNT_WLED:     return F("wledFont");
    case CFNT_CYRILLIC: return F("wledFont_cyrillic");
    case CFNT_SYMBOL:   return F("wledSymbolFont");
    case CFNT_COMPACT:  return F("wledFont_CompactSymbols");
    default:            return String();
  }
}

// ─── Force NTP Sync ─────────────────────────────────────────────────────────

bool schedulerForceNtpSync() {
  bool ok = timeClient.forceUpdate();
  if (ok) {
    ntpSyncedOnce = true;
    previousNTPsyncMillis = millis();
    Serial.println(F("[SCHED] Force NTP sync OK"));
  } else {
    Serial.println(F("[SCHED] Force NTP sync FAILED"));
  }
  return ok;
}

// ─── Apply Profile ──────────────────────────────────────────────────────────

void schedulerApplyProfile(uint8_t idx) {
  if (idx >= MAX_PROFILES) return;
  SchedulerWorkGuard work;

#if defined(ESP8266)
  // Load full profile from NVS on demand (saves ~5KB of global RAM)
  if (!schedulerLoadProfile(idx, sharedProfileBuffer)) {
    Serial.printf("[SCHED] Failed to load profile %u from NVS\n", idx);
    return;
  }
  ScheduleProfile &p = sharedProfileBuffer;
#else
  ScheduleProfile &p = scheduleProfiles[idx];
#endif
  if (!schedulerProfileValid(p, MAX_DEVICES)) {
    Serial.println(F("[SCHED] Invalid profile; apply cancelled"));
    return;
  }
  // Validate partial layout overrides against the current layout before any
  // power, brightness, or zone state is changed.
  for (uint8_t z = 0; z < (p.zoneCount ? p.zoneCount : zoneNumbers); ++z) {
    uint8_t begin = p.zoneBegin[z] == 255 ? zones[z].begin : p.zoneBegin[z];
    uint8_t end = p.zoneEnd[z] == 255 ? zones[z].end : p.zoneEnd[z];
    if (begin > end || end >= MAX_DEVICES) {
      Serial.println(F("[SCHED] Invalid resulting layout; apply cancelled"));
      return;
    }
  }
  bool needReboot = false;

  Serial.printf("[SCHED] Applying profile %u: '%s'\n", idx, p.name);

  // ── Power ──
  if (p.power != SCHED_NO_CHANGE_U8) {
    if (p.power == 1 && !power) {
      // Wake immediately, but tell initConfig to preserve the visible frame.
      // The old implementation cleared/reset every zone a few seconds later,
      // producing a visible ON/OFF/ON flash.
      power = true;
      displayWakePending = true;
      P.displayShutdown(0);
    } else if (p.power == 0 && power) {
      displayWakePending = false;
      power = false;
      P.displayShutdown(1);
    }
  }

  // ── Intensity ──
  if (p.intensity > 0 && p.intensity <= 16) {
    intensity = p.intensity - 1;
    P.setIntensity(intensity);
    queueConfigSave("intensity", 0);
  }

  // Explicit scheduler values must survive MQTT reconnects caused by browser
  // API priority. Stale retained commands are cleared before resubscription.
  bool schedulerTurnsPowerOff = p.power == 0;
  MQTTProtectScheduledState(
      p.power != SCHED_NO_CHANGE_U8,
      schedulerTurnsPowerOff || (p.intensity > 0 && p.intensity <= 16));

  // ── Zone count ──
  if (p.zoneCount > 0 && p.zoneCount <= 4 && p.zoneCount != zoneNumbers) {
    zoneNumbers = p.zoneCount;
    needReboot = true;
  }

  // ── Zone layout (begin/end) ──
  for (uint8_t z = 0; z < 4; z++) {
    if (p.zoneBegin[z] != SCHED_NO_CHANGE_U8) {
      if (zones[z].begin != p.zoneBegin[z]) {
        zones[z].begin = p.zoneBegin[z];
        needReboot = true;
      }
    }
    if (p.zoneEnd[z] != SCHED_NO_CHANGE_U8) {
      if (zones[z].end != p.zoneEnd[z]) {
        zones[z].end = p.zoneEnd[z];
        needReboot = true;
      }
    }
  }

  // ── Per-zone overrides ──
  for (uint8_t z = 0; z < 4; z++) {
    // Work mode
    if (p.zoneWorkMode[z] != CWRK_NO_CHANGE) {
      String newMode = cronWorkModeToString(p.zoneWorkMode[z]);
      if (newMode.length() > 0) zones[z].workMode = newMode;
    }
    // HA sensor ID
    if (schedHasStringValue(p.zoneHaSensorId[z])) {
      zones[z].haSensorId = String(p.zoneHaSensorId[z]);
    }
    // HA postfix
    if (schedHasStringValue(p.zoneHaPostfix[z])) {
      zones[z].haSensorPostfix = String(p.zoneHaPostfix[z]);
    }
    // MQTT topic
    if (schedHasStringValue(p.zoneMqttTopic[z])) {
      MQTTZones[z].message = String(p.zoneMqttTopic[z]);
      shouldMqttPublish = true;
    }
    // OWM display
    if (p.zoneOwmDisplay[z] != COWM_NO_CHANGE) {
      String val = cronOwmDisplayToString(p.zoneOwmDisplay[z]);
      if (val.length() > 0) zones[z].owmWhatToDisplay = val;
    }
    // Clock format
    if (p.zoneClockFmt[z] != CCLK_NO_CHANGE) {
      String val = cronClockFmtToString(p.zoneClockFmt[z]);
      if (val.length() > 0) zones[z].clockDisplayFormat = val;
    }
    // Font
    if (p.zoneFont[z] != CFNT_NO_CHANGE) {
      String val = cronFontToString(p.zoneFont[z]);
      if (val.length() > 0) {
        zones[z].font = val;
        zones[z].updateFont = true;
      }
    }

    if (p.zoneScrollSpeed[z] != SCHED_NO_CHANGE_U8) {
      zones[z].scrollSpeed = p.zoneScrollSpeed[z];
      P.setSpeed(z, zones[z].scrollSpeed);
    }
    if (p.zoneScrollPause[z] != SCHED_NO_CHANGE_U8) {
      zones[z].scrollPause = p.zoneScrollPause[z];
      P.setPause(z, zones[z].scrollPause * 1000);
    }
    if (p.zoneCharspacing[z] != SCHED_NO_CHANGE_U8) {
      zones[z].charspacing = p.zoneCharspacing[z];
      P.setCharSpacing(z, zones[z].charspacing);
    }
    if (schedHasStringValue(p.zoneScrollAlign[z])) {
      zones[z].scrollAlign = String(p.zoneScrollAlign[z]);
      P.setTextAlignment(z, stringToTextPositionT(zones[z].scrollAlign));
    }
    if (schedHasStringValue(p.zoneScrollEffectIn[z])) {
      zones[z].scrollEffectIn = String(p.zoneScrollEffectIn[z]);
      updateSpriteData(z);
    }
    if (schedHasStringValue(p.zoneScrollEffectOut[z])) {
      zones[z].scrollEffectOut = String(p.zoneScrollEffectOut[z]);
      updateSpriteData(z);
    }
    if (schedHasStringValue(p.zoneMqttPostfix[z])) {
      zones[z].mqttPostfix = String(p.zoneMqttPostfix[z]);
      shouldMqttPublish = true;
    }
    if (schedHasStringValue(p.zoneDs18b20Postfix[z])) {
      zones[z].ds18b20Postfix = String(p.zoneDs18b20Postfix[z]);
      previousDsMillis = -1000;
    }
    if (schedHasStringValue(p.zoneCountdownTarget[z])) {
      parseCountdownTarget(String(p.zoneCountdownTarget[z]), z,
                           timeClient.getEpochTime(), ntpTimeZone);
      zones[z].curTime = "";
    }
    if (schedHasStringValue(p.zoneCountdownFormat[z])) {
      zones[z].countdownFormat = String(p.zoneCountdownFormat[z]);
      zones[z].curTime = "";
    }
    if (schedHasStringValue(p.zoneCountdownFinish[z])) {
      zones[z].countdownFinish = String(p.zoneCountdownFinish[z]);
    }
    if (schedHasStringValue(p.zoneCountdownPrefix[z])) {
      zones[z].countdownPrefix = String(p.zoneCountdownPrefix[z]);
      zones[z].curTime = "";
    }
    if (schedHasStringValue(p.zoneCountdownSuffix[z])) {
      zones[z].countdownSuffix = String(p.zoneCountdownSuffix[z]);
      zones[z].curTime = "";
    }
    if (schedHasStringValue(p.zoneStockSymbols[z])) {
      zones[z].stockSymbols = String(p.zoneStockSymbols[z]);
      stockParseSymbols(z, zones[z].stockSymbols);
      stockLastTime = 0;
    }
    if (schedHasStringValue(p.zoneStockDisplayFormat[z])) {
      zones[z].stockDisplayFormat = String(p.zoneStockDisplayFormat[z]);
      zones[z].curTime = "";
    }
    if (schedHasStringValue(p.zoneStockPrefix[z])) {
      zones[z].stockPrefix = String(p.zoneStockPrefix[z]);
      zones[z].curTime = "";
    }
    if (schedHasStringValue(p.zoneStockPostfix[z])) {
      zones[z].stockPostfix = String(p.zoneStockPostfix[z]);
      zones[z].curTime = "";
    }
  }

  // ── Complete per-profile progress-bar settings ──
  for (uint8_t z = 0; z < 4; z++) {
    if (p.pbEnable & (1 << (z + 4))) {  // change flag set
      bool stored =
          schedulerLoadProgressBar(idx, z, sharedProgressBarBuffer);
      applyProgressBar(z, sharedProgressBarBuffer);
      progressBars[z].enabled = (p.pbEnable & (1 << z)) != 0;
      Serial.printf("[SCHED] PB zone %u: %s, %s, mode=%s\n", z,
                    progressBars[z].enabled ? "ON" : "OFF",
                    stored ? "profile settings" : "legacy current settings",
                    progressBars[z].dynamicMode ? "dynamic" : "regular");
    }
    if (p.zoneScrollInfinite & (1 << (z + 4))) {
      zones[z].scrollInfinite = (p.zoneScrollInfinite & (1 << z)) != 0;
    }
    if (p.zoneCountdownShowUnits & (1 << (z + 4))) {
      zones[z].countdownShowUnits = (p.zoneCountdownShowUnits & (1 << z)) != 0;
      zones[z].curTime = "";
    }
    if (p.zoneStockShowArrows & (1 << (z + 4))) {
      zones[z].stockShowArrows = (p.zoneStockShowArrows & (1 << z)) != 0;
      zones[z].curTime = "";
    }
  }

  // ── Save to NVS ──
  if (needReboot) {
    queueConfigSave("displaySettings", 0);
  }
  for (uint8_t z = 0; z < 4; z++) {
    queueConfigSave("zoneSettings", z);
  }
  queueConfigSave("pbSettings", 0);

  // ── Update runtime state ──
  profileLastRun[idx] = timeClient.getEpochTime();
  lastActiveProfile = idx;

  if (needReboot) {
    Serial.println(F("[SCHED] Zone layout changed, rebooting..."));
    shouldReboot = true;
  } else {
    initConfig = true;
  }
}

// ─── Scheduler Check (called from loop) ─────────────────────────────────────

void schedulerCheck() {
  if (!schedulerEnabled || !ntpSyncedOnce || schedulerEditActive()) return;

  // Check on a calendar-minute transition, not every 60000 ms from boot.
  // A millis()-based interval can stay permanently aligned near :59 and make
  // a profile appear to run one minute late.
  time_t epoch = timeClient.getEpochTime();
  uint32_t currentScheduleMinute = static_cast<uint32_t>(epoch / 60UL);
  static uint32_t lastCheckedScheduleMinute = UINT32_MAX;
  if (currentScheduleMinute == lastCheckedScheduleMinute)
    return;
  lastCheckedScheduleMinute = currentScheduleMinute;

  if (currentScheduleMinute == suppressScheduleMinute) {
    Serial.println(F("[SCHED] Current minute suppressed after profile save"));
    return;
  }
  suppressScheduleMinute = UINT32_MAX;
  struct tm *now = gmtime(&epoch);
  uint8_t curSecond = now->tm_sec;
  uint8_t curMinute = now->tm_min;
  uint8_t curHour   = now->tm_hour;
  uint8_t curDow    = now->tm_wday;  // 0=Sunday
  uint8_t curDom    = now->tm_mday;  // 1-31

  Serial.printf("[SCHED] Check: %02u:%02u:%02u dow=%u dom=%u\n",
                curHour, curMinute, curSecond, curDow, curDom);

  // Evaluate each profile
  for (uint8_t i = 0; i < MAX_PROFILES; i++) {
#if defined(ESP8266)
    // ESP8266: use lightweight metadata for matching (no full profile in RAM)
    ScheduleProfileMeta &m = scheduleMeta[i];
#else
    ScheduleProfile &m = scheduleProfiles[i];
#endif
    if (!schedulerTimeMatches(m.enabled, m.minute, m.hour, m.dayOfWeek,
                              m.dayOfMonth, *now))
      continue;

    // All criteria matched — apply profile
    if (profileLastRun[i] / 60UL == currentScheduleMinute)
      continue;
    Serial.printf("[SCHED] Profile %u matched, applying '%s'\n",
                  i, m.name);
    schedulerApplyProfile(i);
  }
}

bool schedulerStorageWriteFailed() { return storageWriteFailed; }

bool schedulerLoadStoredProfile(uint8_t idx, ScheduleProfile &out) {
  return profileStorage.load(idx, out);
}

namespace {
char editOwner[25] = {};
uint8_t editProfile = 0;
unsigned long editTouched = 0;
}
bool schedulerEditActive() {
  if (editOwner[0] && millis() - editTouched > 90000UL &&
      !pendingProfileSave && !pendingProgressBarSave) {
    profileStorage.abandon(editProfile);
    editOwner[0] = 0;
  }
  return editOwner[0] != 0;
}
bool schedulerOwnsEdit(uint8_t idx, const char *token) {
  if (!schedulerEditActive() || idx != editProfile || !token || strcmp(token, editOwner))
    return false;
  editTouched = millis();
  return true;
}
bool schedulerBeginEdit(uint8_t idx, const char *token) {
  if (idx >= MAX_PROFILES || !token || strlen(token) < 8 || strlen(token) > 24 ||
      pendingProfileSave || pendingProgressBarSave || pendingProfileApply) return false;
  if (schedulerEditActive()) return schedulerOwnsEdit(idx, token);
  profileStorage.abandon(idx);
  memcpy(editOwner, token, strlen(token) + 1);
  editProfile = idx;
  editTouched = millis();
  return true;
}
void schedulerEndEdit() {
  if (pendingProfileSave || pendingProgressBarSave) return;
  if (editOwner[0]) profileStorage.abandon(editProfile);
  editOwner[0] = 0;
}

namespace {
#if defined(ESP8266)
// Cooperative callbacks can re-enter only at yield points, never mid-increment.
volatile uint8_t schedulerWorkDepth = 0;
#else
std::atomic<uint32_t> schedulerWorkDepth{0};
#endif
}
void schedulerBeginWork() { ++schedulerWorkDepth; }
void schedulerEndWork() { --schedulerWorkDepth; }
bool schedulerWorkActive() {
#if defined(ESP8266)
  return schedulerWorkDepth != 0;
#else
  return schedulerWorkDepth.load() != 0;
#endif
}
