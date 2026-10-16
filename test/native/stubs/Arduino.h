#pragma once

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iomanip>
#include <sstream>
#include <string>

using byte = uint8_t;

#ifndef PROGMEM
#define PROGMEM
#endif

#ifndef F
#define F(value) value
#endif

inline uint8_t pgm_read_byte(const void *address) {
  return *static_cast<const uint8_t *>(address);
}

class String {
public:
  String() = default;
  String(const char *value) : value_(value ? value : "") {}
  String(const std::string &value) : value_(value) {}
  String(char value) : value_(1, value) {}
  String(float value, unsigned int decimals) {
    std::ostringstream stream;
    stream << std::fixed << std::setprecision(decimals) << value;
    value_ = stream.str();
  }
  String(const String &) = default;
  String(String &&) = default;
  String &operator=(const String &) = default;
  String &operator=(String &&) = default;
  String &operator=(const char *value) {
    value_ = value ? value : "";
    return *this;
  }

  size_t length() const { return value_.size(); }
  const char *c_str() const { return value_.c_str(); }

  bool reserve(size_t size) {
    try {
      value_.reserve(size);
      return true;
    } catch (...) {
      return false;
    }
  }

  void trim() {
    auto first = std::find_if_not(value_.begin(), value_.end(), [](char ch) {
      return std::isspace(static_cast<unsigned char>(ch)) != 0;
    });
    auto last = std::find_if_not(value_.rbegin(), value_.rend(), [](char ch) {
                  return std::isspace(static_cast<unsigned char>(ch)) != 0;
                }).base();
    if (first >= last)
      value_.clear();
    else
      value_ = std::string(first, last);
  }

  int indexOf(char needle) const {
    size_t position = value_.find(needle);
    return position == std::string::npos ? -1 : static_cast<int>(position);
  }

  int indexOf(const char *needle) const {
    size_t position = value_.find(needle ? needle : "");
    return position == std::string::npos ? -1 : static_cast<int>(position);
  }

  String substring(unsigned int begin, unsigned int end) const {
    begin = std::min<unsigned int>(begin, value_.size());
    end = std::min<unsigned int>(end, value_.size());
    if (begin > end)
      std::swap(begin, end);
    return value_.substr(begin, end - begin);
  }

  String substring(unsigned int begin) const {
    begin = std::min<unsigned int>(begin, value_.size());
    return value_.substr(begin);
  }

  long toInt() const { return std::strtol(value_.c_str(), nullptr, 10); }

  void toUpperCase() {
    std::transform(value_.begin(), value_.end(), value_.begin(), [](char ch) {
      return static_cast<char>(std::toupper(static_cast<unsigned char>(ch)));
    });
  }

  void remove(unsigned int index) {
    if (index < value_.size())
      value_.erase(index);
  }

  char charAt(unsigned int index) const {
    return index < value_.size() ? value_[index] : '\0';
  }

  char operator[](unsigned int index) const { return charAt(index); }

  String &operator+=(const String &other) {
    value_ += other.value_;
    return *this;
  }
  String &operator+=(const char *other) {
    value_ += other ? other : "";
    return *this;
  }
  String &operator+=(char other) {
    value_ += other;
    return *this;
  }

  friend String operator+(const String &left, const String &right) {
    return String(left.value_ + right.value_);
  }
  friend String operator+(const String &left, const char *right) {
    return String(left.value_ + (right ? right : ""));
  }
  friend String operator+(const char *left, const String &right) {
    return String((left ? left : "") + right.value_);
  }

  friend bool operator==(const String &left, const String &right) {
    return left.value_ == right.value_;
  }
  friend bool operator==(const String &left, const char *right) {
    return left.value_ == (right ? right : "");
  }
  friend bool operator==(const char *left, const String &right) {
    return right == left;
  }
  friend bool operator!=(const String &left, const String &right) {
    return !(left == right);
  }
  friend bool operator!=(const String &left, const char *right) {
    return !(left == right);
  }

private:
  std::string value_;
};

struct TestSerial {
  template <typename... Args> void printf(const char *, Args...) {}
  template <typename T> void print(const T &) {}
  template <typename T> void println(const T &) {}
  void println() {}
};

inline TestSerial Serial;
inline void yield() {}
