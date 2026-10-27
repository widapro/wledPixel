#pragma once
#include "Scheduler.h"
#include <ArduinoJson.h>
template <typename TString>
inline TString restoredLiveString(JsonVariantConst value) {
  // Live settings have no profile sentinel. Legacy null means an unset value,
  // never the literal text "null" produced by ArduinoJson's String converter.
  return value.isNull() ? TString() : value.template as<TString>();
}
#include <cstring>
#include <initializer_list>

// Missing keys preserve old backups; JSON null means "no change", whereas an
// empty string explicitly clears a value. Keep these states distinct.
template <size_t N>
inline void restoreProfileString(JsonVariantConst value, char (&out)[N]) {
  memset(out, 0, N);
  if (value.isNull()) {
    out[0] = static_cast<char>(SCHED_STR_NO_CHANGE);
  } else {
    strncpy(out, value.as<const char *>() ? value.as<const char *>() : "", N - 1);
  }
}

inline bool profileSourceIdsFit(JsonObjectConst po) {
  for (JsonVariantConst value : po["zoneMqttTopic"].as<JsonArrayConst>())
    if (value.as<JsonString>().size() >= SCHED_SOURCE_ID_SIZE) return false;
  return true;
}

inline bool progressBarSourceIdsFit(JsonObjectConst po) {
  for (const char *key : {"dataSourceId", "conditionSourceId", "dynSourceId"})
    if (po[key].as<JsonString>().size() >= SCHED_SOURCE_ID_SIZE) return false;
  return true;
}

inline bool applyRestoreProfile(JsonObjectConst po, ScheduleProfile &profile) {
  if (!profileSourceIdsFit(po)) return false;
  if (po.containsKey("enabled")) profile.enabled = po["enabled"].as<decltype(profile.enabled)>();
  if (po.containsKey("minute")) profile.minute = po["minute"].as<decltype(profile.minute)>();
  if (po.containsKey("hour")) profile.hour = po["hour"].as<decltype(profile.hour)>();
  if (po.containsKey("dayOfWeek")) profile.dayOfWeek = po["dayOfWeek"].as<decltype(profile.dayOfWeek)>();
  if (po.containsKey("dayOfMonth")) profile.dayOfMonth = po["dayOfMonth"].as<decltype(profile.dayOfMonth)>();
  if (po.containsKey("power")) profile.power = po["power"].as<decltype(profile.power)>();
  if (po.containsKey("intensity")) profile.intensity = po["intensity"].as<decltype(profile.intensity)>();
  if (po.containsKey("zoneCount")) profile.zoneCount = po["zoneCount"].as<decltype(profile.zoneCount)>();
  if (po.containsKey("pbEnable")) profile.pbEnable = po["pbEnable"].as<decltype(profile.pbEnable)>();
  if (po.containsKey("zoneScrollInfinite")) profile.zoneScrollInfinite = po["zoneScrollInfinite"].as<decltype(profile.zoneScrollInfinite)>();
  if (po.containsKey("zoneCountdownShowUnits")) profile.zoneCountdownShowUnits = po["zoneCountdownShowUnits"].as<decltype(profile.zoneCountdownShowUnits)>();
  if (po.containsKey("zoneStockShowArrows")) profile.zoneStockShowArrows = po["zoneStockShowArrows"].as<decltype(profile.zoneStockShowArrows)>();
  if (po.containsKey("name")) {
    strncpy(profile.name, po["name"] | "", sizeof(profile.name) - 1);
    profile.name[sizeof(profile.name) - 1] = '\0';
  }
  for (uint8_t z = 0; z < 4; ++z) {
    if (z < po["zoneBegin"].size()) profile.zoneBegin[z] = po["zoneBegin"][z].as<uint8_t>();
    if (z < po["zoneEnd"].size()) profile.zoneEnd[z] = po["zoneEnd"][z].as<uint8_t>();
    if (z < po["zoneWorkMode"].size()) profile.zoneWorkMode[z] = po["zoneWorkMode"][z].as<uint8_t>();
    if (z < po["zoneOwmDisplay"].size()) profile.zoneOwmDisplay[z] = po["zoneOwmDisplay"][z].as<uint8_t>();
    if (z < po["zoneClockFmt"].size()) profile.zoneClockFmt[z] = po["zoneClockFmt"][z].as<uint8_t>();
    if (z < po["zoneFont"].size()) profile.zoneFont[z] = po["zoneFont"][z].as<uint8_t>();
    if (z < po["zoneScrollSpeed"].size()) profile.zoneScrollSpeed[z] = po["zoneScrollSpeed"][z].as<uint8_t>();
    if (z < po["zoneScrollPause"].size()) profile.zoneScrollPause[z] = po["zoneScrollPause"][z].as<uint8_t>();
    if (z < po["zoneCharspacing"].size()) profile.zoneCharspacing[z] = po["zoneCharspacing"][z].as<uint8_t>();
    if (z < po["zoneHaSensorId"].size()) restoreProfileString(po["zoneHaSensorId"][z], profile.zoneHaSensorId[z]);
    if (z < po["zoneHaPostfix"].size()) restoreProfileString(po["zoneHaPostfix"][z], profile.zoneHaPostfix[z]);
    if (z < po["zoneMqttTopic"].size()) restoreProfileString(po["zoneMqttTopic"][z], profile.zoneMqttTopic[z]);
    if (z < po["zoneScrollAlign"].size()) restoreProfileString(po["zoneScrollAlign"][z], profile.zoneScrollAlign[z]);
    if (z < po["zoneScrollEffectIn"].size()) restoreProfileString(po["zoneScrollEffectIn"][z], profile.zoneScrollEffectIn[z]);
    if (z < po["zoneScrollEffectOut"].size()) restoreProfileString(po["zoneScrollEffectOut"][z], profile.zoneScrollEffectOut[z]);
    if (z < po["zoneMqttPostfix"].size()) restoreProfileString(po["zoneMqttPostfix"][z], profile.zoneMqttPostfix[z]);
    if (z < po["zoneDs18b20Postfix"].size()) restoreProfileString(po["zoneDs18b20Postfix"][z], profile.zoneDs18b20Postfix[z]);
    if (z < po["zoneCountdownTarget"].size()) restoreProfileString(po["zoneCountdownTarget"][z], profile.zoneCountdownTarget[z]);
    if (z < po["zoneCountdownFormat"].size()) restoreProfileString(po["zoneCountdownFormat"][z], profile.zoneCountdownFormat[z]);
    if (z < po["zoneCountdownFinish"].size()) restoreProfileString(po["zoneCountdownFinish"][z], profile.zoneCountdownFinish[z]);
    if (z < po["zoneCountdownPrefix"].size()) restoreProfileString(po["zoneCountdownPrefix"][z], profile.zoneCountdownPrefix[z]);
    if (z < po["zoneCountdownSuffix"].size()) restoreProfileString(po["zoneCountdownSuffix"][z], profile.zoneCountdownSuffix[z]);
    if (z < po["zoneStockSymbols"].size()) restoreProfileString(po["zoneStockSymbols"][z], profile.zoneStockSymbols[z]);
    if (z < po["zoneStockDisplayFormat"].size()) restoreProfileString(po["zoneStockDisplayFormat"][z], profile.zoneStockDisplayFormat[z]);
    if (z < po["zoneStockPrefix"].size()) restoreProfileString(po["zoneStockPrefix"][z], profile.zoneStockPrefix[z]);
    if (z < po["zoneStockPostfix"].size()) restoreProfileString(po["zoneStockPostfix"][z], profile.zoneStockPostfix[z]);
  }
  return true;
}

inline bool applyRestoreProgressBar(JsonObjectConst po, ScheduleProgressBar &pb) {
  if (!progressBarSourceIdsFit(po)) return false;
  pb.version = SCHED_PROGRESS_BAR_VERSION;
  if (po.containsKey("dataSourceType")) {
    strncpy(pb.dataSourceType, po["dataSourceType"] | "", sizeof(pb.dataSourceType) - 1);
    pb.dataSourceType[sizeof(pb.dataSourceType) - 1] = '\0';
  }
  if (po.containsKey("dataSourceId")) {
    strncpy(pb.dataSourceId, po["dataSourceId"] | "", sizeof(pb.dataSourceId) - 1);
    pb.dataSourceId[sizeof(pb.dataSourceId) - 1] = '\0';
  }
  if (po.containsKey("conditionSourceType")) {
    strncpy(pb.conditionSourceType, po["conditionSourceType"] | "", sizeof(pb.conditionSourceType) - 1);
    pb.conditionSourceType[sizeof(pb.conditionSourceType) - 1] = '\0';
  }
  if (po.containsKey("conditionSourceId")) {
    strncpy(pb.conditionSourceId, po["conditionSourceId"] | "", sizeof(pb.conditionSourceId) - 1);
    pb.conditionSourceId[sizeof(pb.conditionSourceId) - 1] = '\0';
  }
  if (po.containsKey("conditionValue")) {
    strncpy(pb.conditionValue, po["conditionValue"] | "", sizeof(pb.conditionValue) - 1);
    pb.conditionValue[sizeof(pb.conditionValue) - 1] = '\0';
  }
  if (po.containsKey("dynSourceType")) {
    strncpy(pb.dynSourceType, po["dynSourceType"] | "", sizeof(pb.dynSourceType) - 1);
    pb.dynSourceType[sizeof(pb.dynSourceType) - 1] = '\0';
  }
  if (po.containsKey("dynSourceId")) {
    strncpy(pb.dynSourceId, po["dynSourceId"] | "", sizeof(pb.dynSourceId) - 1);
    pb.dynSourceId[sizeof(pb.dynSourceId) - 1] = '\0';
  }
  if (po.containsKey("dynRule1Value")) {
    strncpy(pb.dynRule1Value, po["dynRule1Value"] | "", sizeof(pb.dynRule1Value) - 1);
    pb.dynRule1Value[sizeof(pb.dynRule1Value) - 1] = '\0';
  }
  if (po.containsKey("dynRule2Value")) {
    strncpy(pb.dynRule2Value, po["dynRule2Value"] | "", sizeof(pb.dynRule2Value) - 1);
    pb.dynRule2Value[sizeof(pb.dynRule2Value) - 1] = '\0';
  }
  if (po.containsKey("minValue")) pb.minValue = po["minValue"].as<float>();
  if (po.containsKey("maxValue")) pb.maxValue = po["maxValue"].as<float>();
  if (po.containsKey("conditionEnabled")) pb.conditionEnabled = po["conditionEnabled"].as<bool>();
  if (po.containsKey("dynamicMode")) pb.dynamicMode = po["dynamicMode"].as<bool>();
  if (po.containsKey("dynamicAnimation")) pb.dynamicAnimation = po["dynamicAnimation"].as<uint8_t>();
  if (po.containsKey("dynamicSpeedMs")) pb.dynamicSpeedMs = po["dynamicSpeedMs"].as<uint16_t>();
  if (po.containsKey("dynRule2Animation")) pb.dynRule2Animation = po["dynRule2Animation"].as<uint8_t>();
  if (po.containsKey("dynRule2SpeedMs")) pb.dynRule2SpeedMs = po["dynRule2SpeedMs"].as<uint16_t>();
  return true;
}

static const char *const profileRestoreKeys[] = {
  "enabled", "minute", "hour", "dayOfWeek", "dayOfMonth", "power", "intensity", "zoneCount", "pbEnable", "zoneScrollInfinite", "zoneCountdownShowUnits", "zoneStockShowArrows", "name", "zoneBegin", "zoneEnd", "zoneWorkMode", "zoneOwmDisplay", "zoneClockFmt", "zoneFont", "zoneScrollSpeed", "zoneScrollPause", "zoneCharspacing", "zoneHaSensorId", "zoneHaPostfix", "zoneMqttTopic", "zoneScrollAlign", "zoneScrollEffectIn", "zoneScrollEffectOut", "zoneMqttPostfix", "zoneDs18b20Postfix", "zoneCountdownTarget", "zoneCountdownFormat", "zoneCountdownFinish", "zoneCountdownPrefix", "zoneCountdownSuffix", "zoneStockSymbols", "zoneStockDisplayFormat", "zoneStockPrefix", "zoneStockPostfix"
};
