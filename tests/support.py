"""Shared native compiler setup for desktop tests."""
from pathlib import Path
import os
import shutil
import subprocess

ROOT = Path(__file__).resolve().parents[1]
BUILD = ROOT / 'build'
BUILD.mkdir(exist_ok=True)
ENV = os.environ.copy()
ENV.setdefault('ZIG_GLOBAL_CACHE_DIR', str(BUILD / 'zig-cache'))

def compile_test(name, sources):
    zig = os.environ.get('ZIG_EXE') or shutil.which('zig')
    local = ROOT / '.reference/python/ziglang/zig.exe'
    if not zig and local.exists():
        zig = str(local)
    cc = os.environ.get('CC') or shutil.which('cc') or shutil.which('gcc') or shutil.which('clang')
    if zig:
        compiler = [zig, 'cc']
    elif cc:
        compiler = [cc]
    else:
        raise SystemExit('Install Zig, GCC or Clang; optionally set ZIG_EXE or CC to the compiler executable.')
    exe = BUILD / (name + ('.exe' if os.name == 'nt' else ''))
    subprocess.run(compiler + ['-Wall', '-Wextra', '-Werror', '-Iinclude',
                   *map(str, sources), '-o', str(exe)], cwd=ROOT, env=ENV, check=True)
    return exe
