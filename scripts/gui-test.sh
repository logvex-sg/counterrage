#!/usr/bin/env bash
# Headless GUI behaviour test: drives the GUI control paths via scripts/gui_demo.txt
# under Xvfb and captures a screenshot as evidence.
set -euo pipefail

cd "$(dirname "$0")/.."

xvfb-run -a -s "-screen 0 1400x860x24" \
  ./build/logvex-practice --script scripts/gui_demo.txt

echo "screenshot written to /tmp/logvex_gui.png"
