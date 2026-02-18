#!/usr/bin/env bash

set -euo pipefail
shopt -s nullglob nocasematch

# =============================
# Defaults
# =============================

BAUD=460800
FLASH_MODE="dio"
FLASH_FREQ="40m"
FLASH_SIZE="detect"
FW_DIR="./firmware"
LOG_DIR="./logs"
ERASE=0
DEVICE=""
VERSION=""
FW=""

# =============================
# Colors
# =============================

if [[ -t 1 ]]; then
    RED="\033[31m"
    GREEN="\033[32m"
    YELLOW="\033[33m"
    BLUE="\033[34m"
    NC="\033[0m"
else
    RED=""; GREEN=""; YELLOW=""; BLUE=""; NC=""
fi

info()    { echo -e "${BLUE}[INFO]${NC} $1"; }
warn()    { echo -e "${YELLOW}[WARN]${NC} $1"; }
success() { echo -e "${GREEN}[OK]${NC} $1"; }
error()   { echo -e "${RED}[ERROR]${NC} $1" >&2; }
die()     { error "$1"; exit 1; }

# =============================
# Help
# =============================

show_help() {
cat <<EOF
ESP Auto Flash Tool with Menu

Usage:
  $0 [OPTIONS]

Options:
  -d <device>     Serial device (ttyUSB0, /dev/ttyACM0)
  -v <version>    Firmware version (e.g. 2.7)
  -f <file>       Firmware file (override auto selection)
  -b <baud>       Baud rate (default: $BAUD)
  -e              Erase flash before writing
  -h              Show help

Firmware naming:
  *-v<VERSION>_ESP8266.bin
  *-v<VERSION>_ESP32.bin

Interactive menus appear if device, version, or firmware are missing.

EOF
exit 0
}

# =============================
# Parse Options
# =============================

while getopts ":d:v:f:b:eh" opt; do
    case "$opt" in
        d)
            [[ "$OPTARG" != /dev/* ]] && DEVICE="/dev/$OPTARG" || DEVICE="$OPTARG"
            ;;
        v) VERSION="$OPTARG" ;;
        f) FW="$OPTARG" ;;
        b) BAUD="$OPTARG" ;;
        e) ERASE=1 ;;
        h) show_help ;;
        \?) die "Invalid option: -$OPTARG" ;;
        :) die "Option -$OPTARG requires argument" ;;
    esac
done

# =============================
# Logging Setup
# =============================

command -v esptool >/dev/null 2>&1 || die "esptool not found"

mkdir -p "$LOG_DIR"
LOG_FILE="$LOG_DIR/flash_$(date +%Y%m%d_%H%M%S).log"
exec > >(tee -a "$LOG_FILE") 2>&1

info "Log file: $LOG_FILE"

# =============================
# Device Selection
# =============================

DEVICES=(/dev/ttyUSB* /dev/ttyACM*)

select_device_menu() {
    (( ${#DEVICES[@]} > 0 )) || die "No serial devices found"
    echo "Available serial devices:"
    for i in "${!DEVICES[@]}"; do
        printf "%d) %s\n" "$((i+1))" "${DEVICES[$i]}"
    done
    echo
    read -rp "Select device: " choice
    [[ "$choice" =~ ^[0-9]+$ ]] || die "Invalid selection"
    DEVICE="${DEVICES[$((choice-1))]:-}"
    [[ -n "$DEVICE" ]] || die "Invalid selection"
}

[[ -z "$DEVICE" ]] && select_device_menu
[[ -e "$DEVICE" ]] || die "Device not found"

# =============================
# Detect Chip
# =============================

info "Detecting chip..."
CHIP_OUTPUT=$(esptool --port "$DEVICE" chip_id 2>&1 || true)

if [[ "$CHIP_OUTPUT" == *ESP8266* ]]; then
    CHIP="ESP8266"
    CHIP_ARG="esp8266"
elif [[ "$CHIP_OUTPUT" == *ESP32* ]]; then
    CHIP="ESP32"
    CHIP_ARG="esp32"
else
    die "Unable to detect chip"
fi
success "Detected $CHIP"

# =============================
# Version Selection Menu
# =============================

select_version_menu() {
    VERSIONS=()
    for f in "$FW_DIR"/*"_${CHIP}.bin"; do
        [[ -f "$f" ]] || continue
        v=$(basename "$f" | grep -oP '(?<=-v)[0-9]+\.[0-9]+')
        [[ -n "$v" ]] && VERSIONS+=("$v")
    done
    (( ${#VERSIONS[@]} > 0 )) || die "No firmware versions found for $CHIP"

    echo "Available firmware versions:"
    for i in "${!VERSIONS[@]}"; do
        printf "%d) %s\n" "$((i+1))" "${VERSIONS[$i]}"
    done
    echo
    read -rp "Select version: " choice
    [[ "$choice" =~ ^[0-9]+$ ]] || die "Invalid selection"
    VERSION="${VERSIONS[$((choice-1))]:-}"
    [[ -n "$VERSION" ]] || die "Invalid selection"
}

[[ -z "$VERSION" && -z "$FW" ]] && select_version_menu

# =============================
# Firmware Selection Menu
# =============================

select_firmware_menu() {
    CANDIDATES=()
    if [[ -n "$VERSION" ]]; then
        CANDIDATES=("$FW_DIR"/*"-v${VERSION}_${CHIP}.bin")
    else
        CANDIDATES=("$FW_DIR"/*"_${CHIP}.bin")
    fi
    (( ${#CANDIDATES[@]} > 0 )) || die "No firmware found"

    if (( ${#CANDIDATES[@]} == 1 )); then
        FW="${CANDIDATES[0]}"
    else
        echo "Multiple firmware files found:"
        for i in "${!CANDIDATES[@]}"; do
            printf "%d) %s\n" "$((i+1))" "${CANDIDATES[$i]}"
        done
        echo
        read -rp "Select firmware: " choice
        [[ "$choice" =~ ^[0-9]+$ ]] || die "Invalid selection"
        FW="${CANDIDATES[$((choice-1))]:-}"
        [[ -n "$FW" ]] || die "Invalid selection"
    fi
}

[[ -z "$FW" ]] && select_firmware_menu
[[ -f "$FW" ]] || die "Firmware file not found"

success "Selected firmware: $FW"

# =============================
# Optional Erase
# =============================

if [[ "$ERASE" -eq 1 ]]; then
    warn "Erasing flash..."
    esptool --chip "$CHIP_ARG" --port "$DEVICE" erase_flash
    success "Erase completed"
fi

# =============================
# Flash
# =============================

info "Flashing $CHIP on $DEVICE at $BAUD baud..."
esptool \
    --chip "$CHIP_ARG" \
    --port "$DEVICE" \
    --baud "$BAUD" \
    write_flash \
    -fm "$FLASH_MODE" \
    -ff "$FLASH_FREQ" \
    -fs "$FLASH_SIZE" \
    0x00000 "$FW"

success "Flash completed successfully"
