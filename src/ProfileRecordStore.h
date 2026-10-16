#pragma once
#include <Preferences.h>

#if defined(ESP32) || defined(WLED_TEST_PROFILE_FS)
#include <LittleFS.h>
#include <cstdio>
#include <cstring>
#include <initializer_list>

// ESP32's default NVS partition is too small for live settings plus eight
// profiles with two banks of bars. Use the existing application filesystem;
// retain read-only access to earlier NVS records until a new bank is committed.
class ProfileRecordStore {
public:
  bool begin(const char *name, bool readOnly = false) {
    if (!name || strlen(name) >= sizeof(ns_)) return false;
    strcpy(ns_, name);
    if (!mounted_) mounted_ = LittleFS.begin(true);
    if (readOnly) return true; // Legacy NVS remains readable if mounting fails.
    if (!mounted_) return false;
    if (!LittleFS.exists("/profiles") && !LittleFS.mkdir("/profiles")) return false;
    char dir[40]; snprintf(dir, sizeof(dir), "/profiles/%s", ns_);
    return LittleFS.exists(dir) || LittleFS.mkdir(dir);
  }
  void end() {}
  bool isKey(const char *key) {
    char path[64]; pathFor(path, key);
    if (mounted_ && LittleFS.exists(path)) return true;
    Preferences legacy;
    return legacy.begin(ns_, true) && legacy.isKey(key);
  }
  size_t getBytesLength(const char *key) {
    char path[64]; pathFor(path, key);
    if (mounted_ && LittleFS.exists(path)) {
      File file = LittleFS.open(path, "r");
      return file ? file.size() : 0;
    }
    Preferences legacy;
    return legacy.begin(ns_, true) ? legacy.getBytesLength(key) : 0;
  }
  size_t getBytes(const char *key, void *out, size_t capacity) {
    char path[64]; pathFor(path, key);
    if (mounted_ && LittleFS.exists(path)) {
      File file = LittleFS.open(path, "r");
      if (!file || file.size() > capacity) return 0;
      return file.read(static_cast<uint8_t *>(out), file.size());
    }
    Preferences legacy;
    return legacy.begin(ns_, true) ? legacy.getBytes(key, out, capacity) : 0;
  }
  uint8_t getUChar(const char *key, uint8_t fallback) {
    char path[64]; pathFor(path, key);
    if (mounted_ && LittleFS.exists(path)) {
      uint8_t value;
      return getBytesLength(key) == 1 && getBytes(key, &value, 1) == 1 ? value : 255;
    }
    Preferences legacy;
    return legacy.begin(ns_, true) ? legacy.getUChar(key, fallback) : fallback;
  }
  size_t putBytes(const char *key, const void *value, size_t size) {
    char path[64], temp[72]; pathFor(path, key);
    snprintf(temp, sizeof(temp), "%s.tmp", path);
    File file = LittleFS.open(temp, "w");
    if (!file) return 0;
    const size_t written = file.write(static_cast<const uint8_t *>(value), size);
    file.flush(); file.close();
    if (written != size || !LittleFS.rename(temp, path)) {
      LittleFS.remove(temp);
      return 0;
    }
    return size;
  }
  size_t putUChar(const char *key, uint8_t value) { return putBytes(key, &value, 1); }
  bool remove(const char *key) {
    char path[64]; pathFor(path, key);
    if (mounted_ && LittleFS.exists(path) && !LittleFS.remove(path)) return false;
    // Remove a legacy inactive slot too, so an absent bar cannot fall back to it.
    Preferences legacy;
    return !legacy.begin(ns_, true) || !legacy.isKey(key) || removeLegacy(key);
  }
  static void clearNamespace(const char *ns) {
    ProfileRecordStore store;
    if (store.begin(ns)) {
      for (const char *key : {"data", "data1", "data2", "bank"}) {
        char path[64], temp[72]; store.pathFor(path, key);
        snprintf(temp, sizeof(temp), "%s.tmp", path);
        LittleFS.remove(path); LittleFS.remove(temp);
      }
    }
  }
private:
  char ns_[16] = {};
  static bool mounted_;
  void pathFor(char (&path)[64], const char *key) {
    snprintf(path, sizeof(path), "/profiles/%s/%s", ns_, key);
  }
  bool removeLegacy(const char *key) {
    Preferences legacy;
    return legacy.begin(ns_, false) && legacy.remove(key);
  }
};
#else
using ProfileRecordStore = Preferences;
#endif
