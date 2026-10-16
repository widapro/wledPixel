#include "ProfileStorage.h"
#include <cassert>
#include <iostream>
#include <vector>
int main() {
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
  ScheduleProfile oldProfile, newProfile, loaded;
  initProfileDefaults(oldProfile, 0);
  newProfile = oldProfile; newProfile.intensity = 12;
  ScheduleProgressBar oldBar = {}, newBar = {}, loadedBar;
  oldBar.version = newBar.version = 1;
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
  assert(blob.size() < sizeof(ScheduleProfile));
  blob.back() ^= 1;
  assert(!store.load(0, loaded));
  std::cout << "PASS compressed storage/checksum/bounds, atomic profile/PB commit, interrupted writes, reboot, alternate banks, abort\n";
}
