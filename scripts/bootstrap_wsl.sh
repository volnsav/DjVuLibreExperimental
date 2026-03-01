#!/usr/bin/env bash
set -euo pipefail

# Bootstrap DjVuLibreExperimental build/test environment inside WSL (Ubuntu).
# Usage examples:
#   ./scripts/bootstrap_wsl.sh --qt-major 6
#   ./scripts/bootstrap_wsl.sh --qt-major 5 --no-install-deps
#   ./scripts/bootstrap_wsl.sh --qt-major 6 --jobs 8 --prefix "$PWD/.wsl-prefix"

QT_MAJOR="6"
INSTALL_DEPS="1"
JOBS=""
PREFIX=""
RUN_INSTALL="0"
ENABLE_GTEST="0"

usage() {
  cat <<'EOF'
Usage: bootstrap_wsl.sh [options]

Options:
  --qt-major {5|6}      Qt major version profile (default: 6)
  --no-install-deps      Do not install apt dependencies
  --jobs N               Parallel jobs for make (default: nproc)
  --prefix PATH          Installation prefix (default: ./_wsl-prefix)
  --run-install          Run "make install" after build
  --enable-gtest         Enable GoogleTest-based unit tests
  -h, --help             Show this help
EOF
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --qt-major)
      QT_MAJOR="${2:-}"
      shift 2
      ;;
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

if [[ "$QT_MAJOR" != "5" && "$QT_MAJOR" != "6" ]]; then
  echo "ERROR: --qt-major must be 5 or 6" >&2
  exit 2
fi

if [[ -z "$JOBS" ]]; then
  JOBS="$(nproc)"
fi

if [[ -z "$PREFIX" ]]; then
  PREFIX="$(pwd)/_wsl-prefix"
fi

if [[ "$INSTALL_DEPS" == "1" ]]; then
  export DEBIAN_FRONTEND=noninteractive
  sudo apt-get update
  sudo apt-get install -y \
    autoconf \
    automake \
    libtool \
    pkg-config \
    g++ \
    make \
    libtiff-dev \
    libgl1-mesa-dev \
    libjpeg-dev \
    zlib1g-dev

  if [[ "$ENABLE_GTEST" == "1" ]]; then
    sudo apt-get install -y libgtest-dev
  fi

  if [[ "$QT_MAJOR" == "6" ]]; then
    sudo apt-get install -y \
      qt6-base-dev \
      qt6-base-dev-tools \
      qt6-tools-dev-tools \
      qt6-l10n-tools
  else
    sudo apt-get install -y \
      qtbase5-dev \
      qttools5-dev-tools
  fi
fi

if [[ "$QT_MAJOR" == "6" ]]; then
  if command -v qmake6 >/dev/null 2>&1; then
    export QMAKE="$(command -v qmake6)"
  elif command -v qmake-qt6 >/dev/null 2>&1; then
    export QMAKE="$(command -v qmake-qt6)"
  else
    echo "ERROR: Qt6 qmake not found (expected qmake6 or qmake-qt6)." >&2
    exit 1
  fi
else
  if command -v qmake >/dev/null 2>&1; then
    export QMAKE="$(command -v qmake)"
  elif command -v qmake-qt5 >/dev/null 2>&1; then
    export QMAKE="$(command -v qmake-qt5)"
  else
    echo "ERROR: Qt5 qmake not found (expected qmake or qmake-qt5)." >&2
    exit 1
  fi
fi

echo "==> Qt profile: $QT_MAJOR (QMAKE=$QMAKE)"
echo "==> Prefix: $PREFIX"
echo "==> Jobs: $JOBS"

CONFIGURE_ARGS=(
  --prefix="$PREFIX"
  --disable-desktopfiles
)
if [[ "$ENABLE_GTEST" == "1" ]]; then
  CONFIGURE_ARGS+=(--enable-gtest)
fi

./autogen.sh "${CONFIGURE_ARGS[@]}"
make -j"$JOBS"
make -j"$JOBS" check

if [[ "$RUN_INSTALL" == "1" ]]; then
  make install
fi

echo "==> Done"
