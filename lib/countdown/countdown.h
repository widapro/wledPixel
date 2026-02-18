#ifndef countdown_h
#define countdown_h

#include <Arduino.h>
#include <time.h>

// Countdown state per zone
struct CountdownState {
  unsigned long target;      // epoch seconds of target
  String targetStr;          // "YYYY-MM-DDTHH:MM:SS" for UI
  bool reached;              // true when countdown hit zero
  uint8_t blinkCount;        // blinks done so far (for blink3)
  bool blinkVisible;         // current blink state
  unsigned long lastBlinkMs; // last blink toggle time
};

extern CountdownState countdownState[4];

// Initialize countdown state
void countdownInit();

// Parse a target string and store it. Accepts:
//   "2026-12-31T23:59:59" - full ISO datetime
//   "2026-12-31"          - date only (time = 00:00:00)
//   "23:59:59"            - time only (uses current date)
// Returns true on success
bool parseCountdownTarget(const String &input, int zone,
                          unsigned long currentEpoch, float tzOffsetHours);

// Get the formatted countdown text for a zone
// Parameters:
//   zone           - zone index (0-3)
//   format         - format string (SS, MM, HH, DD, YY, HHMM, HHMMSS, DDHH,
//   DDHHMM, DDHHMMSS, YYMM, YYMMDDHHMM, YYMMDDHHMMSS, auto) showUnits      - if
//   true, show h/m/s suffixes prefix         - text before countdown suffix -
//   text after countdown currentEpoch   - current time as epoch seconds
//   tzOffsetHours  - timezone offset in hours
// Returns formatted string
String getCountdownText(int zone, const String &format, bool showUnits,
                        const String &prefix, const String &suffix,
                        unsigned long currentEpoch, float tzOffsetHours);

// Check if countdown has reached zero and handle blink behavior
// Returns true if display should show text (false = blank for blink)
// finishMode: "blinkForever", "blink3", "none"
bool countdownHandleBlink(int zone, const String &finishMode,
                          unsigned long currentMillis);

#endif
