#pragma once

#include <stdint.h>

enum class NetworkRequestResult : uint8_t { Success, Deferred, Failed };

namespace networkMemory {
struct Heap {
  uint32_t freeBytes;
  uint32_t largestBlock;
};

// BearSSL's constructor always allocates its 6200-byte stack in DRAM,
// even when its other allocations use the secondary IRAM heap. Leave room
// for TCP and incoming web requests as well as allocator overhead.
constexpr bool canStartTls(Heap dram, Heap iram, bool compactRecords) {
  return dram.freeBytes >= (compactRecords ? 11264u : 16384u) &&
         dram.largestBlock >= (compactRecords ? 7168u : 14336u) &&
         iram.freeBytes >= (compactRecords ? 12288u : 18432u) &&
         iram.largestBlock >= (compactRecords ? 10240u : 16720u);
}

// Keep the existing interval anchor compatible with mode/power changes.
// Deferral retries in five seconds rather than waiting a whole OWM interval.
constexpr uint32_t pollAnchor(uint32_t now, uint32_t intervalMs,
                              NetworkRequestResult result) {
  return result == NetworkRequestResult::Deferred && intervalMs > 5000u
             ? now - (intervalMs - 5000u)
             : now;
}
} // namespace networkMemory
