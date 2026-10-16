#pragma once

#include "Bitmaps.h"
#include "DisplayRotation.h"
#include "Globals.h"
#include "TextEncoding.h"
#include <Arduino.h>

// ─── Display Helper Functions
// ──────────────────────────────────────────────────

textPosition_t stringToTextPositionT(const String &val);
void applyZoneFont(int zone, const String &font);
textEffect_t stringToTextEffectT(const String &val);
void initDisplay();
void serviceDisplayOrientation();

void zoneNewMessage(int zone, const String &newMessage, const String &postfix,
                    bool restoreEffects = true, bool force = false);
void zoneShowModeMessage(int zone, const String &modeName);

void updateSpriteData(uint8_t z);
String flashClockDots(const String &t, const String &font);
String getCurTime(const String &curZoneFont, const String &displayFormat);
void ntpUpdateTime();
bool isNumeric(const String &str);

void displayAnimation();
void updateWOPREffect(uint8_t zone);
void updateProgressBars();
void testZones(uint8_t n);

String haApiGet(const String &sensorId, const String &sensorPostfix);
String haApiGetState(const String &sensorId);
void wifiApWelcomeMessage(AsyncWiFiManager *wifiManager);
