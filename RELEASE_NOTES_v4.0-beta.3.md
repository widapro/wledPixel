# v4.0-beta.3 — Profiles, Scheduling & Display Rotation

This beta brings display profiles, time-based scheduling, animated progress bars and a global display rotation setting to wledPixel. It includes all changes since [v3.6](https://github.com/widapro/wledPixel/releases/tag/v3.6).

**Beta release for ESP8266 and ESP32.** Save a settings backup before updating and use the firmware file for your board.

## New features

### Display profiles and scheduling

- The **Profiles** page replaces Zone Settings. Store **4 profiles on ESP8266** or **8 on ESP32**, including the default profile.
- Keep a default baseline and create profiles that override selected settings. **No change** leaves the current setting in place.
- Schedule profiles by hour, minute, days of the week and days of the month, using the device's configured NTP time and time-zone offset. Scheduling waits for a successful NTP sync; the page shows sync status and provides **Force NTP Sync**.
- Each profile can control power, brightness, zone count and layout, work modes, fonts, clock/weather formats, scrolling, MQTT/HA sources, countdown settings, stock ticker settings and per-zone progress bars.
- Use **Apply Now** to save and activate a profile immediately. Saving alone does not activate it. Layout changes can restart the device.
- The page shows the active profile and schedule status. To return to the baseline after another profile runs, apply or schedule the default profile explicitly.

### Animated progress bars

- Choose **Regular** mode for numeric HA/MQTT values or time, or **Dynamic** mode for animated status indicators.
- Dynamic mode provides **16 animations**, including fill/erase, snakes, Larson scanner, heartbeat, wave, breathing, bouncing pixel, comet, firework, Morse SOS, alternating flash and stack builder.
- Match an HA entity or MQTT topic against two configurable values, each with its own animation and speed. With neither value configured, Rule 1 runs continuously; configured rules hide the bar when no value matches.
- Store complete progress-bar settings independently for each profile and zone, including sources, limits, conditions and animation rules.

### Rotation and clocks

- **Service Settings → System settings → Rotate display 180°** rotates the entire display, including zone placement, text, animations and progress bars. Apply the setting to save it. Rotation is global, survives reboot and is shared by all profiles.
- Added **12-hour clock formats**, with or without am/pm, alongside the existing 24-hour formats, as requested in [#86 — AM/PM Clock](https://github.com/widapro/wledPixel/issues/86).

## Improvements and fixes

- Profile editing now uses isolated drafts and one editing session at a time. Competing editors are rejected, abandoned sessions expire, and cancelled edits leave the saved profile intact.
- A profile and its progress bars are committed together. The UI waits for storage completion and reports write failures before confirming a save or applying the profile.
- ESP32 profiles and progress-bar settings use compact LittleFS storage, keeping the NVS settings partition available for device configuration. The application and filesystem partition layout is unchanged.
- Settings backups now include complete profile and progress-bar settings, rotation, dynamic rules and more precise numeric values. Restore preserves the distinction between an omitted setting, **No change** and an empty string. Unset live strings in older settings backups no longer become the literal text `null`.
- ESP8266 restore runs after upload and reports completion before reboot. Settings writes and reboots wait for outstanding work and HTTP responses; factory reset clears the new profile/bar storage and stock settings.
- ESP32 restore sizes its JSON workspace to the uploaded backup and skips unchanged profile records. Redundant live zone defaults and unset progress-bar strings no longer consume NVS entries, preventing profile application from losing brightness changes on a full settings partition.
- The ESP8266 web interface takes priority over background requests, uses smaller shared workspaces and retries busy/network responses. Pages are served without caching to avoid stale controls after firmware updates.
- Display-off mode pauses animation and sensor/provider polling while keeping web/OTA, MQTT control, NTP and scheduling available. Waking refreshes data, and scheduled wake avoids clearing the visible frame during reconfiguration.
- Scheduled power/brightness changes clear stale retained MQTT commands so reconnects do not undo the scheduled state. Brightness updates while off keep the display off.
- HA entity IDs in profiles accept up to **63 characters**; missing HA entities produce clearer diagnostics.
- DS18B20 conversion no longer blocks the main loop for the full conversion interval, and fresh readings update all relevant zones. Countdown input validation, UTF-8/font conversion, base64 handling and stock-fetch allocation failures were also hardened.
- Added `/api/diagnostics`, pinned build dependencies and reproducible compressed HTML generation.

## Settings backups and compatibility

A backup is a **JSON copy of exported settings**. It contains no firmware image, and restoring it does not install or roll back firmware.

The settings format depends on the firmware version that created the backup. Keep a separate settings backup for each firmware version and label the file with that version; the exact firmware version is not currently recorded in the JSON.

- **v3.6 backup → this release:** supported for the settings present in the file. Settings introduced in this release cannot be recovered from an older backup that does not contain them.
- **This release's backup → v3.6 or older firmware:** full restoration is not supported. Older firmware does not understand the new settings and may reject the larger file. To downgrade, flash the target firmware separately and restore a settings backup created by that version.

## Updating

1. Download a settings backup before updating, label it with your current firmware version and keep it somewhere safe; it contains credentials.
2. Flash `wledPixel-v4.0-beta.3_ESP8266.bin` or `wledPixel-v4.0-beta.3_ESP32.bin` through the existing update workflow. These are application images, not merged ESP32 factory images.
3. After reboot, check Profiles, NTP sync, integrations and progress bars. Create a new settings backup for this firmware version after confirming the settings. Keep the pre-update settings backup and the corresponding firmware file separately for rollback.

## Beta limitations

- The default ESP32 application partition is **96.4% used**, with **47,307 bytes** remaining. The partition layout is unchanged.
- ESP8266 has limited free heap. Opening pages or making TLS requests may temporarily disconnect MQTT; frequent browser polling can delay background work. TLS buffers were reduced, so test compatibility with your HTTPS services.
- ESP8266 no longer publishes new HA discovery entries for work mode and scroll-in/out effects. Their MQTT command topics remain available; existing retained discovery entries are not removed. Incoming MQTT payloads are limited to **512 bytes**.
- Restoring all settings is not a single transaction. A late failure may leave some settings restored; atomic storage applies to an individual profile with its progress bars.
