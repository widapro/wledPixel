# v4.0-beta.4 — ESP8266 Weather Stability

This beta fixes ESP8266 restarts during OpenWeatherMap polling reported in [#86](https://github.com/widapro/wledPixel/issues/86). It also improves MQTT/web coordination and settings-backup streaming. All features from [v4.0-beta.3](RELEASE_NOTES_v4.0-beta.3.md), including profiles, scheduling and display rotation, are included.

**Beta release for ESP8266 and ESP32.** Save a settings backup before updating and choose the firmware file for your board.

## Fixes and improvements

- **ESP8266 weather polling:** BearSSL could start with too little contiguous RAM for its mandatory stack and SSL context, causing an abort or out-of-memory restart before the weather response arrived. TLS now uses a secondary IRAM heap for contexts and record buffers, with separate checks for available DRAM, IRAM and contiguous blocks. Flash-resident constants and the relocated MQTT serialization buffer free approximately **5 KB of DRAM**.
- **TLS record sizing:** A compact receive buffer is used only after the server confirms Maximum Fragment Length Negotiation (MFLN). Other servers require the full TLS-record buffer and its larger memory budget.
- **Weather cache and retries:** Responses are parsed as a filtered stream and a complete reading is validated before replacing the cache. Invalid keys, incomplete data and network errors preserve the last good reading. Busy or memory-constrained requests retry after a short delay; normal polling is timed from request completion.
- **MQTT and web requests:** On ESP8266, disconnects, buffer resizing and subscription changes run from the foreground loop. A web callback can no longer interrupt a socket write and reenter the same MQTT client. Request ownership also handles a browser arriving while MQTT resources are being released.
- **Consistent JSON responses:** Runtime MQTT status, signal strength and temperature are captured at the start of an API response so changes between chunks cannot append or truncate JSON.
- **Settings backups:** Streaming keeps a cursor and loads each profile/progress-bar record once, avoiding repeated filesystem traversal and watchdog resets during backup downloads. The backup comparison tool now handles the `inherited` metadata field consistently.
- **Diagnostics and regression tests:** `/api/diagnostics` adds successful-weather counters and ESP8266 IRAM statistics. Native tests cover request ownership, heap budgets, retry wraparound, filtered/atomic weather decoding and segmented backup output.

## Updating and compatibility

1. Download a settings backup before updating. Label it with the firmware version and keep it private; it contains credentials.
2. Install **`wledPixel-v4.0-beta.4_ESP8266.bin`** or **`wledPixel-v4.0-beta.4_ESP32.bin`** through the existing update workflow. These are application images, not merged ESP32 factory images. `SHA256SUMS` contains their checksums.
3. After reboot, check the displayed version, weather, MQTT/HA integrations and profiles. Save a new settings backup after confirming operation.

The settings format and partition layout are unchanged from v4.0-beta.3. Its backups remain supported. For older backups and downgrades, follow the [v4.0-beta.3 compatibility guidance](RELEASE_NOTES_v4.0-beta.3.md#settings-backups-and-compatibility); a settings backup does not contain firmware.

## Validation and beta limitations

- Native suites pass with AddressSanitizer and UndefinedBehaviorSanitizer; all **11 Python tests** pass. Normal and HIL builds pass for both ESP8266 and ESP32. Only normal firmware images are distributed.
- ESP8266 hardware regression testing completed **7 successful weather updates**, including **three complete 180-second polling intervals** with MQTT, HA, dynamic progress bars and web traffic, without an unexpected restart. **48 page/API responses**, including **3 complete backups**, passed validation. An actual HTTP 401 preserved the weather cache; restoring the valid key resumed updates. Original settings were restored and compared afterward.
- The final ESP8266 release image was installed through OTA, reported **v4.0-beta.4** after boot, and preserved all **243 exported settings keys** on the test device.
- Physical testing was performed on ESP8266 with OpenWeatherMap/MFLN. ESP32 and the larger-buffer TLS path were checked by builds/native tests; other HTTPS providers still need testing on your installation.
- ESP8266 remains memory-constrained. Web activity can temporarily disconnect MQTT and postpone background polling. Frequent browser polling may delay weather updates. The ESP8266 build requires the secondary-heap configuration in `platformio.ini`.
- The normal ESP32 build uses **96.7% of the default application partition** according to the PlatformIO size report.
- Existing [beta limitations](RELEASE_NOTES_v4.0-beta.3.md#beta-limitations), including ESP32 application-partition headroom and nontransactional full-settings restore, continue to apply. Refer to this release's TLS behavior above for the updated memory handling.
