#pragma once

#include <cstddef>

class JsonVariant {
public:
  template <typename T> T as() const { return T{}; }
};

class JsonObject {
public:
  bool containsKey(const char *) const { return false; }
  JsonVariant operator[](const char *) const { return JsonVariant(); }
};

class JsonDocument {
public:
  virtual ~JsonDocument() = default;
  void clear() {}
  template <typename T> T as() { return T{}; }
};

template <size_t Capacity> class StaticJsonDocument : public JsonDocument {};
