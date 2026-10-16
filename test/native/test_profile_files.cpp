#define WLED_TEST_PROFILE_FS
#include "ProfileStorage.h"
#include <cassert>
#include <iostream>
bool ProfileRecordStore::mounted_ = false;
int main() {
  ScheduleProfile oldProfile, newProfile, loaded;
  initProfileDefaults(oldProfile, 0);
  newProfile = oldProfile; newProfile.intensity = 12;
  ScheduleProgressBar oldBar = {}, newBar = {}, loadedBar;
  oldBar.version = newBar.version = 1;
  oldBar.maxValue = 100; newBar.maxValue = 200;
  Preferences prefs;
  prefs.begin("cronP0"); prefs.putBytes("data", &oldProfile, sizeof(oldProfile));
  prefs.begin("cronB00"); prefs.putBytes("data", &oldBar, sizeof(oldBar));
  const auto legacy = Preferences::disk;
  // NVS has no room left. The filesystem must allow new banks independently.
  Preferences::writesLeft = 0;
  bool succeeded = false;
  for (int budget = 0; budget < 30; ++budget) {
    Preferences::disk = legacy;
    fakeFs::disk.clear(); fakeFs::dirs.clear(); fakeFs::writesLeft = budget;
    ProfileStorage store;
    const bool ok = store.stageBar(0, 0, &newBar) && store.commit(0, newProfile);
    fakeFs::writesLeft = -1;
    ProfileStorage rebooted;
    assert(rebooted.load(0, loaded));
    assert(rebooted.loadBar(0, 0, loadedBar) == 1);
    assert(loaded.intensity == (ok ? 12 : 0));
    assert(loadedBar.maxValue == (ok ? 200 : 100));
    succeeded |= ok;
  }
  assert(succeeded);
  const auto firstBank = fakeFs::disk;
  const auto dirs = fakeFs::dirs;
  newProfile.intensity = 13; newBar.maxValue = 300;
  for (int budget = 0; budget < 30; ++budget) {
    fakeFs::disk = firstBank; fakeFs::dirs = dirs; fakeFs::writesLeft = budget;
    ProfileStorage store;
    const bool ok = store.stageBar(0, 0, &newBar) && store.commit(0, newProfile);
    fakeFs::writesLeft = -1;
    ProfileStorage rebooted;
    assert(rebooted.load(0, loaded) && loaded.intensity == (ok ? 13 : 12));
    assert(rebooted.loadBar(0, 0, loadedBar) == 1 && loadedBar.maxValue == (ok ? 300 : 200));
  }
  ProfileStorage store;
  assert(store.stageBar(0, 0, nullptr));
  assert(store.commit(0, oldProfile));
  assert(store.loadBar(0, 0, loadedBar) == 0);
  assert(store.stageBar(0, 0, &newBar)); store.abandon(0);
  assert(store.commit(0, newProfile) && store.loadBar(0, 0, loadedBar) == 0);
  const auto active = store.bank(0);
  // Incomplete temporary pointer files are ignored; an invalid committed
  // pointer must fail closed rather than silently choosing legacy bank 0.
  fakeFs::disk["/profiles/cronP0/bank.tmp"] = {};
  assert(store.bank(0) == active);
  fakeFs::disk["/profiles/cronP0/bank"] = {};
  assert(!store.load(0, loaded));
  Preferences::writesLeft = -1;
  ProfileRecordStore::clearNamespace("cronP0");
  ProfileRecordStore::clearNamespace("cronB00");
  for (const auto &entry : fakeFs::disk)
    assert(entry.first.find("/profiles/cronP0/") != 0 && entry.first.find("/profiles/cronB00/") != 0);
  std::cout << "PASS ESP32 filesystem banks, full-NVS migration, interrupted writes/renames, abort, corrupt pointer and factory cleanup\n";
}
