# Building 0.1.7

The supplied PKG was built on Windows with Python 3 and the legacy [Estwald/PSDK3v2](https://github.com/Estwald/PSDK3v2) toolchain, revision `c920bc3e2c0fcb7c7d0e03e0be323625aa24c0ba`.

## Required layout

Obtain and unpack the toolchain separately. The repository deliberately excludes SDKs, compiler binaries, runtime DLLs and signing support data.

```text
.reference/
  PSDK3v2/psl1ght/ppu/include/
  PSDK3v2/psl1ght/ppu/lib/
  ps3dev/ppu/bin/ppu-gcc.exe
  ps3dev/ppu/bin/ppu-strip.exe
  ps3dev/bin/sprxlinker.exe
  ps3dev/bin/scetool.exe
  ps3dev/bin/sfo.py
  ps3dev/bin/sfo.xml
  ps3dev/bin/pkg.py
  MinGW/bin/
  MinGW/msys/1.0/bin/
  MinGW/Python27/python.exe
```

`ps3dev.7z` and `MinGW.7z` are from PSDK3v2. Preserve the support files expected by its tools. Python 2.7 is used only by the legacy SFO/PKG scripts; run the main build script using Python 3.

```powershell
python tools/build_windows.py
```

The script compiles with PPU GCC, links, creates a compressed NPDRM EBOOT, writes APP_VER `01.07`, copies `assets/ICON0.PNG`, packages the app and independently verifies the decrypted package payload. Output: `dist/PS3-Storage-Explorer-0.1.7.pkg` and `dist/SHA256SUMS.txt`.

Missing `libz-1.dll`, `msys-crypto-1.0.0.dll`, `msys-gmp-10.dll` or `msys-z.dll` means the matching toolchain runtime is incomplete. Restore its MinGW/MSYS directories rather than mixing arbitrary DLL versions. The script supplies their PATH entries and runs scetool in its own directory because it needs relative support data.

Historical diagnostic switches remain available in the build script for development. They are not release packages; run the command above without switches for 0.1.7.

The included Makefile is an alternative PSL1GHT build entry point (`make`, `make pkg`, with PSL1GHT configured). It was not used to produce this release and may require adaptation to your SDK, including package version metadata. Use the Windows script to follow the tested release workflow.

## Desktop checks

Install Python 3 and Zig, GCC or Clang. Put the compiler on PATH or set `ZIG_EXE` / `CC` to its executable path, then run:

```powershell
python tests/run_tests.py
```

The runner creates `build/` automatically. The large sparse-file test may be skipped when the host cannot create sparse files. No PS3 SDK is needed for these checks.

## Icon

The current 320 × 176 RGBA PNG is `assets/ICON0.PNG`. To regenerate it from the supplied transparent artwork on Windows:

```powershell
powershell -File tools/prepare_icon.ps1
python tools/build_windows.py
```
