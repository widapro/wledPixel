#include "owm.h"
#include "sharedScratch.h"
#include "WeatherData.h"

String owmWeatherIcon;
byte owmHumidity;
int owmPressure;
int owmWindSpeed;
int owmTemperature;
static uint32_t successfulUpdates = 0;
static uint32_t lastSuccessMillis = 0;

uint32_t owmSuccessfulUpdates() { return successfulUpdates; }
uint32_t owmLastSuccessMillis() { return lastSuccessMillis; }

NetworkRequestResult owmWeatherUpdate(const String &city,
                                     const String &unitsFormat,
                                     const String &token) {
  Serial.print(F("\nGetting weather from Open Weather Map"));
  StaticJsonDocument<2048> &doc = sharedJsonDocument();
  doc.clear();
  StaticJsonDocument<256> filter;
  makeWeatherFilter(filter);
  NetworkRequestResult result = requestJson("api.openweathermap.org", 443,
                                   "/data/2.5/weather?q=" + city + "&units=" +
                                       unitsFormat + "&appid=" + token,
                                   "", true, doc, &filter);

  // Only update cached values if we got a valid response
  WeatherData reading;
  if (result == NetworkRequestResult::Success &&
      decodeWeatherData(doc.as<JsonVariantConst>(), reading)) {
    owmHumidity = reading.humidity;
    owmPressure = reading.pressure;
    owmWindSpeed = reading.windSpeed;
    owmTemperature = reading.temperature;
    owmWeatherIcon = reading.icon;
    ++successfulUpdates;
    lastSuccessMillis = millis();
    Serial.print(F(" - OK"));
    return NetworkRequestResult::Success;
  } else if (result == NetworkRequestResult::Deferred) {
    Serial.print(F(" - DEFERRED, keeping cached values"));
    return result;
  } else {
    Serial.print(F(" - FAILED, keeping cached values"));
    return NetworkRequestResult::Failed;
  }
}

String openWetherMapGetWeather(const String &whatToDisplay,
                               const String &unitsFormat) {
  if (whatToDisplay == "owmHumidity")
    return String(owmHumidity) + "%";
  if (whatToDisplay == "owmPressure")
    return String(owmPressure);
  if (whatToDisplay == "owmWindSpeed") {
    if (unitsFormat == "imperial")
      return String(owmWindSpeed) + "mph";
    else
      return String(owmWindSpeed) + "m/s";
  }
  if (whatToDisplay == "owmTemperature") {
    if (unitsFormat == "metric")
      return String(owmTemperature) + "°C";
    if (unitsFormat == "imperial")
      return String(owmTemperature) + "°F";
  };
  if (whatToDisplay == "owmWeatherIcon") {
    if (owmWeatherIcon == "01d")
      return "S"; // 83   - 'S'   Sun day
    if (owmWeatherIcon == "01n")
      return "M"; // 77   - 'M'   Moon night
    if (owmWeatherIcon == "02d")
      return "s"; // 115  - 's'   few clouds sun
    if (owmWeatherIcon == "02n")
      return "m"; // 109  - 'm'   few clouds night
    if (owmWeatherIcon == "03d" || owmWeatherIcon == "03n")
      return "c"; // 99   - 'c'   scattered clouds
    if (owmWeatherIcon == "04d" || owmWeatherIcon == "04n")
      return "C"; // 67   - 'C'   broken clouds
    if (owmWeatherIcon == "09d" || owmWeatherIcon == "09n")
      return "R"; // 82   - 'R'   shower rain
    if (owmWeatherIcon == "10d" || owmWeatherIcon == "10n")
      return "r"; // 114  - 'r'   rain
    if (owmWeatherIcon == "11d" || owmWeatherIcon == "11n")
      return "T"; // 84   - 'T'   thunderstorm
    if (owmWeatherIcon == "13d" || owmWeatherIcon == "13n")
      return "F"; // 70   - 'F'   snow
    if (owmWeatherIcon == "50d" || owmWeatherIcon == "50n")
      return "f"; // 102  - 'f'   mist (fog)
  }
  Serial.print(F("Error getting weather from open weather map"));
  return "err";
}
