#include "DisplayUtils.h"
#include "httpsRequestJson.h"
#include <SPI.h>

#if defined(ESP32)
SemaphoreHandle_t displayMutex;
#endif

void initDisplay() {
#if defined(ESP32)
  // displayMutex is replaced by noInterrupts() usage, but we keep the func for
  // init
#endif
}

// ─── String → textPosition_t
// ───────────────────────────────────────────────────
textPosition_t stringToTextPositionT(String val) {
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
void applyZoneFont(int zone, String font) {
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
}

// ─── UTF-8 → Font Encoding
// ────────────────────────────────────────────────────────────
// Converts UTF-8 encoded text to single-byte font indices.
// MD_Parola fonts are indexed 0-255 by single bytes.
// UTF-8 characters above ASCII use 2+ bytes, which MD_Parola
// would treat as separate characters. This function collapses
// them to the correct single-byte index.
void utf8ToFontEncoding(const char *src, char *dst, size_t dstSize,
                        const String &fontName) {

  size_t si = 0, di = 0;
  while (src[si] && di < dstSize - 1) {
    uint8_t b = (uint8_t)src[si];

    // ASCII (0x00-0x7F) — pass through
    if (b < 0x80) {
      if (fontName == "wledFont_CompactSymbols" && (char)src[si] == '~') {
        si++; // Skip '~'
        if ((char)src[si] == '~') {
          // Double tilde -> tilde char
          dst[di++] = '~';
          si++;
        } else if (src[si] >= 32 && src[si] <= 127) {
          if (src[si] == '@') {
            dst[di++] = (char)230;
            si++;
          } else if (src[si] == 'P') {
            // ~P -> Sun day (mapped to 224)
            dst[di++] = (char)224;
            si++;
          } else {
            // ~ + char (32-127) -> map to 128-223
            dst[di++] = (char)(128 + (src[si] - 32));
            si++;
          }
        } else {
          // Tilde followed by something else (or null) -> just print tilde
          dst[di++] = '~';
        }
      } else {
        dst[di++] = src[si++];
      }
      continue;
    }

    // 2-byte UTF-8 sequence (lead byte 0xC0-0xDF)
    if ((b & 0xE0) == 0xC0 && (uint8_t)src[si + 1]) {
      uint16_t codepoint =
          ((uint16_t)(b & 0x1F) << 6) | ((uint8_t)src[si + 1] & 0x3F);

      // Latin-1 supplement (U+0080-U+00FF) — direct mapping
      if (codepoint >= 0x0080 && codepoint <= 0x00FF) {
        dst[di++] = (char)(codepoint & 0xFF);
        si += 2;
        continue;
      }

      // Cyrillic block (U+0410-U+044F = А-я, 64 chars)
      // Maps to font slots 128-191 when using wledFont_cyrillic
      if (fontName == "wledFont_cyrillic" && codepoint >= 0x0410 &&
          codepoint <= 0x044F) {
        if (codepoint == 0x0430) { // 'а' -> 194
          dst[di++] = (char)194;
        } else if (codepoint == 0x0440) { // 'р' -> 195
          dst[di++] = (char)195;
        } else {
          dst[di++] = (char)(128 + (codepoint - 0x0410));
        }
        si += 2;
        continue;
      }

      // Ё (U+0401) and ё (U+0451) — special Cyrillic chars
      if (fontName == "wledFont_cyrillic") {
        if (codepoint == 0x0401) { // Ё → map to slot 192
          dst[di++] = (char)192;
          si += 2;
          continue;
        }
        if (codepoint == 0x0451) { // ё → map to slot 193
          dst[di++] = (char)193;
          si += 2;
          continue;
        }
      }

      // Unknown 2-byte character → '?'
      dst[di++] = '?';
      si += 2;
      continue;
    }

    // 3-byte UTF-8 sequence (lead byte 0xE0-0xEF) — skip
    if ((b & 0xF0) == 0xE0) {
      dst[di++] = '?';
      si += ((uint8_t)src[si + 1] && (uint8_t)src[si + 2]) ? 3 : 1;
      continue;
    }

    // 4-byte UTF-8 sequence (lead byte 0xF0-0xF7) — skip
    if ((b & 0xF8) == 0xF0) {
      dst[di++] = '?';
      si +=
          ((uint8_t)src[si + 1] && (uint8_t)src[si + 2] && (uint8_t)src[si + 3])
              ? 4
              : 1;
      continue;
    }

    // Stray continuation byte or already-encoded Latin-1 — pass through
    // (e.g. \xA0 from flashClockDots is already a valid font index)
    dst[di++] = src[si++];
  }
  dst[di] = '\0';
}

// ─── String → textEffect_t
// ─────────────────────────────────────────────────────
textEffect_t stringToTextEffectT(String val) {
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
void zoneNewMessage(int zone, String newMessage, String postfix,
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
        if (String(zoneMessagesPending[zone]) == String(tempEncoded))
          return;
      } else {
        if (String(zoneMessages[zone]) == String(tempEncoded))
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
void zoneShowModeMessage(int zone, String modeName) {
  if (zone >= 0 && zone < 4) {
    Serial.print("Debug: zoneShowModeMessage called for zone ");
    Serial.print(zone);
    Serial.print(" with: ");
    Serial.println(modeName);

    if (disableServiceMessages)
      return;

    if (String(zoneMessages[zone]) == modeName) {
      Serial.println("Debug: Message already displayed, skipping update.");
      return;
    }

    P.displayClear(zone);

    int16_t textWidth = P.getTextColumns(zone, modeName.c_str());
    if (textWidth == 0 && modeName.length() > 0)
      textWidth = modeName.length() * 6;

    uint16_t startCol, endCol;
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
bool isNumeric(String str) {
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
String flashClockDots(String t, String font) {
  String blinkChar = (font == "default") ? " " : "\xA0";
  if (t.indexOf(":") > 0)
    t.replace(":", blinkChar);
  else if (font != "default")
    t.replace("\xA0", ":");
  return t;
}

// ─── Get Current Time
// ──────────────────────────────────────────────────────────
String getCurTime(String curZoneFont, String displayFormat) {
  if (currentMillis - previousNTPsyncMillis >=
      (unsigned)ntpUpdateInterval * 3600 * 1000)
    ntpUpdateTime();

  String t = (String)timeClient.getFormattedTime();

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

  String weekDays[7] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
  String weekDaysCyrillic[7] = {"Вс", "Пн", "Вт", "Ср", "Чт", "Пт", "Сб"};
  String weekDay;
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
    t = monthDayStr + "." + currentMonthStr + String(weekDay);
  if (displayFormat == "aa")
    t = String(weekDay);
  if (displayFormat == "ddmmaahhmm") {
    t.remove(5, 4);
    t = monthDayStr + "." + currentMonthStr + " " + String(weekDay) + " " + t;
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
  // Now all buffers are consistent — safe to animate.
  P.displayAnimate();

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

        uint16_t startCol, endCol;
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

// ─── HA API Get
// ────────────────────────────────────────────────────────────────
String haApiGet(String sensorId, String sensorPostfix) {
  Serial.printf("\nHA updating...");
  bool https = (haApiHttpType == "https");

  DynamicJsonDocument doc(2048);
  bool success = httpsRequestToDoc(haAddr, haApiPort, "/api/states/" + sensorId,
                                   "Bearer " + haApiToken, https, doc);
  JsonObject haApiRespondPostObj = doc.as<JsonObject>();

  if (success && haApiRespondPostObj.containsKey(F("state"))) {
    return haApiRespondPostObj[F("state")].as<String>() + sensorPostfix;
  }
  return "err";
}

// ─── WiFi AP Welcome
// ───────────────────────────────────────────────────────────
void wifiApWelcomeMessage(AsyncWiFiManager *wifiManager) {
  Serial.println(F("AP Callback Triggered"));
  P.displayClear();
  P.setTextAlignment(0, PA_CENTER);
  P.setIntensity(7);
  P.setSpeed(0, 100);
  P.print("AP Mode");
  P.displayAnimate();
}
