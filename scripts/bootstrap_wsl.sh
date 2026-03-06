#!/usr/bin/env bash
set -euo pipefail

# Bootstrap a Linux/WSL build environment for DjVuLibre.

INSTALL_DEPS="1"
JOBS=""
PREFIX=""
RUN_INSTALL="0"
ENABLE_GTEST="0"
BUILD_TYPE="Release"
BUILD_DIR=""

usage() {
  cat <<'EOF'
Usage: bootstrap_wsl.sh [options]

Options:
  --no-install-deps      Do not install apt dependencies
  --jobs N               Parallel jobs for the build (default: nproc)
  --prefix PATH          Install prefix (default: ./_wsl-prefix)
  --build-type TYPE      CMake build type: Release or Debug (default: Release)
  --build-dir PATH       Build directory (default: ./build-wsl)
  --run-install          Run "cmake --install" after build and test
  --enable-gtest         Enable GoogleTest-based unit tests
  -h, --help             Show this help
EOF
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --no-install-deps)
      INSTALL_DEPS="0"
      shift
      ;;
    --jobs)
      JOBS="${2:-}"
      shift 2
      ;;
    --prefix)
      PREFIX="${2:-}"
      shift 2
      ;;
    --build-type)
      BUILD_TYPE="${2:-}"
      shift 2
      ;;
    --build-dir)
      BUILD_DIR="${2:-}"
      shift 2
      ;;
    --run-install)
      RUN_INSTALL="1"
      shift
      ;;
    --enable-gtest)
      ENABLE_GTEST="1"
      shift
      ;;
    -h|--help)
      usage
      exit 0
      ;;
    *)
      echo "Unknown option: $1" >&2
      usage
      exit 2
      ;;
  esac
done

case "$BUILD_TYPE" in
  Release|Debug)
    ;;
  *)
    echo "ERROR: --build-type must be Release or Debug" >&2
    exit 2
    ;;
esac

if [[ -z "$JOBS" ]]; then
  JOBS="$(nproc)"
fi

if [[ -z "$PREFIX" ]]; then
  PREFIX="$(pwd)/_wsl-prefix"
fi

if [[ -z "$BUILD_DIR" ]]; then
  BUILD_DIR="$(pwd)/build-wsl"
fi

if [[ "$INSTALL_DEPS" == "1" ]]; then
  export DEBIAN_FRONTEND=noninteractive
  sudo apt-get update
  sudo apt-get install -y \
    cmake \
    ninja-build \
    pkg-config \
    g++ \
    libjpeg-dev \
    libtiff-dev \
    zlib1g-dev

  if [[ "$ENABLE_GTEST" == "1" ]]; then
    sudo apt-get install -y libgtest-dev
  fi
fi

echo "==> Build dir: $BUILD_DIR"
echo "==> Prefix: $PREFIX"
echo "==> Build type: $BUILD_TYPE"
echo "==> Jobs: $JOBS"

cmake -S . -B "$BUILD_DIR" -G Ninja \
  -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
  -DCMAKE_INSTALL_PREFIX="$PREFIX" \
  -DDJVULIBRE_ENABLE_GTEST="$ENABLE_GTEST" \
  -DDJVULIBRE_ENABLE_XMLTOOLS=ON \
  -DDJVULIBRE_ENABLE_DESKTOPFILES=OFF

cmake --build "$BUILD_DIR" --parallel "$JOBS"
ctest --test-dir "$BUILD_DIR" --output-on-failure

if [[ "$RUN_INSTALL" == "1" ]]; then
  cmake --install "$BUILD_DIR"
fi

echo "==> Done"
