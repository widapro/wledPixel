#pragma once
#include "ProfileStorage.h"
#include <cassert>
#include <cstring>
#include <vector>

template <typename Record>
void putMigrationRecord(Preferences &prefs, const char *key, const Record &record,
                        bool compressed) {
  if (!compressed) {
    assert(prefs.putBytes(key, &record, sizeof(record)) == sizeof(record));
    return;
  }
  std::vector<uint8_t> bytes(sizeof(record) * 2);
  const size_t length = storageCodec::encode(reinterpret_cast<const uint8_t *>(&record),
                                            sizeof(record), bytes.data(), bytes.size());
  assert(length && length < sizeof(record));
  assert(prefs.putBytes(key, bytes.data(), length) == length);
}

template <typename OldProfile>
void checkProfileMigration(bool compressed, uint8_t bank) {
  Preferences::disk.clear();
#if defined(WLED_TEST_PROFILE_FS)
  fakeFs::disk.clear(); fakeFs::dirs.clear();
#endif
  OldProfile old = {};
  old.enabled = true; old.minute = 34; old.hour = 12; old.intensity = 7;
  old.dayOfMonth = 12345; old.zoneScrollInfinite = 0xa5;
  strcpy(old.name, "legacy");
  for (int z = 0; z < 4; ++z) {
    snprintf(old.zoneHaSensorId[z], sizeof(old.zoneHaSensorId[z]), "sensor.zone%d", z);
    snprintf(old.zoneMqttTopic[z], sizeof(old.zoneMqttTopic[z]), "old/topic/zone%d", z);
    snprintf(old.zoneHaPostfix[z], sizeof(old.zoneHaPostfix[z]), "ha%d", z);
    old.zoneScrollSpeed[z] = 41 + z;
    snprintf(old.zoneStockSymbols[z], sizeof(old.zoneStockSymbols[z]), "STOCK%d", z);
    snprintf(old.zoneStockPostfix[z], sizeof(old.zoneStockPostfix[z]), "post%d", z);
  }
  memset(old.zoneMqttTopic[1], 0, sizeof(old.zoneMqttTopic[1]));
  old.zoneMqttTopic[1][0] = SCHED_STR_NO_CHANGE;
  memset(old.zoneMqttTopic[2], 0, sizeof(old.zoneMqttTopic[2]));
  memset(old.zoneHaSensorId[3], 0, sizeof(old.zoneHaSensorId[3]));
  old.zoneHaSensorId[3][0] = SCHED_STR_NO_CHANGE;
  ScheduleProgressBarV1 oldBar = {};
  oldBar.version = 1;
  strcpy(oldBar.dataSourceType, "mqtt"); strcpy(oldBar.dataSourceId, "old/data/topic");
  oldBar.minValue = -13.5f; oldBar.maxValue = 300.25f;
  oldBar.conditionEnabled = true; strcpy(oldBar.conditionSourceType, "ha");
  strcpy(oldBar.conditionSourceId, "sensor.condition"); strcpy(oldBar.conditionValue, "active");
  oldBar.dynamicMode = true; oldBar.dynamicAnimation = 16; oldBar.dynamicSpeedMs = 1234;
  strcpy(oldBar.dynSourceType, "mqtt"); strcpy(oldBar.dynSourceId, "old/dynamic/topic");
  strcpy(oldBar.dynRule1Value, "on"); strcpy(oldBar.dynRule2Value, "off");
  oldBar.dynRule2Animation = 7; oldBar.dynRule2SpeedMs = 4321;
  const char *key = bank == 0 ? "data" : bank == 1 ? "data1" : "data2";
  Preferences prefs;
  prefs.begin("cronP0");
  assert(prefs.putUChar("bank", bank) == 1);
  putMigrationRecord(prefs, key, old, compressed);
  prefs.end(); prefs.begin("cronB00");
  putMigrationRecord(prefs, key, oldBar, compressed);
  prefs.end();
  ProfileStorage storage;
  ScheduleProfile profile;
  ScheduleProgressBar bar;
  assert(storage.load(0, profile));
  assert(profile.enabled && profile.minute == 34 && profile.hour == 12);
  assert(profile.intensity == 7 && profile.dayOfMonth == 12345);
  assert(profile.zoneScrollInfinite == 0xa5 && !strcmp(profile.name, "legacy"));
  for (int z = 0; z < 4; ++z) {
    assert(!memcmp(profile.zoneHaSensorId[z], old.zoneHaSensorId[z], sizeof(old.zoneHaSensorId[z])));
    assert(!memcmp(profile.zoneMqttTopic[z], old.zoneMqttTopic[z], sizeof(old.zoneMqttTopic[z])));
    assert(!strcmp(profile.zoneHaPostfix[z], old.zoneHaPostfix[z]));
    assert(profile.zoneScrollSpeed[z] == old.zoneScrollSpeed[z]);
    assert(!strcmp(profile.zoneStockSymbols[z], old.zoneStockSymbols[z]));
    assert(!strcmp(profile.zoneStockPostfix[z], old.zoneStockPostfix[z]));
  }
  assert(storage.loadBar(0, 0, bar) == 1 && bar.version == SCHED_PROGRESS_BAR_VERSION);
  assert(!strcmp(bar.dataSourceType, oldBar.dataSourceType) && !strcmp(bar.dataSourceId, oldBar.dataSourceId));
  assert(bar.minValue == oldBar.minValue && bar.maxValue == oldBar.maxValue);
  assert(bar.conditionEnabled && !strcmp(bar.conditionSourceType, oldBar.conditionSourceType));
  assert(!strcmp(bar.conditionSourceId, oldBar.conditionSourceId) && !strcmp(bar.conditionValue, oldBar.conditionValue));
  assert(bar.dynamicMode && bar.dynamicAnimation == 16 && bar.dynamicSpeedMs == 1234);
  assert(!strcmp(bar.dynSourceType, oldBar.dynSourceType) && !strcmp(bar.dynSourceId, oldBar.dynSourceId));
  assert(!strcmp(bar.dynRule1Value, "on") && !strcmp(bar.dynRule2Value, "off"));
  assert(bar.dynRule2Animation == 7 && bar.dynRule2SpeedMs == 4321);
  memset(profile.zoneMqttTopic[0], 't', sizeof(profile.zoneMqttTopic[0]) - 1);
  profile.zoneMqttTopic[0][sizeof(profile.zoneMqttTopic[0]) - 1] = 0;
  strcpy(bar.dataSourceId, profile.zoneMqttTopic[0]);
  strcpy(bar.conditionSourceId, "esp8266_meteo_dacha/sensor/total_power_consumed_today");
  strcpy(bar.dynSourceId, profile.zoneMqttTopic[0]);
  assert(storage.stageBar(0, 0, &bar) && storage.commit(0, profile));
  ScheduleProfile reloaded;
  ScheduleProgressBar reloadedBar;
  assert(storage.load(0, reloaded) && !memcmp(&profile, &reloaded, sizeof(profile)));
  assert(storage.loadBar(0, 0, reloadedBar) == 1 && !memcmp(&bar, &reloadedBar, sizeof(bar)));
}

inline void checkProfileMigrations() {
  for (uint8_t bank = 0; bank < 3; ++bank) {
    for (bool compressed : {false, true}) {
#if defined(ESP8266)
      if (compressed) continue; // ESP8266 has always stored raw records.
#endif
      checkProfileMigration<ScheduleProfileV2>(compressed, bank);
      checkProfileMigration<ScheduleProfileV3>(compressed, bank);
    }
  }
}
