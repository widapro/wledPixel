#!/usr/bin/env bash

set -euo pipefail

project_dir="$(cd "$(dirname "$0")/.." && pwd)"
mode="${1:-unit}"

# Keep all PlatformIO state inside the project so local test runs do not depend
# on permissions for ~/.platformio and do not write outside the repository.
export PLATFORMIO_CORE_DIR="${PLATFORMIO_CORE_DIR:-$project_dir/.pio-core}"
export PLATFORMIO_SETTING_ENABLE_TELEMETRY=no

run_unit_tests() {
  local build_dir
  build_dir="$(mktemp -d "${TMPDIR:-/tmp}/wledpixel-tests.XXXXXX")"
  trap 'rm -rf "$build_dir"' RETURN

  "${CXX:-c++}" \
    -std=c++17 \
    -Wall -Wextra -Werror -Wno-deprecated-declarations \
    -fsanitize=address,undefined \
    -fno-omit-frame-pointer \
    -I"$project_dir/test/native/stubs" \
    -I"$project_dir/src" \
    -I"$project_dir/lib/countdown" \
    -I"$project_dir/lib/stockTicker" \
    "$project_dir/test/native/test_main.cpp" \
    "$project_dir/lib/countdown/countdown.cpp" \
    "$project_dir/lib/stockTicker/stockTicker.cpp" \
    "$project_dir/src/SchedulerLogic.cpp" \
    "$project_dir/src/TextEncoding.cpp" \
    -o "$build_dir/native_tests"

  "$build_dir/native_tests"
  # Exercise the actual ArduinoJson parser, not the minimal network-test stub.
  local json_include="$project_dir/.pio/libdeps/d1_mini/ArduinoJson/src"
  if [[ ! -f "$json_include/ArduinoJson.h" ]]; then
    echo "Profile tests need ArduinoJson: run ./tools/test_local.sh build first" >&2
    return 1
  fi
  "${CXX:-c++}" -std=c++17 -Wall -Wextra -Werror \
    -fsanitize=address,undefined -fno-omit-frame-pointer \
    -I"$json_include" -I"$project_dir/test/native/stubs" -I"$project_dir/src" \
    "$project_dir/test/native/test_profiles.cpp" "$project_dir/src/SchedulerLogic.cpp" \
    -o "$build_dir/profile_tests"
  "$build_dir/profile_tests"
  "${CXX:-c++}" -std=c++17 -Wall -Wextra -Werror \
    -fsanitize=address,undefined -fno-omit-frame-pointer \
    -I"$project_dir/test/native/storage_stubs" -I"$project_dir/test/native/stubs" -I"$project_dir/src" \
    "$project_dir/test/native/test_profile_storage.cpp" "$project_dir/src/SchedulerLogic.cpp" \
    -o "$build_dir/storage_tests"
  "$build_dir/storage_tests"
  "${CXX:-c++}" -std=c++17 -Wall -Wextra -Werror \
    -fsanitize=address,undefined -fno-omit-frame-pointer \
    -I"$project_dir/test/native/storage_stubs" -I"$project_dir/test/native/stubs" -I"$project_dir/src" \
    "$project_dir/test/native/test_profile_files.cpp" "$project_dir/src/SchedulerLogic.cpp" \
    -o "$build_dir/profile_file_tests"
  "$build_dir/profile_file_tests"

  # Capture the real driver's SPI packets to check whole-chain rotation,
  # including spare driver buffers, partial frames and per-zone controls.
  local matrix_src="$project_dir/.pio/libdeps/d1_mini/MD_MAX72XX/src"
  python3 - "$project_dir" "$matrix_src" "$build_dir" <<'PY'
import sys
from pathlib import Path
project, source, build = map(Path, sys.argv[1:])
sys.path.insert(0, str(project / 'tools'))
from patch_display_driver import patched_driver
(build / 'MD_MAX72xx.cpp').write_text(patched_driver(
    (source / 'MD_MAX72xx.cpp').read_text(), project / 'src/DisplayRotation.h'))
PY
  "${CXX:-c++}" -std=c++17 -Wall -Wextra -Werror -Wno-cpp -Wno-expansion-to-defined \
    -fsanitize=address,undefined -fno-omit-frame-pointer \
    -I"$project_dir/test/native/display_stubs" -I"$project_dir/src" -I"$matrix_src" \
    "$project_dir/test/native/test_display_rotation.cpp" \
    "$build_dir/MD_MAX72xx.cpp" "$matrix_src/MD_MAX72xx_buf.cpp" \
    "$matrix_src/MD_MAX72xx_pix.cpp" "$matrix_src/MD_MAX72xx_font.cpp" \
    -o "$build_dir/display_tests"
  "$build_dir/display_tests"

  python3 -m unittest discover -s "$project_dir/test/python" -v
}

run_firmware_builds() {
  (cd "$project_dir" && pio run -e esp32 -e d1_mini)
}

run_hil_firmware_builds() {
  (cd "$project_dir" && pio run -e esp32_hil -e d1_mini_hil)
}

case "$mode" in
  unit)
    run_unit_tests
    ;;
  build)
    run_firmware_builds
    ;;
  hil-build)
    run_hil_firmware_builds
    ;;
  all)
    run_unit_tests
    run_firmware_builds
    run_hil_firmware_builds
    ;;
  *)
    echo "Usage: $0 [unit|build|hil-build|all]" >&2
    exit 2
    ;;
esac
