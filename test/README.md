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

## Profile integration checks

`tools/device_maintenance.py` supports explicit `flash`, `restore`, `verify`
and `profiles` actions with `--host` on ESP8266 and ESP32. Set
`--version v4.0-beta.3` to check the expected version after boot; it is required
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
