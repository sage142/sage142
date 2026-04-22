#!/usr/bin/env bash
set -euo pipefail

BUILD_DIR="${BUILD_DIR:-build}"
DIST_DIR="${DIST_DIR:-dist}"
CREATE_ZIP="${CREATE_ZIP:-1}"

cmake -S . -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE=Release
cmake --build "$BUILD_DIR" -j

PLATFORM="linux"
if [[ "$(uname -s)" == "Darwin" ]]; then
  PLATFORM="macos"
elif [[ "$(uname -s)" =~ MINGW|MSYS|CYGWIN ]]; then
  PLATFORM="windows"
fi

mkdir -p "$DIST_DIR/$PLATFORM"
cp "$BUILD_DIR/sage_engine" "$DIST_DIR/$PLATFORM/"
if [[ -d assets ]]; then
  cp -R assets "$DIST_DIR/$PLATFORM/"
fi

if [[ "$CREATE_ZIP" == "1" ]]; then
  if command -v zip >/dev/null 2>&1; then
    (cd "$DIST_DIR" && zip -r "$PLATFORM.zip" "$PLATFORM" >/dev/null)
  else
    echo "zip not installed; skipping archive creation"
  fi
fi

echo "Published build to $DIST_DIR/$PLATFORM"
