#!/usr/bin/env bash
# Install the build/runtime dependencies on a Debian/Kali host.
set -euo pipefail

sudo apt-get update
sudo apt-get install -y \
  build-essential cmake ninja-build git pkg-config \
  libglm-dev libglfw3-dev libgl1-mesa-dev \
  nlohmann-json3-dev \
  xvfb imagemagick

echo "dependencies installed"
