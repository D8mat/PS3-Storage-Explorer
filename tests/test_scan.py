"""Run against the actual C scanner: python tests/test_scan.py build/scan-test.exe."""
import pathlib
import os
import struct
import subprocess
import sys
import tempfile
import unittest

EXE = str(pathlib.Path(sys.argv.pop(1)).resolve())

def sfo():
    keys = b'TITLE\0TITLE_ID\0'
    title, game_id = b'Test Game\0', b'BLES12345\0'
    start = 20 + 32
    return (struct.pack('<5I', 0x46535000, 0x101, start, start + len(keys), 2)
            + struct.pack('<HHIII', 0, 0x204, len(title), len(title), 0)
            + struct.pack('<HHIII', 6, 0x204, len(game_id), len(game_id), len(title))
            + keys + title + game_id)

class ScannerTests(unittest.TestCase):
    def run_scan(self, root, mode='desc'):
        result = subprocess.run([EXE, str(root), mode], check=True, capture_output=True, text=True)
        lines = [line.split('\t') for line in result.stdout.splitlines()]
        return lines[0], lines[1:]

    def test_nested_metadata_and_sort(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = pathlib.Path(tmp)
            game = root / 'BLES12345'
            (game / 'USRDIR').mkdir(parents=True)
            (game / 'USRDIR' / 'data').write_bytes(b'x' * 5000)
            (game / 'PARAM.SFO').write_bytes(sfo())
            (root / 'small.pkg').write_bytes(b'a' * 10)
            (root / 'empty').mkdir()
            status, rows = self.run_scan(root)
            self.assertEqual(status, ['STATUS', '0', '0'])
            self.assertEqual(rows[0], ['Test Game', str(5000+len(sfo())), '2', '1', '0', 'BLES12345'])
            self.assertEqual([int(r[1]) for r in rows], [5000+len(sfo()), 10, 0])
            _, rows = self.run_scan(root, 'asc')
            self.assertEqual([int(r[1]) for r in rows], [0, 10, 5000+len(sfo())])

    def test_large_file(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = pathlib.Path(tmp) / 'large.iso'
            with path.open('wb') as f:
                if os.name == 'nt':
                    import ctypes
                    import msvcrt
                    from ctypes import wintypes
                    ioctl = ctypes.windll.kernel32.DeviceIoControl
                    ioctl.argtypes = [wintypes.HANDLE, wintypes.DWORD, wintypes.LPVOID,
                                      wintypes.DWORD, wintypes.LPVOID, wintypes.DWORD,
                                      ctypes.POINTER(wintypes.DWORD), wintypes.LPVOID]
                    returned = wintypes.DWORD()
                    if not ioctl(msvcrt.get_osfhandle(f.fileno()), 0x900c4, None, 0, None, 0,
                                 ctypes.byref(returned), None):
                        self.skipTest('Filesystem does not support sparse test files')
                f.truncate(5 * 1024**3 + 17)
            _, rows = self.run_scan(tmp)
            self.assertEqual(int(rows[0][1]), 5 * 1024**3 + 17)

    def test_cancel_and_missing(self):
        with tempfile.TemporaryDirectory() as tmp:
            for i in range(20):
                (pathlib.Path(tmp) / str(i)).write_bytes(b'a')
            status, _ = self.run_scan(tmp, 'cancel')
            self.assertEqual(status[2], '1')
            status, rows = self.run_scan(pathlib.Path(tmp) / 'missing')
            self.assertEqual(status[1], '1')
            self.assertEqual(rows, [])

    def test_corrupt_sfo_falls_back(self):
        with tempfile.TemporaryDirectory() as tmp:
            game = pathlib.Path(tmp) / 'Fallback'
            game.mkdir()
            (game / 'PARAM.SFO').write_bytes(b'\0PSF' + b'\xff' * 60)
            _, rows = self.run_scan(tmp)
            self.assertEqual(rows[0][0], 'Fallback')
            self.assertEqual(rows[0][1], '64')

if __name__ == '__main__':
    unittest.main()
