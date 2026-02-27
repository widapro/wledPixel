#ifndef WLEDPIXEL_OWM_H
#define WLEDPIXEL_OWM_H

#include "httpsRequestJson.h"
#include <Arduino.h>
#include <ArduinoJson.h>

void owmWeatherUpdate(const String &city, const String &unitsFormat,
                      const String &token);
String openWetherMapGetWeather(String whatToDisplay, String unitsFormat);

#endif