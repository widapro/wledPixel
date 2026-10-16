#!/usr/bin/env python3
"""Local Wi-Fi hardware-in-the-loop runner for WLED Pixel HIL firmware."""

from __future__ import annotations

import argparse
import json
import os
import shutil
import subprocess
import sys
import time
import urllib.error
import urllib.parse
import urllib.request
from typing import Any


class HilFailure(RuntimeError):
    pass


class DeviceApi:
    def __init__(self, host: str, timeout: float) -> None:
        if "://" not in host:
            host = "http://" + host
        self.base_url = host.rstrip("/")
        self.timeout = timeout

    def json_request(
        self,
        path: str,
        *,
        data: dict[str, str] | None = None,
        expected_status: tuple[int, ...] = (200,),
        busy_retries: int = 3,
    ) -> dict[str, Any]:
        encoded = None
        method = "GET"
        headers = {"Accept": "application/json", "Connection": "close"}
        if data is not None:
            encoded = urllib.parse.urlencode(data).encode("ascii")
            method = "POST"
            headers["Content-Type"] = "application/x-www-form-urlencoded"

        url = self.base_url + path
        for attempt in range(busy_retries + 1):
            request = urllib.request.Request(
                url, data=encoded, headers=headers, method=method
            )
            try:
                with urllib.request.urlopen(request, timeout=self.timeout) as response:
                    status = response.status
                    raw = response.read().decode("utf-8")
            except urllib.error.HTTPError as exc:
                status = exc.code
                raw = exc.read().decode("utf-8", errors="replace")
                if status in (429, 503) and attempt < busy_retries:
                    retry_after = float(exc.headers.get("Retry-After", "1"))
                    time.sleep(max(retry_after, 0.2))
                    continue
            except (urllib.error.URLError, TimeoutError, OSError) as exc:
                raise HilFailure(f"{method} {url}: {exc}") from exc

            if status not in expected_status:
                raise HilFailure(
                    f"{method} {url}: HTTP {status}, expected "
                    f"{expected_status}; response={raw[:240]!r}"
                )
            try:
                result = json.loads(raw)
            except json.JSONDecodeError as exc:
                raise HilFailure(f"{method} {url}: invalid JSON: {raw[:240]!r}") from exc
            if not isinstance(result, dict):
                raise HilFailure(f"{method} {url}: JSON root is not an object")
            return result

        raise HilFailure(f"{method} {url}: device remained busy")

    def get(self, path: str) -> dict[str, Any]:
        return self.json_request(path)

    def post(self, path: str, data: dict[str, str]) -> dict[str, Any]:
        return self.json_request(path, data=data, expected_status=(200, 202))


def check(condition: bool, message: str) -> None:
    if not condition:
        raise HilFailure(message)


def fnv1a(data: list[int]) -> int:
    value = 2166136261
    for byte in data:
        value ^= byte
        value = (value * 16777619) & 0xFFFFFFFF
    return value


def expected_pattern(name: str, columns: int) -> list[int]:
    if name == "clear":
        return [0] * columns
    if name == "full":
        return [255] * columns
    if name == "checker":
        return [0xAA if column % 2 == 0 else 0x55 for column in range(columns)]
    if name == "diagonal":
        return [1 << (column % 8) for column in range(columns)]
    raise HilFailure(f"unknown local pattern {name!r}")


def validate_framebuffer(frame: dict[str, Any]) -> list[int]:
    check(frame.get("rows") == 8, "framebuffer must report exactly 8 rows")
    columns = frame.get("columns")
    data = frame.get("data")
    check(isinstance(columns, int) and columns > 0, "invalid framebuffer width")
    check(isinstance(data, list), "framebuffer data is not an array")
    check(len(data) == columns, "framebuffer data length does not match columns")
    check(
        all(isinstance(value, int) and 0 <= value <= 255 for value in data),
        "framebuffer contains a value outside 0..255",
    )
    check(frame.get("checksum") == fnv1a(data), "framebuffer checksum mismatch")
    return data


def publish_mqtt(args: argparse.Namespace, topic: str, payload: str) -> None:
    executable = shutil.which("mosquitto_pub")
    if not executable:
        raise HilFailure("--mqtt-broker requires the local mosquitto_pub command")

    command = [
        executable,
        "-h",
        args.mqtt_broker,
        "-p",
        str(args.mqtt_port),
        "-t",
        topic,
        "-m",
        payload,
    ]
    if args.mqtt_username:
        command.extend(["-u", args.mqtt_username])
    if args.mqtt_password:
        command.extend(["-P", args.mqtt_password])
    try:
        subprocess.run(command, check=True, capture_output=True, text=True, timeout=10)
    except (subprocess.CalledProcessError, subprocess.TimeoutExpired) as exc:
        stderr = getattr(exc, "stderr", "") or ""
        raise HilFailure(f"mosquitto_pub failed: {stderr.strip()}") from exc


def test_mqtt_power(
    api: DeviceApi,
    args: argparse.Namespace,
    prefix: str,
    original_power: bool,
) -> None:
    topic = prefix + "/power"
    changed_power = not original_power
    print(f"[HIL] MQTT power command through {topic}")

    changed_command_sent = False
    try:
        # ESP8266 temporarily disconnects MQTT while serving HTTP. Give it time
        # to leave web-priority mode and reconnect before every publish.
        time.sleep(args.mqtt_settle)
        publish_mqtt(args, topic, "on" if changed_power else "off")
        changed_command_sent = True
        time.sleep(1.0)
        changed_status = api.get("/api/hil/status")
        check(
            changed_status.get("power") is changed_power,
            "MQTT power command was not reflected by the firmware",
        )
    finally:
        if changed_command_sent:
            time.sleep(args.mqtt_settle)
            publish_mqtt(args, topic, "on" if original_power else "off")

    time.sleep(1.0)
    restored_status = api.get("/api/hil/status")
    check(
        restored_status.get("power") is original_power,
        "failed to restore power state after MQTT test",
    )
    print("[PASS] MQTT power command and restoration")


def run(args: argparse.Namespace) -> None:
    api = DeviceApi(args.host, args.timeout)
    print(f"[HIL] Device: {api.base_url}")

    status = api.get("/api/hil/status")
    check(status.get("hil") is True, "device is not running a HIL build")
    check(status.get("build") == "WLEDPIXEL_HIL_TEST", "HIL build marker mismatch")
    check(status.get("platform") in ("ESP8266", "ESP32"), "unknown platform")
    if args.platform:
        check(status.get("platform") == args.platform, "unexpected device platform")
    check(status.get("wifiConnected") is True, "device does not report Wi-Fi connected")
    check(isinstance(status.get("heap"), int) and status["heap"] > 0, "invalid heap")
    check(status.get("displayColumns", 0) > 0, "display driver has no columns")
    print(
        f"[PASS] HIL identity: {status['platform']} {status.get('firmware', '')}, "
        f"IP {status.get('ip', '?')}"
    )

    diagnostics = api.get("/api/diagnostics")
    check(diagnostics.get("heap", 0) > 0, "diagnostics returned invalid heap")
    print(f"[PASS] diagnostics API: heap={diagnostics['heap']}")

    zone_settings = api.get("/api/zone-settings")
    check(zone_settings.get("platform") == status["platform"], "API platform mismatch")
    check(zone_settings.get("zoneNumbers") == status["zoneCount"], "zone count mismatch")
    mqtt_prefix = zone_settings.get("mqttDevicePrefix")
    check(isinstance(mqtt_prefix, str) and mqtt_prefix, "MQTT prefix is missing")
    print(f"[PASS] zone settings API: {status['zoneCount']} zone(s)")

    framebuffer = api.get("/api/hil/framebuffer")
    validate_framebuffer(framebuffer)
    check(
        framebuffer["columns"] == status["displayColumns"],
        "framebuffer and status widths differ",
    )
    print(
        f"[PASS] framebuffer snapshot: {framebuffer['columns']} columns, "
        f"checksum={framebuffer['checksum']}"
    )

    initial_heap = status["heap"]
    heaps: list[int] = []
    for _ in range(args.requests):
        sample = api.get("/api/hil/status")
        check(sample.get("hil") is True, "HIL status disappeared during repeat test")
        heaps.append(sample["heap"])
    final_heap = heaps[-1] if heaps else initial_heap
    print(
        f"[PASS] repeated HTTP requests: {args.requests}; "
        f"heap {initial_heap} -> {final_heap}"
    )
    if final_heap < initial_heap - args.heap_drift_warning:
        print(
            f"[WARN] heap fell by {initial_heap - final_heap} bytes "
            f"(warning threshold {args.heap_drift_warning})"
        )

    try:
        if not args.skip_display:
            settle = args.pattern_settle
            if settle is None:
                settle = 1.8 if status["platform"] == "ESP8266" else 0.2

            for pattern in ("checker", "diagonal"):
                response = api.post(
                    "/api/hil/pattern",
                    {"pattern": pattern, "holdMs": str(args.pattern_hold_ms)},
                )
                check(response.get("accepted") is True, f"{pattern} was not accepted")
                time.sleep(settle)
                frame = api.get("/api/hil/framebuffer")
                actual = validate_framebuffer(frame)
                expected = expected_pattern(pattern, frame["columns"])
                check(actual == expected, f"{pattern} framebuffer differs from expected")
                print(f"[PASS] display pattern: {pattern}")
                if args.visual_seconds > 0:
                    print(
                        f"[HIL] Observe the {pattern} pattern for "
                        f"{args.visual_seconds:g} seconds"
                    )
                    time.sleep(args.visual_seconds)
    finally:
        try:
            api.post("/api/hil/release", {})
        except HilFailure as exc:
            print(f"[WARN] could not release display hold: {exc}", file=sys.stderr)

    if args.mqtt_broker:
        test_mqtt_power(api, args, mqtt_prefix, bool(status["power"]))

    print("[PASS] HIL test suite completed")


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Run local Wi-Fi HIL tests against WLED Pixel HIL firmware"
    )
    parser.add_argument("--host", required=True, help="device IP, hostname, or base URL")
    parser.add_argument("--platform", choices=("ESP8266", "ESP32"))
    parser.add_argument("--timeout", type=float, default=5.0)
    parser.add_argument(
        "--requests", type=int, default=5, help="number of repeated status requests"
    )
    parser.add_argument("--heap-drift-warning", type=int, default=4096)
    parser.add_argument("--skip-display", action="store_true")
    parser.add_argument("--pattern-hold-ms", type=int, default=8000)
    parser.add_argument(
        "--visual-seconds",
        type=float,
        default=0.0,
        help="keep each verified pattern visible for manual inspection",
    )
    parser.add_argument(
        "--pattern-settle",
        type=float,
        help="seconds before reading a queued pattern (auto by platform)",
    )
    parser.add_argument("--mqtt-broker", help="optionally test MQTT power through this broker")
    parser.add_argument("--mqtt-port", type=int, default=1883)
    parser.add_argument(
        "--mqtt-username", default=os.environ.get("WLED_HIL_MQTT_USERNAME")
    )
    parser.add_argument(
        "--mqtt-password", default=os.environ.get("WLED_HIL_MQTT_PASSWORD")
    )
    parser.add_argument(
        "--mqtt-settle",
        type=float,
        default=4.0,
        help="seconds allowed for MQTT reconnect after HTTP on ESP8266",
    )
    args = parser.parse_args()
    if args.requests < 0:
        parser.error("--requests must be non-negative")
    if not 1000 <= args.pattern_hold_ms <= 30000:
        parser.error("--pattern-hold-ms must be in 1000..30000")
    if args.visual_seconds < 0:
        parser.error("--visual-seconds must be non-negative")
    if args.visual_seconds * 1000 + 1000 > args.pattern_hold_ms:
        parser.error("--pattern-hold-ms is too short for --visual-seconds")
    return args


def main() -> int:
    try:
        run(parse_args())
    except HilFailure as exc:
        print(f"[FAIL] {exc}", file=sys.stderr)
        return 1
    except KeyboardInterrupt:
        print("[FAIL] interrupted", file=sys.stderr)
        return 130
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
