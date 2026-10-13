#include "test.hpp"

#include "logvex/radar_protocol.hpp"
#include "logvex/scenarios.hpp"

using namespace lvx;

TEST(radar_serializes_players) {
    World world = make_scenario("aim_arena").value();
    auto json = radar::serialize_state(world);
    CHECK(json["map_name"] == "aim_arena");
    CHECK(json["players"].is_array());
    CHECK(json["players"].size() == world.players.size());
    CHECK(json["players"][0].contains("position"));
    CHECK(json["players"][0]["is_local"].get<bool>());
}

TEST(radar_http_root_serves_html) {
    World world = make_scenario("aim_arena").value();
    auto request = radar::parse_request_line("GET / HTTP/1.1\r\nHost: x\r\n\r\n");
    CHECK(request.method == "GET");
    CHECK(request.path == "/");
    auto response = radar::handle_request(request, world);
    CHECK(response.status == 200);
    CHECK(response.body.find("radar") != std::string::npos);
}

TEST(radar_http_state_serves_json) {
    World world = make_scenario("dust2_practice").value();
    auto request = radar::parse_request_line("GET /state?x=1 HTTP/1.1\r\n\r\n");
    CHECK(request.path == "/state");
    CHECK(request.query == "x=1");
    auto response = radar::handle_request(request, world);
    CHECK(response.content_type.find("json") != std::string::npos);
    auto parsed = nlohmann::json::parse(response.body);
    CHECK(parsed["bomb"]["planted"].get<bool>());
}

TEST(radar_http_404) {
    World world = make_scenario("aim_arena").value();
    auto request = radar::parse_request_line("GET /nope HTTP/1.1\r\n\r\n");
    auto response = radar::handle_request(request, world);
    CHECK(response.status == 404);
}

TEST(websocket_frame_round_trip) {
    std::string payload = "hello radar";
    std::string frame = radar::encode_text_frame(payload);
    CHECK(static_cast<unsigned char>(frame[0]) == 0x81);
    bool fin = false;
    int opcode = 0;
    uint64_t len = 0;
    size_t header = 0;
    CHECK(radar::decode_frame_header(reinterpret_cast<const unsigned char*>(frame.data()),
                                     frame.size(), fin, opcode, len, header));
    CHECK(fin);
    CHECK(opcode == 1);
    CHECK(len == payload.size());
    CHECK(frame.substr(header) == payload);
}

TEST(websocket_frame_large_payload) {
    std::string payload(300, 'x');
    std::string frame = radar::encode_text_frame(payload);
    bool fin = false;
    int opcode = 0;
    uint64_t len = 0;
    size_t header = 0;
    CHECK(radar::decode_frame_header(reinterpret_cast<const unsigned char*>(frame.data()),
                                     frame.size(), fin, opcode, len, header));
    CHECK(len == 300);
    CHECK(header == 4);
}
