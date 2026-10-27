import gzip
import re
import shutil
import subprocess
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
PAGES = {
    "index": "indexPage",
    "zone-settings": "zoneSettingsPage",
    "service-settings": "serviceSettingsPage",
    "ota": "otaPage",
    "backup": "backupPage",
}


class WebAssetTests(unittest.TestCase):
    def test_profile_source_id_byte_validation(self):
        html = (ROOT / "html" / "zone-settings.html").read_text(encoding="utf-8")
        start = html.index("    function validateProfileSourceIds(")
        end = html.index("    function saveProfile1(", start)
        script = html[start:end] + r'''
const assert = require('assert');
let fields = {}, warnings = 0, focused = false;
global.document = {getElementById: id => fields[id]};
global.alert = () => warnings++;
for (const prefix of ['p0', 'sp3']) {
  for (const field of ['Zmt', 'ZpbSrcI', 'ZpbCondSrcI', 'ZpbDynSrcI']) {
    for (const value of ['esp8266_meteo_dacha/sensor/total_power_consumed_today',
                         't'.repeat(127), 'я'.repeat(63)]) {
      fields = {[prefix + field + '0']: {value, focus() {focused = true;}}};
      assert(validateProfileSourceIds(prefix));
    }
    for (const value of ['t'.repeat(128), 'я'.repeat(64)]) {
      fields = {[prefix + field + '3']: {value, focus() {focused = true;}}};
      focused = false;
      const before = warnings;
      assert(!validateProfileSourceIds(prefix));
      assert(focused && warnings === before + 1);
      assert.strictEqual(fields[prefix + field + '3'].value, value);
    }
  }
}
'''
        result = subprocess.run(
            [shutil.which("node"), "-"], input=script, text=True,
            capture_output=True, check=False,
        )
        self.assertEqual(result.returncode, 0, result.stderr)

    def test_inline_javascript_has_valid_syntax(self):
        node = shutil.which("node")
        self.assertIsNotNone(node, "Node.js is required for JavaScript checks")

        for page_name in PAGES:
            html_path = ROOT / "html" / f"{page_name}.html"
            html = html_path.read_text(encoding="utf-8")
            scripts = re.findall(
                r"<script(?:\s[^>]*)?>(.*?)</script>",
                html,
                flags=re.IGNORECASE | re.DOTALL,
            )
            for script_index, script in enumerate(scripts, start=1):
                if not script.strip():
                    continue
                result = subprocess.run(
                    [node, "--check", "-"],
                    input=script,
                    text=True,
                    capture_output=True,
                    check=False,
                )
                self.assertEqual(
                    result.returncode,
                    0,
                    f"{html_path.name}, inline script {script_index}:\n"
                    f"{result.stderr}",
                )

    def test_generated_gzip_headers_match_html_sources(self):
        for page_name, variable_name in PAGES.items():
            html_path = ROOT / "html" / f"{page_name}.html"
            header_name = f"{variable_name[0].lower() + variable_name[1:]}.h"
            header_path = ROOT / "lib" / "htmlFiles" / header_name
            header = header_path.read_text(encoding="utf-8")
            match = re.search(
                rf"const uint8_t {re.escape(variable_name)}\[\] PROGMEM = \{{(.*?)\}};",
                header,
                flags=re.DOTALL,
            )
            self.assertIsNotNone(
                match, f"Array {variable_name} missing in {header_path}"
            )
            byte_values = bytes(
                int(value, 16)
                for value in re.findall(r"0x([0-9a-fA-F]{2})", match.group(1))
            )
            self.assertEqual(gzip.decompress(byte_values), html_path.read_bytes())


if __name__ == "__main__":
    unittest.main()
