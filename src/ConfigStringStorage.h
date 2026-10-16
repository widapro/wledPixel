#pragma once

// Use only for settings whose missing-key default is an empty string. Removing
// an unset key both clears an old value and avoids occupying two NVS entries
// with an empty string. Erasing also works when NVS cannot append a new value.
template <typename Storage, typename Text>
bool storeDefaultConfigString(Storage &storage, const char *key,
                              const Text &value, const char *defaultValue) {
  if (value == defaultValue)
    return !storage.isKey(key) || storage.remove(key);
  return storage.putString(key, value.c_str()) == value.length();
}
template <typename Storage, typename Text>
bool storeOptionalConfigString(Storage &storage, const char *key,
                               const Text &value) {
  return storeDefaultConfigString(storage, key, value, "");
}
