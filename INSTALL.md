# logvex — full install guide

This guide takes a clean Debian/Kali/Ubuntu machine to a built, tested `logvex`
installation. Every command is copy-pasteable.

> `logvex` is an offline **simulator** for practising CS2 mechanics. It never
> touches the CS2 process, its memory, or any anti-cheat component.

---

## 1. System requirements

- 64-bit Linux (tested on Debian 13 "trixie"/Kali, Ubuntu 24.04).
- A C++20 compiler: GCC ≥ 11 or Clang ≥ 14.
- CMake ≥ 3.20 and Ninja.
- ~200 MB disk for sources + build.
- X11 or Wayland for the desktop app. On a headless host, `xvfb` is used instead.

Check your compiler:

```bash
g++ --version      # need >= 11
cmake --version    # need >= 3.20
```

## 2. Install dependencies

### Debian / Kali / Ubuntu

```bash
sudo apt-get update
sudo apt-get install -y \
  build-essential cmake ninja-build git pkg-config \
  libglm-dev libglfw3-dev libgl1-mesa-dev \
  libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev \
  nlohmann-json3-dev

# optional: headless test/screenshot support
sudo apt-get install -y xvfb imagemagick \
  libgl1-mesa-dri libglx-mesa0 mesa-utils
```

The repository ships `scripts/install-deps.sh` which runs the same commands:

```bash
scripts/install-deps.sh
```

### Fedora / RHEL

```bash
sudo dnf install -y gcc-c++ cmake ninja-build pkgconf-pkg-config git \
  glm-devel glfw-devel mesa-libGL-devel nlohmann-json-devel \
  libXrandr-devel libXinerama-devel libXcursor-devel libXi-devel
```

### Arch

```bash
sudo pacman -S --needed base-devel cmake ninja git glm glfw mesa nlohmann-json
```

> Dear ImGui is vendored under `third_party/imgui` — no package needed.

## 3. Get the source

```bash
git clone https://github.com/logvex-sg/counterrage.git
cd counterrage
```

## 4. Build

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

Or use the helper:

```bash
scripts/build.sh
```

This produces four binaries in `build/`:

| Binary | Purpose |
| --- | --- |
| `logvex-practice` | desktop trainer (GLFW + Dear ImGui) |
| `logvex-cli` | headless engine + diagnostics |
| `logvex-radar` | web radar server (HTTP + WebSocket) |
| `lvx_tests` | unit test suite |

Build options (pass with `-D...=ON/OFF`):

| Option | Default | Effect |
| --- | --- | --- |
| `LVX_BUILD_GUI` | `ON` | build the desktop trainer |
| `LVX_BUILD_RADAR` | `ON` | build the web radar server |
| `LVX_BUILD_TESTS` | `ON` | build the test suite |
| `LVX_WARNINGS_AS_ERRORS` | `OFF` | treat warnings as errors (CI mode) |

## 5. Verify the install

```bash
scripts/test.sh
```

Expected output:

```
46/46 test cases passed, 0 assertions failed
self-test complete, failures=0
locked_frames=300/300
```

Or run the steps individually:

```bash
ctest --test-dir build --output-on-failure
./build/lvx_tests
./build/logvex-cli --self-test
./build/logvex-cli --benchmark 20000
```

## 6. Run

### Desktop trainer

```bash
scripts/run.sh                       # uses $DISPLAY, or falls back to xvfb-run
./build/logvex-practice --frames 300 # render 300 frames then exit
```

Controls: right mouse captures the cursor, left mouse fires, `WASD`/`Space`/
`Ctrl`/`Shift` move, `Z`/`X` toggle aimbot/triggerbot, `1`–`9` switch weapon,
`Esc` quits.

### Headless CLI

```bash
./build/logvex-cli --list-scenarios
./build/logvex-cli --frames 300 --scenario aim_arena
./build/logvex-cli --dump-environment
./build/logvex-cli --set aim.global.aimbot.fov=4 --save-config configs/my.json
./build/logvex-cli --load-config configs/competitive.json --dump-config
```

### Web radar

```bash
./build/logvex-radar --port 6347 --scenario dust2_practice
# open http://127.0.0.1:6347/ in a browser
```

Serves an HTML canvas page, a JSON `/state` endpoint and a live WebSocket at
`/client`.

## 7. Headless GUI test (no monitor)

```bash
scripts/gui-test.sh
```

Drives the same code paths the ImGui widgets use, asserts the results, and
writes a screenshot to `/tmp/logvex_gui.png`.

## 8. Troubleshooting

**`cmake: command not found` / toolchain vanished**
This can happen if the container image is rebuilt. Re-run section 2.

**`apt-get` says "Permission denied" on `/var/lib/apt/lists/partial`**
Run apt with `sudo`, or as root.

**GLFW reports `XDG_RUNTIME_DIR not set`** — harmless warning; ignore it.

**The trainer exits instantly** — pass `--frames N` to hold it open for N frames,
or run it on a real display.

**`error while loading shared libraries: libGL.so.1`** — install the GL runtime:
`sudo apt-get install -y libgl1 libgl1-mesa-dri libglx-mesa0`.

**Radar port already in use** — choose another: `--port 6400`.

## 9. Uninstall

`logvex` installs nothing outside its own directory. To remove it:

```bash
rm -rf build           # binaries
rm -rf ~/.config/logvex-cs2-practice   # saved config profiles (optional)
```

Then delete the cloned source directory.
