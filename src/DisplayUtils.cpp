#include "DisplayUtils.h"
#include "PixelFont3D.h"
#include "Scheduler.h"
#include "httpsRequestJson.h"
#include "sharedScratch.h"
#include <SPI.h>
#include <cstdio>
#include <cstring>

#if defined(ESP32)
SemaphoreHandle_t displayMutex;
#endif

namespace displayRotation {
bool flipped = false;
uint8_t moduleCount = 1;
} // namespace displayRotation

void initDisplay() {
  displayRotation::flipped = displayFlip;
  uint8_t count = 1;
  for (uint8_t z = 0; z < zoneNumbers && z < 4; ++z) {
    if (zones[z].end < MAX_DEVICES && zones[z].end + 1 > count)
      count = zones[z].end + 1;
  }
  // The physical chain stays the same when a profile uses fewer modules.
  // Include explicit layouts in saved profiles, without adding profile fields.
  for (uint8_t i = 0; i < MAX_PROFILES; ++i) {
    ScheduleProfile &profile = schedulerSharedProfileBuffer();
    if (!schedulerLoadProfile(i, profile))
      continue;
    for (uint8_t z = 0; z < 4; ++z) {
      if (profile.zoneEnd[z] < MAX_DEVICES && profile.zoneEnd[z] + 1 > count)
        count = profile.zoneEnd[z] + 1;
    }
  }
  displayRotation::moduleCount = count;
}

void serviceDisplayOrientation() {
  if (displayRotation::flipped == displayFlip)
    return;
  MD_MAX72XX *mx = P.getGraphicObject();
  if (!mx)
    return;
  displayRotation::flipped = displayFlip;
  // Resend the current frame immediately, including static text, without
  // resetting animations or clearing the screen. Keep logical pixels intact.
  mx->update(MD_MAX72XX::OFF);
  for (uint16_t col = 0; col < mx->getColumnCount(); ++col)
    mx->setColumn(col, mx->getColumn(col));
  mx->update(MD_MAX72XX::ON);
  for (uint8_t z = 0; z < zoneNumbers && z < 4; ++z)
    P.setIntensity(z, P.getIntensity(z));
}

// ─── String → textPosition_t
// ───────────────────────────────────────────────────
textPosition_t stringToTextPositionT(const String &val) {
  if (val == F("PA_CENTER"))
    return PA_CENTER;
  if (val == F("PA_LEFT"))
    return PA_LEFT;
  if (val == F("PA_RIGHT"))
    return PA_RIGHT;
  return PA_LEFT;
}

// ─── Apply Font
// ────────────────────────────────────────────────────────────────
void applyZoneFont(int zone, const String &font) {
  if (font == F("default"))
    P.setFont(zone, nullptr);
  if (font == F("wledFont"))
    P.setFont(zone, wledFont);
  if (font == F("wledFont_cyrillic"))
    P.setFont(zone, wledFont_cyrillic);
  if (font == F("wledSymbolFont"))
    P.setFont(zone, wledSymbolFont);
  if (font == F("wledFont_CompactSymbols"))
    P.setFont(zone, wledFont_CompactSymbols);
  if (font == F("pixelfont_3d_5x5_thin"))
    P.setFont(zone, pixelFont3D);
}

// ─── String → textEffect_t
// ─────────────────────────────────────────────────────
textEffect_t stringToTextEffectT(const String &val) {
  if (val == F("PA_NO_EFFECT"))
    return PA_NO_EFFECT;
  if (val == F("PA_PRINT"))
    return PA_PRINT;
  if (val == F("PA_SCROLL_UP"))
    return PA_SCROLL_UP;
  if (val == F("PA_SCROLL_DOWN"))
    return PA_SCROLL_DOWN;
  if (val == F("PA_SCROLL_LEFT"))
    return PA_SCROLL_LEFT;
  if (val == F("PA_SCROLL_RIGHT"))
    return PA_SCROLL_RIGHT;
  if (val == F("PACMAN"))
    return PA_SPRITE;
  if (val == F("WAVE"))
    return PA_SPRITE;
  if (val == F("ROLL"))
    return PA_SPRITE;
  if (val == F("LINES"))
    return PA_SPRITE;
  if (val == F("ARROW"))
    return PA_SPRITE;
  if (val == F("SAILBOAT"))
    return PA_SPRITE;
  if (val == F("STEAMBOAT"))
    return PA_SPRITE;
  if (val == F("HEART"))
    return PA_SPRITE;
  if (val == F("INVADER"))
    return PA_SPRITE;
  if (val == F("ROCKET"))
    return PA_SPRITE;
  if (val == F("FBALL"))
    return PA_SPRITE;
  if (val == F("CHEVRON"))
    return PA_SPRITE;
  if (val == F("WALKER"))
    return PA_SPRITE;
  if (val == F("MARIO"))
    return PA_SPRITE;
  if (val == F("GHOST"))
    return PA_SPRITE;
  if (val == F("DINO"))
    return PA_SPRITE;
  if (val == F("PA_SPRITE"))
    return PA_SPRITE;
  if (val == F("PA_SLICE"))
    return PA_SLICE;
  if (val == F("PA_MESH"))
    return PA_MESH;
  if (val == F("PA_FADE"))
    return PA_FADE;
  if (val == F("PA_DISSOLVE"))
    return PA_DISSOLVE;
  if (val == F("PA_BLINDS"))
    return PA_BLINDS;
  if (val == F("PA_RANDOM"))
    return PA_RANDOM;
  if (val == F("PA_WIPE"))
    return PA_WIPE;
  if (val == F("PA_WIPE_CURSOR"))
    return PA_WIPE_CURSOR;
  if (val == F("PA_SCAN_HORIZ"))
    return PA_SCAN_HORIZ;
  if (val == F("PA_SCAN_HORIZX"))
    return PA_SCAN_HORIZX;
  if (val == F("PA_SCAN_VERT"))
    return PA_SCAN_VERT;
  if (val == F("PA_SCAN_VERTX"))
    return PA_SCAN_VERTX;
  if (val == F("PA_OPENING"))
    return PA_OPENING;
  if (val == F("PA_OPENING_CURSOR"))
    return PA_OPENING_CURSOR;
  if (val == F("PA_CLOSING"))
    return PA_CLOSING;
  if (val == F("PA_CLOSING_CURSOR"))
    return PA_CLOSING_CURSOR;
  if (val == F("PA_SCROLL_UP_LEFT"))
    return PA_SCROLL_UP_LEFT;
  if (val == F("PA_SCROLL_UP_RIGHT"))
    return PA_SCROLL_UP_RIGHT;
  if (val == F("PA_SCROLL_DOWN_LEFT"))
    return PA_SCROLL_DOWN_LEFT;
  if (val == F("PA_SCROLL_DOWN_RIGHT"))
    return PA_SCROLL_DOWN_RIGHT;
  if (val == F("PA_GROW_UP"))
    return PA_GROW_UP;
  if (val == F("PA_GROW_DOWN"))
    return PA_GROW_DOWN;
  return PA_SCROLL_LEFT;
}

// ─── Zone New Message
// ──────────────────────────────────────────────────────────
void zoneNewMessage(int zone, const String &newMessage, const String &postfix,
                    bool restoreEffects, bool force) {
  if (zone >= 0 && zone < 4) {
    String fullMessage = newMessage + postfix;

    // Encode first to compare with what is already on screen (which is encoded)
    char tempEncoded[100];
    utf8ToFontEncoding(fullMessage.c_str(), tempEncoded, sizeof(tempEncoded),
                       zones[zone].font);

    // Check if message is identical to pending/current to avoid redundant work
    if (!force) {
      if (zones[zone].newMessageAvailable) {
        if (strcmp(zoneMessagesPending[zone], tempEncoded) == 0)
          return;
      } else {
        if (strcmp(zoneMessages[zone], tempEncoded) == 0)
          return;
      }
    }

    // Just copy the data. No Parola calls here to avoid race conditions.
    // Width calculations and textFits will be done in displayAnimation (main
    // loop).
    strcpy(zoneMessagesPending[zone], tempEncoded);
    zones[zone].newMessageAvailable = true;
    zones[zone].restoreEffects = restoreEffects;
  }
}

// ─── Zone Show Mode Message
// ────────────────────────────────────────────────────
void zoneShowModeMessage(int zone, const String &modeName) {
  if (zone >= 0 && zone < 4) {
    Serial.print(F("Debug: zoneShowModeMessage called for zone "));
    Serial.print(zone);
    Serial.print(F(" with: "));
    Serial.println(modeName);

    if (disableServiceMessages)
      return;

    if (strcmp(zoneMessages[zone], modeName.c_str()) == 0) {
      Serial.println(F("Debug: Message already displayed, skipping update."));
      return;
    }

    P.displayClear(zone);

    int16_t textWidth = P.getTextColumns(zone, modeName.c_str());
    if (textWidth == 0 && modeName.length() > 0)
      textWidth = modeName.length() * 6;

    uint16_t startCol = 0, endCol = 0;
    P.getDisplayExtent(zone, startCol, endCol);
    int16_t zoneWidth = abs(startCol - endCol) + 1;

    if (textWidth <= zoneWidth) {
      P.setTextEffect(zone, PA_PRINT, PA_NO_EFFECT);
      P.setPause(zone, 500);
      P.setTextAlignment(zone, PA_CENTER);
      zoneServiceMessageScrolling[zone] = false;
    } else {
      P.setTextEffect(zone, PA_SCROLL_LEFT, PA_SCROLL_LEFT);
      P.setPause(zone, 0);
      P.setTextAlignment(zone, PA_LEFT);
      zoneServiceMessageScrolling[zone] = true;
    }

    utf8ToFontEncoding(modeName.c_str(), zoneMessages[zone],
                       sizeof(zoneMessages[zone]), zones[zone].font);
    strcpy(zoneMessagesPending[zone], "");
    zones[zone].newMessageAvailable = false;
    zones[zone].restoreEffects = true;

    P.setTextBuffer(zone, zoneMessages[zone]);
    P.displayReset(zone);
  }
}

// ─── Numeric Check
// ─────────────────────────────────────────────────────────────
bool isNumeric(const String &str) {
  if (str.length() == 0)
    return false;
  for (byte i = 0; i < str.length(); i++) {
    if (!isDigit(str.charAt(i)))
      return false;
  }
  return true;
}

// ─── NTP Update
// ────────────────────────────────────────────────────────────────
void ntpUpdateTime() {
  if (timeClient.update()) {
    previousNTPsyncMillis = currentMillis;
    ntpSyncedOnce = true;
    Serial.print(F("\nNTP time synchronized successfully"));
  } else {
    previousNTPsyncMillis =
        currentMillis - (ntpUpdateInterval * 3600 * 1000) + 30000;
    Serial.print(
        F("\nERROR: NTP time synchronization failed, will retry in 30s"));
  }
}

// ─── Update Sprite Data
// ────────────────────────────────────────────────────────
void updateSpriteData(uint8_t z) {
  if (zones[z].scrollEffectIn == "PACMAN" ||
      zones[z].scrollEffectOut == "PACMAN") {
    P.setSpriteData(pacman1, W_PMAN1, F_PMAN1, pacman2, W_PMAN2, F_PMAN2);
  } else if (zones[z].scrollEffectIn == "WAVE" ||
             zones[z].scrollEffectOut == "WAVE") {
    P.setSpriteData(wave, W_WAVE, F_WAVE, wave, W_WAVE, F_WAVE);
  } else if (zones[z].scrollEffectIn == "ROLL" ||
             zones[z].scrollEffectOut == "ROLL") {
    P.setSpriteData(roll1, W_ROLL1, F_ROLL1, roll2, W_ROLL2, F_ROLL2);
  } else if (zones[z].scrollEffectIn == "LINES" ||
             zones[z].scrollEffectOut == "LINES") {
    P.setSpriteData(lines, W_LINES, F_LINES, lines, W_LINES, F_LINES);
  } else if (zones[z].scrollEffectIn == "ARROW" ||
             zones[z].scrollEffectOut == "ARROW") {
    P.setSpriteData(arrow1, W_ARROW1, F_ARROW1, arrow2, W_ARROW2, F_ARROW2);
  } else if (zones[z].scrollEffectIn == "SAILBOAT" ||
             zones[z].scrollEffectOut == "SAILBOAT") {
    P.setSpriteData(sailboat, W_SAILBOAT, F_SAILBOAT, sailboat, W_SAILBOAT,
                    F_SAILBOAT);
  } else if (zones[z].scrollEffectIn == "STEAMBOAT" ||
             zones[z].scrollEffectOut == "STEAMBOAT") {
    P.setSpriteData(steamboat, W_STEAMBOAT, F_STEAMBOAT, steamboat, W_STEAMBOAT,
                    F_STEAMBOAT);
  } else if (zones[z].scrollEffectIn == "HEART" ||
             zones[z].scrollEffectOut == "HEART") {
    P.setSpriteData(heart, W_HEART, F_HEART, heart, W_HEART, F_HEART);
  } else if (zones[z].scrollEffectIn == "INVADER" ||
             zones[z].scrollEffectOut == "INVADER") {
    P.setSpriteData(invader, W_INVADER, F_INVADER, invader, W_INVADER,
                    F_INVADER);
  } else if (zones[z].scrollEffectIn == "ROCKET" ||
             zones[z].scrollEffectOut == "ROCKET") {
    P.setSpriteData(rocket, W_ROCKET, F_ROCKET, rocket, W_ROCKET, F_ROCKET);
  } else if (zones[z].scrollEffectIn == "FBALL" ||
             zones[z].scrollEffectOut == "FBALL") {
    P.setSpriteData(fireball, W_FBALL, F_FBALL, fireball, W_FBALL, F_FBALL);
  } else if (zones[z].scrollEffectIn == "CHEVRON" ||
             zones[z].scrollEffectOut == "CHEVRON") {
    P.setSpriteData(chevron, W_CHEVRON, F_CHEVRON, chevron, W_CHEVRON,
                    F_CHEVRON);
  } else if (zones[z].scrollEffectIn == "WALKER" ||
             zones[z].scrollEffectOut == "WALKER") {
    P.setSpriteData(walker, W_WALKER, F_WALKER, walker, W_WALKER, F_WALKER);
  } else if (zones[z].scrollEffectIn == "MARIO" ||
             zones[z].scrollEffectOut == "MARIO") {
    P.setSpriteData(mario, W_MARIO, F_MARIO, mario, W_MARIO, F_MARIO);
  } else if (zones[z].scrollEffectIn == "GHOST" ||
             zones[z].scrollEffectOut == "GHOST") {
    P.setSpriteData(ghost, W_GHOST, F_GHOST, ghost, W_GHOST, F_GHOST);
  } else if (zones[z].scrollEffectIn == "DINO" ||
             zones[z].scrollEffectOut == "DINO") {
    P.setSpriteData(dino, W_DINO, F_DINO, dino, W_DINO, F_DINO);
  }
}

// ─── Flash Clock Dots
// ──────────────────────────────────────────────────────────
String flashClockDots(const String &t, const String &font) {
  String result = t;
  String blinkChar = (font == "default") ? " " : "\xA0";
  if (result.indexOf(":") > 0)
    result.replace(":", blinkChar);
  else if (font != "default")
    result.replace("\xA0", ":");
  return result;
}

// ─── Get Current Time
// ──────────────────────────────────────────────────────────
String getCurTime(const String &curZoneFont, const String &displayFormat) {
  if (currentMillis - previousNTPsyncMillis >=
      (unsigned)ntpUpdateInterval * 3600 * 1000)
    ntpUpdateTime();

  String t = (String)timeClient.getFormattedTime();

  if (displayFormat == "HHMM12" || displayFormat == "HHMM12AP") {
    const uint8_t hour24 = timeClient.getHours();
    const uint8_t hour12 = (hour24 % 12 == 0) ? 12 : hour24 % 12;
    String hour;
    if (hour12 < 10)
      hour += '0';
    hour += hour12;

    t = hour + t.substring(2, 5);
    if (displayFormat == "HHMM12AP")
      t += hour24 < 12 ? F(" am") : F(" pm");
    return t;
  }

  if (displayFormat == "HHMM") {
    t.remove(5, 4);
    return t;
  }
  if (displayFormat == "HH") {
    t.remove(2, 7);
    return t;
  }
  if (displayFormat == "MM") {
    t.remove(5, 4);
    t.remove(0, 3);
    return t;
  }

  time_t epochTime = timeClient.getEpochTime();
  struct tm *ptm = gmtime((time_t *)&epochTime);
  int monthDay = ptm->tm_mday;
  int currentMonth = ptm->tm_mon + 1;

  String currentMonthStr = "";
  if (currentMonth < 10)
    currentMonthStr += '0';
  currentMonthStr += currentMonth;

  String monthDayStr = "";
  if (monthDay < 10)
    monthDayStr += '0';
  monthDayStr += monthDay;

  static const char *const weekDays[7] = {"Sun", "Mon", "Tue", "Wed",
                                          "Thu", "Fri", "Sat"};
  static const char *const weekDaysCyrillic[7] = {
      "Вс", "Пн", "Вт", "Ср", "Чт", "Пт", "Сб"};
  const char *weekDay = nullptr;
  if (curZoneFont == "wledFont_cyrillic") {
    weekDay = weekDaysCyrillic[timeClient.getDay()];
  } else {
    weekDay = weekDays[timeClient.getDay()];
  }
  int currentYear = ptm->tm_year + 1900;

  if (displayFormat == "ddmmyyyy")
    t = monthDayStr + "." + currentMonthStr + "." + String(currentYear);
  if (displayFormat == "ddmm")
    t = monthDayStr + "." + currentMonthStr;
  if (displayFormat == "ddmmaa")
    t = monthDayStr + "." + currentMonthStr + weekDay;
  if (displayFormat == "aa")
    t = weekDay;
  if (displayFormat == "ddmmaahhmm") {
    t.remove(5, 4);
    t = monthDayStr + "." + currentMonthStr + " " + weekDay + " " + t;
  }
  return t;
}

// ─── Display Animation
// ────────────────────────────────────────────────────────
void displayAnimation() {
  // ── Diagnostic counters (reported every 10s) ──
  static unsigned long lastDiagMs = 0;
  static uint16_t frameCount = 0;
  static uint16_t midSwapCount = 0;

  // ── Phase 1: Apply all pending mid-scroll buffer swaps BEFORE rendering ──
  // This is critical: if we swap the buffer AFTER P.displayAnimate(), Parola's
  // cached internal state (scroll offset, column positions) was computed for
  // the OLD text but will be applied to the NEW text → pixel corruption.
  for (uint8_t z = 0; z < zoneNumbers && z < 4; z++) {
    if (woprZones[z].active)
      continue;

    // ─── Mid-Scroll "Ticker" Update ───
    // Only intercept while the zone is actively animating (scrolling).
    // If the zone has finished (PA_PRINT done, or scroll completed),
    // let Phase 3 handle it with displayReset() so the text re-renders.
    if (zones[z].newMessageAvailable && !zones[z].restoreEffects &&
        zones[z].scrollInfinite &&
        (zones[z].workMode == "wallClock" || zones[z].workMode == "countdown" ||
         zones[z].workMode == "stockTicker") &&
        !P.getZoneStatus(z)) {

      int16_t oldWidth = P.getTextColumns(z, zoneMessages[z]);
      int16_t newWidth = P.getTextColumns(z, zoneMessagesPending[z]);

      if (oldWidth == newWidth) {
        strcpy(zoneMessages[z], zoneMessagesPending[z]);
        zones[z].newMessageAvailable = false;
        P.setTextBuffer(z, zoneMessages[z]);
        midSwapCount++;
        // Updated in-place, let scroll continue without reset (Smooth Update)
      }
    }
  }

  // ── Phase 2: Render the current frame ──
  P.displayAnimate();

  // Immediately re-draw progress bar overlay and flush affected devices.
  // Parola clears row 7 during zone animation and flushes before returning.
  // We re-draw PB and flush the affected devices immediately after.
  updateProgressBars();

  frameCount++;

  // ── Periodic diagnostics ──
  if (currentMillis - lastDiagMs >= 10000) {
    frameCount = 0;
    midSwapCount = 0;
    lastDiagMs = currentMillis;
  }

  // ── Phase 3: Handle scroll-finish events and deferred updates ──
  for (uint8_t z = 0; z < zoneNumbers && z < 4; z++) {
    if (woprZones[z].active)
      continue;

    // Check if we need to handle a pending message or a scroll finish
    // We only process 'newMessageAvailable' if:
    // 1. It wasn't handled by the mid-scroll update above (width mismatch).
    // 2. AND (It's not a Clock/Countdown Infinite Scroll OR The scroll has
    // finished). This prevents resetting mid-scroll for variable-width clocks,
    // waiting for the end instead.

    bool isClockInfinite =
        zones[z].scrollInfinite &&
        (zones[z].workMode == "wallClock" || zones[z].workMode == "countdown" ||
         zones[z].workMode == "stockTicker");
    bool scrollFinished = P.getZoneStatus(z);
    bool shouldUpdate = zones[z].newMessageAvailable;

    if (shouldUpdate && isClockInfinite && !scrollFinished) {
      // Defer update. Wait for scroll to finish.
      shouldUpdate = false;
    }

    if (scrollFinished || shouldUpdate) {
      if (zones[z].newMessageAvailable && shouldUpdate) {
        // Commit the new message
        strcpy(zoneMessages[z], zoneMessagesPending[z]);
        zones[z].newMessageAvailable = false;
        zoneServiceMessageScrolling[z] = false;

        // Link the buffer to the zone
        P.setTextBuffer(z, zoneMessages[z]);

        // Perform Layout Calculations
        int16_t textWidth = P.getTextColumns(z, zoneMessages[z]);
        if (textWidth == 0 && strlen(zoneMessages[z]) > 0)
          textWidth = strlen(zoneMessages[z]) * 6;

        uint16_t startCol = 0, endCol = 0;
        P.getDisplayExtent(z, startCol, endCol);
        int16_t zoneWidth = abs(startCol - endCol) + 1;
        zones[z].textFits = (textWidth <= zoneWidth);

        if (zones[z].restoreEffects) {
          textEffect_t effIn = stringToTextEffectT(zones[z].scrollEffectIn);
          textEffect_t effOut = stringToTextEffectT(zones[z].scrollEffectOut);

          if (zones[z].workMode == "wallClock" ||
              zones[z].workMode == "owmWeather" ||
              zones[z].workMode == "countdown" ||
              zones[z].workMode == "stockTicker") {
            effOut = PA_NO_EFFECT;
          }

          if (!zones[z].textFits) {
            if ((zones[z].workMode == "wallClock" ||
                 zones[z].workMode == "countdown" ||
                 zones[z].workMode == "stockTicker") &&
                !zones[z].scrollInfinite) {
              effIn = PA_PRINT;
              effOut = PA_NO_EFFECT;
              P.setTextAlignment(z, PA_LEFT);
              P.setPause(z, 100);
            } else {
              P.setTextAlignment(z, PA_LEFT);
              effIn = PA_SCROLL_LEFT;
              effOut = PA_SCROLL_LEFT;
              P.setPause(z, 0);
              P.displayClear(z);
            }
          } else {
            P.setTextAlignment(z, stringToTextPositionT(zones[z].scrollAlign));
            P.setPause(z, zones[z].scrollPause * 1000);
          }

          updateSpriteData(z);
          P.setTextEffect(z, effIn, effOut);
        }
        P.displayReset(z);
      } else if (scrollFinished) {
        if (zoneServiceMessageScrolling[z]) {
          zoneServiceMessageScrolling[z] = false;
          P.setTextEffect(z, PA_PRINT, PA_NO_EFFECT);
          P.setTextAlignment(z, PA_LEFT);
          P.displayReset(z);
        } else if ((allTestsFinish && zones[z].scrollInfinite &&
                    (zones[z].workMode == "mqttClient" ||
                     zones[z].workMode == "manualInput")) ||
                   (allTestsFinish &&
                    (zones[z].workMode == "wallClock" ||
                     zones[z].workMode == "countdown" ||
                     zones[z].workMode == "stockTicker") &&
                    !zones[z].textFits && zones[z].scrollInfinite)) {

          if ((zones[z].workMode == "wallClock" ||
               zones[z].workMode == "countdown" ||
               zones[z].workMode == "stockTicker") &&
              !zones[z].textFits) {
            if (zoneScrollCompleteTime[z] == 0) {
              zoneScrollCompleteTime[z] = millis();
            }
            if (millis() - zoneScrollCompleteTime[z] >=
                (unsigned long)(zones[z].scrollPause * 1000)) {
              zoneScrollCompleteTime[z] = 0;
              P.displayReset(z);
            }
          } else if ((zones[z].workMode == "mqttClient" ||
                      zones[z].workMode == "manualInput") &&
                     zones[z].textFits &&
                     zones[z].scrollEffectOut == "PA_NO_EFFECT") {
            // Do nothing, static
          } else {
            P.displayReset(z);
          }
        }
      }
    }
  }
}

// ─── WOPR Effect
// ───────────────────────────────────────────────────────────────
void updateWOPREffect(uint8_t zone) {
  if (!woprZones[zone].active)
    return;

  if (currentMillis - woprZones[zone].lastUpdate >=
      woprZones[zone].updateInterval) {
    woprZones[zone].lastUpdate = currentMillis;

    MD_MAX72XX *mx = P.getGraphicObject();
    if (!mx)
      return;

    int startCol = zones[zone].begin * 8;
    int endCol = (zones[zone].end * 8) + 7;

    for (int col = startCol; col <= endCol; col++) {
      if (random(0, 100) < 30) {
        for (int row = 0; row < 8; row++) {
          int pattern = random(0, 4);
          bool state = false;

          switch (pattern) {
          case 0:
            state = random(100) < 65;
            break;
          case 1:
            state = (currentMillis / 100 + col) % 2;
            break;
          case 2:
            state = (currentMillis / 100 + row) % 2;
            break;
          case 3:
            state = ((row + col + (currentMillis / 200)) % 2);
            break;
          case 4:
            if (random(0, 100) < 20) {
              state = true;
              mx->setPoint(row, col, state);
              for (int i = 1; i < 3 && row + i < 8; i++) {
                mx->setPoint(row + i, col, state);
              }
              continue;
            }
            break;
          }
          mx->setPoint(row, col, state);
        }
      }
    }

    if (random(0, 100) < 5) {
      for (int col = startCol; col <= endCol; col++) {
        for (int row = 0; row < 8; row++) {
          mx->setPoint(row, col, true);
        }
      }
    }
  }
}

// ─── Test Zones
// ────────────────────────────────────────────────────────────────
void testZones(uint8_t n) {
  P.displayClear();
  P.setFont(n, wledSymbolFont);
  P.setCharSpacing(n, 0);
  P.setPause(n, 500);
  P.setSpeed(n, 25);
  if (n < zoneNumbers) {
    P.setTextEffect(n, PA_GROW_UP, PA_NO_EFFECT);
    String testMessage = "8";
    for (int i = 0; i < int(zones[n].end - zones[n].begin); i++)
      testMessage += 8;
    zoneNewMessage(n, testMessage, "", false);
  }
  if (n == zoneNumbers) {
    P.setCharSpacing(0, 1);
    P.setFont(0, wledFont_cyrillic);
    P.setSpeed(0, 50);
    P.setTextEffect(0, PA_SCROLL_LEFT, PA_NO_EFFECT);
    zoneNewMessage(0, "ip: " + WiFi.localIP().toString(), "", false);
  }
}

namespace {
bool fetchHaStateValue(const String &sensorId, String &outState) {
  if (sensorId.length() == 0 || haAddr.length() == 0 || haApiToken.length() == 0)
    return false;

  static String path;
  static String auth;
  if (!path.reserve(96) || !auth.reserve(220)) {
    Serial.println(F("\n[HA] OOM while reserving request buffers"));
    return false;
  }
  path = F("/api/states/");
  path += sensorId;
  auth = F("Bearer ");
  auth += haApiToken;
  if (path.length() == 0 || auth.length() == 0) {
    Serial.println(F("\n[HA] Failed to build request path/token"));
    return false;
  }

  bool https = (haApiHttpType == "https");
  Serial.printf_P(PSTR("\n[HA] GET %s://%s:%u%s (entity length=%u, token length=%u)"),
                https ? "https" : "http", haAddr.c_str(), haApiPort,
                path.c_str(), static_cast<unsigned>(sensorId.length()),
                static_cast<unsigned>(haApiToken.length()));
  StaticJsonDocument<2048> &doc = sharedJsonDocument();
  doc.clear();
  bool success = httpsRequestToDoc(haAddr, haApiPort, path, auth, https, doc);
  if (!success)
    return false;
  JsonVariant state = doc[F("state")];
  if (state.isNull())
    return false;
  const char *stateValue = state.as<const char *>();
  if (!stateValue)
    return false;
  outState = stateValue;
  return true;
}
} // namespace

// ─── HA API Get
// ────────────────────────────────────────────────────────────────
String haApiGet(const String &sensorId, const String &sensorPostfix) {
  Serial.printf_P(PSTR("\nHA updating..."));
  String state;
  if (!fetchHaStateValue(sensorId, state))
    return "err";
  state += sensorPostfix;
  return state;
}

// ─── HA API Get State Only (for Progress Bar)
// ─────────────────────────────────
String haApiGetState(const String &sensorId) {
  String state;
  if (!fetchHaStateValue(sensorId, state))
    return "";
  return state;
}

// ─── Dynamic Progress Bar Pixel Helper
// ──────────────────────────────────────────
// Determines if pixel at index i (0=leftmost) should be ON
// given the animation type, phase, position, and total columns.
// Animations 1-6: fill/erase two-phase model.
// Animations 7-14: standalone patterns (phase/pos have animation-specific meaning).
//
// Morse SOS spatial pattern — scrolls across the strip like a ticker.
// Dot = 1 pixel ON, Dash = 3 pixels ON
// Inter-element gap = 1 pixel OFF, letter gap = 3 pixels OFF, word gap = 7 pixels OFF
// S(●○●○●) + letter_gap(○○○) + O(●●●○●●●○●●●) + letter_gap(○○○) + S(●○●○●) + word_gap(○○○○○○○)
static const uint8_t morseSOSBits[] = {
  // S: dot gap dot gap dot
  1,0,1,0,1,
  // letter gap
  0,0,0,
  // O: dash gap dash gap dash
  1,1,1,0,1,1,1,0,1,1,1,
  // letter gap
  0,0,0,
  // S: dot gap dot gap dot
  1,0,1,0,1,
  // word gap
  0,0,0,0,0,0,0
};
static const uint8_t morseSOSBitsLen = sizeof(morseSOSBits);

static bool isDynPixelOn(uint8_t anim, uint8_t phase, int16_t pos,
                         int16_t i, int16_t totalCols,
                         int8_t direction, uint16_t extra) {
  // ── Animations 1-6: Fill / Erase two-phase model ──
  if (anim >= 1 && anim <= 6) {
    if (phase == 0) { // Fill phase
      switch (anim) {
      case 1:
      case 3:
        return i <= pos; // L→R fill
      case 2:
      case 4:
        return i >= totalCols - 1 - pos; // R→L fill
      case 5:
      case 6:
        return i <= pos || i >= totalCols - 1 - pos; // edges→center
      }
    } else { // Erase phase
      switch (anim) {
      case 1:
        return i > pos; // L→R erase
      case 2:
        return i < totalCols - 1 - pos; // R→L erase
      case 3:
        return i < totalCols - 1 - pos; // R→L erase (snake)
      case 4:
        return i > pos; // L→R erase (snake)
      case 5:
        return i > pos && i < totalCols - 1 - pos; // edges→center erase
      case 6: {
        int16_t cL = (totalCols - 1) / 2;
        int16_t cR = totalCols / 2;
        return i < cL - pos || i > cR + pos; // center→edges erase
      }
      }
    }
    return false;
  }

  // ── Animation 7: Larson Scanner ──
  // pos = absolute position in bounce cycle (0 to 2*(totalCols-2)-1)
  // Maps to physical position that oscillates 0→totalCols-1→1
  if (anim == 7) {
    int16_t maxIdx = totalCols - 1;
    int16_t physPos;
    if (pos <= maxIdx) {
      physPos = pos;
    } else {
      physPos = 2 * maxIdx - pos; // bounce back
    }
    int16_t dist = i - physPos;
    if (dist < 0) dist = -dist;
    return dist <= 1; // 3-pixel-wide dot
  }

  // ── Animation 8: Heartbeat ──
  // pos cycles 0-7: ON(0), OFF(1), ON(2), OFF(3,4,5,6,7)
  // All pixels share the same state
  if (anim == 8) {
    return (pos == 0 || pos == 2); // two quick beats then long pause
  }

  // ── Animation 9: Running Wave → ──
  // Pattern ●●●○○○ scrolling right. pos increments each step.
  if (anim == 9) {
    int16_t p = ((i - pos) % 6 + 6) % 6; // ensure positive modulo
    return p < 3; // first 3 of 6-pixel group are ON
  }

  // ── Animation 10: Breathing (center expand/contract) ──
  // phase 0: expand from center, phase 1: contract to center
  // pos = current radius from center
  if (anim == 10) {
    int16_t center = totalCols / 2;
    int16_t dist = i - center;
    if (dist < 0) dist = -dist;
    if (phase == 0) {
      return dist <= pos; // expanding
    } else {
      return dist <= pos; // contracting (pos decreasing)
    }
  }

  // ── Animation 11: Bouncing Pixel ──
  // Single pixel bouncing off edges (1 pixel, unlike Larson's 3).
  // pos = position of the pixel
  if (anim == 11) {
    return (i == pos);
  }

  // ── Animation 12: Comet → ──
  // Head at pos, pattern: [tail●●●][gap○][head●]
  // i.e., at pos: head ON, pos-1: OFF (gap), pos-2,pos-3,pos-4: ON (tail)
  if (anim == 12) {
    int16_t rel = ((pos - i) % totalCols + totalCols) % totalCols;
    // rel=0: head, rel=1: gap, rel=2,3,4: tail
    return (rel == 0 || (rel >= 2 && rel <= 4));
  }

  // ── Animation 13: Firework / Sparkle ──
  // Random pixels based on pos as changing seed. ~30% density.
  // Uses a simple LCG to generate deterministic pattern per frame.
  if (anim == 13) {
    uint16_t seed = (uint16_t)(pos * 7919 + i * 6271 + extra);
    seed ^= (seed >> 3);
    seed *= 2654435761U;
    seed ^= (seed >> 7);
    return (seed % 100) < 30; // ~30% density
  }

  // ── Animation 14: Morse Code (SOS) ──
  // The SOS bit pattern scrolls across the strip from left to right.
  // pos = scroll offset, increments each step.
  // Pixel i is ON if the corresponding bit in the pattern is 1.
  if (anim == 14) {
    // Map pixel position to pattern index (scrolling left-to-right)
    int16_t patIdx = ((i - pos) % morseSOSBitsLen + morseSOSBitsLen) % morseSOSBitsLen;
    return morseSOSBits[patIdx] != 0;
  }

  // ── Animation 15: Alternating Flash ──
  // Even and odd pixels flash in opposite phases.
  // pos toggles 0/1 each step.
  if (anim == 15) {
    bool evenPhase = (pos % 2 == 0);
    bool isEven = (i % 2 == 0);
    return (evenPhase == isEven);
  }

  // ── Animation 16: Stack Builder ──
  // A single pixel runs from left to the stack boundary, then "lands".
  // extra = number of pixels already stacked (from right side).
  // pos = current position of the running pixel (0 to boundary).
  // phase 0 = pixel is running, phase 1 = pixel just landed (brief pause).
  if (anim == 16) {
    int16_t stackStart = totalCols - extra; // first stacked column
    // Show the stacked pixels (always ON)
    if (i >= stackStart) return true;
    // Show the running pixel (during run phase)
    if (phase == 0 && i == pos) return true;
    return false;
  }

  return false;
}

// ─── Progress Bar Overlay
// ─────────────────────────────────────────────────────
void updateProgressBars() {
  MD_MAX72XX *mx = P.getGraphicObject();
  if (!mx)
    return;

  uint8_t devCount = mx->getDeviceCount();
  if (devCount == 0)
    return;

  // Disable auto-update — all writes go to buffer only, no SPI
  mx->update(MD_MAX72XX::OFF);

  for (uint8_t n = 0; n < zoneNumbers && n < 4; n++) {
    ProgressBarData &pb = progressBars[n];

    // Skip if not enabled or zone is in WOPR mode
    if (!pb.enabled || woprZones[n].active) {
      continue;
    }

    // Compute active state (Regular mode only — dynamic handles its own visibility)
    if (!pb.dynamicMode) {
      if (pb.conditionEnabled) {
        pb.barActive = pb.conditionMet;
      } else {
        pb.barActive = true;
      }
    }

    int startCol = zones[n].begin * 8;
    int endCol = (zones[n].end * 8) + 7;
    int totalCols = endCol - startCol + 1;

    if (!pb.dynamicMode && !pb.barActive) {
      // Condition not met — clear row 7 for zone devices
      for (uint8_t dev = zones[n].begin; dev <= zones[n].end; dev++) {
        mx->setRow(dev, 7, 0x00);
      }
      continue;
    }

    // ── Dynamic animation mode ──
    if (pb.dynamicMode) {
      // Resolve which animation/speed to use based on rules
      uint8_t resolvedAnim = 0;
      uint16_t resolvedSpeed = 100;

      if (pb.dynSourceId.length() > 0 &&
          pb.dynCurrentSensorValue.length() > 0) {
        if (pb.dynRule1Value.length() > 0 &&
            pb.dynCurrentSensorValue == pb.dynRule1Value) {
          resolvedAnim = pb.dynamicAnimation;
          resolvedSpeed = pb.dynamicSpeedMs;
        } else if (pb.dynRule2Value.length() > 0 &&
                   pb.dynCurrentSensorValue == pb.dynRule2Value) {
          resolvedAnim = pb.dynRule2Animation;
          resolvedSpeed = pb.dynRule2SpeedMs;
        }
      }

      // No rule matches → bar off
      if (resolvedAnim == 0) {
        for (uint8_t dev = zones[n].begin; dev <= zones[n].end; dev++) {
          mx->setRow(dev, 7, 0x00);
        }
        pb.dynActiveAnim = 0;
        continue;
      }

      // Animation changed? Reset state to restart cycle
      if (resolvedAnim != pb.dynActiveAnim) {
        pb.dynActiveAnim = resolvedAnim;
        pb.dynPhase = 0;
        pb.dynPosition = 0;
        pb.dynLastStep = 0;
        pb.dynDirection = 1;
        pb.dynExtra = 0;
      }

      unsigned long now = millis();

      // Render current state
      for (uint8_t dev = zones[n].begin; dev <= zones[n].end; dev++) {
        uint8_t rowBits = 0;
        int devStartCol = dev * 8;
        for (uint8_t bit = 0; bit < 8; bit++) {
          int absCol = devStartCol + bit;
          int16_t pixelIdx = endCol - absCol; // 0=left, totalCols-1=right
          if (pixelIdx >= 0 && pixelIdx < totalCols) {
            if (isDynPixelOn(resolvedAnim, pb.dynPhase,
                             pb.dynPosition, pixelIdx, totalCols,
                             pb.dynDirection, pb.dynExtra)) {
              rowBits |= (1 << bit);
            }
          }
        }
        mx->setRow(dev, 7, rowBits);
      }

      // Advance state on timer
      if (pb.dynLastStep == 0) {
        pb.dynLastStep = now; // First frame — mark time, don't advance
      } else if (now - pb.dynLastStep >= resolvedSpeed) {
        pb.dynLastStep = now;

        if (resolvedAnim >= 1 && resolvedAnim <= 6) {
          // ── Animations 1-6: fill/erase two-phase ──
          int16_t maxPos;
          if (resolvedAnim >= 5)
            maxPos = (totalCols + 1) / 2 - 1;
          else
            maxPos = totalCols - 1;

          pb.dynPosition++;
          if (pb.dynPosition > maxPos) {
            pb.dynPhase = (pb.dynPhase == 0) ? 1 : 0;
            pb.dynPosition = 0;
          }
        } else if (resolvedAnim == 7) {
          // ── Larson Scanner: bounce 0 → 2*(totalCols-2) ──
          int16_t maxPos = 2 * (totalCols - 1) - 1;
          if (maxPos < 1) maxPos = 1;
          pb.dynPosition++;
          if (pb.dynPosition > maxPos) {
            pb.dynPosition = 0;
          }
        } else if (resolvedAnim == 8) {
          // ── Heartbeat: cycle 0-7 ──
          pb.dynPosition++;
          if (pb.dynPosition > 7) {
            pb.dynPosition = 0;
          }
        } else if (resolvedAnim == 9) {
          // ── Running Wave: scroll offset ──
          pb.dynPosition++;
          if (pb.dynPosition >= 6) {
            pb.dynPosition = 0;
          }
        } else if (resolvedAnim == 10) {
          // ── Breathing: expand/contract from center ──
          int16_t maxRadius = (totalCols + 1) / 2 - 1;
          if (pb.dynPhase == 0) {
            // Expanding
            pb.dynPosition++;
            if (pb.dynPosition > maxRadius) {
              pb.dynPhase = 1;
              pb.dynPosition = maxRadius;
            }
          } else {
            // Contracting
            pb.dynPosition--;
            if (pb.dynPosition < 0) {
              pb.dynPhase = 0;
              pb.dynPosition = 0;
            }
          }
        } else if (resolvedAnim == 11) {
          // ── Bouncing Pixel: single pixel bouncing ──
          int16_t maxPos = totalCols - 1;
          if (maxPos < 1) maxPos = 1;
          pb.dynPosition += pb.dynDirection;
          if (pb.dynPosition >= maxPos) {
            pb.dynPosition = maxPos;
            pb.dynDirection = -1;
          } else if (pb.dynPosition <= 0) {
            pb.dynPosition = 0;
            pb.dynDirection = 1;
          }
        } else if (resolvedAnim == 12) {
          // ── Comet: wraps continuously L→R ──
          pb.dynPosition++;
          if (pb.dynPosition >= totalCols) {
            pb.dynPosition = 0;
          }
        } else if (resolvedAnim == 13) {
          // ── Sparkle: just increment pos for new random pattern each frame ──
          pb.dynPosition++;
          if (pb.dynPosition > 255) {
            pb.dynPosition = 0;
            pb.dynExtra++; // extra adds more randomness between cycles
          }
        } else if (resolvedAnim == 14) {
          // ── Morse Code: scroll the bit pattern across the strip ──
          pb.dynPosition++;
          if (pb.dynPosition >= morseSOSBitsLen) {
            pb.dynPosition = 0;
          }
        } else if (resolvedAnim == 15) {
          // ── Alternating Flash: toggle 0/1 ──
          pb.dynPosition = (pb.dynPosition == 0) ? 1 : 0;
        } else if (resolvedAnim == 16) {
          // ── Stack Builder: pixel runs left→right, stacks ──
          int16_t stackStart = totalCols - pb.dynExtra;
          if (pb.dynPhase == 0) {
            // Running phase
            pb.dynPosition++;
            if (pb.dynPosition >= stackStart) {
              // Pixel reached the stack — land it
              pb.dynExtra++; // one more pixel in the stack
              pb.dynPhase = 1;
              pb.dynPosition = 0;
            }
          } else {
            // Brief pause after landing, then start next pixel
            pb.dynPhase = 0;
            pb.dynPosition = 0;
            // If stack is full, reset
            if (pb.dynExtra >= (uint16_t)totalCols) {
              pb.dynExtra = 0;
            }
          }
        }
      }
      continue; // skip value-driven rendering
    }

    // ── Value-driven mode (existing) ──
    // For time-based source, compute value from NTP clock
    float useValue = pb.currentValue;
    float useMin = pb.minValue;
    float useMax = pb.maxValue;

    if (pb.dataSourceType == "time") {
      time_t epochTime = timeClient.getEpochTime();
      struct tm *ptm = gmtime(&epochTime);
      if (ptm) {
        if (pb.dataSourceId == "seconds") {
          useValue = ptm->tm_sec;
          useMin = 0;
          useMax = 59;
        } else if (pb.dataSourceId == "minutes") {
          useValue = ptm->tm_min;
          useMin = 0;
          useMax = 59;
        } else if (pb.dataSourceId == "hours") {
          useValue = ptm->tm_hour;
          useMin = 0;
          useMax = 23;
        }
      }
    }

    // Clamp value to [min, max]
    float clamped = useValue;
    if (clamped < useMin)
      clamped = useMin;
    if (clamped > useMax)
      clamped = useMax;

    // Calculate how many pixels to light
    float range = useMax - useMin;
    int filledCols = 0;
    if (range > 0.0001f) {
      filledCols = (int)((clamped - useMin) / range * totalCols);
    }
    // Ensure at least 1 pixel when value is above minimum
    if (filledCols == 0 && clamped > useMin) {
      filledCols = 1;
    }

    // Build row 7 bitmask per device and write with setRow()
    // MD_MAX72XX col numbering: higher col = physical left
    // Fill left-to-right: start from endCol going down
    for (uint8_t dev = zones[n].begin; dev <= zones[n].end; dev++) {
      uint8_t rowBits = 0;
      int devStartCol = dev * 8;
      for (uint8_t bit = 0; bit < 8; bit++) {
        int absCol = devStartCol + bit;
        // Map absolute column to the fill index (left-to-right)
        int fillIdx = endCol - absCol;
        if (fillIdx >= 0 && fillIdx < totalCols && fillIdx < filledCols) {
          rowBits |= (1 << bit);
        }
      }
      mx->setRow(dev, 7, rowBits);
    }
  }

  // Re-enable auto-update — flushes ALL dirty rows in one batch
  mx->update(MD_MAX72XX::ON);
}

// ─── WiFi AP Welcome
// ───────────────────────────────────────────────────────────
void wifiApWelcomeMessage(AsyncWiFiManager *wifiManager) {
  Serial.println(F("AP Callback Triggered"));
  P.displayClear();
  P.setTextAlignment(0, PA_CENTER);
  P.setIntensity(7);
  P.setSpeed(0, 100);
  P.print(F("AP Mode"));
  P.displayAnimate();
}
