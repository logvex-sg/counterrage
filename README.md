# logvex — CS2 offline practice suite (Linux, C++20)

`logvex` is a Linux desktop application for practising CS2 mechanics **offline**.
It re-implements the architecture of the Rust reference project
[`avitran0/deadlocked`](https://github.com/avitran0/deadlocked) in C++20 with CMake,
GLFW and Dear ImGui.

> **Scope and safety boundary.** This is a pure simulator/analyser. It never
> touches the CS2 process, its memory, its network traffic, or any anti-cheat
> component, and it never injects input into the game. Everything runs against a
> deterministic in-process world model. The reference project's `unsafe`
> features (no-flash memory writes, FOV changer memory writes, no-smoke) are
> deliberately **not** implemented because they require process manipulation,
> which is out of scope and would be anti-cheat detectable. See
> [Feature coverage](#feature-coverage) for the exact list.

## What is ported from the reference

| Reference (Rust) | logvex (C++20) | Notes |
| --- | --- | --- |
| `shared/src/weapon.rs`, `shared/src/data.rs` | `src/core/weapon.cpp`, `src/core/world.cpp` | Weapon index table, names, classes, damage/armor/penetration/recoil stats, per-bone hitbox scale |
| `cheat/src/math.rs` | `src/core/math.cpp` | `angles_from_vector`, `angles_to_fov`, `vec2_clamp`, `angle_delta`, `soft_clamp_acceleration`, `distance_scale`, `world_to_screen` |
| `cheat/src/config/mod.rs` | `src/core/config.cpp` | aim / hud / player / radar / unsafe config model, per-weapon overrides, JSON persistence, `$XDG_CONFIG_HOME` paths |
| `cheat/src/target.rs` | `src/core/features.cpp` | Bone-priority target selection, FOV/distance targeting, visibility + penetration checks |
| `cheat/src/aimbot`, `rcs`, `triggerbot` | `src/core/features.cpp` | Aim smoothing/inertia, recoil control, triggerbot gating (velocity/scope/flash/head-only) |
| `parser/bvh.rs` (penetration) | `src/core/ballistics.cpp` | Ray/AABB, line of sight, wall-penetration damage estimation |
| `server/src/ws.rs`, `server/src/state.rs` | `src/core/radar_protocol.cpp`, `src/core/radar_server.cpp` | Radar JSON protocol, HTTP page + `/state`, WebSocket `/client` streaming |
| scenario / map data | `src/core/scenarios.cpp` | Synthetic practice scenarios with players, hitboxes, walls, bomb |

## Build

```bash
scripts/install-deps.sh          # Debian/Kali packages
scripts/build.sh                 # cmake + ninja, Release
```

Dependencies: a C++20 compiler, CMake ≥ 3.20, Ninja, GLM, GLFW 3, nlohmann-json,
OpenGL, and (for the desktop app) a display or `xvfb-run`.

Third-party: GLM, GLFW, nlohmann-json come from the system; Dear ImGui is
vendored under `third_party/imgui`.

## Run

```bash
scripts/run.sh                   # interactive trainer (xvfb fallback if headless)
./build/logvex-practice --frames 300   # render a fixed number of frames then exit
```

Controls in the trainer:

| Input | Action |
| --- | --- |
| Right mouse | capture cursor / free look |
| Left mouse | fire |
| `W A S D`, `Shift`, `Ctrl`, `Space` | move / walk / crouch / jump |
| `R` | reload |
| `Z` / `X` | aimbot / triggerbot (hold or toggle, set per config) |
| `1`–`9` | switch weapon |
| `Esc` | quit |

## Headless CLI

```bash
./build/logvex-cli --self-test
./build/logvex-cli --list-scenarios
./build/logvex-cli --frames 300 --scenario aim_arena
./build/logvex-cli --benchmark 20000
./build/logvex-cli --dump-environment
./build/logvex-cli --set aim.global.aimbot.fov=4 --save-config configs/my.json
./build/logvex-cli --load-config configs/competitive.json --dump-config
```

## Web radar

```bash
./build/logvex-radar --port 6347 --scenario dust2_practice
# open http://127.0.0.1:6347/
```

The radar serves an HTML canvas page, a JSON `/state` endpoint and a live
WebSocket at `/client`. It runs the same deterministic simulation locally, so no
game process is required.

## GUI behaviour test

`scripts/gui_demo.txt` drives the exact code paths the ImGui widgets use
(HUD toggles, scenario switching, movement, aimbot activation, weapon selection,
config save/load, radar start/stop) and writes a screenshot. Run it headlessly:

```bash
scripts/gui-test.sh
```

## Feature coverage

Implemented and exercised:

- **Aimbot** — hotkey (hold/toggle), FOV, smooth, inertia, start bullet,
  targeting mode (FOV/distance), visibility check, bone priority list
  (head/spine), distance-adjusted FOV, flash check, penetration check, FOV circle.
- **ESP** — box, skeleton (bone connections), health bar, armor bar, name,
  weapon, bomb timer.
- **Triggerbot** — activation mode, min/max delay, shot duration, visibility,
  flash check, scope check, velocity threshold, head-only.
- **RCS** — standalone recoil control with per-axis strength.
- **Per-weapon overrides** — aimbot/triggerbot/RCS override the global config.
- **Config** — JSON profiles under `$XDG_CONFIG_HOME/logvex-cs2-practice/`,
  example profiles in `configs/`.
- **Web radar** — HTTP + WebSocket streaming.
- **Diagnostics** — read-only environment/Steam/CS2 detection.

Intentionally **not** implemented (documented, not stubbed):

- **Unsafe / memory features** — no-flash, FOV changer via memory, no-smoke,
  smoke colour. These require writing to the CS2 process, which this project
  does not do. The in-simulator equivalents (flash alpha decay, simulated camera
  FOV) are implemented instead.
- **Live game integration** — there is no supported public interface to read CS2
  game state from another process without anti-cheat interference, so the world
  model is synthetic.

## Tests

```bash
scripts/test.sh          # unit tests + CLI self-test + demo + benchmark
ctest --test-dir build --output-on-failure
```

## License

GPL-3.0. See `LICENSE`. The architecture and data tables are derived from the
GPL-3.0 reference project `avitran0/deadlocked`; this port is therefore also
GPL-3.0.
