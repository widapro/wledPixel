#pragma once

#include <Arduino.h>
#include <time.h>

// Pure schedule predicate shared by the firmware and native unit tests.
bool schedulerTimeMatches(bool enabled, uint8_t minute, uint8_t hour,
                          uint8_t dayOfWeekMask, uint32_t dayOfMonthMask,
                          const struct tm &now);
