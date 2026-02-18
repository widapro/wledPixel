#include "owm.h"

String owmWeatherIcon;
byte owmHumidity;
int owmPressure;
int owmWindSpeed;
int owmTemperature;

void owmWeatherUpdate(String city, String unitsFormat, String token) {
  Serial.print(F("\nGetting weather from Open Weather Map"));
  JsonObject owmWeatherPostObj =
      httpsRequest("api.openweathermap.org", 443,
                   "/data/2.5/weather?q=" + city + "&units=" + unitsFormat +
                       "&appid=" + token,
                   "", true);

  // Only update cached values if we got a valid response
  if (owmWeatherPostObj.containsKey(F("main"))) {
    owmHumidity = owmWeatherPostObj[F("main")][F("humidity")].as<byte>();
    owmPressure = owmWeatherPostObj[F("main")][F("pressure")].as<int>();
    owmWindSpeed = owmWeatherPostObj[F("wind")][F("speed")].as<int>();
    owmTemperature = (int)(owmWeatherPostObj[F("main")][F("temp")].as<int>());
    owmWeatherIcon = owmWeatherPostObj[F("weather")][0][F("icon")].as<String>();
    Serial.print(F(" - OK"));
  } else {
    Serial.print(F(" - FAILED, keeping cached values"));
  }
}

String openWetherMapGetWeather(String whatToDisplay, String unitsFormat) {
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