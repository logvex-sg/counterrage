#include "logvex/log.hpp"

#include <cstdio>
#include <ctime>
#include <fstream>
#include <mutex>
#include <string>

namespace lvx::log {
namespace {

std::mutex g_mutex;
std::ofstream g_file;
Level g_min_level = Level::Info;

const char* timestamp() {
    static thread_local char buffer[32];
    std::time_t now = std::time(nullptr);
    std::tm tm{};
    localtime_r(&now, &tm);
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &tm);
    return buffer;
}

}  // namespace

const char* level_name(Level level) {
    switch (level) {
        case Level::Debug:
            return "DEBUG";
        case Level::Info:
            return "INFO";
        case Level::Warn:
            return "WARN";
        case Level::Error:
            return "ERROR";
    }
    return "INFO";
}

void init(const std::string& file_path, Level min_level) {
    std::lock_guard<std::mutex> lock(g_mutex);
    g_min_level = min_level;
    if (!file_path.empty()) {
        g_file.open(file_path, std::ios::out | std::ios::app);
    }
}

void shutdown() {
    std::lock_guard<std::mutex> lock(g_mutex);
    if (g_file.is_open()) {
        g_file.flush();
        g_file.close();
    }
}

void write(Level level, std::string_view message) {
    std::lock_guard<std::mutex> lock(g_mutex);
    if (level < g_min_level) {
        return;
    }
    std::string line = "[" + std::string(timestamp()) + "][" + level_name(level) + "] " +
                       std::string(message) + "\n";
    std::fputs(line.c_str(), stderr);
    if (g_file.is_open()) {
        g_file << line;
        g_file.flush();
    }
}

}  // namespace lvx::log
