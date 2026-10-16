#include "ProfileJson.h"
#include "ConfigStringStorage.h"
#include <cassert>
#include <iostream>
#include <string>
#include <map>

struct LimitedStringStorage {
  std::map<std::string, std::string> values;
  bool removalFails = false;
  bool isKey(const char *key) const { return values.count(key) != 0; }
  bool remove(const char *key) {
    if (removalFails) return false;
    return values.erase(key) != 0;
  }
  size_t putString(const char *key, const char *value) {
    // Simulate full NVS: an append needs space even when replacing a key.
    if (values.size() >= 2) return 0;
    values[key] = value;
    return values[key].size();
  }
};

int main() {
  LimitedStringStorage strings;
  strings.values = {{"empty", ""}, {"rule", "old"}};
  assert(!storeOptionalConfigString(strings, "rule", std::string("new")));
  assert(storeOptionalConfigString(strings, "empty", std::string()));
  assert(!strings.isKey("empty"));
  assert(storeOptionalConfigString(strings, "rule", std::string("new")));
  strings.removalFails = true;
  assert(!storeOptionalConfigString(strings, "rule", std::string()));
  assert(strings.values["rule"] == "new");
  strings.removalFails = false;
  assert(storeOptionalConfigString(strings, "rule", std::string()));
  assert(storeOptionalConfigString(strings, "absent", std::string()));
  strings.values = {{"font", "wledFont_cyrillic"}, {"rule", "old"}};
  assert(storeDefaultConfigString(strings, "font", std::string("wledFont_cyrillic"), "wledFont_cyrillic"));
  assert(!strings.isKey("font"));
  assert(storeDefaultConfigString(strings, "font", std::string(), "wledFont_cyrillic"));
  assert(strings.isKey("font") && strings.values["font"].empty());

  // Reproduce the legacy live-setting restore failure seen on the ESP32 board.
  // Profile null still has its separate "no change" semantics below.
  DynamicJsonDocument live(512);
  assert(!deserializeJson(live, R"({"unset":null,"empty":"","literal":"null","text":"off","number":123})"));
  assert(restoredLiveString<std::string>(live["unset"]).empty());
  assert(restoredLiveString<std::string>(live["empty"]).empty());
  assert(restoredLiveString<std::string>(live["literal"]) == "null");
  assert(restoredLiveString<std::string>(live["text"]) == "off");
  assert(restoredLiveString<std::string>(live["number"]) == "123");

  ScheduleProfile p;
  initProfileDefaults(p, 2);
  assert(static_cast<uint8_t>(p.zoneHaSensorId[0][0]) == SCHED_STR_NO_CHANGE);
  assert(schedulerProfileValid(p, 16));
  p.minute = 60;
  assert(!schedulerProfileValid(p, 16));
  p.minute = 255;
  p.zoneEnd[0] = 16;
  assert(!schedulerProfileValid(p, 16));
  p.zoneEnd[0] = 1;
  p.zoneBegin[0] = 2;
  assert(!schedulerProfileValid(p, 16));
  initProfileDefaults(p, 2);
  DynamicJsonDocument doc(8192);
  assert(!deserializeJson(doc, R"({"zoneHaSensorId":[null,"","sensor.temperature"],"zoneScrollSpeed":[42],"zoneCountdownPrefix":["left"],"zoneStockSymbols":["AAPL"],"zoneScrollInfinite":17})"));
  applyRestoreProfile(doc.as<JsonObjectConst>(), p);
  assert(static_cast<uint8_t>(p.zoneHaSensorId[0][0]) == SCHED_STR_NO_CHANGE);
  assert(p.zoneHaSensorId[1][0] == '\0');
  assert(!strcmp(p.zoneHaSensorId[2], "sensor.temperature"));
  assert(static_cast<uint8_t>(p.zoneHaSensorId[3][0]) == SCHED_STR_NO_CHANGE);
  assert(p.zoneScrollSpeed[0] == 42 && p.zoneScrollSpeed[1] == 255);
  assert(!strcmp(p.zoneCountdownPrefix[0], "left"));
  assert(!strcmp(p.zoneStockSymbols[0], "AAPL"));
  assert(p.zoneScrollInfinite == 17);

  // Every string override must preserve null, empty, and bounded values.
  doc.clear();
  doc["zoneHaSensorId"][0] = nullptr;
  doc["zoneHaSensorId"][1] = "";
  doc["zoneHaSensorId"][2] = "value";
  applyRestoreProfile(doc.as<JsonObjectConst>(), p);
  assert(static_cast<uint8_t>(p.zoneHaSensorId[0][0]) == SCHED_STR_NO_CHANGE);
  assert(p.zoneHaSensorId[1][0] == '\0');
  assert(!strcmp(p.zoneHaSensorId[2], "value"));
  doc.clear();
  doc["zoneHaPostfix"][0] = nullptr;
  doc["zoneHaPostfix"][1] = "";
  doc["zoneHaPostfix"][2] = "value";
  applyRestoreProfile(doc.as<JsonObjectConst>(), p);
  assert(static_cast<uint8_t>(p.zoneHaPostfix[0][0]) == SCHED_STR_NO_CHANGE);
  assert(p.zoneHaPostfix[1][0] == '\0');
  assert(!strcmp(p.zoneHaPostfix[2], "value"));
  doc.clear();
  doc["zoneMqttTopic"][0] = nullptr;
  doc["zoneMqttTopic"][1] = "";
  doc["zoneMqttTopic"][2] = "value";
  applyRestoreProfile(doc.as<JsonObjectConst>(), p);
  assert(static_cast<uint8_t>(p.zoneMqttTopic[0][0]) == SCHED_STR_NO_CHANGE);
  assert(p.zoneMqttTopic[1][0] == '\0');
  assert(!strcmp(p.zoneMqttTopic[2], "value"));
  doc.clear();
  doc["zoneScrollAlign"][0] = nullptr;
  doc["zoneScrollAlign"][1] = "";
  doc["zoneScrollAlign"][2] = "value";
  applyRestoreProfile(doc.as<JsonObjectConst>(), p);
  assert(static_cast<uint8_t>(p.zoneScrollAlign[0][0]) == SCHED_STR_NO_CHANGE);
  assert(p.zoneScrollAlign[1][0] == '\0');
  assert(!strcmp(p.zoneScrollAlign[2], "value"));
  doc.clear();
  doc["zoneScrollEffectIn"][0] = nullptr;
  doc["zoneScrollEffectIn"][1] = "";
  doc["zoneScrollEffectIn"][2] = "value";
  applyRestoreProfile(doc.as<JsonObjectConst>(), p);
  assert(static_cast<uint8_t>(p.zoneScrollEffectIn[0][0]) == SCHED_STR_NO_CHANGE);
  assert(p.zoneScrollEffectIn[1][0] == '\0');
  assert(!strcmp(p.zoneScrollEffectIn[2], "value"));
  doc.clear();
  doc["zoneScrollEffectOut"][0] = nullptr;
  doc["zoneScrollEffectOut"][1] = "";
  doc["zoneScrollEffectOut"][2] = "value";
  applyRestoreProfile(doc.as<JsonObjectConst>(), p);
  assert(static_cast<uint8_t>(p.zoneScrollEffectOut[0][0]) == SCHED_STR_NO_CHANGE);
  assert(p.zoneScrollEffectOut[1][0] == '\0');
  assert(!strcmp(p.zoneScrollEffectOut[2], "value"));
  doc.clear();
  doc["zoneMqttPostfix"][0] = nullptr;
  doc["zoneMqttPostfix"][1] = "";
  doc["zoneMqttPostfix"][2] = "value";
  applyRestoreProfile(doc.as<JsonObjectConst>(), p);
  assert(static_cast<uint8_t>(p.zoneMqttPostfix[0][0]) == SCHED_STR_NO_CHANGE);
  assert(p.zoneMqttPostfix[1][0] == '\0');
  assert(!strcmp(p.zoneMqttPostfix[2], "value"));
  doc.clear();
  doc["zoneDs18b20Postfix"][0] = nullptr;
  doc["zoneDs18b20Postfix"][1] = "";
  doc["zoneDs18b20Postfix"][2] = "value";
  applyRestoreProfile(doc.as<JsonObjectConst>(), p);
  assert(static_cast<uint8_t>(p.zoneDs18b20Postfix[0][0]) == SCHED_STR_NO_CHANGE);
  assert(p.zoneDs18b20Postfix[1][0] == '\0');
  assert(!strcmp(p.zoneDs18b20Postfix[2], "value"));
  doc.clear();
  doc["zoneCountdownTarget"][0] = nullptr;
  doc["zoneCountdownTarget"][1] = "";
  doc["zoneCountdownTarget"][2] = "value";
  applyRestoreProfile(doc.as<JsonObjectConst>(), p);
  assert(static_cast<uint8_t>(p.zoneCountdownTarget[0][0]) == SCHED_STR_NO_CHANGE);
  assert(p.zoneCountdownTarget[1][0] == '\0');
  assert(!strcmp(p.zoneCountdownTarget[2], "value"));
  doc.clear();
  doc["zoneCountdownFormat"][0] = nullptr;
  doc["zoneCountdownFormat"][1] = "";
  doc["zoneCountdownFormat"][2] = "value";
  applyRestoreProfile(doc.as<JsonObjectConst>(), p);
  assert(static_cast<uint8_t>(p.zoneCountdownFormat[0][0]) == SCHED_STR_NO_CHANGE);
  assert(p.zoneCountdownFormat[1][0] == '\0');
  assert(!strcmp(p.zoneCountdownFormat[2], "value"));
  doc.clear();
  doc["zoneCountdownFinish"][0] = nullptr;
  doc["zoneCountdownFinish"][1] = "";
  doc["zoneCountdownFinish"][2] = "value";
  applyRestoreProfile(doc.as<JsonObjectConst>(), p);
  assert(static_cast<uint8_t>(p.zoneCountdownFinish[0][0]) == SCHED_STR_NO_CHANGE);
  assert(p.zoneCountdownFinish[1][0] == '\0');
  assert(!strcmp(p.zoneCountdownFinish[2], "value"));
  doc.clear();
  doc["zoneCountdownPrefix"][0] = nullptr;
  doc["zoneCountdownPrefix"][1] = "";
  doc["zoneCountdownPrefix"][2] = "value";
  applyRestoreProfile(doc.as<JsonObjectConst>(), p);
  assert(static_cast<uint8_t>(p.zoneCountdownPrefix[0][0]) == SCHED_STR_NO_CHANGE);
  assert(p.zoneCountdownPrefix[1][0] == '\0');
  assert(!strcmp(p.zoneCountdownPrefix[2], "value"));
  doc.clear();
  doc["zoneCountdownSuffix"][0] = nullptr;
  doc["zoneCountdownSuffix"][1] = "";
  doc["zoneCountdownSuffix"][2] = "value";
  applyRestoreProfile(doc.as<JsonObjectConst>(), p);
  assert(static_cast<uint8_t>(p.zoneCountdownSuffix[0][0]) == SCHED_STR_NO_CHANGE);
  assert(p.zoneCountdownSuffix[1][0] == '\0');
  assert(!strcmp(p.zoneCountdownSuffix[2], "value"));
  doc.clear();
  doc["zoneStockSymbols"][0] = nullptr;
  doc["zoneStockSymbols"][1] = "";
  doc["zoneStockSymbols"][2] = "value";
  applyRestoreProfile(doc.as<JsonObjectConst>(), p);
  assert(static_cast<uint8_t>(p.zoneStockSymbols[0][0]) == SCHED_STR_NO_CHANGE);
  assert(p.zoneStockSymbols[1][0] == '\0');
  assert(!strcmp(p.zoneStockSymbols[2], "value"));
  doc.clear();
  doc["zoneStockDisplayFormat"][0] = nullptr;
  doc["zoneStockDisplayFormat"][1] = "";
  doc["zoneStockDisplayFormat"][2] = "value";
  applyRestoreProfile(doc.as<JsonObjectConst>(), p);
  assert(static_cast<uint8_t>(p.zoneStockDisplayFormat[0][0]) == SCHED_STR_NO_CHANGE);
  assert(p.zoneStockDisplayFormat[1][0] == '\0');
  assert(!strcmp(p.zoneStockDisplayFormat[2], "value"));
  doc.clear();
  doc["zoneStockPrefix"][0] = nullptr;
  doc["zoneStockPrefix"][1] = "";
  doc["zoneStockPrefix"][2] = "value";
  applyRestoreProfile(doc.as<JsonObjectConst>(), p);
  assert(static_cast<uint8_t>(p.zoneStockPrefix[0][0]) == SCHED_STR_NO_CHANGE);
  assert(p.zoneStockPrefix[1][0] == '\0');
  assert(!strcmp(p.zoneStockPrefix[2], "value"));
  doc.clear();
  doc["zoneStockPostfix"][0] = nullptr;
  doc["zoneStockPostfix"][1] = "";
  doc["zoneStockPostfix"][2] = "value";
  applyRestoreProfile(doc.as<JsonObjectConst>(), p);
  assert(static_cast<uint8_t>(p.zoneStockPostfix[0][0]) == SCHED_STR_NO_CHANGE);
  assert(p.zoneStockPostfix[1][0] == '\0');
  assert(!strcmp(p.zoneStockPostfix[2], "value"));
  doc.clear();
  doc["zoneHaSensorId"][0] = std::string(300, 'x');
  applyRestoreProfile(doc.as<JsonObjectConst>(), p);
  assert(strlen(p.zoneHaSensorId[0]) == 63);

  // Version 3 filtered restore must retain null array elements in a 2KB arena.
  DynamicJsonDocument filtered(2048), filter(192);
  filter["scheduleProfile0"]["zoneHaSensorId"] = true;
  const char *backup = R"({"backupVersion":3,"scheduleProfile0":{"zoneHaSensorId":[null,"","sensor.a","sensor.b"],"zoneStockSymbols":["ignored"]}})";
  assert(!deserializeJson(filtered, backup, DeserializationOption::Filter(filter)));
  initProfileDefaults(p, 0);
  applyRestoreProfile(filtered["scheduleProfile0"].as<JsonObjectConst>(), p);
  assert(static_cast<uint8_t>(p.zoneHaSensorId[0][0]) == SCHED_STR_NO_CHANGE);
  assert(p.zoneHaSensorId[1][0] == '\0');
  assert(!strcmp(p.zoneHaSensorId[2], "sensor.a"));
  assert(static_cast<uint8_t>(p.zoneStockSymbols[0][0]) == SCHED_STR_NO_CHANGE);

  ScheduleProfileV2 old = {};
  for (int z = 0; z < 4; ++z) {
    snprintf(old.zoneHaSensorId[z], 32, "sensor.%d", z);
    snprintf(old.zoneStockSymbols[z], 64, "stock%d", z);
  }
  old.zoneHaSensorId[1][0] = static_cast<char>(SCHED_STR_NO_CHANGE);
  strcpy(old.name, "old");
  memcpy(&p, &old, sizeof(old));
  schedulerMigrateProfileV2InPlace(p);
  assert(sizeof(p) == 1487);
  assert(!strcmp(p.name, "old"));
  for (int z = 0; z < 4; ++z) {
    assert(!strcmp(p.zoneStockSymbols[z], old.zoneStockSymbols[z]));
    assert(!memcmp(p.zoneHaSensorId[z], old.zoneHaSensorId[z], 32));
    assert(p.zoneHaSensorId[z][63] == '\0');
  }
  memset(p.zoneMqttTopic[0], 'x', sizeof(p.zoneMqttTopic[0]));
  sanitizeScheduleProfile(p);
  assert(strlen(p.zoneMqttTopic[0]) == 31);

  ScheduleProgressBar pb = {};
  doc.clear();
  assert(!deserializeJson(doc, R"({"dataSourceId":"sensor.progress","minValue":-12.5,"maxValue":99.25,"dynamicMode":true,"dynRule2SpeedMs":1234,"dynRule1Value":"active"})"));
  applyRestoreProgressBar(doc.as<JsonObjectConst>(), pb);
  assert(pb.version == 1 && pb.dynamicMode);
  assert(pb.minValue == -12.5f && pb.maxValue == 99.25f);
  assert(pb.dynRule2SpeedMs == 1234 && !strcmp(pb.dynRule1Value, "active"));
  std::cout << "PASS profile restore, null/empty/absent, bounds, V2 migration, progress bars\n";
}
