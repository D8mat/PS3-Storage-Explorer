"""Run all desktop checks with Python 3 and Zig, GCC or Clang."""
import subprocess
import sys
from support import ROOT, ENV, compile_test

scanner = compile_test('scan-test', ['source/scan.c', 'tests/host.c'])
subprocess.run([sys.executable, 'tests/test_scan.py', str(scanner)], cwd=ROOT, env=ENV, check=True)
for name in ('paging', 'paths'):
    exe = compile_test('test-' + name, ['tests/' + name + '.c'])
    subprocess.run([str(exe)], cwd=ROOT, env=ENV, check=True)
for name in ('multi', 'display'):
    subprocess.run([sys.executable, 'tests/test_' + name + '.py'], cwd=ROOT, env=ENV, check=True)
print('All desktop checks passed.')
