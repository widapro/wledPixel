import os
import shutil
import subprocess
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]


class NtpStatusTests(unittest.TestCase):
    def test_device_timezone_clock_sync_progress_and_failure(self):
        html = (ROOT / "html/service-settings.html").read_text(encoding="utf-8")
        start = html.index("    let ntpStatusData =")
        end = html.index("    function fetchWithRetry(", start)
        script = html[start:end] + r'''
const assert = require('assert');
const elements = {};
for (const id of ['ntpClockTime', 'ntpSyncStatus', 'ntpSyncNow', 'ntpSyncSpinner',
                  'ntpClockZone', 'ntpLastSync', 'ntpSyncMessage']) {
  elements[id] = {textContent: '', hidden: false, disabled: false, className: ''};
}
global.document = {getElementById: id => elements[id]};
let now = 0, scheduled = new Map(), timerId = 0;
global.performance = {now: () => now};
global.setTimeout = (callback, delay) => {
  scheduled.set(++timerId, {callback, delay}); return timerId;
};
global.clearTimeout = id => scheduled.delete(id);
const utc = Date.parse('2026-10-28T23:59:58Z') / 1000;
const sample = {state: 'synced', synced: true, configured: true,
  localEpoch: utc + 20700, utcOffsetSeconds: 20700, lastSyncUtc: utc};
let payload = sample, postResponse, requests = [];
global.fetch = async (url, options) => {
  requests.push({url, options});
  assert.strictEqual(options.cache, 'no-store');
  if (url === '/api/ntp-sync') {
    assert.strictEqual(options.method, 'POST');
    return new Promise(resolve => { postResponse = resolve; });
  }
  return {ok: true, json: async () => ({...payload})};
};
(async () => {
  // The test process uses a different local zone from the device.
  assert.strictEqual(formatNtpTime(sample.localEpoch), '2026-10-29 05:44:58');
  await refreshNtpStatus();
  assert.strictEqual(elements.ntpClockTime.textContent, '2026-10-29 05:44:58');
  assert.strictEqual(elements.ntpClockZone.textContent, '(UTC+05:45)');
  assert.strictEqual(elements.ntpLastSync.textContent, '2026-10-29 05:44:58');
  assert.strictEqual(elements.ntpSyncStatus.textContent, 'Clock synchronized');
  now = 2500;
  updateNtpClock();
  assert.strictEqual(elements.ntpClockTime.textContent, '2026-10-29 05:45:00');
  assert.strictEqual(elements.ntpLastSync.textContent, '2026-10-29 05:44:58');

  const sync = syncNtpNow();
  assert(elements.ntpSyncNow.disabled);
  assert(!elements.ntpSyncSpinner.hidden);
  assert.strictEqual(elements.ntpSyncNow.textContent, 'Syncing...');
  await syncNtpNow();
  assert.strictEqual(requests.filter(r => r.url === '/api/ntp-sync').length, 1);
  postResponse({ok: true, json: async () => ({state: 'syncing'})});
  await sync;
  assert(elements.ntpSyncNow.disabled, 'queued request shown as completed');
  assert.strictEqual(scheduled.get(ntpPollTimer).delay, 3000);
  payload = {...sample, state: 'syncing'};
  await refreshNtpStatus();
  assert(elements.ntpSyncNow.disabled);
  assert.strictEqual(elements.ntpSyncMessage.textContent, '');
  payload = {...sample, localEpoch: utc + 20710, lastSyncUtc: utc + 10};
  await refreshNtpStatus();
  assert(!elements.ntpSyncNow.disabled);
  assert(elements.ntpSyncSpinner.hidden);
  assert.strictEqual(elements.ntpLastSync.textContent, '2026-10-29 05:45:08');
  assert(elements.ntpSyncMessage.textContent.includes('completed successfully'));
  assert.strictEqual(scheduled.get(ntpPollTimer).delay, 10000);

  payload = {...sample, state: 'failed'};
  await refreshNtpStatus();
  assert.strictEqual(elements.ntpSyncStatus.textContent, 'Last synchronization failed');
  assert(elements.ntpSyncMessage.textContent.includes('clock continues'));
  assert.strictEqual(elements.ntpLastSync.textContent, '2026-10-29 05:44:58');
  assert(!elements.ntpSyncNow.disabled, 'failed sync cannot be retried');
  payload = {...sample, localEpoch: utc - 12600, utcOffsetSeconds: -12600};
  await refreshNtpStatus();
  assert.strictEqual(elements.ntpClockZone.textContent, '(UTC−03:30)');
  assert.strictEqual(elements.ntpClockTime.textContent, '2026-10-28 20:29:58');
  assert.strictEqual(elements.ntpLastSync.textContent, '2026-10-28 20:29:58');

  payload = {...sample, state: 'never', synced: false, localEpoch: 0, lastSyncUtc: 0};
  await refreshNtpStatus();
  assert.strictEqual(elements.ntpClockTime.textContent, '—');
  assert.strictEqual(elements.ntpLastSync.textContent, 'Never since restart');
  assert.strictEqual(elements.ntpSyncStatus.textContent, 'Clock not synchronized');
  payload = {...payload, configured: false};
  await refreshNtpStatus();
  assert(elements.ntpSyncNow.disabled);
  assert.strictEqual(elements.ntpSyncStatus.textContent, 'NTP server not configured');
  // A GET started before the button click must not overwrite its progress.
  payload = sample;
  await refreshNtpStatus();
  let staleResponse;
  const normalFetch = global.fetch;
  global.fetch = (url, options) => url === '/api/ntp-status'
    ? new Promise(resolve => { staleResponse = resolve; }) : normalFetch(url, options);
  const staleRefresh = refreshNtpStatus();
  const secondSync = syncNtpNow();
  postResponse({ok: true, json: async () => ({state: 'syncing'})});
  await secondSync;
  staleResponse({ok: true, json: async () => ({...sample})});
  await staleRefresh;
  assert.strictEqual(ntpStatusData.state, 'syncing');
  assert(elements.ntpSyncNow.disabled);
  assert(!elements.ntpSyncSpinner.hidden);
  global.fetch = normalFetch;
  await refreshNtpStatus();

  // Rejected requests must not show a success message.
  global.fetch = async () => ({ok: false, json: async () => ({error: 'Device busy'})});
  await syncNtpNow();
  assert(elements.ntpSyncMessage.textContent.includes('Device busy'));
  assert(elements.ntpSyncMessage.className.includes('text-danger'));
  global.fetch = async () => { throw new Error('offline'); };
  await refreshNtpStatus();
  assert.strictEqual(elements.ntpSyncStatus.textContent, 'NTP status unavailable');
  assert.strictEqual(elements.ntpClockTime.textContent, '—');
  assert(elements.ntpSyncMessage.className.includes('text-danger'));
  assert.strictEqual(scheduled.get(ntpPollTimer).delay, 3000);
})().catch(error => { console.error(error); process.exitCode = 1; });
'''
        result = subprocess.run(
            [shutil.which("node"), "-"], input=script, text=True,
            env={**os.environ, "TZ": "America/Los_Angeles"},
            capture_output=True, check=False,
        )
        self.assertEqual(result.returncode, 0, result.stderr)


if __name__ == "__main__":
    unittest.main()
