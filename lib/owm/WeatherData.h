#pragma once

#include <ArduinoJson.h>
#include <cmath>
#include <cstring>

struct WeatherData {
  unsigned char humidity;
  int pressure;
  int windSpeed;
  int temperature;
  char icon[4];
};

inline void makeWeatherFilter(JsonDocument &filter) {
  filter.clear();
  filter["main"]["temp"] = true;
  filter["main"]["humidity"] = true;
  filter["main"]["pressure"] = true;
  filter["wind"]["speed"] = true;
  filter["weather"][0]["icon"] = true;
}

// Validate a whole reading before committing it, so incomplete responses do
// not replace good cached data with zeroes or a mismatched weather icon.
inline bool decodeWeatherData(JsonVariantConst json, WeatherData &out) {
  const auto main = json["main"];
  const auto wind = json["wind"]["speed"];
  const char *icon = json["weather"][0]["icon"];
  if (!main["temp"].is<float>() || !main["humidity"].is<int>() ||
      !main["pressure"].is<int>() || !wind.is<float>() || !icon ||
      std::strlen(icon) != 3 || icon[0] < '0' || icon[0] > '9' ||
      icon[1] < '0' || icon[1] > '9' || (icon[2] != 'd' && icon[2] != 'n'))
    return false;
  const float temperature = main["temp"].as<float>();
  const float speed = wind.as<float>();
  const int humidity = main["humidity"].as<int>();
  const int pressure = main["pressure"].as<int>();
  if (!std::isfinite(temperature) || temperature < -500 || temperature > 1000 ||
      !std::isfinite(speed) || speed < 0 || speed > 1000 ||
      humidity < 0 || humidity > 100 || pressure <= 0 || pressure > 2000)
    return false;
  WeatherData next{static_cast<unsigned char>(humidity), pressure,
                   static_cast<int>(speed), static_cast<int>(temperature), {0}};
  std::memcpy(next.icon, icon, sizeof(next.icon));
  out = next;
  return true;
}
