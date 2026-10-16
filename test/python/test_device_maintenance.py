import importlib.util
from pathlib import Path
from tempfile import TemporaryDirectory
import unittest
from unittest.mock import patch


ROOT = Path(__file__).resolve().parents[2]
SPEC = importlib.util.spec_from_file_location('device_maintenance', ROOT / 'tools/device_maintenance.py')
MAINTENANCE = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(MAINTENANCE)


class FirmwareGuardTests(unittest.TestCase):
    def test_correct_platform_and_version(self):
        for platform in ('ESP8266', 'ESP32'):
            image = b'\xe9headerWLEDPIXEL_PLATFORM_' + platform.encode() + b'\0v4.0-beta.3\0'
            MAINTENANCE.validate_firmware(image, platform, 'v4.0-beta.3')

    def test_reject_wrong_platform_missing_marker_and_wrong_version(self):
        esp32 = b'\xe9WLEDPIXEL_PLATFORM_ESP32\0v4.0-beta.3\0'
        for image, platform, version in (
            (esp32, 'ESP8266', 'v4.0-beta.3'),
            (esp32, 'ESP32', 'v4.0-beta.2'),
            (esp32, 'ESP32', None),
            (esp32, 'unknown', 'v4.0-beta.3'),
            (b'\xe9ESP32\0v4.0-beta.3\0', 'ESP32', 'v4.0-beta.3'),
            (esp32[1:], 'ESP32', 'v4.0-beta.3'),
            (esp32 + b'WLEDPIXEL_PLATFORM_ESP8266', 'ESP32', 'v4.0-beta.3'),
            (esp32.replace(b'beta.3\0', b'beta.30\0'), 'ESP32', 'v4.0-beta.3'),
        ):
            with self.subTest(platform=platform, version=version, image=image):
                with self.assertRaises(RuntimeError):
                    MAINTENANCE.validate_firmware(image, platform, version)

    def test_rejected_image_never_starts_ota(self):
        class FakeDevice(MAINTENANCE.Device):
            def request(self, *args, **kwargs):
                return {'platform': 'ESP32'}

            def multipart(self, *args, **kwargs):
                raise AssertionError('An invalid image must never be uploaded')

        with TemporaryDirectory() as directory:
            image = Path(directory) / 'wrong.bin'
            image.write_bytes(b'\xe9WLEDPIXEL_PLATFORM_ESP8266\0v4.0-beta.3\0')
            with self.assertRaises(RuntimeError):
                FakeDevice('unused', 'v4.0-beta.3').flash(image)

    def test_legacy_unset_rule_does_not_relax_profile_or_v3_comparison(self):
        self.assertTrue(MAINTENANCE.legacy_empty_rule({}, 'pbDynR2ValZone2', None, ''))
        self.assertFalse(MAINTENANCE.legacy_empty_rule({'backupVersion': 3}, 'pbDynR2ValZone2', None, ''))
        self.assertFalse(MAINTENANCE.legacy_empty_rule({}, 'zoneHaSensorId', None, ''))
        self.assertFalse(MAINTENANCE.legacy_empty_rule({}, 'pbDynR2ValZone2', None, 'on'))

    def test_same_version_old_image_is_not_accepted_after_ota(self):
        class FakeDevice(MAINTENANCE.Device):
            diagnostics_calls = 0

            def request(self, route, **kwargs):
                if route == '/api/service-settings':
                    return {'firmwareVer': 'v4.0-beta.3'}
                self.diagnostics_calls += 1
                return {'bootComplete': True, 'sketchMD5': 'b' * 32 if self.diagnostics_calls == 1 else 'a' * 32}

        device = FakeDevice('unused', 'v4.0-beta.3')
        with patch.object(MAINTENANCE.time, 'sleep'):
            device.ready(sketch_md5='a' * 32)
        self.assertEqual(device.diagnostics_calls, 2)


if __name__ == '__main__':
    unittest.main()
