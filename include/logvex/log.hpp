#pragma once

#include <string>
#include <string_view>

namespace lvx::log {

enum class Level { Debug, Info, Warn, Error };

// Initialise file logging. Empty path logs to stderr only. If a path is given,
// every record is appended to the file as well (used to preserve test logs).
void init(const std::string& file_path = "", Level min_level = Level::Info);

void shutdown();

void write(Level level, std::string_view message);

const char* level_name(Level level);

}  // namespace lvx::log

#define LVX_LOG_DEBUG(msg) ::lvx::log::write(::lvx::log::Level::Debug, msg)
#define LVX_LOG_INFO(msg) ::lvx::log::write(::lvx::log::Level::Info, msg)
#define LVX_LOG_WARN(msg) ::lvx::log::write(::lvx::log::Level::Warn, msg)
#define LVX_LOG_ERROR(msg) ::lvx::log::write(::lvx::log::Level::Error, msg)
