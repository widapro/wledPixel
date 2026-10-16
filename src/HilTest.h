#pragma once

#include <Arduino.h>

#if defined(WLED_HIL_TEST)

// Registers test-only HTTP endpoints. These routes are not present in normal
// firmware builds.
void setupHilTestRoutes();

// Applies commands queued by the async HTTP callbacks from the main loop so
// display-buffer writes never race Parola animation on ESP32.
void hilTestService(unsigned long now);

// While a deterministic HIL pattern is visible, normal display animation is
// paused. Networking remains available because the web server is asynchronous.
bool hilTestDisplayHoldActive(unsigned long now);

#else

inline void setupHilTestRoutes() {}
inline void hilTestService(unsigned long) {}
inline bool hilTestDisplayHoldActive(unsigned long) { return false; }

#endif
