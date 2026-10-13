#include "logvex/radar_server.hpp"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <poll.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cctype>
#include <cstring>
#include <vector>

#include "logvex/log.hpp"
#include "logvex/radar_protocol.hpp"

namespace lvx::radar {
namespace {

// Minimal SHA-1 (RFC 3174) used only for the WebSocket handshake.
void sha1(const unsigned char* data, size_t len, unsigned char out[20]) {
    uint32_t h[5] = {0x67452301, 0xEFCDAB89, 0x98BADCFE, 0x10325476, 0xC3D2E1F0};
    std::vector<unsigned char> message(data, data + len);
    uint64_t bit_len = static_cast<uint64_t>(len) * 8;
    message.push_back(0x80);
    while (message.size() % 64 != 56) message.push_back(0x00);
    for (int i = 7; i >= 0; --i) {
        message.push_back(static_cast<unsigned char>((bit_len >> (i * 8)) & 0xFF));
    }
    for (size_t chunk = 0; chunk < message.size(); chunk += 64) {
        uint32_t w[80];
        for (int i = 0; i < 16; ++i) {
            w[i] = (message[chunk + i * 4] << 24) | (message[chunk + i * 4 + 1] << 16) |
                   (message[chunk + i * 4 + 2] << 8) | (message[chunk + i * 4 + 3]);
        }
        for (int i = 16; i < 80; ++i) {
            uint32_t v = w[i - 3] ^ w[i - 8] ^ w[i - 14] ^ w[i - 16];
            w[i] = (v << 1) | (v >> 31);
        }
        uint32_t a = h[0], b = h[1], c = h[2], d = h[3], e = h[4];
        for (int i = 0; i < 80; ++i) {
            uint32_t f, k;
            if (i < 20) {
                f = (b & c) | ((~b) & d);
                k = 0x5A827999;
            } else if (i < 40) {
                f = b ^ c ^ d;
                k = 0x6ED9EBA1;
            } else if (i < 60) {
                f = (b & c) | (b & d) | (c & d);
                k = 0x8F1BBCDC;
            } else {
                f = b ^ c ^ d;
                k = 0xCA62C1D6;
            }
            uint32_t temp = ((a << 5) | (a >> 27)) + f + e + k + w[i];
            e = d;
            d = c;
            c = (b << 30) | (b >> 2);
            b = a;
            a = temp;
        }
        h[0] += a;
        h[1] += b;
        h[2] += c;
        h[3] += d;
        h[4] += e;
    }
    for (int i = 0; i < 5; ++i) {
        out[i * 4] = static_cast<unsigned char>((h[i] >> 24) & 0xFF);
        out[i * 4 + 1] = static_cast<unsigned char>((h[i] >> 16) & 0xFF);
        out[i * 4 + 2] = static_cast<unsigned char>((h[i] >> 8) & 0xFF);
        out[i * 4 + 3] = static_cast<unsigned char>(h[i] & 0xFF);
    }
}

std::string base64(const unsigned char* data, size_t len) {
    static const char* table = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string out;
    for (size_t i = 0; i < len; i += 3) {
        uint32_t n = data[i] << 16;
        if (i + 1 < len) n |= data[i + 1] << 8;
        if (i + 2 < len) n |= data[i + 2];
        out.push_back(table[(n >> 18) & 63]);
        out.push_back(table[(n >> 12) & 63]);
        out.push_back(i + 1 < len ? table[(n >> 6) & 63] : '=');
        out.push_back(i + 2 < len ? table[n & 63] : '=');
    }
    return out;
}

std::string websocket_accept(const std::string& key) {
    const std::string magic = key + "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
    unsigned char digest[20];
    sha1(reinterpret_cast<const unsigned char*>(magic.data()), magic.size(), digest);
    return base64(digest, 20);
}

bool send_all(int fd, const std::string& data) {
    size_t sent = 0;
    while (sent < data.size()) {
        ssize_t n = ::send(fd, data.data() + sent, data.size() - sent, MSG_NOSIGNAL);
        if (n <= 0) return false;
        sent += static_cast<size_t>(n);
    }
    return true;
}

std::string read_request(int fd) {
    std::string request;
    char buffer[1024];
    while (request.find("\r\n\r\n") == std::string::npos && request.size() < 16384) {
        ssize_t n = ::recv(fd, buffer, sizeof(buffer), 0);
        if (n <= 0) break;
        request.append(buffer, static_cast<size_t>(n));
    }
    return request;
}

std::string header_value(const std::string& request, const std::string& name) {
    std::string lower;
    lower.reserve(request.size());
    for (char c : request) lower.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
    std::string needle;
    needle.reserve(name.size() + 1);
    for (char c : name) needle.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
    needle.push_back(':');
    auto pos = lower.find(needle);
    if (pos == std::string::npos) return {};
    auto start = pos + needle.size();
    while (start < request.size() && (request[start] == ' ' || request[start] == '\t')) start++;
    auto end = request.find("\r\n", start);
    return request.substr(start, end == std::string::npos ? std::string::npos : end - start);
}

}  // namespace

void StateFeed::publish(const World& world) {
    std::string payload = serialize_state(world).dump();
    std::lock_guard<std::mutex> lock(mutex_);
    json_ = std::move(payload);
}

std::string StateFeed::json() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return json_;
}

Server::Server(uint16_t port, std::string bind_address, StateFeed* feed)
    : port_(port), bind_address_(std::move(bind_address)), feed_(feed) {}

Server::~Server() { stop(); }

bool Server::start() {
    listen_fd_ = ::socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd_ < 0) {
        LVX_LOG_ERROR("radar: socket() failed");
        return false;
    }
    int one = 1;
    ::setsockopt(listen_fd_, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));
    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_port = htons(port_);
    if (::inet_pton(AF_INET, bind_address_.c_str(), &address.sin_addr) != 1) {
        address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    }
    if (::bind(listen_fd_, reinterpret_cast<sockaddr*>(&address), sizeof(address)) < 0) {
        LVX_LOG_ERROR("radar: bind failed on port " + std::to_string(port_));
        ::close(listen_fd_);
        listen_fd_ = -1;
        return false;
    }
    ::listen(listen_fd_, 8);
    stop_ = false;
    thread_ = std::thread([this] { serve_loop(); });
    LVX_LOG_INFO("radar: listening on " + bind_address_ + ":" + std::to_string(port_));
    return true;
}

void Server::stop() {
    stop_ = true;
    if (thread_.joinable()) thread_.join();
    if (listen_fd_ >= 0) {
        ::close(listen_fd_);
        listen_fd_ = -1;
    }
}

void Server::serve_loop() {
    while (!stop_) {
        pollfd pfd{listen_fd_, POLLIN, 0};
        int ready = ::poll(&pfd, 1, 100);
        if (ready <= 0) continue;
        int client = ::accept(listen_fd_, nullptr, nullptr);
        if (client < 0) continue;
        handle_client(client);
        ::close(client);
    }
}

void Server::handle_client(int fd) {
    std::string request = read_request(fd);
    if (request.empty()) return;

    std::string upgrade = header_value(request, "Upgrade");
    if (!upgrade.empty() && upgrade.find("websocket") != std::string::npos) {
        std::string key = header_value(request, "Sec-WebSocket-Key");
        std::string response = "HTTP/1.1 101 Switching Protocols\r\n"
                               "Upgrade: websocket\r\nConnection: Upgrade\r\n"
                               "Sec-WebSocket-Accept: " +
                               websocket_accept(key) + "\r\n\r\n";
        if (!send_all(fd, response)) return;
        // Stream state frames until the peer disconnects or we stop.
        for (int i = 0; i < 600 && !stop_; ++i) {
            std::string frame = encode_text_frame(feed_ ? feed_->json() : "{}");
            if (!send_all(fd, frame)) break;
            pollfd pfd{fd, POLLIN, 0};
            if (::poll(&pfd, 1, 100) > 0) {
                char scratch[64];
                ssize_t n = ::recv(fd, scratch, sizeof(scratch), 0);
                if (n <= 0) break;
            }
        }
        return;
    }

    HttpRequest parsed = parse_request_line(request);
    // The standalone server has no world, so an empty World renders 404/static.
    World empty;
    HttpResponse response = handle_request(parsed, empty);
    if (parsed.path == "/state" || parsed.path == "/client" || parsed.path == "/stats") {
        response.content_type = "application/json; charset=utf-8";
        response.body = feed_ ? feed_->json() : "{}";
        response.status = 200;
    }
    std::string out = "HTTP/1.1 " + std::to_string(response.status) + " OK\r\n" +
                      "Content-Type: " + response.content_type + "\r\n" +
                      "Content-Length: " + std::to_string(response.body.size()) + "\r\n" +
                      "Connection: close\r\n\r\n" + response.body;
    send_all(fd, out);
}

}  // namespace lvx::radar
