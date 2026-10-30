# Local tests

The test suite does not require a connected board or additional Python packages.
Install PlatformIO dependencies once with `./tools/test_local.sh build` before
running the profile tests: these use the real ArduinoJson headers from `.pio`.
A C++ compiler, Python 3, and Node.js must be available.

Display rotation tests compile the pinned MD_MAX72XX driver with the same output
hook as the firmware and capture both hardware and software SPI. They check
whole-chain 180° rotation, partial pixel updates, progress-bar rows, per-zone
control registers and preservation of the logical framebuffer with sanitizers.

```sh
# Fast native C++ tests plus HTML/JavaScript checks
./tools/test_local.sh unit

# Compile firmware for both supported boards
./tools/test_local.sh build

# Run everything
./tools/test_local.sh all

# Compile separate HIL_TEST images for both supported boards
./tools/test_local.sh hil-build
```

Native tests compile the production countdown, base64, stock ticker,
scheduler-matching, profile migration/validation/JSON restoration, and text-encoding code with AddressSanitizer and
UndefinedBehaviorSanitizer enabled. Temporary binaries are built under the
system temporary directory and removed automatically.

Weather/network tests additionally execute the production request guard and
decoder. They cover reentrant calls, a browser arriving during MQTT release,
the observed ESP8266 heap failures, fragmented heaps, retry timing across millis
wraparound, large unused JSON fields, and invalid/incomplete readings preserving
the previous snapshot. Segmented backup tests cover 4/8 profiles, stored/null
bars, UTF-8 and 1-byte through large chunks. They assert that a record is loaded
once and that each callback loads at most one record, preventing repeated
filesystem traversal inside the async response callback.

PlatformIO state remains under `.pio-core` in the project, so the script does
not write to `~/.platformio`.

The storage test runs the production `ProfileStorage` implementation against a
Preferences stub that interrupts individual writes. Reopening storage must yield
either the previous profile and all its progress bars, or the complete new set.
It covers legacy migration, both alternating banks, inherited bars and aborts.
ESP32 filesystem tests separately interrupt file creation, writes and atomic
renames, with NVS unable to accept new writes. They verify migration from old NVS
records, both banks, cancellation, corrupt commit pointers and factory cleanup.
The compact-record codec is checked against truncation, corruption and bounds.
MQTT regression tests cover the reported long topic, the 127-byte boundary,
ASCII/UTF-8 overflow rejection, and migration of V2/V3 profiles and V1 bars.
Migration is exercised for all banks, with raw ESP8266 records and both raw and
compressed ESP32 records, including filesystem storage and a save/reload cycle.

## Profile integration checks

`tools/device_maintenance.py` supports explicit `flash`, `restore`, `verify`
and `profiles` actions with `--host` on ESP8266 and ESP32. Set
`--version v4.0` to check the expected version after boot; it is required
for `flash`, which checks the device platform, embedded platform marker and
version before uploading any chunk. For ESP32, it also waits for the running
sketch MD5 to match the exact uploaded
image, so an older image with the same version cannot satisfy OTA verification.
Store the complete version 3 backup outside
the repository: it contains credentials. The `profiles` action needs HIL firmware
and a `--backup` path. It temporarily changes profile 3, brightness and scheduling,
tests competing editors, cancellation, commit, reboot, Apply Now and real NTP
scheduling, then restores and compares the saved configuration. Keep the backup
even if a test fails; interrupted network access can prevent automatic cleanup.

The `manual-zones --backup /private/path/backup.json` action works with normal
firmware and `/api/display/frame`. It temporarily uses the last profile and two
zones to check empty Manual/MQTT buffers after profile application and reboot,
clock-to-Manual transitions, new input and the service-message setting. It
restores and verifies the original configuration in cleanup. Save a fresh
complete backup before running it; the initial configuration must match it.

For API clients, begin with `POST /api/scheduler/edit?idx=N` and a random
`X-Profile-Edit` header (up to 24 characters). Send the same header on all profile
and progress-bar writes. Stage progress bars before the final `cronProfile`
commit, then poll scheduler metadata until `storagePending` is false and check
`storageError`. Cancel with `POST /api/scheduler/edit-cancel?idx=N`. Only one
editor may own a session; inactive sessions expire after 90 seconds. These tokens
prevent mixed edits; they are not authentication.

ESP8266 restore returns HTTP 202 after upload. Poll `GET /api/restore-status`:
1 means queued, 2 running, 3 successful and 4 failed. HTTP 429/503 means retry
later. Success schedules a reboot. Wait for diagnostics `bootComplete` before
comparing the restored backup; an HTTP response alone does not mean boot finished.
Profile commits are atomic as a group with their bars; the entire configuration
restore is not one transaction.

## Tests on a device

For the OWM regression, use a normal ESP8266 build with the secondary IRAM heap
flag from `platformio.ini`. Set a valid OWM key directly in the device UI, save a
complete settings backup outside the repository, and enable a weather zone.
`/api/diagnostics` reports `owmUpdates`, `owmLastSuccessMs`, `iramHeap`, and
`iramMaxBlock` in addition to DRAM and uptime. Check several complete 180-second
intervals: both OWM counters must advance without an uptime reset. Repeat with
MQTT, HA progress bars and web page/API requests; leave gaps between page loads
because web priority intentionally postpones background polling. A deliberate
401 must leave the last successful timestamp unchanged, and restoring the valid
key must resume updates. Restore and verify the saved configuration afterward.
UART at 115200 provides TLS budgets and parsing/error results. Treat UART logs
and settings backups as private because configuration logging can contain keys.
Start a settings/API request while MQTT is connected and validate the complete
JSON response, including the next request after web priority disconnects MQTT.
Runtime values must stay fixed across response chunks; changing `true` to
`false` during serialization must not append or truncate bytes. MQTT disconnects
and subscription changes on ESP8266 must run from the foreground loop.

ESP8266 uses a 16KB instruction cache and the remaining 48KB IRAM region for
code and a secondary heap. TLS still requires its stack in DRAM. Build both
normal and HIL environments after changing these flags; ESP32 retains its
existing memory configuration. HTTP/1.0 is used for streamed JSON so chunked
transfer framing does not reach ArduinoJson.

The `d1_mini_hil` and `esp32_hil` environments define `WLED_HIL_TEST` and add
test-only APIs. Normal firmware builds do not expose them:

- `GET /api/hil/status` reports the HIL marker, platform, heap, connectivity,
  display geometry, and current test-pattern state.
- `GET /api/hil/framebuffer` returns all MAX72XX columns plus an FNV-1a
  checksum.
- `GET /api/hil/storage` (ESP32 only) reports NVS capacity and legacy profile
  record sizes without exposing configuration values.
- `POST /api/hil/pattern` queues a non-persistent `clear`, `full`, `checker`, or
  `diagonal` pattern. `POST /api/hil/release` resumes normal animation.

Build and upload the appropriate HIL environment locally. The first upload is
normally over USB; later HIL images can also be installed through OTA:

```sh
pio run -e d1_mini_hil -t upload
# or: pio run -e esp32_hil -t upload
```

Once the device is on Wi-Fi, run the local standard-library-only client:

```sh
./tools/hil_test.py --host display.example --platform ESP8266
```

It checks the HIL identity, diagnostics and zone APIs, framebuffer integrity,
repeat-request heap behavior, and deterministic checker/diagonal buffers. The
display test changes only runtime state and always requests release in cleanup;
it never saves configuration. Use `--skip-display` for read-only checks.
Add `--visual-seconds 5` to keep each verified pattern visible for a person to
inspect. A framebuffer match proves the firmware generated and sent the expected
columns, but the MAX72XX interface has no readback: a camera or visual check is
still needed to prove the LED modules, power, and wiring are physically working.

An optional MQTT power round-trip needs a local `mosquitto_pub` executable and
the broker already configured on the device:

```sh
./tools/hil_test.py --host display.example \
  --mqtt-broker mqtt.example
```

Replace `display.example` and `mqtt.example` with your device and broker addresses.
Broker credentials can be passed with `--mqtt-username` / `--mqtt-password`, or
through `WLED_HIL_MQTT_USERNAME` / `WLED_HIL_MQTT_PASSWORD`.
