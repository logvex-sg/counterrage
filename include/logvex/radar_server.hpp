#pragma once

#include <atomic>
#include <cstdint>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "logvex/world.hpp"

namespace lvx::radar {

// Thread-safe snapshot feed. The simulation thread publishes; the server thread
// reads the latest serialised JSON. Reference: deadlocked `server/src/state.rs`.
class StateFeed {
public:
    void publish(const World& world);
    std::string json() const;

private:
    mutable std::mutex mutex_;
    std::string json_;
};

// Minimal blocking HTTP + WebSocket server built on POSIX sockets. It serves the
// radar page, a JSON /state endpoint and a live /client WebSocket, mirroring the
// reference axum server routes (server/src/ws.rs).
class Server {
public:
    Server(uint16_t port, std::string bind_address, StateFeed* feed);
    ~Server();

    // Binds and starts listening. Returns false on bind failure.
    bool start();

    // Stops the server and joins the thread.
    void stop();

    uint16_t port() const { return port_; }

private:
    void serve_loop();
    void handle_client(int fd);

    uint16_t port_;
    std::string bind_address_;
    StateFeed* feed_;
    int listen_fd_ = -1;
    std::atomic<bool> stop_{false};
    std::thread thread_;
};

}  // namespace lvx::radar
