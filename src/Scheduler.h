#pragma once

#include <Arduino.h>

// ─── Constants ──────────────────────────────────────────────────────────────
#if defined(ESP8266)
#define MAX_PROFILES 4
#else
#define MAX_PROFILES 8
#endif
#define SCHED_NO_CHANGE_U8  255
#define SCHED_NO_CHANGE_U32 0x7FFFFFFF
#define SCHED_STR_NO_CHANGE 0xFFu // first byte sentinel for "no change"

// ─── Enums ──────────────────────────────────────────────────────────────────

enum CronWorkMode : uint8_t {
  CWRK_NO_CHANGE  = 0,
  CWRK_MANUAL     = 1,
  CWRK_CLOCK      = 2,
  CWRK_OWM        = 3,
  CWRK_HA         = 4,
  CWRK_MQTT       = 5,
  CWRK_DS18       = 6,
  CWRK_WOPR       = 7,
  CWRK_COUNTDOWN  = 8,
  CWRK_STOCK      = 9
};

enum CronOwmDisplay : uint8_t {
  COWM_NO_CHANGE    = 0,
  COWM_TEMPERATURE  = 1,
  COWM_ICON         = 2,
  COWM_HUMIDITY     = 3,
  COWM_PRESSURE     = 4,
  COWM_WIND         = 5,
  COWM_DESCRIPTION  = 6
};

enum CronClockFmt : uint8_t {
  CCLK_NO_CHANGE  = 0,
  CCLK_HHMM       = 1,
  CCLK_HHMMSS     = 2,
  CCLK_HH         = 3,
  CCLK_MM         = 4,
  CCLK_DDMMYYYY   = 5,
  CCLK_DDMM       = 6,
  CCLK_DDMMAA     = 7,
  CCLK_AA         = 8,
  CCLK_DDMMAAHHMM = 9,
  CCLK_HHMM12     = 10,
  CCLK_HHMM12AP   = 11
};

enum CronFont : uint8_t {
  CFNT_NO_CHANGE = 0,
  CFNT_DEFAULT   = 1,
  CFNT_WLED      = 2,
  CFNT_CYRILLIC  = 3,
  CFNT_SYMBOL    = 4,
  CFNT_COMPACT   = 5,
  CFNT_PIXEL_3D  = 6
};

// ─── Profile Struct ─────────────────────────────────────────────────────────

// Keep the old packed layouts readable when field capacities change.
constexpr size_t SCHED_SOURCE_ID_SIZE = 128; // 127 UTF-8 bytes plus NUL
constexpr uint8_t SCHED_PROGRESS_BAR_VERSION = 2;
template <size_t HaSize, size_t TopicSize>
struct __attribute__((packed)) ScheduleProfileRecord {
  // Schedule (8 bytes)
  bool     enabled;                // job active
  uint8_t  minute;                 // 0-59, 255=every
  uint8_t  hour;                   // 0-23, 255=every
  uint8_t  dayOfWeek;              // bitmask bit0=Sun..bit6=Sat, 0x7F=daily
  uint32_t dayOfMonth;             // bitmask bit0=day1..bit30=day31

  // Global state (4 bytes)
  uint8_t  power;                  // 0=off, 1=on, 255=no change
  uint8_t  intensity;              // 1-16, 0=no change
  uint8_t  zoneCount;              // 1-4, 0=no change
  uint8_t  pbEnable;               // bits 0-3: value, bits 4-7: change flag

  // Zone layout (8 bytes)
  uint8_t  zoneBegin[4];           // 255=no change
  uint8_t  zoneEnd[4];             // 255=no change

  // Per-zone enums (16 bytes)
  uint8_t  zoneWorkMode[4];        // CronWorkMode
  uint8_t  zoneOwmDisplay[4];      // CronOwmDisplay
  uint8_t  zoneClockFmt[4];        // CronClockFmt
  uint8_t  zoneFont[4];            // CronFont

  // Per-zone strings
  char     zoneHaSensorId[4][HaSize];  // 0xFF first byte = no change
  char     zoneHaPostfix[4][6];    // 0xFF first byte = no change
  char     zoneMqttTopic[4][TopicSize];   // 0xFF first byte = no change

  // Extended per-zone state
  uint8_t  zoneScrollSpeed[4];      // 255=no change
  uint8_t  zoneScrollPause[4];      // 255=no change
  uint8_t  zoneCharspacing[4];      // 255=no change
  uint8_t  zoneScrollInfinite;      // bits 0-3: value, bits 4-7: change flag
  uint8_t  zoneCountdownShowUnits;  // bits 0-3: value, bits 4-7: change flag
  uint8_t  zoneStockShowArrows;     // bits 0-3: value, bits 4-7: change flag
  char     zoneScrollAlign[4][10];  // 0xFF first byte = no change
  char     zoneScrollEffectIn[4][24];
  char     zoneScrollEffectOut[4][24];
  char     zoneMqttPostfix[4][8];
  char     zoneDs18b20Postfix[4][8];
  char     zoneCountdownTarget[4][20];
  char     zoneCountdownFormat[4][16];
  char     zoneCountdownFinish[4][16];
  char     zoneCountdownPrefix[4][12];
  char     zoneCountdownSuffix[4][12];
  char     zoneStockSymbols[4][64];
  char     zoneStockDisplayFormat[4][16];
  char     zoneStockPrefix[4][12];
  char     zoneStockPostfix[4][12];

  // Metadata (12 bytes)
  char     name[12];
};
using ScheduleProfileV2 = ScheduleProfileRecord<32, 32>;
using ScheduleProfileV3 = ScheduleProfileRecord<64, 32>;
using ScheduleProfile = ScheduleProfileRecord<64, SCHED_SOURCE_ID_SIZE>;
static_assert(sizeof(ScheduleProfileV2) == 1359, "V2 flash layout changed");
static_assert(sizeof(ScheduleProfileV3) == 1487, "V3 flash layout changed");

// Full progress-bar configuration is stored separately from ScheduleProfile.
// Keeping it out of the profile block avoids retaining four additional bar
// records per resident profile. Older on-flash layouts remain readable.
template <size_t SourceSize>
struct __attribute__((packed)) ScheduleProgressBarRecord {
  uint8_t version;
  char dataSourceType[6];
  char dataSourceId[SourceSize];
  float minValue;
  float maxValue;
  bool conditionEnabled;
  char conditionSourceType[6];
  char conditionSourceId[SourceSize];
  char conditionValue[24];
  bool dynamicMode;
  uint8_t dynamicAnimation;
  uint16_t dynamicSpeedMs;
  char dynSourceType[6];
  char dynSourceId[SourceSize];
  char dynRule1Value[24];
  char dynRule2Value[24];
  uint8_t dynRule2Animation;
  uint16_t dynRule2SpeedMs;
};
using ScheduleProgressBarV1 = ScheduleProgressBarRecord<48>;
using ScheduleProgressBar = ScheduleProgressBarRecord<SCHED_SOURCE_ID_SIZE>;
static_assert(sizeof(ScheduleProgressBarV1) == 251, "V1 progress-bar flash layout changed");

// ─── Lightweight metadata for schedule matching (ESP8266 keeps only this in RAM)
typedef struct __attribute__((packed)) {
  bool     enabled;
  uint8_t  minute;
  uint8_t  hour;
  uint8_t  dayOfWeek;
  uint32_t dayOfMonth;
  char     name[12];
} ScheduleProfileMeta;  // ~20 bytes per profile

// ─── Extern Declarations ────────────────────────────────────────────────────

#if defined(ESP8266)
// ESP8266: only metadata in RAM, full profiles loaded from NVS on demand
extern ScheduleProfileMeta scheduleMeta[MAX_PROFILES];
#else
// ESP32: full profiles in RAM (plenty of memory)
extern ScheduleProfile scheduleProfiles[MAX_PROFILES];
#endif

extern bool     schedulerEnabled;
extern bool     ntpSyncedOnce;
extern uint32_t profileLastRun[MAX_PROFILES];
extern int8_t   lastActiveProfile;

// ─── Functions ──────────────────────────────────────────────────────────────

void schedulerCheck();
void schedulerApplyProfile(uint8_t idx);
bool schedulerForceNtpSync();
void schedulerInitDefaults();

// On-demand profile loading from NVS (for ESP8266 memory savings)
bool schedulerLoadProfile(uint8_t idx, ScheduleProfile &out);
void schedulerMigrateProfileV2InPlace(ScheduleProfile &profile);
void schedulerMigrateProfileV3InPlace(ScheduleProfile &profile);
void schedulerMigrateProgressBarV1InPlace(ScheduleProgressBar &bar);
void schedulerInitProfileForEdit(uint8_t idx, ScheduleProfile &out);
bool schedulerSaveProfile(uint8_t idx, const ScheduleProfile &p);
void schedulerSyncMeta(uint8_t idx, const ScheduleProfile &p);
ScheduleProfile &schedulerSharedProfileBuffer();
uint32_t schedulerProfileBufferGeneration();
bool schedulerSaveProgressBar(uint8_t profileIdx, uint8_t zone, const ScheduleProgressBar &in);
bool schedulerStageProfileSave(uint8_t idx);
bool schedulerServicePendingProfileSave();
bool schedulerHasPendingProfileSave();
bool schedulerStageProfileApply(uint8_t idx);
bool schedulerServicePendingProfileApply();
bool schedulerHasPendingProfileApply();

// Per-profile progress bars. The shared buffer is deliberately reused for API
// transfers and NVS writes; only one small block is ever resident at a time.
ScheduleProgressBar &schedulerSharedProgressBarBuffer();
bool schedulerLoadProgressBar(uint8_t profileIdx, uint8_t zone,
                              ScheduleProgressBar &out);
bool schedulerStageProgressBarSave(uint8_t profileIdx, uint8_t zone);
bool schedulerServicePendingProgressBarSave();
bool schedulerHasPendingProgressBarSave();

// Enum ↔ String conversion helpers
String cronWorkModeToString(uint8_t mode);
String cronOwmDisplayToString(uint8_t val);
String cronClockFmtToString(uint8_t val);
String cronFontToString(uint8_t val);

void initProfileDefaults(ScheduleProfile &p, uint8_t idx);
void sanitizeScheduleProfile(ScheduleProfile &profile);
bool schedulerStorageWriteFailed();
bool schedulerProfileValid(const ScheduleProfile &profile, uint8_t modules);

bool schedulerClearProgressBar(uint8_t idx, uint8_t zone);
uint8_t schedulerProfileBank(uint8_t idx);
void schedulerAbandonStaging(uint8_t idx);
bool schedulerLoadStoredProfile(uint8_t idx, ScheduleProfile &out);

bool schedulerEditActive();
bool schedulerOwnsEdit(uint8_t idx, const char *token);
bool schedulerBeginEdit(uint8_t idx, const char *token);
void schedulerEndEdit();

// Avoid re-entering flash/profile workspaces from async callbacks during yield().
void schedulerBeginWork();
void schedulerEndWork();
bool schedulerWorkActive();
struct SchedulerWorkGuard {
  SchedulerWorkGuard() { schedulerBeginWork(); }
  ~SchedulerWorkGuard() { schedulerEndWork(); }
};
