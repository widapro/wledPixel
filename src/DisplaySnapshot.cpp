#include "DisplaySnapshot.h"
#include "Globals.h"
#include "Scheduler.h"

namespace displaySnapshot {
namespace {
std::shared_ptr<Request> pending;
#if defined(ESP32)
portMUX_TYPE pendingMux = portMUX_INITIALIZER_UNLOCKED;
#endif

void swapPending(std::shared_ptr<Request> &value) {
#if defined(ESP32)
  portENTER_CRITICAL(&pendingMux);
#endif
  pending.swap(value);
#if defined(ESP32)
  portEXIT_CRITICAL(&pendingMux);
#endif
}
} // namespace

std::shared_ptr<Request> request() {
  auto candidate = std::make_shared<Request>();
  std::shared_ptr<Request> result;
#if defined(ESP32)
  portENTER_CRITICAL(&pendingMux);
#endif
  // Requests arriving before the next loop share its capture. Replacing a
  // queued request would leave the earlier response waiting indefinitely.
  if (!pending)
    pending = candidate;
  result = pending;
#if defined(ESP32)
  portEXIT_CRITICAL(&pendingMux);
#endif
  return result;
}

void service() {
  std::shared_ptr<Request> current;
  swapPending(current);
  if (!current)
    return;
  MD_MAX72XX *matrix = P.getGraphicObject();
  if (matrix)
    capture(current->frame, *matrix, zones, zoneNumbers, power,
            displayRotation::flipped, millis());
#if defined(ESP8266)
  captureProfile(current->frame, scheduleMeta, MAX_PROFILES, lastActiveProfile);
#else
  captureProfile(current->frame, scheduleProfiles, MAX_PROFILES, lastActiveProfile);
#endif
  current->ready.store(true, std::memory_order_release);
}

} // namespace displaySnapshot
