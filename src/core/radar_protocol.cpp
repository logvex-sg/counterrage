#include "logvex/radar_protocol.hpp"

#include <sstream>

#include "logvex/math.hpp"

namespace lvx::radar {

std::string version_string() {
    return std::to_string(kProtocolVersion.major) + "." + std::to_string(kProtocolVersion.minor) +
           "." + std::to_string(kProtocolVersion.patch);
}

nlohmann::json serialize_state(const World& world) {
    using nlohmann::json;
    json out;
    out["version"] = version_string();
    out["in_game"] = world.in_game;
    out["map_name"] = world.map_name;
    out["is_ffa"] = world.config.free_for_all;
    out["bomb"] = {{"planted", world.bomb.planted},
                   {"timer", world.bomb.timer},
                   {"being_defused", world.bomb.being_defused},
                   {"position", {world.bomb.position.x, world.bomb.position.y, world.bomb.position.z}}};

    json players = json::array();
    for (const auto& player : world.players) {
        json p;
        p["steam_id"] = player.steam_id;
        p["name"] = player.name;
        p["team"] = team_name(player.team);
        p["health"] = player.health;
        p["armor"] = player.armor;
        p["has_helmet"] = player.has_helmet;
        p["has_defuser"] = player.has_defuser;
        p["has_bomb"] = player.has_bomb;
        p["weapon"] = std::string(weapon_name(player.weapon));
        p["position"] = {player.position.x, player.position.y, player.position.z};
        p["rotation"] = player.yaw;
        p["visible"] = player.visible;
        p["is_local"] = (&player - world.players.data()) == world.local_index;
        players.push_back(p);
    }
    out["players"] = players;

    json entities = json::array();
    for (const auto& entity : world.entities) {
        entities.push_back({{"kind", entity_kind_name(entity.kind)},
                            {"entity", entity.entity},
                            {"label", entity.label},
                            {"position", {entity.position.x, entity.position.y, entity.position.z}}});
    }
    out["entities"] = entities;
    return out;
}

HttpRequest parse_request_line(const std::string& request) {
    HttpRequest parsed;
    std::istringstream stream(request);
    std::string line;
    std::getline(stream, line);
    if (!line.empty() && line.back() == '\r') line.pop_back();
    std::istringstream first(line);
    std::string target;
    first >> parsed.method >> target;
    auto query_pos = target.find('?');
    if (query_pos == std::string::npos) {
        parsed.path = target;
    } else {
        parsed.path = target.substr(0, query_pos);
        parsed.query = target.substr(query_pos + 1);
    }
    return parsed;
}

namespace {

const char* kIndexPage = R"HTML(<!doctype html>
<html><head><meta charset="utf-8"><title>Logvex Radar</title>
<style>body{background:#111;color:#eee;font-family:sans-serif;margin:0}
canvas{display:block;margin:auto;background:#1b1b1b}#hud{padding:8px;font-size:13px}</style></head>
<body><div id="hud">Logvex practice radar</div>
<canvas id="radar" width="720" height="720"></canvas>
<script>
const cv=document.getElementById('radar'),ctx=cv.getContext('2d');
function draw(state){
  ctx.clearRect(0,0,cv.width,cv.height);
  const cx=cv.width/2,cy=cv.height/2,scale=0.12;
  const local=state.players.find(p=>p.is_local)||state.players[0];
  if(!local)return;
  ctx.strokeStyle='#333';ctx.beginPath();ctx.moveTo(cx,0);ctx.lineTo(cx,cv.height);
  ctx.moveTo(0,cy);ctx.lineTo(cv.width,cy);ctx.stroke();
  for(const p of state.players){
    const x=cx+(p.position[0]-local.position[0])*scale;
    const y=cy-(p.position[1]-local.position[1])*scale;
    ctx.fillStyle=p.is_local?'#3b82f6':(p.visible?'#ef4444':'#7f1d1d');
    ctx.beginPath();ctx.arc(x,y,6,0,Math.PI*2);ctx.fill();
    ctx.fillStyle='#ccc';ctx.font='11px sans-serif';ctx.fillText(p.name,x+8,y-8);
  }
  document.getElementById('hud').textContent='map='+state.map_name+' players='+state.players.length;
}
async function poll(){try{const r=await fetch('/state');draw(await r.json());}catch(e){}}
setInterval(poll,200);poll();
</script></body></html>)HTML";

}  // namespace

HttpResponse handle_request(const HttpRequest& request, const World& world) {
    HttpResponse response;
    if (request.path == "/" || request.path == "/index.html") {
        response.body = kIndexPage;
        return response;
    }
    if (request.path == "/state" || request.path == "/client") {
        response.content_type = "application/json; charset=utf-8";
        response.body = serialize_state(world).dump();
        return response;
    }
    if (request.path == "/stats") {
        response.content_type = "application/json; charset=utf-8";
        response.body = nlohmann::json{{"version", version_string()},
                                       {"players", world.players.size()},
                                       {"map", world.map_name}}
                            .dump();
        return response;
    }
    response.status = 404;
    response.body = "not found";
    return response;
}

std::string encode_text_frame(const std::string& payload) {
    std::string frame;
    frame.push_back(static_cast<char>(0x81));  // FIN + text opcode
    size_t len = payload.size();
    if (len < 126) {
        frame.push_back(static_cast<char>(len));
    } else if (len <= 0xFFFF) {
        frame.push_back(static_cast<char>(126));
        frame.push_back(static_cast<char>((len >> 8) & 0xFF));
        frame.push_back(static_cast<char>(len & 0xFF));
    } else {
        frame.push_back(static_cast<char>(127));
        for (int i = 7; i >= 0; --i) {
            frame.push_back(static_cast<char>((len >> (i * 8)) & 0xFF));
        }
    }
    frame += payload;
    return frame;
}

bool decode_frame_header(const unsigned char* data, size_t len, bool& fin, int& opcode,
                         uint64_t& payload_len, size_t& header_len) {
    if (len < 2) return false;
    fin = (data[0] & 0x80) != 0;
    opcode = data[0] & 0x0F;
    bool masked = (data[1] & 0x80) != 0;
    uint64_t length = data[1] & 0x7F;
    size_t offset = 2;
    if (length == 126) {
        if (len < 4) return false;
        length = (static_cast<uint64_t>(data[2]) << 8) | data[3];
        offset = 4;
    } else if (length == 127) {
        if (len < 10) return false;
        length = 0;
        for (int i = 0; i < 8; ++i) {
            length = (length << 8) | data[2 + i];
        }
        offset = 10;
    }
    if (masked) offset += 4;
    payload_len = length;
    header_len = offset;
    return true;
}

}  // namespace lvx::radar
