#pragma once

#include <string>
#include <vector>

namespace lvx {

// Result of a read-only environment probe. No file outside the user's own
// config/home directories is touched and no process is modified.
struct Detection {
    std::string name;
    bool found = false;
    std::string detail;
};

struct EnvironmentReport {
    std::string os_name;
    std::string kernel;
    std::string cpu;
    std::string session_type;      // x11 / wayland / none
    std::string display;
    std::string runtime_dir;
    bool x11_available = false;
    bool wayland_available = false;
    bool opengl_available = false;
    bool steam_installed = false;
    bool cs2_installed = false;
    std::string steam_root;
    std::string cs2_path;
    std::string cs2_build;
    std::vector<Detection> checks;
};

// Reads /proc, /etc/os-release and the Steam library VDF/ACF metadata, exactly
// like the reference's read-only detection path. Never opens game memory.
EnvironmentReport detect_environment();

std::string format_environment_report(const EnvironmentReport& report);

// Self-test: exercises the core subsystems on synthetic data and returns the
// number of failed checks. Writes a line per check through lvx::log.
int run_self_test();

// Micro-benchmark of the hot paths (targeting + ballistics). Returns 0.
int run_benchmark(int iterations);

}  // namespace lvx
