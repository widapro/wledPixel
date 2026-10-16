#pragma once
#include "Scheduler.h"
#include "StorageCodec.h"
#include "ProfileRecordStore.h"
#include <cstdio>

// Two slots per profile. The bank byte is the only commit point. Legacy records
// stay readable as bank 0 until the first successful commit to bank 1.
class ProfileStorage {
public:
  uint8_t bank(uint8_t idx) {
    char ns[12]; profileNamespace(ns, idx);
    ProfileRecordStore prefs;
    if (!prefs.begin(ns, true)) return 0;
    uint8_t value = prefs.getUChar("bank", 0);
    prefs.end();
    return value;
  }
  void abandon(uint8_t idx) { if (idx < MAX_PROFILES) dirty_[idx] = 0; }
  bool load(uint8_t idx, ScheduleProfile &out) {
    uint8_t active = bank(idx);
    if (active > 2) return false;
    char ns[12]; profileNamespace(ns, idx);
    ProfileRecordStore prefs;
    if (!prefs.begin(ns, true)) return false;
    const char *key = slotKey(active);
    bool legacyV2 = false;
    bool ok = readRecord(prefs, key, out, sizeof(ScheduleProfileV2), &legacyV2);
    prefs.end();
    if (ok && legacyV2) schedulerMigrateProfileV2InPlace(out);
    if (ok) sanitizeScheduleProfile(out);
    return ok;
  }
  // 0 = absent, 1 = present, -1 = unreadable/corrupt.
  int loadBar(uint8_t idx, uint8_t zone, ScheduleProgressBar &out) {
    return readBar(idx, zone, bank(idx), out);
  }
  bool stageBar(uint8_t idx, uint8_t zone, const ScheduleProgressBar *in) {
    if (idx >= MAX_PROFILES || zone >= 4 || bank(idx) > 2) return false;
    if (!writeBar(idx, zone, inactive(bank(idx)), in)) return false;
    dirty_[idx] |= 1U << zone;
    return true;
  }
  bool commit(uint8_t idx, const ScheduleProfile &profile) {
    if (idx >= MAX_PROFILES) return false;
    const uint8_t active = bank(idx), next = inactive(active);
    if (active > 2) return false;
    // Copy unchanged records one at a time: no complete bundle in RAM.
    for (uint8_t z = 0; z < 4; ++z) {
      if (dirty_[idx] & (1U << z)) continue;
      ScheduleProgressBar bar;
      int found = readBar(idx, z, active, bar);
      if (found < 0 || !writeBar(idx, z, next, found ? &bar : nullptr)) return false;
    }
    char ns[12]; profileNamespace(ns, idx);
    ProfileRecordStore prefs;
    if (!prefs.begin(ns, false)) return false;
    bool ok = writeRecord(prefs, slotKey(next), profile);
    if (ok) ok = prefs.putUChar("bank", next) == 1;
    prefs.end();
    if (ok) dirty_[idx] = 0;
    return ok;
  }
private:
  template <typename Record>
  static bool readRecord(ProfileRecordStore &prefs, const char *key, Record &out,
                         size_t legacySize = 0, bool *legacy = nullptr) {
    const size_t len = prefs.getBytesLength(key);
    if (!len || len > sizeof(out)) return false;
#if defined(ESP8266)
    if (len != sizeof(out) && len != legacySize) return false;
    if (legacy) *legacy = len == legacySize;
    return prefs.getBytes(key, &out, sizeof(out)) == len;
#else
    uint8_t bytes[sizeof(out)];
    if (prefs.getBytes(key, bytes, sizeof(bytes)) != len) return false;
    if (storageCodec::encoded(bytes, len))
      return storageCodec::decode(bytes, len, reinterpret_cast<uint8_t *>(&out), sizeof(out));
    if (len != sizeof(out) && len != legacySize) return false;
    if (legacy) *legacy = len == legacySize;
    memcpy(&out, bytes, len);
    return true;
#endif
  }
  template <typename Record>
  static bool writeRecord(ProfileRecordStore &prefs, const char *key, const Record &in) {
#if !defined(ESP8266)
    uint8_t bytes[sizeof(in) + (sizeof(in) + 127) / 128 + storageCodec::headerSize];
    const size_t len = storageCodec::encode(reinterpret_cast<const uint8_t *>(&in), sizeof(in), bytes, sizeof(bytes));
    if (len && len < sizeof(in)) return prefs.putBytes(key, bytes, len) == len;
#endif
    return prefs.putBytes(key, &in, sizeof(in)) == sizeof(in);
  }
  uint8_t dirty_[MAX_PROFILES] = {};
  static uint8_t inactive(uint8_t active) { return active == 1 ? 2 : 1; }
  static const char *slotKey(uint8_t slot) { return slot == 0 ? "data" : slot == 1 ? "data1" : "data2"; }
  static void profileNamespace(char *ns, uint8_t idx) { snprintf(ns, 12, "cronP%u", idx); }
  static int readBar(uint8_t idx, uint8_t zone, uint8_t slot, ScheduleProgressBar &out) {
    if (idx >= MAX_PROFILES || zone >= 4 || slot > 2) return -1;
    char ns[12]; snprintf(ns, sizeof(ns), "cronB%u%u", idx, zone);
    ProfileRecordStore prefs;
    if (!prefs.begin(ns, true)) return 0;
    const char *key = slotKey(slot);
    if (!prefs.isKey(key)) { prefs.end(); return 0; }
    bool ok = readRecord(prefs, key, out) && out.version == 1;
    prefs.end();
    return ok ? 1 : -1;
  }
  static bool writeBar(uint8_t idx, uint8_t zone, uint8_t slot, const ScheduleProgressBar *in) {
    char ns[12]; snprintf(ns, sizeof(ns), "cronB%u%u", idx, zone);
    ProfileRecordStore prefs;
    if (!prefs.begin(ns, false)) return false;
    const char *key = slotKey(slot);
    bool ok = in ? writeRecord(prefs, key, *in)
                 : (!prefs.isKey(key) || prefs.remove(key));
    prefs.end();
    return ok;
  }
};
