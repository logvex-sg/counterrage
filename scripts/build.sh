#!/usr/bin/env bash
# Configure and build logvex. Usage: scripts/build.sh [Release|Debug]
set -euo pipefail

cd "$(dirname "$0")/.."
BUILD_TYPE="${1:-Release}"

cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE="${BUILD_TYPE}"
cmake --build build
echo "built: build/logvex-practice build/logvex-cli build/logvex-radar build/lvx_tests"
