# v4.0.1 — Fix truncated MQTT topics

**Bug-fix release for ESP8266 and ESP32.** MQTT topics saved in display profiles
now support up to **127 UTF-8 bytes**. Previously, they were silently cut to
31 bytes, so longer topics stopped receiving messages after saving or applying
a profile.

## Fixed

- Expanded profile MQTT topics from 31 to **127 bytes**.
- Expanded all three progress-bar source IDs from 47 to **127 bytes**, covering
  regular data, conditions and dynamic animation sources for MQTT and HA.
- Added length validation to the profile editor, configuration APIs and backup
  restore. Overlong values are rejected instead of silently truncated; the
  editor keeps the entered text so it can be corrected.
- Added a hint for profile topics that may have been truncated by older
  firmware.
- Preserved existing profiles and progress-bar settings through migration of
  older raw and compressed storage records on both platforms.

## Updating

1. Download and keep a private settings backup before updating.
2. Install **`wledPixel-v4.0.1_ESP8266.bin`** or
   **`wledPixel-v4.0.1_ESP32.bin`** for your board through the firmware-update
   page. These are application images, not merged ESP32 factory images.
   Check the download against **`SHA256SUMS`**.
3. Confirm **v4.0.1** after reboot. If a topic was already truncated, enter its
   **full value again and save the profile**. Missing characters cannot be
   recovered from the stored prefix.

Existing v4.0 and beta profiles and settings backups remain supported. The
backup JSON format is unchanged. Keep a backup made with the older firmware if
you need to downgrade: install that firmware and restore its matching backup.

## Validation

- Native C++ tests with AddressSanitizer and UndefinedBehaviorSanitizer cover
  the reported topic, the 127-byte boundary, ASCII/UTF-8 overflow rejection,
  old-record migration, storage round trips and interrupted writes.
- Python and JavaScript tests check byte-length validation, web assets, OTA
  guards and backup comparison. Normal and HIL builds pass for both platforms;
  only normal firmware images are distributed.
- The fix was tested on physical ESP8266 and ESP32 devices: full topics survived
  saving, backup export and reboot, and real messages on 127-byte MQTT topics
  reached data, condition and dynamic animation sources. Original settings
  were restored and compared with their backups after testing.

All other v4.0 features remain available; see the
[v4.0 release notes](https://github.com/widapro/wledPixel/releases/tag/v4.0)
for the full overview.
