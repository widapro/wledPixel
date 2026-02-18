#include "countdown.h"

CountdownState countdownState[4];

void countdownInit() {
  for (int i = 0; i < 4; i++) {
    countdownState[i].target = 0;
    countdownState[i].targetStr = "";
    countdownState[i].reached = false;
    countdownState[i].blinkCount = 0;
    countdownState[i].blinkVisible = true;
    countdownState[i].lastBlinkMs = 0;
  }
}

// ---- Internal helpers ----

// Check if a year is a leap year
static bool isLeapYear(int year) {
  return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
}

// Days in a given month (1-12) for a given year
static int daysInMonth(int month, int year) {
  static const int days[] = {0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
  if (month == 2 && isLeapYear(year))
    return 29;
  return days[month];
}

// Calendar-aware diff: compute years, months, days, hours, mins, secs
// between 'from' and 'to' (to >= from)
struct CalendarDiff {
  int years, months, days, hours, mins, secs;
};

static CalendarDiff calendarDiff(time_t fromEpoch, time_t toEpoch) {
  CalendarDiff d = {0, 0, 0, 0, 0, 0};
  if (toEpoch <= fromEpoch)
    return d;

  struct tm fromTm, toTm;
  gmtime_r(&fromEpoch, &fromTm);
  gmtime_r(&toEpoch, &toTm);

  // Work with copies
  int fy = fromTm.tm_year + 1900, fm = fromTm.tm_mon + 1, fd = fromTm.tm_mday;
  int fh = fromTm.tm_hour, fmin = fromTm.tm_min, fs = fromTm.tm_sec;

  int ty = toTm.tm_year + 1900, tm_month = toTm.tm_mon + 1, td = toTm.tm_mday;
  int th = toTm.tm_hour, tmin = toTm.tm_min, ts = toTm.tm_sec;

  // Borrow seconds
  if (ts < fs) {
    ts += 60;
    tmin--;
  }
  d.secs = ts - fs;

  // Borrow minutes
  if (tmin < fmin) {
    tmin += 60;
    th--;
  }
  d.mins = tmin - fmin;

  // Borrow hours
  if (th < fh) {
    th += 24;
    td--;
  }
  d.hours = th - fh;

  // Borrow days
  if (td < fd) {
    tm_month--;
    if (tm_month < 1) {
      tm_month = 12;
      ty--;
    }
    td += daysInMonth(tm_month, ty);
  }
  d.days = td - fd;

  // Borrow months
  if (tm_month < fm) {
    tm_month += 12;
    ty--;
  }
  d.months = tm_month - fm;

  d.years = ty - fy;

  return d;
}

// ---- Parsing ----

bool parseCountdownTarget(const String &input, int zone,
                          unsigned long currentEpoch, float tzOffsetHours) {
  if (zone < 0 || zone > 3)
    return false;

  String s = input;
  s.trim();
  if (s.length() == 0)
    return false;

  // HTML datetime-local sends "YYYY-MM-DDTHH:MM" (16 chars, no seconds)
  // Append ":00" if seconds are missing
  if (s.length() == 16 && s.indexOf('T') == 10) {
    s += ":00";
  }

  struct tm t;
  memset(&t, 0, sizeof(t));

  int tzOffsetSec = (int)(tzOffsetHours * 3600);

  if (s.length() >= 19 && s.indexOf('T') == 10) {
    // Full ISO: "2026-12-31T23:59:59"
    t.tm_year = s.substring(0, 4).toInt() - 1900;
    t.tm_mon = s.substring(5, 7).toInt() - 1;
    t.tm_mday = s.substring(8, 10).toInt();
    t.tm_hour = s.substring(11, 13).toInt();
    t.tm_min = s.substring(14, 16).toInt();
    t.tm_sec = s.substring(17, 19).toInt();
  } else if (s.length() == 10 && s.charAt(4) == '-') {
    // Date only: "2026-12-31"
    t.tm_year = s.substring(0, 4).toInt() - 1900;
    t.tm_mon = s.substring(5, 7).toInt() - 1;
    t.tm_mday = s.substring(8, 10).toInt();
    t.tm_hour = 0;
    t.tm_min = 0;
    t.tm_sec = 0;
  } else if (s.length() == 8 && s.charAt(2) == ':') {
    // Time only: "23:59:59" — use current date
    time_t localNow = currentEpoch + tzOffsetSec;
    struct tm nowTm;
    gmtime_r(&localNow, &nowTm);
    t.tm_year = nowTm.tm_year;
    t.tm_mon = nowTm.tm_mon;
    t.tm_mday = nowTm.tm_mday;
    t.tm_hour = s.substring(0, 2).toInt();
    t.tm_min = s.substring(3, 5).toInt();
    t.tm_sec = s.substring(6, 8).toInt();
  } else {
    return false;
  }

  // Convert local time to UTC epoch
  time_t epoch = mktime(&t);
  if (epoch == -1)
    return false;

  // mktime assumes local time on some platforms, but on ESP32 it assumes UTC
  // We need to subtract timezone offset since user enters local time
  epoch -= tzOffsetSec;

  countdownState[zone].target = (unsigned long)epoch;
  countdownState[zone].reached = false;
  countdownState[zone].blinkCount = 0;
  countdownState[zone].blinkVisible = true;

  // Store the display string (always as full ISO)
  char buf[24];
  struct tm localTm;
  time_t localEpoch = epoch + tzOffsetSec;
  gmtime_r(&localEpoch, &localTm);
  sprintf(buf, "%04d-%02d-%02dT%02d:%02d:%02d", localTm.tm_year + 1900,
          localTm.tm_mon + 1, localTm.tm_mday, localTm.tm_hour, localTm.tm_min,
          localTm.tm_sec);
  countdownState[zone].targetStr = String(buf);

  return true;
}

// ---- Formatting ----

static String formatSimple(long totalSeconds, const String &format,
                           bool showUnits) {
  if (totalSeconds < 0)
    totalSeconds = 0;

  char buf[48];

  if (format == "SS") {
    if (showUnits)
      sprintf(buf, "%lds", totalSeconds);
    else
      sprintf(buf, "%ld", totalSeconds);
  } else if (format == "MM") {
    long m = totalSeconds / 60;
    if (showUnits)
      sprintf(buf, "%ldm", m);
    else
      sprintf(buf, "%ld", m);
  } else if (format == "HH") {
    long h = totalSeconds / 3600;
    if (showUnits)
      sprintf(buf, "%ldh", h);
    else
      sprintf(buf, "%ld", h);
  } else if (format == "DD") {
    long d = totalSeconds / 86400;
    if (showUnits)
      sprintf(buf, "%ldd", d);
    else
      sprintf(buf, "%ld", d);
  } else if (format == "HHMM") {
    long h = totalSeconds / 3600;
    long m = (totalSeconds % 3600) / 60;
    if (showUnits)
      sprintf(buf, "%ldh %02ldm", h, m);
    else
      sprintf(buf, "%ld:%02ld", h, m);
  } else if (format == "HHMMSS") {
    long h = totalSeconds / 3600;
    long m = (totalSeconds % 3600) / 60;
    long s = totalSeconds % 60;
    if (showUnits)
      sprintf(buf, "%ldh %02ldm %02lds", h, m, s);
    else
      sprintf(buf, "%ld:%02ld:%02ld", h, m, s);
  } else if (format == "DDHH") {
    long d = totalSeconds / 86400;
    long h = (totalSeconds % 86400) / 3600;
    if (showUnits)
      sprintf(buf, "%ldd %ldh", d, h);
    else
      sprintf(buf, "%ld %ld", d, h);
  } else if (format == "DDHHMM") {
    long d = totalSeconds / 86400;
    long h = (totalSeconds % 86400) / 3600;
    long m = (totalSeconds % 3600) / 60;
    if (showUnits)
      sprintf(buf, "%ldd %ldh %02ldm", d, h, m);
    else
      sprintf(buf, "%ldd %02ld:%02ld", d, h, m);
  } else if (format == "DDHHMMSS") {
    long d = totalSeconds / 86400;
    long h = (totalSeconds % 86400) / 3600;
    long m = (totalSeconds % 3600) / 60;
    long s = totalSeconds % 60;
    if (showUnits)
      sprintf(buf, "%ldd %ldh %02ldm %02lds", d, h, m, s);
    else
      sprintf(buf, "%ldd %02ld:%02ld:%02ld", d, h, m, s);
  } else {
    sprintf(buf, "%ld", totalSeconds);
  }

  return String(buf);
}

static String formatCalendar(const CalendarDiff &cd, const String &format,
                             bool showUnits) {
  char buf[64];

  if (format == "YY") {
    if (showUnits)
      sprintf(buf, "%dy", cd.years);
    else
      sprintf(buf, "%d", cd.years);
  } else if (format == "YYMM") {
    if (showUnits)
      sprintf(buf, "%dy %dm", cd.years, cd.months);
    else
      sprintf(buf, "%dy %d", cd.years, cd.months);
  } else if (format == "YYMMDDHHMM") {
    if (showUnits)
      sprintf(buf, "%dy %dm %dd %dh %02dm", cd.years, cd.months, cd.days,
              cd.hours, cd.mins);
    else
      sprintf(buf, "%dy %dm %dd %02d:%02d", cd.years, cd.months, cd.days,
              cd.hours, cd.mins);
  } else if (format == "YYMMDDHHMMSS") {
    if (showUnits)
      sprintf(buf, "%dy %dm %dd %dh %02dm %02ds", cd.years, cd.months, cd.days,
              cd.hours, cd.mins, cd.secs);
    else
      sprintf(buf, "%dy %dm %dd %02d:%02d:%02d", cd.years, cd.months, cd.days,
              cd.hours, cd.mins, cd.secs);
  } else {
    // Shouldn't reach here
    sprintf(buf, "%dy %dm %dd", cd.years, cd.months, cd.days);
  }

  return String(buf);
}

static String formatAuto(const CalendarDiff &cd, long totalSeconds,
                         bool showUnits) {
  // Auto mode: show only non-zero leading units
  // If years > 0: show y m d h:m:s
  // If months > 0 (no years): show m d h:m:s
  // If days > 0 (no months/years): show d h:m:s
  // If hours > 0 (no days): show h:m:s
  // If minutes > 0 (no hours): show m:s
  // Otherwise: show s

  if (totalSeconds <= 0) {
    return showUnits ? "0s" : "0";
  }

  char buf[64];

  if (cd.years > 0) {
    if (showUnits)
      sprintf(buf, "%dy %dm %dd %dh %02dm %02ds", cd.years, cd.months, cd.days,
              cd.hours, cd.mins, cd.secs);
    else
      sprintf(buf, "%dy %dm %dd %02d:%02d:%02d", cd.years, cd.months, cd.days,
              cd.hours, cd.mins, cd.secs);
  } else if (cd.months > 0) {
    if (showUnits)
      sprintf(buf, "%dm %dd %dh %02dm %02ds", cd.months, cd.days, cd.hours,
              cd.mins, cd.secs);
    else
      sprintf(buf, "%dm %dd %02d:%02d:%02d", cd.months, cd.days, cd.hours,
              cd.mins, cd.secs);
  } else if (cd.days > 0) {
    if (showUnits)
      sprintf(buf, "%dd %dh %02dm %02ds", cd.days, cd.hours, cd.mins, cd.secs);
    else
      sprintf(buf, "%dd %02d:%02d:%02d", cd.days, cd.hours, cd.mins, cd.secs);
  } else if (cd.hours > 0) {
    if (showUnits)
      sprintf(buf, "%dh %02dm %02ds", cd.hours, cd.mins, cd.secs);
    else
      sprintf(buf, "%d:%02d:%02d", cd.hours, cd.mins, cd.secs);
  } else if (cd.mins > 0) {
    if (showUnits)
      sprintf(buf, "%dm %02ds", cd.mins, cd.secs);
    else
      sprintf(buf, "%d:%02d", cd.mins, cd.secs);
  } else {
    if (showUnits)
      sprintf(buf, "%ds", cd.secs);
    else
      sprintf(buf, "%d", cd.secs);
  }

  return String(buf);
}

// ---- Public API ----

String getCountdownText(int zone, const String &format, bool showUnits,
                        const String &prefix, const String &suffix,
                        unsigned long currentEpoch, float tzOffsetHours) {
  if (zone < 0 || zone > 3 || countdownState[zone].target == 0)
    return prefix + "---" + suffix;

  long diff = (long)countdownState[zone].target - (long)currentEpoch;
  if (diff < 0)
    diff = 0;

  if (diff == 0) {
    countdownState[zone].reached = true;
  }

  String result;
  bool needsCalendar =
      (format == "YY" || format == "YYMM" || format == "YYMMDDHHMM" ||
       format == "YYMMDDHHMMSS" || format == "auto");

  if (needsCalendar) {
    int tzOffsetSec = (int)(tzOffsetHours * 3600);
    time_t localNow = currentEpoch + tzOffsetSec;
    time_t localTarget = countdownState[zone].target + tzOffsetSec;
    CalendarDiff cd = calendarDiff(localNow, localTarget);

    if (format == "auto") {
      result = formatAuto(cd, diff, showUnits);
    } else {
      result = formatCalendar(cd, format, showUnits);
    }
  } else {
    result = formatSimple(diff, format, showUnits);
  }

  return prefix + result + suffix;
}

bool countdownHandleBlink(int zone, const String &finishMode,
                          unsigned long currentMillis) {
  if (zone < 0 || zone > 3)
    return true;

  if (!countdownState[zone].reached)
    return true; // Not reached zero yet, always show

  if (finishMode == "none")
    return true; // No blink, always show

  if (finishMode == "blink3" && countdownState[zone].blinkCount >= 6) {
    // 6 toggles = 3 full blinks (on-off-on-off-on-off), then stay visible
    countdownState[zone].blinkVisible = true;
    return true;
  }

  // Blink at 500ms interval
  if (currentMillis - countdownState[zone].lastBlinkMs >= 500) {
    countdownState[zone].lastBlinkMs = currentMillis;
    countdownState[zone].blinkVisible = !countdownState[zone].blinkVisible;
    if (finishMode == "blink3")
      countdownState[zone].blinkCount++;
  }

  return countdownState[zone].blinkVisible;
}
