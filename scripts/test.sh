#!/usr/bin/env bash
# Run the unit tests and the CLI self-test.
set -euo pipefail

cd "$(dirname "$0")/.."

echo "=== unit tests ==="
./build/lvx_tests

echo "=== CLI self-test ==="
./build/logvex-cli --self-test

echo "=== CLI aim_arena demo ==="
./build/logvex-cli --frames 300 --scenario aim_arena

echo "=== CLI benchmark ==="
./build/logvex-cli --benchmark 20000
