# Third-party dependencies

When `DJVULIBRE_USE_BUNDLED_DEPS=ON` (the default on Windows), the CMake
build downloads, patches, and builds the following libraries as static
archives via `ExternalProject`:

| Library        | Version |
|----------------|---------|
| zlib           | 1.3.1   |
| libjpeg-turbo  | 3.1.3   |
| libtiff        | 4.7.1   |

Source trees are extracted into this directory.  They are **not** checked in
and are listed in `.gitignore`.

On Linux and macOS the build defaults to `DJVULIBRE_USE_BUNDLED_DEPS=OFF`
and uses system-installed packages found via `find_package()`.

Optional patches live in `patches/<name>/` and are applied automatically
the first time a source tree is extracted.

## GoogleTest

Unit tests (`DJVULIBRE_ENABLE_GTEST=ON`) look for GoogleTest in this order:

1. `third_party/googletest/` (vendored clone)
2. System `GTest` via `find_package()`
3. Automatic download via `FetchContent` (v1.17.0)
