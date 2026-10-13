#include <atomic>
#include <chrono>
#include <cstdlib>
#include <csignal>
#include <iostream>
#include <string>
#include <thread>

#include "logvex/log.hpp"
#include "logvex/radar_server.hpp"
#include "logvex/scenarios.hpp"
#include "logvex/simulation.hpp"

namespace {
std::atomic<bool> g_stop{false};
void handle_signal(int) { g_stop = true; }
}  // namespace

// Standalone web radar. Runs the deterministic simulation locally and streams it
// to the browser, mirroring the reference `server` crate (axum routes /client,
// /stats) but without needing a running game.
int main(int argc, char** argv) {
    uint16_t port = 6347;
    std::string bind = "127.0.0.1";
    std::string scenario = "dust2_practice";
    bool follow = true;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--port" && i + 1 < argc) port = static_cast<uint16_t>(std::stoi(argv[++i]));
        if (arg == "--bind" && i + 1 < argc) bind = argv[++i];
        if (arg == "--scenario" && i + 1 < argc) scenario = argv[++i];
        if (arg == "--no-follow") follow = false;
        if (arg == "--help") {
            std::cout << "logvex-radar [--port N] [--bind ADDR] [--scenario NAME] [--no-follow]\n";
            return 0;
        }
    }

    lvx::log::init("", lvx::log::Level::Debug);
    lvx::Simulation sim(4242);
    if (!sim.load_scenario(scenario)) {
        std::cerr << "unknown scenario: " << scenario << "\n";
        return 2;
    }
    (void)follow;

    lvx::radar::StateFeed feed;
    lvx::radar::Server server(port, bind, &feed);
    if (!server.start()) {
        std::cerr << "failed to bind " << bind << ":" << port << "\n";
        return 1;
    }
    std::cout << "radar serving on http://" << bind << ":" << server.port() << "/\n";

    std::signal(SIGINT, handle_signal);
    std::signal(SIGTERM, handle_signal);

    lvx::InputState input;
    while (!g_stop) {
        sim.step(input, 1.0f / 64.0f);
        feed.publish(sim.world());
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    server.stop();
    return 0;
}
