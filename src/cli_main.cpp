#include <cstring>
#include <iostream>
#include <string>
#include <vector>

#include "logvex/config.hpp"
#include "logvex/diagnostics.hpp"
#include "logvex/features.hpp"
#include "logvex/log.hpp"
#include "logvex/scenarios.hpp"
#include "logvex/simulation.hpp"

namespace {

void print_usage() {
    std::cout << "logvex-cli - headless CS2 offline-practice engine\n\n"
                 "Usage: logvex-cli [options]\n\n"
                 "  --self-test              run the built-in self test\n"
                 "  --benchmark [N]          benchmark targeting + ballistics (default 5000)\n"
                 "  --dump-config            print the resolved config as JSON\n"
                 "  --load-config PATH       load a config profile before running\n"
                 "  --save-config PATH       write the resolved config to PATH and exit\n"
                 "  --dump-environment       read-only environment / Steam / CS2 detection\n"
                 "  --scenario NAME          load a scenario (default aim_arena)\n"
                 "  --list-scenarios         list available scenarios\n"
                 "  --frames N               simulate N frames and print a summary\n"
                 "  --set key=value          override a config value (repeatable)\n"
                 "  --log PATH               also append logs to PATH\n"
                 "  --help                   this message\n";
}

}  // namespace

int main(int argc, char** argv) {
    std::vector<std::string> args(argv + 1, argv + argc);
    std::string log_path;
    for (size_t i = 0; i < args.size(); ++i) {
        if (args[i] == "--log" && i + 1 < args.size()) log_path = args[i + 1];
    }
    lvx::log::init(log_path, lvx::log::Level::Debug);

    if (args.empty()) {
        print_usage();
        return 0;
    }

    for (const auto& arg : args) {
        if (arg == "--help" || arg == "-h") {
            print_usage();
            return 0;
        }
    }

    lvx::Config config = lvx::Config::defaults();
    std::string scenario = "aim_arena";

    auto value_after = [&](const std::string& flag) -> std::string {
        for (size_t i = 0; i + 1 < args.size(); ++i) {
            if (args[i] == flag) return args[i + 1];
        }
        return {};
    };
    auto flag_present = [&](const std::string& flag) {
        for (const auto& a : args) {
            if (a == flag) return true;
        }
        return false;
    };

    for (size_t i = 0; i < args.size(); ++i) {
        if (args[i] == "--set" && i + 1 < args.size()) {
            std::string error;
            if (!lvx::apply_config_override(config, args[i + 1], &error)) {
                std::cerr << "config override failed: " << error << "\n";
                return 2;
            }
        }
    }

    if (flag_present("--self-test")) {
        int failures = lvx::run_self_test();
        std::cout << (failures == 0 ? "self-test OK\n" : "self-test FAILED\n");
        return failures == 0 ? 0 : 1;
    }

    if (flag_present("--benchmark")) {
        int iterations = 5000;
        std::string value = value_after("--benchmark");
        if (!value.empty()) {
            try {
                iterations = std::stoi(value);
            } catch (...) {
            }
        }
        return lvx::run_benchmark(iterations);
    }

    if (flag_present("--dump-environment")) {
        std::cout << lvx::format_environment_report(lvx::detect_environment());
        return 0;
    }

    if (flag_present("--list-scenarios")) {
        for (const auto& s : lvx::scenarios()) {
            std::cout << "  " << s.name << " - " << s.description << "\n";
        }
        return 0;
    }

    if (flag_present("--load-config")) {
        std::string path = value_after("--load-config");
        std::string error;
        auto loaded = lvx::load_config(path, &error);
        if (loaded) {
            config = *loaded;
        } else {
            std::cerr << "load failed: " << error << "\n";
            return 2;
        }
    }

    if (flag_present("--save-config")) {
        std::string path = value_after("--save-config");
        if (path.empty()) {
            std::cerr << "--save-config needs a path\n";
            return 2;
        }
        std::string error;
        if (!lvx::save_config(config, path, &error)) {
            std::cerr << "save failed: " << error << "\n";
            return 2;
        }
        std::cout << "saved config to " << path << "\n";
        return 0;
    }

    if (flag_present("--dump-config")) {
        std::cout << lvx::config_to_string(config) << "\n";
        return 0;
    }

    if (!value_after("--scenario").empty()) {
        scenario = value_after("--scenario");
    }

    if (flag_present("--frames")) {
        int frames = 0;
        try {
            frames = std::stoi(value_after("--frames"));
        } catch (...) {
            frames = 0;
        }
        lvx::Simulation sim(1337);
        if (!sim.load_scenario(scenario)) {
            std::cerr << "unknown scenario: " << scenario << "\n";
            return 2;
        }
        lvx::InputState input;
        input.move = {0.0f, 0.0f};
        lvx::Target last_target;
        int locked_frames = 0;
        for (int frame = 0; frame < frames; ++frame) {
            // Aim assist drives the view toward the current target each frame.
            lvx::Target target = lvx::find_target(sim.world(), config);
            input.look_delta =
                lvx::compute_aim_step(sim.world(), config, target, 1.0f / 128.0f, target.valid());
            sim.step(input, 1.0f / 128.0f);
            last_target = target;
            if (target.valid()) locked_frames++;
        }
        std::cout << "scenario=" << scenario << " frames=" << frames
                  << " time=" << sim.time() << "s ticks=" << sim.tick() << "\n";
        std::cout << "players=" << sim.world().players.size()
                  << " hits=" << sim.hit_events().size()
                  << " shots=" << sim.shot_traces().size() << "\n";
        std::cout << "locked_frames=" << locked_frames << "/" << frames << "\n";
        std::cout << "target=" << (last_target.valid() ? "locked" : "none");
        if (last_target.valid()) {
            std::cout << " bone=" << lvx::bone_name(last_target.bone)
                      << " fov=" << last_target.fov;
        }
        std::cout << "\n";
        return 0;
    }

    print_usage();
    return 0;
}
