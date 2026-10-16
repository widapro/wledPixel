import importlib.util
import pathlib
import unittest


PROJECT_ROOT = pathlib.Path(__file__).resolve().parents[2]
SPEC = importlib.util.spec_from_file_location(
    "hil_test_runner", PROJECT_ROOT / "tools" / "hil_test.py"
)
HIL = importlib.util.module_from_spec(SPEC)
assert SPEC.loader is not None
SPEC.loader.exec_module(HIL)


class HilRunnerTests(unittest.TestCase):
    def test_fnv1a_known_vector(self):
        self.assertEqual(HIL.fnv1a([]), 0x811C9DC5)
        self.assertEqual(HIL.fnv1a(list(b"hello")), 0x4F9F2CAB)

    def test_pattern_generation(self):
        self.assertEqual(HIL.expected_pattern("clear", 3), [0, 0, 0])
        self.assertEqual(HIL.expected_pattern("full", 3), [255, 255, 255])
        self.assertEqual(
            HIL.expected_pattern("checker", 4), [0xAA, 0x55, 0xAA, 0x55]
        )
        self.assertEqual(
            HIL.expected_pattern("diagonal", 10),
            [1, 2, 4, 8, 16, 32, 64, 128, 1, 2],
        )

    def test_framebuffer_validation(self):
        data = [0xAA, 0x55, 0xAA, 0x55]
        frame = {
            "rows": 8,
            "columns": len(data),
            "data": data,
            "checksum": HIL.fnv1a(data),
        }
        self.assertEqual(HIL.validate_framebuffer(frame), data)

        frame["checksum"] ^= 1
        with self.assertRaises(HIL.HilFailure):
            HIL.validate_framebuffer(frame)


if __name__ == "__main__":
    unittest.main()
