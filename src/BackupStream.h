#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>

class ByteWindow {
public:
  ByteWindow(uint8_t *buffer, size_t capacity, size_t skip)
      : buffer_(buffer), capacity_(capacity), skip_(skip) {}
  size_t write(const uint8_t *data, size_t len) {
    const size_t start = total_;
    total_ += len;
    if (!buffer_ || copied_ >= capacity_ || start + len <= skip_)
      return len;
    const size_t offset = skip_ > start ? skip_ - start : 0;
    const size_t count = (len - offset < capacity_ - copied_)
                             ? len - offset : capacity_ - copied_;
    std::memcpy(buffer_ + copied_, data + offset, count);
    copied_ += count;
    return len;
  }
  size_t copied() const { return copied_; }
  size_t total() const { return total_; }
private:
  uint8_t *buffer_;
  size_t capacity_, skip_, total_ = 0, copied_ = 0;
};

enum class BackupPart : uint8_t { Prefix, Profile, Bar, End, Done };

// A request owns its cursor; the existing API lock owns the shared profile/PB
// workspaces. Cache one record until all of its bytes have been transmitted.
// Each callback loads at most one filesystem record, regardless of chunk size.
template <typename Writer, typename Window = ByteWindow> class BackupStream {
public:
  explicit BackupStream(uint8_t profiles) : profiles_(profiles) {}
  size_t fill(uint8_t *buffer, size_t capacity) {
    if (!capacity || part_ == BackupPart::Done) return 0;
    if (!loaded_) {
      writer_.load(part_, profile_, zone_);
      loaded_ = true;
    }
    Window out(buffer, capacity, offset_);
    writer_.render(out, part_, profile_, zone_);
    offset_ += out.copied();
    if (offset_ == out.total()) {
      advance();
      offset_ = 0;
      loaded_ = false;
    }
    return out.copied();
  }
private:
  void advance() {
    switch (part_) {
    case BackupPart::Prefix:
      part_ = profiles_ ? BackupPart::Profile : BackupPart::End;
      break;
    case BackupPart::Profile: part_ = BackupPart::Bar; break;
    case BackupPart::Bar:
      if (++zone_ == 4) {
        zone_ = 0;
        part_ = ++profile_ == profiles_ ? BackupPart::End : BackupPart::Profile;
      }
      break;
    default: part_ = BackupPart::Done; break;
    }
  }
  Writer writer_;
  size_t offset_ = 0;
  uint8_t profiles_, profile_ = 0, zone_ = 0;
  BackupPart part_ = BackupPart::Prefix;
  bool loaded_ = false;
};
