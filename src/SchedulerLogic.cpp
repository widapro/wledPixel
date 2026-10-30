#include "SchedulerLogic.h"
#include "Scheduler.h"
#include "ScrollEffects.h"

bool schedulerTimeMatches(bool enabled, uint8_t minute, uint8_t hour,
                          uint8_t dayOfWeekMask, uint32_t dayOfMonthMask,
                          const struct tm &now) {
  if (!enabled || now.tm_wday < 0 || now.tm_wday > 6 || now.tm_mday < 1 ||
      now.tm_mday > 31)
    return false;
  if (minute != SCHED_NO_CHANGE_U8 &&
      minute != static_cast<uint8_t>(now.tm_min))
    return false;
  if (hour != SCHED_NO_CHANGE_U8 && hour != static_cast<uint8_t>(now.tm_hour))
    return false;
  if (dayOfWeekMask != 0x7F &&
      !(dayOfWeekMask & (1U << static_cast<uint8_t>(now.tm_wday))))
    return false;
  if (dayOfMonthMask != SCHED_NO_CHANGE_U32 &&
      !(dayOfMonthMask & (1UL << static_cast<uint8_t>(now.tm_mday - 1))))
    return false;
  return true;
}

#include <cstddef>
#include <cstdio>
#include <cstring>

void initProfileDefaults(ScheduleProfile &p, uint8_t idx) {
  memset(&p, 0, sizeof(ScheduleProfile));
  p.enabled    = false;
  p.minute     = SCHED_NO_CHANGE_U8;
  p.hour       = SCHED_NO_CHANGE_U8;
  p.dayOfWeek  = 0x7F;
  p.dayOfMonth = SCHED_NO_CHANGE_U32;
  p.power      = SCHED_NO_CHANGE_U8;
  p.intensity  = 0;
  p.zoneCount  = 0;
  p.pbEnable   = 0;
  for (uint8_t z = 0; z < 4; z++) {
    p.zoneBegin[z]     = SCHED_NO_CHANGE_U8;
    p.zoneEnd[z]       = SCHED_NO_CHANGE_U8;
    p.zoneWorkMode[z]  = CWRK_NO_CHANGE;
    p.zoneOwmDisplay[z]= COWM_NO_CHANGE;
    p.zoneClockFmt[z]  = CCLK_NO_CHANGE;
    p.zoneFont[z]      = CFNT_NO_CHANGE;
    p.zoneScrollSpeed[z] = SCHED_NO_CHANGE_U8;
    p.zoneScrollPause[z] = SCHED_NO_CHANGE_U8;
    p.zoneCharspacing[z] = SCHED_NO_CHANGE_U8;
    p.zoneHaSensorId[z][0] = SCHED_STR_NO_CHANGE;
    p.zoneHaPostfix[z][0]  = SCHED_STR_NO_CHANGE;
    p.zoneMqttTopic[z][0]  = SCHED_STR_NO_CHANGE;
    p.zoneScrollAlign[z][0] = SCHED_STR_NO_CHANGE;
    p.zoneScrollEffectIn[z][0] = SCHED_STR_NO_CHANGE;
    p.zoneScrollEffectOut[z][0] = SCHED_STR_NO_CHANGE;
    p.zoneMqttPostfix[z][0] = SCHED_STR_NO_CHANGE;
    p.zoneDs18b20Postfix[z][0] = SCHED_STR_NO_CHANGE;
    p.zoneCountdownTarget[z][0] = SCHED_STR_NO_CHANGE;
    p.zoneCountdownFormat[z][0] = SCHED_STR_NO_CHANGE;
    p.zoneCountdownFinish[z][0] = SCHED_STR_NO_CHANGE;
    p.zoneCountdownPrefix[z][0] = SCHED_STR_NO_CHANGE;
    p.zoneCountdownSuffix[z][0] = SCHED_STR_NO_CHANGE;
    p.zoneStockSymbols[z][0] = SCHED_STR_NO_CHANGE;
    p.zoneStockDisplayFormat[z][0] = SCHED_STR_NO_CHANGE;
    p.zoneStockPrefix[z][0] = SCHED_STR_NO_CHANGE;
    p.zoneStockPostfix[z][0] = SCHED_STR_NO_CHANGE;
  }
  snprintf(p.name, sizeof(p.name), "Profile %u", idx + 1);
}

void schedulerInitProfileForEdit(uint8_t idx, ScheduleProfile &out) {
  initProfileDefaults(out, idx);
}

void schedulerMigrateProfileV2InPlace(ScheduleProfile &profile) {
  uint8_t *bytes = reinterpret_cast<uint8_t *>(&profile);
  constexpr size_t oldHaOffset =
      offsetof(ScheduleProfileV2, zoneHaSensorId);
  constexpr size_t oldTailOffset =
      offsetof(ScheduleProfileV2, zoneHaPostfix);
  constexpr size_t newTailOffset =
      offsetof(ScheduleProfileV3, zoneHaPostfix);

  // Move the unchanged tail first, from high addresses to high addresses.
  // memmove makes the migration safe while old and new formats share a single
  // buffer.
  memmove(bytes + newTailOffset, bytes + oldTailOffset,
          sizeof(ScheduleProfileV2) - oldTailOffset);

  // Expand HA IDs in reverse order so a destination never overwrites a source
  // that has not been moved yet.
  for (int8_t z = 3; z >= 0; z--) {
    uint8_t *source = bytes + oldHaOffset + static_cast<size_t>(z) * 32;
    uint8_t *dest = bytes + oldHaOffset + static_cast<size_t>(z) * 64;
    memmove(dest, source, 32);
    memset(dest + 32, 0, 32);
    if (dest[0] != static_cast<uint8_t>(SCHED_STR_NO_CHANGE))
      dest[31] = '\0';
  }
  schedulerMigrateProfileV3InPlace(profile);
}

void schedulerMigrateProfileV3InPlace(ScheduleProfile &profile) {
  uint8_t *bytes = reinterpret_cast<uint8_t *>(&profile);
  constexpr size_t oldOffset = offsetof(ScheduleProfileV3, zoneMqttTopic);
  constexpr size_t oldTail = offsetof(ScheduleProfileV3, zoneScrollSpeed);
  constexpr size_t newTail = offsetof(ScheduleProfile, zoneScrollSpeed);
  memmove(bytes + newTail, bytes + oldTail, sizeof(ScheduleProfileV3) - oldTail);
  for (int z = 3; z >= 0; --z) {
    uint8_t *dest = reinterpret_cast<uint8_t *>(profile.zoneMqttTopic[z]);
    memmove(dest, bytes + oldOffset + z * 32, 32);
    dest[31] = 0;
    memset(dest + 32, 0, SCHED_SOURCE_ID_SIZE - 32);
  }
}

void schedulerMigrateProgressBarV1InPlace(ScheduleProgressBar &bar) {
  uint8_t *bytes = reinterpret_cast<uint8_t *>(&bar);
  // Move each unchanged segment and its preceding string from back to front.
  // This keeps migration within the shared buffer on the ESP8266.
#define EXPAND_BAR_STRING(field, next, end) do { \
    constexpr size_t oldStart = offsetof(ScheduleProgressBarV1, field); \
    constexpr size_t oldEnd = offsetof(ScheduleProgressBarV1, next); \
    constexpr size_t newEnd = offsetof(ScheduleProgressBar, next); \
    memmove(bytes + newEnd, bytes + oldEnd, (end) - oldEnd); \
    memmove(bar.field, bytes + oldStart, 48); \
    bar.field[47] = '\0'; \
    memset(bar.field + 48, 0, sizeof(bar.field) - 48); \
  } while (0)
  EXPAND_BAR_STRING(dynSourceId, dynRule1Value, sizeof(ScheduleProgressBarV1));
  EXPAND_BAR_STRING(conditionSourceId, conditionValue, offsetof(ScheduleProgressBarV1, dynSourceId));
  EXPAND_BAR_STRING(dataSourceId, minValue, offsetof(ScheduleProgressBarV1, conditionSourceId));
#undef EXPAND_BAR_STRING
  bar.version = SCHED_PROGRESS_BAR_VERSION;
}

template <size_t N>
void sanitizeProfileString(char (&value)[N]) {
  if ((uint8_t)value[0] == SCHED_STR_NO_CHANGE) {
    if (N > 1)
      value[1] = '\0';
    return;
  }
  value[N - 1] = '\0';
}

void sanitizeScheduleProfile(ScheduleProfile &profile) {
  profile.name[sizeof(profile.name) - 1] = '\0';
  for (uint8_t z = 0; z < 4; z++) {
    sanitizeProfileString(profile.zoneHaSensorId[z]);
    sanitizeProfileString(profile.zoneHaPostfix[z]);
    sanitizeProfileString(profile.zoneMqttTopic[z]);
    sanitizeProfileString(profile.zoneScrollAlign[z]);
    sanitizeProfileString(profile.zoneScrollEffectIn[z]);
    sanitizeProfileString(profile.zoneScrollEffectOut[z]);
    migrateScrollEffect(profile.zoneScrollEffectIn[z]);
    migrateScrollEffect(profile.zoneScrollEffectOut[z]);
    sanitizeProfileString(profile.zoneMqttPostfix[z]);
    sanitizeProfileString(profile.zoneDs18b20Postfix[z]);
    sanitizeProfileString(profile.zoneCountdownTarget[z]);
    sanitizeProfileString(profile.zoneCountdownFormat[z]);
    sanitizeProfileString(profile.zoneCountdownFinish[z]);
    sanitizeProfileString(profile.zoneCountdownPrefix[z]);
    sanitizeProfileString(profile.zoneCountdownSuffix[z]);
    sanitizeProfileString(profile.zoneStockSymbols[z]);
    sanitizeProfileString(profile.zoneStockDisplayFormat[z]);
    sanitizeProfileString(profile.zoneStockPrefix[z]);
    sanitizeProfileString(profile.zoneStockPostfix[z]);
  }
}

bool schedulerProfileValid(const ScheduleProfile &p, uint8_t modules) {
  if ((p.minute != 255 && p.minute > 59) ||
      (p.hour != 255 && p.hour > 23) || p.dayOfWeek > 127 ||
      p.dayOfMonth > 0x7FFFFFFFUL ||
      (p.power != 255 && p.power > 1) || p.intensity > 16 || p.zoneCount > 4)
    return false;
  for (uint8_t z = 0; z < 4; ++z) {
    if ((p.zoneBegin[z] != 255 && p.zoneBegin[z] >= modules) ||
        (p.zoneEnd[z] != 255 && p.zoneEnd[z] >= modules) ||
        (p.zoneBegin[z] != 255 && p.zoneEnd[z] != 255 && p.zoneBegin[z] > p.zoneEnd[z]) ||
        p.zoneWorkMode[z] > CWRK_STOCK || p.zoneOwmDisplay[z] > COWM_DESCRIPTION ||
        p.zoneClockFmt[z] > CCLK_HHMM12AP || p.zoneFont[z] > CFNT_PIXEL_3D)
      return false;
  }
  return true;
}
