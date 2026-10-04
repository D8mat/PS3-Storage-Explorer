"""Build with the locally unpacked Estwald/PSDK3v2 toolchain. Python 3."""
from pathlib import Path
import ctypes
import hashlib
import os
import shutil
import struct
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
SDK = ROOT / '.reference/ps3dev'
PSL = ROOT / '.reference/PSDK3v2/psl1ght'
MINGW = ROOT / '.reference/MinGW'
GAME_TEST = '--game-test' in sys.argv[1:]
DIAGNOSTIC = GAME_TEST or '--diagnostic' in sys.argv[1:]
APPID = 'STORD0002' if GAME_TEST else ('STORD0001' if DIAGNOSTIC else 'STOR00001')
TITLE = 'PS3 Game Test NPUD21736' if GAME_TEST else ('PS3 Storage Diagnostic' if DIAGNOSTIC else 'PS3 Storage Explorer')
BUILD = ROOT / ('build/game-test' if GAME_TEST else ('build/diagnostic' if DIAGNOSTIC else 'build'))
DIST = ROOT / 'dist'
CONTENT = 'UP0001-'+APPID+'_00-0000000000000000'
PKG = DIST / ('PS3-Game-Test-NPUD21736.pkg' if GAME_TEST else ('PS3-Storage-Diagnostic-0.1.2.pkg' if DIAGNOSTIC else 'PS3-Storage-Explorer-0.1.7.pkg'))

def run(args, cwd=ROOT):
    subprocess.run([str(x) for x in args], cwd=cwd, check=True)

def verify_package():
    """Independent debug-PKG decryption and full payload comparison, no SDK parser."""
    raw = PKG.read_bytes()
    assert raw[:8] == b'\x7fPKG\x00\x00\x00\x01'
    count = struct.unpack_from('>I', raw, 20)[0]
    size, offset, length = struct.unpack_from('>QQQ', raw, 24)
    assert size == len(raw) and offset + length + 96 == size
    assert raw[48:96].rstrip(b'\0').decode() == CONTENT
    assert raw[128:144] == hashlib.sha1(raw[:128]).digest()[3:19]
    digest = raw[96:112]
    key = digest[:8]*2 + digest[8:]*2 + bytes(24)
    encrypted = raw[offset:offset+length]
    decrypted = bytearray()
    for i in range(0, length, 16):
        mask = hashlib.sha1(key + struct.pack('>Q', i//16)).digest()
        decrypted.extend(a ^ b for a, b in zip(encrypted[i:i+16], mask))
    seen = set()
    for i in range(count):
        nameoff, namelen, fileoff, filelen, flags, _ = struct.unpack_from('>IIQQII', decrypted, i*32)
        name = decrypted[nameoff:nameoff+namelen].decode('ascii')
        seen.add(name)
        if flags & 255 == 4:
            assert name == 'USRDIR'
            continue
        assert name in {'ICON0.PNG', 'PARAM.SFO', 'USRDIR/EBOOT.BIN'}
        expected = (BUILD / 'pkg' / name).read_bytes()
        actual = decrypted[fileoff:fileoff+filelen]
        assert actual[:len(expected)] == expected, name
        assert not any(actual[len(expected):]), name
        print('Verified payload:', name, len(expected), 'bytes')
    assert seen == {'ICON0.PNG', 'PARAM.SFO', 'USRDIR', 'USRDIR/EBOOT.BIN'}
    sha = hashlib.sha256(raw).hexdigest()
    sums = DIST / 'SHA256SUMS.txt'
    existing = sums.read_text().splitlines() if sums.exists() else []
    existing = [line for line in existing if not line.endswith('  '+PKG.name)]
    sums.write_text('\n'.join(existing+[f'{sha}  {PKG.name}'])+'\n', encoding='ascii')
    print('PKG verified:', len(raw), 'bytes;', sha)

def main():
    if os.name != 'nt':
        raise SystemExit('This script uses the Windows PSDK3v2 binaries.')
    # Do not show Windows loader/crash dialogs if a dependency is missing.
    ctypes.windll.kernel32.SetErrorMode(0x8003)
    os.environ['PSL1GHT'] = PSL.as_posix()
    os.environ['PS3DEV'] = SDK.as_posix()
    os.environ['PATH'] = os.pathsep.join(str(x) for x in (
        MINGW/'msys/1.0/bin', MINGW/'bin', SDK/'ppu/bin', SDK/'bin')) + os.pathsep + os.environ['PATH']
    for folder in (BUILD, DIST, BUILD/'pkg/USRDIR'):
        folder.mkdir(parents=True, exist_ok=True)
    gcc = SDK/'ppu/bin/ppu-gcc.exe'
    for source in ('main', 'scan'):
        run([gcc, '-O2', '-Wall', '-Wextra', '-Werror', '-Wframe-larger-than=8192', '-std=gnu99', '-D__PPU__',
             '-DPSDK3_LEGACY', *(['-DSTORAGE_DIAGNOSTIC'] if DIAGNOSTIC else []),
             *(['-DSTORAGE_GAME_TEST'] if GAME_TEST else []), '-mcpu=cell', '-mhard-float', '-Iinclude',
             '-I'+str(PSL/'ppu/include'), '-I'+str(PSL/'ppu/include/simdmath'),
             '-c', f'source/{source}.c', '-o', BUILD/f'{source}.o'])
    elf = BUILD/'ps3-storage-explorer.elf'
    stripped = BUILD/'ps3-storage-explorer.stripped.elf'
    run([gcc, BUILD/'main.o', BUILD/'scan.o', '-L'+str(PSL/'ppu/lib'),
         '-lrsx', '-lgcm_sys', '-lsysutil', '-lio', '-lrt', '-llv2', '-o', elf])
    run([SDK/'ppu/bin/ppu-strip.exe', elf, '-o', stripped])
    run([SDK/'bin/sprxlinker.exe', stripped])
    run([SDK/'bin/scetool.exe', '--self-app-version=0001000000000000', '--sce-type=SELF',
         '--compress-data=TRUE', '--self-add-shdrs=TRUE', '--skip-sections=FALSE',
         '--key-revision=1', '--self-auth-id=1010000001000003', '--self-vendor-id=01000002',
         '--self-fw-version=0003004000000000', '--self-type=NPDRM', '--np-license-type=FREE',
         '--np-app-type=EXEC', '--np-real-fname=EBOOT.BIN', '--np-content-id='+CONTENT,
         '--encrypt', stripped, BUILD/'pkg/USRDIR/EBOOT.BIN'], cwd=SDK/'bin')
    py2 = MINGW/'Python27/python.exe'
    run([py2, SDK/'bin/sfo.py', '--title', TITLE, '--appid', APPID,
         '-f', SDK/'bin/sfo.xml', BUILD/'pkg/PARAM.SFO'])
    sfo_path = BUILD/'pkg/PARAM.SFO'
    sfo = bytearray(sfo_path.read_bytes())
    keys, values, count = struct.unpack_from('<III', sfo, 8)
    for i in range(count):
        keyoff, _, length, _, valueoff = struct.unpack_from('<HHIII', sfo, 20+i*16)
        end = sfo.index(0, keys+keyoff)
        if sfo[keys+keyoff:end] == b'APP_VER':
            assert length == 6
            sfo[values+valueoff:values+valueoff+6] = b'01.02\0' if DIAGNOSTIC else b'01.07\0'
    sfo_path.write_bytes(sfo)
    shutil.copyfile(SDK/'bin/ICON0.PNG' if DIAGNOSTIC else ROOT/'assets/ICON0.PNG', BUILD/'pkg/ICON0.PNG')
    run([py2, SDK/'bin/pkg.py', '--contentid', CONTENT, (BUILD/'pkg').as_posix()+'/', PKG])
    verify_package()

if __name__ == '__main__':
    main()
