#!/usr/bin/env bash
# Launch the interactive trainer. Falls back to xvfb-run on a headless host.
set -euo pipefail

cd "$(dirname "$0")/.."

if [[ -n "${DISPLAY:-}" ]]; then
  exec ./build/logvex-practice "$@"
fi

if command -v xvfb-run >/dev/null 2>&1; then
  echo "no DISPLAY set; running under xvfb-run (window will not be visible)"
  exec xvfb-run -a -s "-screen 0 1400x860x24" ./build/logvex-practice "$@"
fi

echo "no DISPLAY and xvfb-run is not installed" >&2
exit 1
