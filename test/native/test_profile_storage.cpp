#include "ProfileStorage.h"
#include "SchedulerStateStorage.h"
#include "test_profile_migration.h"
#include <cassert>
#include <iostream>
#include <vector>
int main() {
  // Reproduce Profile 3 surviving in the live settings while its ON AIR marker
  // was lost on reboot. Exercise the same storage helpers as ConfigStore.
  Preferences::disk.clear();
  {
    Preferences state;
    state.begin("cronSettings");
    assert(loadLastActiveProfile(state) == -1);
    assert(storeLastActiveProfile(state, 2));
  }
  {
    Preferences rebooted;
    rebooted.begin("cronSettings");
    assert(loadLastActiveProfile(rebooted) == 2);
    Preferences::writesLeft = 0;
    assert(storeLastActiveProfile(rebooted, 2)); // no rewrite every minute
    assert(!storeLastActiveProfile(rebooted, 0));
    assert(loadLastActiveProfile(rebooted) == 2);
    Preferences::writesLeft = -1;
    for (int8_t idx = 0; idx < MAX_PROFILES; ++idx) {
      assert(storeLastActiveProfile(rebooted, idx));
      assert(loadLastActiveProfile(rebooted) == idx);
    }
    assert(storeLastActiveProfile(rebooted, -1));
    assert(loadLastActiveProfile(rebooted) == -1);
    assert(rebooted.putUChar("lastActive", MAX_PROFILES));
    assert(loadLastActiveProfile(rebooted) == -1);
    assert(rebooted.putUChar("lastActive", 254));
    assert(loadLastActiveProfile(rebooted) == -1);
  }
  Preferences::disk.clear();
  checkProfileMigrations();
  Preferences::disk.clear();
  for (size_t size : {size_t(1), size_t(2), size_t(3), size_t(127), size_t(128),
                      size_t(129), sizeof(ScheduleProgressBar), sizeof(ScheduleProfile)}) {
    for (int pattern = 0; pattern < 3; ++pattern) {
      std::vector<uint8_t> raw(size), decoded(size), encoded(size + (size + 127) / 128 + 10);
      for (size_t i = 0; i < size; ++i)
        raw[i] = pattern == 0 ? 0 : pattern == 1 ? static_cast<uint8_t>(i * 37) : static_cast<uint8_t>((i / 100) % 2 ? 0xff : i);
      size_t length = storageCodec::encode(raw.data(), raw.size(), encoded.data(), encoded.size());
      assert(length && storageCodec::decode(encoded.data(), length, decoded.data(), decoded.size()));
      assert(raw == decoded);
      for (size_t cut = 0; cut < length; ++cut)
        assert(!storageCodec::decode(encoded.data(), cut, decoded.data(), decoded.size()));
      for (size_t byte = 0; byte < length; ++byte) {
        encoded[byte] ^= 1;
        assert(!storageCodec::decode(encoded.data(), length, decoded.data(), decoded.size()));
        encoded[byte] ^= 1;
      }
      assert(!storageCodec::encode(raw.data(), raw.size(), encoded.data(), 9));
    }
  }
  // Existing raw records and subsequently committed records must both replace
  // the retired sprite without changing the remaining profile settings.
  {
    ScheduleProfile legacyEffect, loadedEffect;
    initProfileDefaults(legacyEffect, 0);
    strcpy(legacyEffect.zoneScrollEffectIn[0], "MARIO");
    strcpy(legacyEffect.zoneScrollEffectOut[1], "MARIO");
    strcpy(legacyEffect.zoneScrollEffectIn[2], "HEART");
    legacyEffect.zoneScrollEffectOut[3][0] = '\0';
    Preferences legacySettings;
    legacySettings.begin("cronP0");
    legacySettings.putBytes("data", &legacyEffect, sizeof(legacyEffect));
    legacySettings.end();
    ProfileStorage sprites;
    assert(sprites.load(0, loadedEffect));
    assert(!strcmp(loadedEffect.zoneScrollEffectIn[0], "ROBOT"));
    assert(!strcmp(loadedEffect.zoneScrollEffectOut[1], "ROBOT"));
    assert(!strcmp(loadedEffect.zoneScrollEffectIn[2], "HEART"));
    assert(static_cast<uint8_t>(loadedEffect.zoneScrollEffectIn[3][0]) == SCHED_STR_NO_CHANGE);
    assert(loadedEffect.zoneScrollEffectOut[3][0] == '\0');
    assert(sprites.commit(0, legacyEffect));
    ProfileStorage rebooted;
    ScheduleProfile afterReboot;
    assert(rebooted.load(0, afterReboot));
    assert(!memcmp(&afterReboot, &loadedEffect, sizeof(afterReboot)));
    Preferences::disk.clear();
  }
  ScheduleProfile oldProfile, newProfile, loaded;
  initProfileDefaults(oldProfile, 0);
  newProfile = oldProfile; newProfile.intensity = 12;
  ScheduleProgressBar oldBar = {}, newBar = {}, loadedBar;
  oldBar.version = newBar.version = SCHED_PROGRESS_BAR_VERSION;
  oldBar.maxValue = 100; newBar.maxValue = 200;
  Preferences prefs;
  prefs.begin("cronP0"); prefs.putBytes("data", &oldProfile, sizeof(oldProfile));
  prefs.begin("cronB00"); prefs.putBytes("data", &oldBar, sizeof(oldBar));
  prefs.begin("cronB01"); prefs.putBytes("data", &oldBar, sizeof(oldBar));
  const auto legacy = Preferences::disk;
  // Interrupt at every possible persistent write, then reboot with no RAM state.
  for (int budget = 0; budget < 10; ++budget) {
    Preferences::disk = legacy; Preferences::writesLeft = budget;
    ProfileStorage store;
    bool ok = store.stageBar(0, 0, &newBar) && store.commit(0, newProfile);
    Preferences::writesLeft = -1;
    ProfileStorage rebooted;
    assert(rebooted.load(0, loaded));
    assert(rebooted.loadBar(0, 0, loadedBar) == 1);
    assert(loaded.intensity == (ok ? 12 : 0));
    assert(loadedBar.maxValue == (ok ? 200 : 100));
  }
  // Repeat failures after migration, including copies of unchanged PB records.
  const auto bankOne = Preferences::disk;
  newProfile.intensity = 13; newBar.maxValue = 300;
  for (int budget = 0; budget < 10; ++budget) {
    Preferences::disk = bankOne; Preferences::writesLeft = budget;
    ProfileStorage store;
    bool ok = store.stageBar(0, 0, &newBar) && store.commit(0, newProfile);
    Preferences::writesLeft = -1;
    ProfileStorage rebooted;
    assert(rebooted.load(0, loaded));
    assert(rebooted.loadBar(0, 0, loadedBar) == 1);
    assert(loaded.intensity == (ok ? 13 : 12));
    assert(loadedBar.maxValue == (ok ? 300 : 200));
    assert(rebooted.loadBar(0, 1, loadedBar) == 1 && loadedBar.maxValue == 100);
  }
  // Alternate banks, inherit unchanged records, and roll back an abandoned edit.
  ProfileStorage store;
  assert(store.stageBar(0, 0, nullptr));
  assert(store.commit(0, oldProfile));
  assert(store.loadBar(0, 0, loadedBar) == 0);
  assert(store.stageBar(0, 0, &newBar));
  store.abandon(0);
  assert(store.commit(0, newProfile));
  assert(store.loadBar(0, 0, loadedBar) == 0);
  assert(!store.stageBar(MAX_PROFILES, 0, &newBar));
  assert(!store.stageBar(0, 4, &newBar));
  // New ESP32 records are compact; legacy raw records remain readable. A bad
  // checksum must never be mistaken for a valid profile or progress bar.
  assert(store.load(0, loaded));
  const uint8_t active = store.bank(0);
  auto &blob = Preferences::disk[active == 1 ? "cronP0/data1" : "cronP0/data2"];
#if !defined(ESP8266)
  assert(blob.size() < sizeof(ScheduleProfile));
  blob.back() ^= 1;
  assert(!store.load(0, loaded));
#else
  assert(blob.size() == sizeof(ScheduleProfile));
#endif
  std::cout << "PASS compressed storage/checksum/bounds, atomic profile/PB commit, interrupted writes, reboot, alternate banks, abort\n";
}
