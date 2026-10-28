#!/usr/bin/env python3
"""Explicitly invoked OTA and profile integration checks on a supplied device.
Backups can contain credentials; keep --backup and --output outside the repo.
"""
import argparse
import hashlib
import json
import math
import secrets
import time
import urllib.error
import urllib.parse
import urllib.request
from pathlib import Path

def equivalent(a, b):
    if isinstance(a, dict) and isinstance(b, dict):
        return a.keys() == b.keys() and all(equivalent(a[k], b[k]) for k in a)
    if isinstance(a, list) and isinstance(b, list):
        return len(a) == len(b) and all(equivalent(x, y) for x, y in zip(a, b))
    if isinstance(a, (int, float)) and isinstance(b, (int, float)):
        return math.isclose(a, b, rel_tol=1e-6, abs_tol=1e-6)
    return a == b

def validate_firmware(data, platform, version):
    if platform not in ('ESP8266', 'ESP32'):
        raise RuntimeError('Device did not report a supported platform')
    marker = ('WLEDPIXEL_PLATFORM_' + platform).encode()
    other = b'WLEDPIXEL_PLATFORM_ESP32' if platform == 'ESP8266' else b'WLEDPIXEL_PLATFORM_ESP8266'
    if not data or data[0] != 0xE9 or marker not in data or other in data:
        raise RuntimeError('Firmware image does not match device platform ' + platform)
    if not version or version.encode() + b'\0' not in data:
        raise RuntimeError('Firmware image does not contain the requested --version')

def legacy_empty_rule(expected, key, want, got):
    # Some older unversioned exports represented unset dynamic-rule Strings as
    # JSON null. Live rule values have no profile "no change" sentinel: both
    # representations mean an unset trigger. Keep profile null/empty strict.
    return (expected.get('backupVersion', 1) < 3 and want is None and got == '' and
            key.startswith(('pbDynR1ValZone', 'pbDynR2ValZone')))

class Device:
    def __init__(self, host, version=None):
        self.base = 'http://' + host
        self.version = version
    def request(self, route, data=None, token=None, expected=(200, 202), timeout=40):
        headers = {'Connection': 'close'}
        if token:
            headers['X-Profile-Edit'] = token
        if isinstance(data, dict):
            data = urllib.parse.urlencode(data).encode()
            headers['Content-Type'] = 'application/x-www-form-urlencoded'
        for attempt in range(10):
            req = urllib.request.Request(self.base + route, data=data, headers=headers)
            try:
                with urllib.request.urlopen(req, timeout=timeout) as response:
                    raw, code = response.read(), response.status
            except urllib.error.HTTPError as error:
                raw, code = error.read(), error.code
                if code in (429, 503) and code not in expected and attempt < 9:
                    time.sleep(2); continue
            if code not in expected:
                raise RuntimeError(f'{route}: HTTP {code}: {raw[:200]!r}')
            try: return json.loads(raw)
            except ValueError: return raw.decode()
        raise RuntimeError('Device remained busy')
    def multipart(self, route, field, filename, contents, timeout=90):
        boundary = 'wledtest' + secrets.token_hex(8)
        body = (f'--{boundary}\r\nContent-Disposition: form-data; name="{field}"; filename="{filename}"\r\n'
                'Content-Type: application/octet-stream\r\n\r\n').encode() + contents + f'\r\n--{boundary}--\r\n'.encode()
        req = urllib.request.Request(self.base + route, data=body, headers={
            'Content-Type': 'multipart/form-data; boundary=' + boundary, 'Connection': 'close'})
        try:
            with urllib.request.urlopen(req, timeout=timeout) as response:
                raw = response.read()
        except urllib.error.HTTPError as error:
            raise RuntimeError(f'{route}: HTTP {error.code}: {error.read()[:200]!r}') from error
        try: return json.loads(raw)
        except ValueError: return raw.decode()
    def ready(self, version=None, sketch_md5=None):
        version = version or self.version
        time.sleep(5)
        for _ in range(40):
            try:
                value = self.request('/api/service-settings', timeout=5)
                if isinstance(value, dict) and (version is None or value.get('firmwareVer') == version):
                    status = self.request('/api/diagnostics', timeout=5)
                    if 'bootComplete' not in status:
                        status = self.request('/api/hil/status', timeout=5)
                    if status.get('bootComplete') and (sketch_md5 is None or status.get('sketchMD5') == sketch_md5):
                        return value
            except (OSError, ValueError, RuntimeError): pass
            time.sleep(5)
        raise RuntimeError('Device did not become ready')
    def settled(self):
        for _ in range(15):
            time.sleep(2)
            status = self.request('/api/scheduler/meta')
            if not isinstance(status, dict): raise RuntimeError('Invalid scheduler metadata: ' + repr(status))
            if status.get('storageError'): raise RuntimeError('Profile flash write failed')
            print('Storage state:', status.get('storagePending'), status.get('storageError'), flush=True)
            if not status.get('storagePending'): return
        raise RuntimeError('Save did not finish')
    def flash(self, binary):
        data = Path(binary).read_bytes()
        if not self.version:
            raise RuntimeError('--version is required for OTA verification')
        platform = self.request('/api/service-settings').get('platform')
        validate_firmware(data, platform, self.version)
        count = math.ceil(len(data)/10240)
        for i in range(count):
            result = self.multipart(f'/api/ota?chunk={i}&total={count}&size={len(data)}',
                                    'firmware', Path(binary).name, data[i*10240:(i+1)*10240])
            if not isinstance(result, dict) or result.get('status') != 'ok':
                raise RuntimeError(f'OTA chunk {i}: {result}')
            if i % 10 == 0 or i == count-1: print(f'OTA {i+1}/{count}', flush=True)
        self.ready(sketch_md5=hashlib.md5(data).hexdigest() if platform == 'ESP32' else None)
        print('PASS OTA and boot: ' + self.version + ' on ' + platform, flush=True)
    def restore(self, backup):
        result = self.multipart('/api/restore', 'backup', 'backup.json', Path(backup).read_bytes(), timeout=180)
        if isinstance(result, dict) and result.get('status') == 'queued':
            for _ in range(90):
                time.sleep(2)
                try: status = self.request('/api/restore-status', timeout=10, expected=(200, 429, 503))
                except OSError: break  # Successful restore may already be rebooting.
                if status.get('state') == 4: raise RuntimeError('Deferred restore failed')
                if status.get('state') in (0, 3): break
            else: raise RuntimeError('Deferred restore timed out')
        elif result != 'OK': raise RuntimeError('Restore failed: ' + str(result))
        self.ready()
        print('PASS restore and reboot', flush=True)
    def verify(self, backup):
        expected = json.loads(Path(backup).read_text())
        actual = self.request('/api/backup', timeout=90)
        if actual.get('backupVersion') != 3: raise RuntimeError('Unexpected backup version')
        for key, value in expected.items():
            if key.startswith('scheduleProfile'):
                got = dict(actual[key]); got.pop('lastRun', None)
                want = dict(value); want.pop('lastRun', None)
                if not equivalent(got, want): raise RuntimeError('Profile mismatch: ' + key + ' fields ' + str([k for k in want if got.get(k) != want[k]]))
            elif key.startswith('scheduleBar'):
                got = actual[key]
                want = value
                if got is not None: got = dict(got); got.pop('inherited', None)
                if want is not None: want = dict(want); want.pop('inherited', None)
                if not equivalent(got, want): raise RuntimeError('Progress-bar mismatch: ' + key)
            elif legacy_empty_rule(expected, key, value, actual.get(key)):
                print('Legacy unset rule normalized: ' + key + ' null -> empty string', flush=True)
            elif not equivalent(actual.get(key), value):
                raise RuntimeError('Configuration mismatch: ' + key)
        print('PASS configuration, all profiles and progress bars match the saved backup', flush=True)
        return actual


def profile_tests(device, backup_path):
    backup = json.loads(Path(backup_path).read_text())
    device.verify(backup_path)
    runtime = device.request('/api/hil/status')
    Path(backup_path).with_name('runtime-before-tests.json').write_text(json.dumps(runtime))
    idx, token = 3, secrets.token_hex(10)
    original = device.request('/api/scheduler/profile?idx=3')
    original_bar = device.request('/api/scheduler/progress-bar?idx=3&zone=0')
    modified = False
    try:
        device.request('/api/scheduler/enabled?enabled=0', {})
        time.sleep(2)
        device.request('/api/scheduler/edit?idx=3', {}, token)
        device.request('/api/scheduler/edit?idx=3', {}, 'competing-client', expected=(409,))
        device.request('/api/config', {'key':'cronProfile','idx':3,'name':'WRONG'}, 'competing-client', expected=(409,))
        device.request('/api/scheduler/profile?idx=garbage', expected=(400,))
        device.request('/api/scheduler/progress-bar', {'idx':3,'zone':0,'maxValue':987.654}, token)
        device.settled()
        assert device.request('/api/scheduler/progress-bar?idx=3&zone=0') == original_bar
        assert device.request('/api/scheduler/edit-cancel?idx=3', {}, token)['status'] == 'closed'
        assert device.request('/api/scheduler/profile?idx=3') == original
        print('PASS competing editor rejected; staged PB invisible; abort preserves original', flush=True)

        token = secrets.token_hex(10)
        device.request('/api/scheduler/edit?idx=3', {}, token)
        device.request('/api/config', {'key':'cronProfile','idx':3,'reset':1,'draft':1,
                                      'name':'HW test','enabled':'false','intensity':7}, token)
        device.request('/api/config', {'key':'cronProfile','idx':3,'draft':1,
                                      'zspd0':42,'zct0':'null','zcpre0':'prefix','zstk0':'AAPL,MSFT',
                                      'zhi0':'','zhi1':'null','zmp0':'unit'}, token)
        device.request('/api/scheduler/progress-bar', {'idx':3,'zone':0,'minValue':12.345,'maxValue':987.654,
                                                    'dynamicMode':'true','dynamicSpeedMs':1234,'dynRule1Value':'test'}, token)
        device.settled()
        modified = True
        device.request('/api/config', {'key':'cronProfile','idx':3,'commit':1,'enabled':'false'}, token)
        device.settled()
        saved = device.request('/api/scheduler/profile?idx=3')
        bar = device.request('/api/scheduler/progress-bar?idx=3&zone=0')
        assert saved['zoneScrollSpeed'][0] == 42 and saved['zoneCountdownPrefix'][0] == 'prefix'
        assert saved['zoneHaSensorId'][0] == '' and saved['zoneHaSensorId'][1] is None
        assert saved['zoneStockSymbols'][0] == 'AAPL,MSFT'
        assert abs(bar['minValue']-12.345) < .0001 and abs(bar['maxValue']-987.654) < .001
        assert bar['dynamicMode'] and bar['dynamicSpeedMs'] == 1234
        print('PASS multi-request profile + PB commit and fractional values', flush=True)
        device.request('/api/reboot', {})
        device.ready()
        assert device.request('/api/scheduler/profile?idx=3') == saved
        assert device.request('/api/scheduler/progress-bar?idx=3&zone=0') == bar
        print('PASS committed profile and PB survive reboot', flush=True)
        device.request('/api/scheduler/apply?idx=3', {})
        deadline = time.monotonic() + 90
        while time.monotonic() < deadline:
            time.sleep(3)
            status = device.request('/api/hil/status')
            metadata = device.request('/api/scheduler/meta')
            if status['intensity'] == 7 and metadata['lastActiveProfile'] == 3: break
        else:
            raise RuntimeError('Apply Now did not finish: intensity=' + str(status['intensity']) +
                               ', active profile=' + str(metadata['lastActiveProfile']))
        print('PASS Apply Now changes intensity to 7', flush=True)
        device.request('/api/reboot', {})
        device.ready()
        assert device.request('/api/scheduler/meta')['lastActiveProfile'] == idx
        assert device.request('/api/hil/status')['intensity'] == 7
        print('PASS applied profile and ON AIR identity survive reboot with scheduling disabled', flush=True)

        token = secrets.token_hex(10)
        device.request('/api/scheduler/edit?idx=3', {}, token)
        device.request('/api/config', {'key':'cronProfile','idx':3,'enabled':'true','minute':255,'hour':255,
                                      'intensity':6}, token)
        device.settled()
        device.request('/api/scheduler/enabled?enabled=1', {})
        metadata = device.request('/api/scheduler/meta')
        assert metadata['schedulerEnabled'] and metadata['ntpSynced'], 'Scheduler not enabled or NTP not synced'
        deadline = time.monotonic() + 80
        while time.monotonic() < deadline:
            time.sleep(3)
            if device.request('/api/hil/status')['intensity'] == 6: break
        else: raise RuntimeError('Scheduled profile did not run at the next minute boundary')
        print('PASS real NTP scheduler applies profile at minute boundary', flush=True)
        device.request('/api/scheduler/enabled?enabled=0', {})
        time.sleep(2)
    finally:
        # Always release our own edit before restoring; never reset Wi-Fi.
        try: device.request('/api/scheduler/edit-cancel?idx=3', {}, token, expected=(200,409))
        except (OSError,RuntimeError): pass
        if modified:
            device.restore(backup_path)
            device.verify(backup_path)
            device.request('/api/config', {'key':'intensity','intensity':runtime['intensity'] if runtime['power'] else 0})
            time.sleep(2)
        else:
            device.request('/api/scheduler/enabled?enabled=' + ('1' if backup.get('schedulerEnabled') else '0'), {})
        print('User configuration restored after tests', flush=True)


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--host', required=True)
    p.add_argument('action', choices=['flash', 'verify', 'restore', 'profiles'])
    p.add_argument('--binary')
    p.add_argument('--backup')
    p.add_argument('--version', help='Expected firmware version; required for flash')
    args = p.parse_args()
    device = Device(args.host, args.version)
    if args.action == 'flash': device.flash(args.binary)
    elif args.action == 'profiles': profile_tests(device, args.backup)
    elif args.action == 'restore': device.restore(args.backup); device.verify(args.backup)
    else: device.verify(args.backup)
if __name__ == '__main__': main()
