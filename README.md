# DjVuLibre

[![CI](https://github.com/volnsav/DjVuLibreExperimental/actions/workflows/ci.yml/badge.svg?branch=develop)](https://github.com/volnsav/DjVuLibreExperimental/actions/workflows/ci.yml)

DjVuLibre is the reference C++ library and command-line toolset for the DjVu
document format. This repository contains only the library, command-line
utilities, XML tools, tests, and packaging metadata.

**Current version:** 3.5.30

`DjView` is not part of this repository. The Qt-based viewer lives in a
separate repository and should be built and packaged independently.


## Contents

This repository builds:

- `libdjvulibre`
- command-line tools under `tools/`
- XML utilities under `xmltools/`
- smoke and GoogleTest-based library tests


## Build system

The project uses CMake only.

Minimum toolchain:

- CMake 3.21+
- a C++17-capable compiler
- `libjpeg`
- `libtiff`
- optional: GoogleTest for the extended unit test suite

On Linux, `pkg-config`, `cmake`, `ninja-build`, `libjpeg-dev`, `libtiff-dev`,
and `libgtest-dev` are the normal package names on Debian/Ubuntu.


## Linux and macOS

Typical out-of-tree build:

```bash
cmake -S . -B build -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DDJVULIBRE_ENABLE_GTEST=ON \
  -DDJVULIBRE_ENABLE_DESKTOPFILES=OFF
cmake --build build
ctest --test-dir build --output-on-failure
cmake --install build --prefix /usr/local
```

Useful CMake options:

- `-DDJVULIBRE_ENABLE_GTEST=ON` builds `libdjvu_gtest`
- `-DDJVULIBRE_ENABLE_XMLTOOLS=OFF` disables XML utilities
- `-DDJVULIBRE_ENABLE_DESKTOPFILES=OFF` skips desktop integration assets
- `-DDJVULIBRE_THIRD_PARTY_DIR=/path/to/third_party` adds an explicit third-party root
- `-DDJVULIBRE_EXTRA_INCLUDE_DIRS=/path/a;/path/b`
- `-DDJVULIBRE_EXTRA_LIBRARY_DIRS=/path/a;/path/b`

For WSL there is a helper script:

```bash
./scripts/bootstrap_wsl.sh --enable-gtest --run-install
```


## Windows

Windows support is CMake-first and targets Visual Studio 2019 or newer (tested with VS 2026).
Third-party integration is expected to live under `third_party/`, with
vendored source trees as the default Windows dependency source.

Typical PowerShell flow:

```powershell
.\scripts\run_windows_tests.ps1 -Configuration Debug -Platform x64
```

That script will:

- detect a supported Visual Studio generator
- build vendored `zlib`, `libjpeg-turbo`, and `libtiff`
- use vendored `googletest` sources for the gtest target
- configure a Visual Studio CMake build
- build the library, tools, and tests
- run `ctest`

Manual CMake example:

```powershell
cmake -S third_party\zlib -B build-windows\third_party\zlib `
  -G "Visual Studio 18 2026" `
  -A x64 `
  -DCMAKE_INSTALL_PREFIX="$PWD\\third_party\\install\\x64\\Release"
cmake --build build-windows\third_party\zlib --config Release
cmake --install build-windows\third_party\zlib --config Release

cmake -S . -B build-windows `
  -G "Visual Studio 18 2026" `
  -A x64 `
  -DCMAKE_PREFIX_PATH="$PWD\\third_party\\install\\x64\\Release" `
  -DDJVULIBRE_ENABLE_GTEST=ON
cmake --build build-windows --config Release
ctest --test-dir build-windows -C Release --output-on-failure
```

Installer and archive packages are generated through CPack. On Windows the
intended outputs are:

- `ZIP` for portable artifacts
- `NSIS` for a normal installer

Example:

```powershell
cd build-windows
cpack -C Release -G ZIP
cpack -C Release -G NSIS
```


## Installation layout

`cmake --install` installs:

- executables to `bin/`
- the shared library to `lib/`
- public headers to `include/libdjvu/`
- man pages to `share/man/`
- shared data to `share/djvu/`
- `pkg-config` metadata to `lib/pkgconfig/`
- CMake package metadata to `lib/cmake/DjVuLibre/`


## Testing

Two test layers are supported:

- `libdjvu_smoke_test` is always available
- `libdjvu_gtest` is built when `DJVULIBRE_ENABLE_GTEST=ON`

Linux example:

```bash
ctest --test-dir build --output-on-failure
```

Windows example:

```powershell
ctest --test-dir build-windows -C Debug --output-on-failure
```

Some GoogleTest cases are intentionally excluded from the current Windows
target set until symbol-export cleanup is finished. See
`tests/gtest/WINDOWS_NONLINKING.md`.
