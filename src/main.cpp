#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "logvex/diagnostics.hpp"
#include "logvex/gui/app.hpp"
#include "logvex/log.hpp"

int main(int argc, char** argv) {
    std::vector<std::string> args(argv + 1, argv + argc);
    std::string log_path;
    std::string script_path;
    int frames = 0;

    for (size_t i = 0; i < args.size(); ++i) {
        if (args[i] == "--log" && i + 1 < args.size()) log_path = args[i + 1];
        if (args[i] == "--script" && i + 1 < args.size()) script_path = args[i + 1];
        if (args[i] == "--frames" && i + 1 < args.size()) {
            try {
                frames = std::stoi(args[i + 1]);
            } catch (...) {
                frames = 0;
            }
        }
        if (args[i] == "--self-test") {
            lvx::log::init("", lvx::log::Level::Debug);
            int failures = lvx::run_self_test();
            std::cout << (failures == 0 ? "self-test OK\n" : "self-test FAILED\n");
            return failures == 0 ? 0 : 1;
        }
        if (args[i] == "--dump-environment") {
            lvx::log::init("", lvx::log::Level::Debug);
            std::cout << lvx::format_environment_report(lvx::detect_environment());
            return 0;
        }
    }

    lvx::log::init(log_path, lvx::log::Level::Debug);

    lvx::gui::App app;
    if (!app.init(1400, 860, "Logvex CS2 Practice")) {
        std::cerr << "failed to create the window: no usable display.\n"
                     "Run with a display or: xvfb-run -a -s \"-screen 0 1400x860x24\" "
                     "./build/logvex-practice\n";
        return 1;
    }

    if (!script_path.empty()) {
        std::ifstream script(script_path);
        if (!script) {
            std::cerr << "cannot open script: " << script_path << "\n";
            return 1;
        }
        std::vector<std::string> commands;
        std::string line;
        while (std::getline(script, line)) commands.push_back(line);
        std::cout << "running GUI script: " << script_path << "\n";
        int failures = app.run_script(commands);
        return failures == 0 ? 0 : 1;
    }

    return app.run(frames);
}
