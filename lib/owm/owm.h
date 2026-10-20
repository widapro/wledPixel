#ifndef WLEDPIXEL_OWM_H
#define WLEDPIXEL_OWM_H

#include "httpsRequestJson.h"
#include <Arduino.h>
#include <ArduinoJson.h>

NetworkRequestResult owmWeatherUpdate(const String &city,
                                     const String &unitsFormat,
                                     const String &token);
uint32_t owmSuccessfulUpdates();
uint32_t owmLastSuccessMillis();
String openWetherMapGetWeather(const String &whatToDisplay,
                               const String &unitsFormat);

#endif
