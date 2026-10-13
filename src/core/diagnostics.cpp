#include "logvex/diagnostics.hpp"

#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <regex>
#include <sstream>

#include <unistd.h>

#include "logvex/ballistics.hpp"
#include "logvex/features.hpp"
#include "logvex/log.hpp"
#include "logvex/math.hpp"
#include "logvex/simulation.hpp"

namespace lvx {
namespace {

std::string read_file(const std::string& path) {
    std::ifstream in(path);
    if (!in) return {};
    std::stringstream buffer;
    buffer << in.rdbuf();
    return buffer.str();
}

std::string first_line(const std::string& s) {
    auto pos = s.find('\n');
    return pos == std::string::npos ? s : s.substr(0, pos);
}

std::string find_steam_root() {
    const char* home = std::getenv("HOME");
    if (!home) return {};
    std::vector<std::string> candidates = {
        std::string(home) + "/.steam/steam",
        std::string(home) + "/.steam/root",
        std::string(home) + "/.local/share/Steam",
        std::string(home) + "/.var/app/com.valvesoftware.Steam/data/Steam",
    };
    for (const auto& candidate : candidates) {
        if (std::filesystem::exists(candidate)) {
            return candidate;
        }
    }
    return {};
}

}  // namespace

EnvironmentReport detect_environment() {
    EnvironmentReport report;
    report.os_name = first_line(read_file("/etc/os-release"));
    // /etc/os-release PRETTY_NAME is more useful than the first line.
    {
        std::istringstream stream(read_file("/etc/os-release"));
        std::string line;
        while (std::getline(stream, line)) {
            if (line.rfind("PRETTY_NAME=", 0) == 0) {
                std::string value = line.substr(12);
                if (!value.empty() && value.front() == '"') value.erase(0, 1);
                if (!value.empty() && value.back() == '"') value.pop_back();
                report.os_name = value;
            }
        }
    }
    report.kernel = first_line(read_file("/proc/version"));

    // CPU model from /proc/cpuinfo.
    {
        std::istringstream stream(read_file("/proc/cpuinfo"));
        std::string line;
        while (std::getline(stream, line)) {
            if (line.rfind("model name", 0) == 0) {
                auto colon = line.find(':');
                if (colon != std::string::npos) {
                    report.cpu = line.substr(colon + 2);
                }
                break;
            }
        }
    }

    if (const char* session = std::getenv("XDG_SESSION_TYPE")) {
        report.session_type = session;
    }
    if (const char* display = std::getenv("DISPLAY")) {
        report.display = display;
    }
    if (const char* runtime = std::getenv("XDG_RUNTIME_DIR")) {
        report.runtime_dir = runtime;
    }

    report.wayland_available = !report.runtime_dir.empty() &&
                               std::filesystem::exists(report.runtime_dir + "/wayland-0");
    report.x11_available = !report.display.empty() || std::filesystem::exists("/tmp/.X11-unix");
    report.opengl_available = std::filesystem::exists("/usr/lib/x86_64-linux-gnu/libGL.so.1") ||
                              std::filesystem::exists("/usr/lib/x86_64-linux-gnu/libGLX.so.0");

    report.steam_root = find_steam_root();
    report.steam_installed = !report.steam_root.empty();

    // Parse libraryfolders.vdf for the CS2 install (read-only, like the
    // reference's Steam environment detection).
    if (report.steam_installed) {
        std::string vdf = read_file(report.steam_root + "/steamapps/libraryfolders.vdf");
        std::istringstream stream(vdf);
        std::string line;
        std::regex path_re("\"path\"\\s+\"([^\"]+)\"");
        std::smatch match;
        while (std::getline(stream, line)) {
            if (std::regex_search(line, match, path_re)) {
                std::string library = match[1].str();
                std::string candidate = library + "/steamapps/common/Counter-Strike Global Offensive";
                if (std::filesystem::exists(candidate)) {
                    report.cs2_path = candidate;
                    report.cs2_installed = true;
                    break;
                }
            }
        }
        // Fall back to the default library.
        if (!report.cs2_installed) {
            std::string candidate =
                report.steam_root + "/steamapps/common/Counter-Strike Global Offensive";
            if (std::filesystem::exists(candidate)) {
                report.cs2_path = candidate;
                report.cs2_installed = true;
            }
        }
        if (report.cs2_installed) {
            // appmanifest_730.acf buildid (read-only).
            std::string acf = read_file(report.steam_root + "/steamapps/appmanifest_730.acf");
            std::regex build_re("\"buildid\"\\s+\"([0-9]+)\"");
            if (std::regex_search(acf, match, build_re)) {
                report.cs2_build = match[1].str();
            }
        }
    }

    report.checks = {
        {"x11_available", report.x11_available, report.x11_available ? "display reachable" : "no DISPLAY"},
        {"wayland_available", report.wayland_available, report.wayland_available ? "wayland socket" : "not present"},
        {"opengl_available", report.opengl_available, report.opengl_available ? "libGL present" : "missing"},
        {"steam_installed", report.steam_installed, report.steam_root},
        {"cs2_installed", report.cs2_installed, report.cs2_path},
    };
    return report;
}

std::string format_environment_report(const EnvironmentReport& report) {
    std::ostringstream out;
    out << "Logvex CS2 Practice - Environment\n";
    out << "=================================\n";
    out << "OS           : " << report.os_name << "\n";
    out << "Kernel       : " << report.kernel << "\n";
    out << "CPU          : " << report.cpu << "\n";
    out << "Session      : " << (report.session_type.empty() ? "unknown" : report.session_type) << "\n";
    out << "DISPLAY      : " << (report.display.empty() ? "(none)" : report.display) << "\n";
    out << "X11          : " << (report.x11_available ? "available" : "unavailable") << "\n";
    out << "Wayland      : " << (report.wayland_available ? "available" : "unavailable") << "\n";
    out << "OpenGL       : " << (report.opengl_available ? "available" : "unavailable") << "\n";
    out << "Steam        : " << (report.steam_installed ? report.steam_root : "not installed") << "\n";
    out << "CS2          : " << (report.cs2_installed ? report.cs2_path : "not installed") << "\n";
    if (!report.cs2_build.empty()) {
        out << "CS2 build    : " << report.cs2_build << "\n";
    }
    out << "Checks:\n";
    for (const auto& check : report.checks) {
        out << "  [" << (check.found ? "ok" : "--") << "] " << check.name << ": " << check.detail << "\n";
    }
    return out.str();
}

int run_self_test() {
    int failures = 0;
    auto check = [&](const std::string& name, bool condition) {
        LVX_LOG_INFO(std::string(condition ? "PASS " : "FAIL ") + name);
        if (!condition) failures++;
    };

    // Math.
    glm::vec2 angles = math::angles_from_vector(glm::vec3{1.0f, 0.0f, 0.0f});
    check("angles_from_vector +x -> yaw 0", std::fabs(angles.y) < 0.01f && std::fabs(angles.x) < 0.01f);
    check("angles_to_fov identity", math::angles_to_fov({10.0f, 20.0f}, {10.0f, 20.0f}) < 0.001f);
    check("angles_to_fov wrap", std::fabs(math::angles_to_fov({0.0f, 179.0f}, {0.0f, -179.0f}) - 2.0f) < 0.01f);

    // Weapon data.
    check("weapon_from_index ak47", weapon_from_index(7) == Weapon::AK47);
    check("weapon token parse", weapon_from_token("ak47").value_or(Weapon::None) == Weapon::AK47);
    check("ak47 stats present", weapon_stats(Weapon::AK47).has_value());

    // Simulation / ballistics.
    Simulation sim(42);
    check("scenario load", sim.load_scenario("aim_arena"));
    check("players present", sim.world().players.size() >= 4);
    const Player* local = sim.world().local_player();
    check("local player set", local != nullptr);

    if (local) {
        glm::vec3 eye = local->eye_position();
        // Fire straight down the lane at bot_close (y = 400).
        glm::vec3 target = sim.world().players[1].hitbox_for(Bone::Spine3)->center;
        ShotTrace trace = sim.fire_local(target - eye, true);
        check("shot hits a player", trace.hit_player);
    }

    check("no_flash clamps alpha", [] {
        World w;
        Player p;
        p.flash_alpha = 1.0f;
        w.players.push_back(p);
        apply_no_flash(w, 0.3f);
        return w.players[0].flash_alpha <= 0.3f + 1e-4f;
    }());

    // Config round-trip.
    Config config = Config::defaults();
    config.aim.global.aimbot.fov = 7.5f;
    std::string path = config_dir() + "/selftest.json";
    std::string err;
    bool saved = save_config(config, path, &err);
    check("config save", saved);
    if (saved) {
        auto loaded = load_config(path, &err);
        check("config round-trip fov", loaded && std::fabs(loaded->aim.global.aimbot.fov - 7.5f) < 1e-4f);
    }

    LVX_LOG_INFO("self-test complete, failures=" + std::to_string(failures));
    return failures;
}

int run_benchmark(int iterations) {
    Simulation sim(7);
    sim.load_scenario("aim_arena");
    Config config = Config::defaults();

    auto start = std::chrono::steady_clock::now();
    uint64_t targets = 0;
    for (int i = 0; i < iterations; ++i) {
        InputState input;
        input.look_delta = {0.01f, 0.02f};
        sim.step(input, 1.0f / 128.0f);
        Target target = find_target(sim.world(), config);
        if (target.valid()) targets++;
    }
    auto end = std::chrono::steady_clock::now();
    double ms = std::chrono::duration<double, std::milli>(end - start).count();
    double per_iter = ms / static_cast<double>(iterations);
    LVX_LOG_INFO("benchmark: " + std::to_string(iterations) + " iterations in " +
                 std::to_string(ms) + " ms (" + std::to_string(per_iter) + " ms/iter), targets=" +
                 std::to_string(targets));
    return 0;
}

}  // namespace lvx
