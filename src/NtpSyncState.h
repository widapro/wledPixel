#pragma once

#include <atomic>
#include <stdint.h>

// HTTP only queues requests; the main loop owns the UDP client.
class NtpSyncState {
public:
  enum class Status : uint8_t { Never, Pending, Running, Succeeded, Failed };

  bool request() {
    Status expected = status.load();
    while (expected != Status::Pending && expected != Status::Running) {
      if (transition(expected, Status::Pending))
        return true;
    }
    return false;
  }

  bool pending() const { return status.load() == Status::Pending; }
  bool begin() {
    Status expected = status.load();
    while (expected != Status::Running) {
      if (transition(expected, Status::Running))
        return true;
    }
    return false;
  }

  void finish(bool success, uint32_t utcEpoch) {
    if (success)
      lastSuccessUtc.store(utcEpoch);
    status.store(success ? Status::Succeeded : Status::Failed);
  }

  Status getStatus() const { return status.load(); }
  uint32_t lastSyncUtc() const { return lastSuccessUtc.load(); }

private:
  bool transition(Status &expected, Status next) {
#if defined(ESP8266)
    // ESP8266 callbacks are cooperative; these operations never yield. Its
    // toolchain supports atomic loads/stores, but not compare_exchange.
    const Status current = status.load();
    if (current != expected) {
      expected = current;
      return false;
    }
    status.store(next);
    return true;
#else
    return status.compare_exchange_weak(expected, next);
#endif
  }

  std::atomic<Status> status{Status::Never};
  std::atomic<uint32_t> lastSuccessUtc{0};
};
