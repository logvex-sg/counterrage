#pragma once

#include <string>

#include <nlohmann/json.hpp>

#include "logvex/world.hpp"

namespace lvx::radar {

// Reference: deadlocked `shared/src/data.rs` Data -> postcard payload, and the
// radar Svelte front-end consuming it. The C++ port keeps the same field names
// but serialises to JSON so a stock browser can consume it directly.
nlohmann::json serialize_state(const World& world);

// Reference: deadlocked `shared/src/version.rs`.
struct ProtocolVersion {
    int major = 1;
    int minor = 0;
    int patch = 0;
};

inline constexpr ProtocolVersion kProtocolVersion{};

std::string version_string();

// HTTP response for "/" (radar page) and "/state" (JSON snapshot).
struct HttpRequest {
    std::string method;
    std::string path;
    std::string query;
};

struct HttpResponse {
    int status = 200;
    std::string content_type = "text/html; charset=utf-8";
    std::string body;
};

HttpRequest parse_request_line(const std::string& request);
HttpResponse handle_request(const HttpRequest& request, const World& world);

// Minimal RFC6455 frame helpers used by the WebSocket server.
std::string encode_text_frame(const std::string& payload);
bool decode_frame_header(const unsigned char* data, size_t len, bool& fin, int& opcode,
                         uint64_t& payload_len, size_t& header_len);

}  // namespace lvx::radar
