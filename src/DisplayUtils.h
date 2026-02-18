#pragma once

#include "Bitmaps.h"
#include "Globals.h"
#include <Arduino.h>

// ─── Display Helper Functions
// ──────────────────────────────────────────────────

textPosition_t stringToTextPositionT(String val);
void applyZoneFont(int zone, String font);
void utf8ToFontEncoding(const char *src, char *dst, size_t dstSize,
                        const String &fontName);
textEffect_t stringToTextEffectT(String val);
void initDisplay();

void zoneNewMessage(int zone, String newMessage, String postfix,
                    bool restoreEffects = true, bool force = false);
void zoneShowModeMessage(int zone, String modeName);

void updateSpriteData(uint8_t z);
String flashClockDots(String t, String font);
String getCurTime(String curZoneFont, String displayFormat);
void ntpUpdateTime();
bool isNumeric(String str);

void displayAnimation();
void updateWOPREffect(uint8_t zone);
void testZones(uint8_t n);

String haApiGet(String sensorId, String sensorPostfix);
void wifiApWelcomeMessage(AsyncWiFiManager *wifiManager);
