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
    def test_on_air_follows_applied_profile_with_scheduling_on_or_off(self):
        html = (ROOT / "html" / "zone-settings.html").read_text(encoding="utf-8")
        start = html.index("    function buildProfilesAccordion(")
        end = html.index("    function snapshotProfile(", start)
        script = html[start:end] + r'''
const assert = require('assert');
const accordion = {innerHTML: ''};
global.document = {getElementById: () => accordion};
global.LIVE_DATA = {};
global.PROFILE_SNAPSHOTS = {};
global.DOW = ['Su', 'Mo', 'Tu', 'We', 'Th', 'Fr', 'Sa'];
for (const name of ['WM_OPTS', 'FNT_OPTS', 'CLK_OPTS', 'OWM_OPTS', 'ALIGN_OPTS',
                    'EFFECT_OPTS', 'COUNTDOWN_FORMAT_OPTS', 'COUNTDOWN_FINISH_OPTS',
                    'STOCK_FORMAT_OPTS']) global[name] = [];
for (const name of ['spNumSel', 'spOpts', 'spSummary', 'profileProgressBarFields'])
  global[name] = () => '';
global.escH = global.escA = value => String(value ?? '');
global.spArr = (values, zone) => values[zone] || '';
global.spBits = () => -1;
global.spInputVal = value => value;
global.updateProfileUI = () => {};
global.snapshotProfile = () => ({});
global.profileRenderCount = () => SCHED_DATA.profileCount;
global.getProfileObj = index => new Proxy({name: 'test', enabled: false}, {
  get(target, key) {
    return key in target ? target[key] : key.startsWith('zone') ? ['', '', '', ''] : 0;
  }
});
for (const profileCount of [4, 8]) {
  for (const schedulerEnabled of [false, true]) {
    for (const lastActiveProfile of [-1, ...Array(profileCount).keys()]) {
      global.SCHED_DATA = {lastActiveProfile, schedulerEnabled, profileCount};
      buildProfilesAccordion();
      const headers = [...accordion.innerHTML.matchAll(/<h2\b[^>]*>(.*?)<\/h2>/gs)];
      assert.strictEqual(headers.length, profileCount);
      const active = headers.flatMap((header, index) => header[1].includes('ON AIR') ? [index] : []);
      assert.deepStrictEqual(active, [lastActiveProfile < 0 ? 0 : lastActiveProfile],
        JSON.stringify(SCHED_DATA));
    }
  }
}
'''
        result = subprocess.run(
            [shutil.which("node"), "-"], input=script, text=True,
            capture_output=True, check=False,
        )
        self.assertEqual(result.returncode, 0, result.stderr)

    def test_display_snapshot_geometry_pixels_and_manual_refresh(self):
        html = (ROOT / "html" / "index.html").read_text(encoding="utf-8")
        start = html.index("    const SNAPSHOT_COLORS =")
        end = html.index("  function preparePostRequest(", start)
        script = html[start:end]
        self.assertNotIn("setInterval", script)
        self.assertNotIn("requestAnimationFrame", script)
        script += r'''
const assert = require('assert');
let dots = [], outlines = [], fetchCount = 0, finishFetch;
const ctx = {
  fillRect() {}, strokeRect(...args) { outlines.push(args); }, beginPath() {},
  arc(...args) { this.point = args; },
  fill() { dots.push({point: this.point, color: this.fillStyle, glow: this.shadowBlur, shadow: this.shadowColor}); }
};
const canvas = {hidden: true, getContext: () => ctx, setAttribute() {}};
const button = {disabled: false, textContent: 'Refresh'};
const flipButton = {attributes: {}, setAttribute(name, value) { this.attributes[name] = value; }};
const status = {textContent: ''};
const swatches = ['red', 'yellowGreen', 'neonBlue', 'white'].map(value => ({value, checked: false}));
const savedPreferences = new Map();
global.localStorage = {
  getItem: key => savedPreferences.get(key),
  setItem: (key, value) => savedPreferences.set(key, value)
};
global.document = {querySelectorAll: () => swatches, getElementById: id => ({
  displaySnapshotCanvas: canvas, refreshDisplaySnapshot: button, displaySnapshotStatus: status,
  flipDisplaySnapshot: flipButton
})[id]};
global.setTimeout = () => 1;
global.clearTimeout = () => {};
const frame = {modules: 2, columns: 16, rows: 8, power: true, data: Array(16).fill(0),
  profileIndex: 2, profileName: 'Night'};
frame.data[0] = 1;
frame.data[15] = 128;
drawDisplaySnapshot(frame);
assert.strictEqual(dots.length, 128);
assert.strictEqual(outlines.length, 1);
assert.strictEqual(canvas.width, 224);
assert.strictEqual(canvas.height, 128);
assert(!canvas.hidden);
assert.strictEqual(dots.filter(dot => dot.glow > 0).length, 2);
assert.deepStrictEqual(dots[0].point.slice(0, 2), [22, 22]);
for (let column = 1; column < frame.columns; column++) {
  assert.strictEqual(dots[column * 8].point[0] - dots[(column - 1) * 8].point[0], 12,
      'pixel spacing changes at a module boundary');
}
assert(dots[0].glow > 0 && dots[127].glow > 0);
assert.strictEqual(dots[1].glow, 0);
assert.throws(() => drawDisplaySnapshot({...frame, data: [0]}));
assert.throws(() => drawDisplaySnapshot({...frame, modules: 17}));
assert.throws(() => drawDisplaySnapshot({...frame, data: Array(16).fill(256)}));
status.textContent = 'Snapshot at 12:00:00';
for (const [name, lit, glow] of [
  ['red', '#ff4a32', '#ff3018'], ['yellowGreen', '#d5f542', '#b3ed00'],
  ['neonBlue', '#54eaff', '#00cfff'], ['white', '#f3f7ff', '#dce9ff']
]) {
  dots = [];
  setDisplaySnapshotColor(name);
  assert.strictEqual(savedPreferences.get('wledPixel.snapshotColor'), name);
  assert.deepStrictEqual(swatches.filter(input => input.checked).map(input => input.value), [name]);
  assert.strictEqual(dots.length, 128, 'color did not redraw the cached snapshot');
  assert(dots.filter(dot => dot.glow > 0).every(dot => dot.color === lit && dot.shadow === glow));
  assert(dots.filter(dot => dot.glow === 0).every(dot => dot.color === '#252b34'));
  assert.strictEqual(status.textContent, 'Snapshot at 12:00:00', 'color changed the capture time');
  assert.strictEqual(fetchCount, 0, 'color change fetched a new frame');
}
savedPreferences.set('wledPixel.snapshotColor', 'neonBlue');
initDisplaySnapshotColor();
assert(swatches.find(input => input.value === 'neonBlue').checked);
savedPreferences.set('wledPixel.snapshotColor', 'constructor');
initDisplaySnapshotColor();
assert(swatches.find(input => input.value === 'red').checked);
global.localStorage = {getItem() { throw new Error('storage blocked'); }, setItem() { throw new Error('storage blocked'); }};
initDisplaySnapshotColor();
initDisplaySnapshotFlip();
setDisplaySnapshotColor('white');
setDisplaySnapshotFlip(true);
assert(swatches.find(input => input.value === 'white').checked);
assert.strictEqual(flipButton.attributes['aria-pressed'], 'true');
global.localStorage = {getItem: key => savedPreferences.get(key), setItem: (key, value) => savedPreferences.set(key, value)};
setDisplaySnapshotColor('red');
setDisplaySnapshotFlip(false);
const rotationFrame = {...frame, data: Array(16).fill(0)};
rotationFrame.data[1] = 1 << 2;
const originalData = rotationFrame.data.slice();
dots = [];
drawDisplaySnapshot(rotationFrame);
assert.deepStrictEqual(dots.find(dot => dot.glow > 0).point.slice(0, 2), [34, 46]);
dots = [];
setDisplaySnapshotFlip(true);
assert.strictEqual(flipButton.attributes['aria-pressed'], 'true');
assert.strictEqual(savedPreferences.get('wledPixel.snapshotFlip'), '1');
assert.strictEqual(dots.length, 128);
assert.deepStrictEqual(dots.find(dot => dot.glow > 0).point.slice(0, 2), [190, 82]);
assert.deepStrictEqual(rotationFrame.data, originalData, 'UI rotation mutated the source frame');
assert.strictEqual(status.textContent, 'Snapshot at 12:00:00', 'rotation changed the capture time');
assert.strictEqual(fetchCount, 0, 'UI rotation requested a new frame');
dots = [];
setDisplaySnapshotColor('neonBlue');
assert.deepStrictEqual(dots.find(dot => dot.glow > 0).point.slice(0, 2), [190, 82]);
assert.strictEqual(dots.find(dot => dot.glow > 0).color, '#54eaff');
savedPreferences.set('wledPixel.snapshotFlip', '1');
initDisplaySnapshotFlip();
assert.strictEqual(flipButton.attributes['aria-pressed'], 'true');
savedPreferences.set('wledPixel.snapshotFlip', 'invalid');
initDisplaySnapshotFlip();
assert.strictEqual(flipButton.attributes['aria-pressed'], 'false');
dots = [];
setDisplaySnapshotFlip(false);
drawDisplaySnapshot(rotationFrame);
assert.deepStrictEqual(dots.find(dot => dot.glow > 0).point.slice(0, 2), [34, 46]);
setDisplaySnapshotColor('red');
dots = []; outlines = [];
drawDisplaySnapshot({...frame, power: false});
assert(dots.every(dot => dot.glow === 0));
(async () => {
  global.fetch = (url, options) => {
    fetchCount++;
    assert.strictEqual(url, '/api/display/frame');
    assert.strictEqual(options.cache, 'no-store');
    return new Promise(resolve => { finishFetch = resolve; });
  };
  const first = refreshDisplaySnapshot();
  assert(button.disabled);
  await refreshDisplaySnapshot();
  assert.strictEqual(fetchCount, 1, 'duplicate refresh started a second request');
  setDisplaySnapshotFlip(true);
  dots = [];
  finishFetch({ok: true, json: async () => rotationFrame});
  await first;
  assert(!button.disabled);
  assert.strictEqual(button.textContent, 'Refresh');
  assert(status.textContent.startsWith('2 modules'));
  assert(status.textContent.includes(' · Active profile: 3 (Night) · Snapshot at '));
  assert.deepStrictEqual(dots.find(dot => dot.glow > 0).point.slice(0, 2), [190, 82],
      'a fresh snapshot did not retain the UI rotation');
  assert.strictEqual(fetchCount, 1, 'snapshot automatically refreshed');
  const width = canvas.width;
  global.fetch = async () => ({ok: false, status: 503});
  await refreshDisplaySnapshot();
  assert.strictEqual(canvas.width, width);
  assert(status.textContent.includes('Previous snapshot kept.'));
  assert(status.textContent.includes('Device is busy.'));
  assert(!button.disabled);
  for (const [profileIndex, profileName, expected] of [
    [0, 'Default', 'Active profile: 1 (Default)'],
    [7, '<b>Night</b>', 'Active profile: 8 (<b>Night</b>)'],
    [3, '', 'Active profile: 4 ·'],
    [-1, 'Unknown', null], [8, 'Invalid', null], [undefined, undefined, null]
  ]) {
    global.fetch = async () => ({ok: true, json: async () => ({...frame, profileIndex, profileName})});
    await refreshDisplaySnapshot();
    if (expected) assert(status.textContent.includes(expected), status.textContent);
    else assert(!status.textContent.includes('Active profile:'));
  }
  global.fetch = async () => ({ok: true, json: async () => ({
    modules: 1, columns: 8, rows: 8, power: false, data: Array(8).fill(255)
  })});
  dots = []; outlines = [];
  await refreshDisplaySnapshot();
  assert.strictEqual(outlines.length, 1);
  assert.strictEqual(dots.length, 64);
  assert.strictEqual(canvas.width, 128);
  assert(dots.every(dot => dot.glow === 0));
  assert(status.textContent.startsWith('1 module · Display off'));
})().catch(error => { console.error(error); process.exitCode = 1; });
'''
        result = subprocess.run(
            [shutil.which("node"), "-"], input=script, text=True,
            capture_output=True, check=False,
        )
        self.assertEqual(result.returncode, 0, result.stderr)

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
