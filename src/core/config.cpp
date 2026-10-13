#include "logvex/config.hpp"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>

#include "logvex/log.hpp"

namespace lvx {
namespace {

Bone bone_from_token(const std::string& token) {
    for (Bone bone : all_bones()) {
        if (bone_name(bone) == token) return bone;
    }
    return Bone::Head;
}

}  // namespace

const char* key_mode_name(KeyMode mode) { return mode == KeyMode::Hold ? "hold" : "toggle"; }
const char* targeting_mode_name(TargetingMode mode) {
    return mode == TargetingMode::Fov ? "fov" : "distance";
}

WeaponConfig WeaponConfig::enabled(bool on) {
    WeaponConfig config;
    config.aimbot.enable_override = on;
    return config;
}

Config Config::defaults() {
    Config config;
    // The reference seeds a HashMap entry for every weapon. Those entries keep
    // enable_override=false so they fall back to the global config; only the
    // global entry has the override flag set.
    for (Weapon weapon : all_weapons()) {
        config.aim.weapons[weapon] = WeaponConfig{};
    }
    config.aim.global = WeaponConfig::enabled(true);
    return config;
}

std::string config_dir() {
    if (const char* xdg = std::getenv("XDG_CONFIG_HOME"); xdg && *xdg) {
        return std::string(xdg) + "/logvex-cs2-practice";
    }
    if (const char* home = std::getenv("HOME"); home && *home) {
        return std::string(home) + "/.config/logvex-cs2-practice";
    }
    return ".";
}

std::string default_config_path() { return config_dir() + "/default.json"; }

namespace {

using nlohmann::json;

json to_json(const AimbotConfig& a) {
    json bones = json::array();
    for (Bone bone : a.bones) bones.push_back(bone_name(bone));
    return json{{"enable_override", a.enable_override},
                {"enabled", a.enabled},
                {"mode", key_mode_name(a.mode)},
                {"target_friendlies", a.target_friendlies},
                {"distance_adjusted_fov", a.distance_adjusted_fov},
                {"start_bullet", a.start_bullet},
                {"visibility_check", a.visibility_check},
                {"flash_check", a.flash_check},
                {"fov", a.fov},
                {"smooth", a.smooth},
                {"inertia", a.inertia},
                {"prediction_time", a.prediction_time},
                {"bones", bones},
                {"targeting_mode", targeting_mode_name(a.targeting_mode)},
                {"penetration_check", a.penetration_check},
                {"minimum_penetration_damage", a.minimum_penetration_damage}};
}

json to_json(const RcsConfig& r) {
    return json{{"enable_override", r.enable_override},
                {"enabled", r.enabled},
                {"strength", {r.strength.x, r.strength.y}}};
}

json to_json(const TriggerbotConfig& t) {
    return json{{"enable_override", t.enable_override},
                {"enabled", t.enabled},
                {"delay", {t.delay_min, t.delay_max}},
                {"shot_duration", t.shot_duration},
                {"mode", key_mode_name(t.mode)},
                {"flash_check", t.flash_check},
                {"scope_check", t.scope_check},
                {"velocity_check", t.velocity_check},
                {"velocity_threshold", t.velocity_threshold},
                {"head_only", t.head_only},
                {"penetration_check", t.penetration_check},
                {"minimum_penetration_damage", t.minimum_penetration_damage}};
}

json to_json(const WeaponConfig& w) {
    return json{{"aimbot", to_json(w.aimbot)}, {"rcs", to_json(w.rcs)},
                {"triggerbot", to_json(w.triggerbot)}};
}

json to_json(const Config& c) {
    json weapons = json::object();
    for (const auto& [weapon, wc] : c.aim.weapons) {
        weapons[std::string(weapon_name(weapon))] = to_json(wc);
    }
    json aim{{"aimbot_hotkey", c.aim.aimbot_hotkey},
             {"triggerbot_hotkey", c.aim.triggerbot_hotkey},
             {"grenade_align",
              {{"enabled", c.aim.grenade_align.enabled},
               {"hotkey", c.aim.grenade_align.hotkey},
               {"fov", c.aim.grenade_align.fov},
               {"smooth", c.aim.grenade_align.smooth}}},
             {"global", to_json(c.aim.global)},
             {"weapons", weapons}};
    json hud{{"draw_fov_circle", c.hud.draw_fov_circle},
             {"draw_skeleton", c.hud.draw_skeleton},
             {"draw_boxes", c.hud.draw_boxes},
             {"draw_health_bar", c.hud.draw_health_bar},
             {"draw_armor_bar", c.hud.draw_armor_bar},
             {"draw_names", c.hud.draw_names},
             {"draw_weapon", c.hud.draw_weapon},
             {"draw_bomb_timer", c.hud.draw_bomb_timer},
             {"draw_crosshair", c.hud.draw_crosshair},
             {"box_color", {c.hud.box_color.r, c.hud.box_color.g, c.hud.box_color.b,
                            c.hud.box_color.a}}};
    json radar{{"enabled", c.radar.enabled},
               {"port", c.radar.port},
               {"follow_player", c.radar.follow_player},
               {"bind_address", c.radar.bind_address}};
    json practice{{"god_mode", c.practice.god_mode},
                  {"infinite_ammo", c.practice.infinite_ammo},
                  {"show_impact_markers", c.practice.show_impact_markers},
                  {"show_damage_numbers", c.practice.show_damage_numbers}};
    return json{{"aim", aim},
                {"hud", hud},
                {"radar", radar},
                {"practice", practice},
                {"accent_color", c.accent_color},
                {"fps", c.fps},
                {"font", c.font}};
}

template <typename T>
T get_or(const json& j, const char* key, T fallback) {
    if (j.contains(key) && !j.at(key).is_null()) {
        try {
            return j.at(key).get<T>();
        } catch (const std::exception&) {
            return fallback;
        }
    }
    return fallback;
}

void from_json(const json& j, AimbotConfig& a) {
    a.enable_override = get_or(j, "enable_override", a.enable_override);
    a.enabled = get_or(j, "enabled", a.enabled);
    a.target_friendlies = get_or(j, "target_friendlies", a.target_friendlies);
    a.distance_adjusted_fov = get_or(j, "distance_adjusted_fov", a.distance_adjusted_fov);
    a.start_bullet = get_or(j, "start_bullet", a.start_bullet);
    a.visibility_check = get_or(j, "visibility_check", a.visibility_check);
    a.flash_check = get_or(j, "flash_check", a.flash_check);
    a.fov = get_or(j, "fov", a.fov);
    a.smooth = get_or(j, "smooth", a.smooth);
    a.inertia = get_or(j, "inertia", a.inertia);
    a.prediction_time = get_or(j, "prediction_time", a.prediction_time);
    a.penetration_check = get_or(j, "penetration_check", a.penetration_check);
    a.minimum_penetration_damage = get_or(j, "minimum_penetration_damage", a.minimum_penetration_damage);
    if (j.contains("mode")) a.mode = j.at("mode").get<std::string>() == "toggle" ? KeyMode::Toggle : KeyMode::Hold;
    if (j.contains("targeting_mode")) {
        a.targeting_mode = j.at("targeting_mode").get<std::string>() == "distance"
                               ? TargetingMode::Distance
                               : TargetingMode::Fov;
    }
    if (j.contains("bones") && j.at("bones").is_array()) {
        a.bones.clear();
        for (const auto& item : j.at("bones")) {
            a.bones.push_back(bone_from_token(item.get<std::string>()));
        }
    }
}

void from_json(const json& j, RcsConfig& r) {
    r.enable_override = get_or(j, "enable_override", r.enable_override);
    r.enabled = get_or(j, "enabled", r.enabled);
    if (j.contains("strength") && j.at("strength").is_array() && j.at("strength").size() == 2) {
        r.strength = {j.at("strength")[0].get<float>(), j.at("strength")[1].get<float>()};
    }
}

void from_json(const json& j, TriggerbotConfig& t) {
    t.enable_override = get_or(j, "enable_override", t.enable_override);
    t.enabled = get_or(j, "enabled", t.enabled);
    if (j.contains("delay") && j.at("delay").is_array() && j.at("delay").size() == 2) {
        t.delay_min = j.at("delay")[0].get<uint64_t>();
        t.delay_max = j.at("delay")[1].get<uint64_t>();
    }
    t.shot_duration = get_or(j, "shot_duration", t.shot_duration);
    t.flash_check = get_or(j, "flash_check", t.flash_check);
    t.scope_check = get_or(j, "scope_check", t.scope_check);
    t.velocity_check = get_or(j, "velocity_check", t.velocity_check);
    t.velocity_threshold = get_or(j, "velocity_threshold", t.velocity_threshold);
    t.head_only = get_or(j, "head_only", t.head_only);
    t.penetration_check = get_or(j, "penetration_check", t.penetration_check);
    t.minimum_penetration_damage = get_or(j, "minimum_penetration_damage", t.minimum_penetration_damage);
    if (j.contains("mode")) t.mode = j.at("mode").get<std::string>() == "toggle" ? KeyMode::Toggle : KeyMode::Hold;
}

void from_json(const json& j, WeaponConfig& w) {
    if (j.contains("aimbot")) from_json(j.at("aimbot"), w.aimbot);
    if (j.contains("rcs")) from_json(j.at("rcs"), w.rcs);
    if (j.contains("triggerbot")) from_json(j.at("triggerbot"), w.triggerbot);
}

}  // namespace

bool save_config(const Config& config, const std::string& path, std::string* error) {
    try {
        std::filesystem::path p(path);
        if (p.has_parent_path()) {
            std::filesystem::create_directories(p.parent_path());
        }
        std::ofstream out(path);
        if (!out) {
            if (error) *error = "cannot open " + path + " for writing";
            return false;
        }
        out << to_json(config).dump(2) << "\n";
        return true;
    } catch (const std::exception& e) {
        if (error) *error = e.what();
        return false;
    }
}

std::optional<Config> load_config(const std::string& path, std::string* error) {
    std::ifstream in(path);
    if (!in) {
        if (error) *error = "cannot open " + path;
        return std::nullopt;
    }
    try {
        json j;
        in >> j;
        Config config = Config::defaults();
        if (j.contains("aim")) {
            const json& aim = j.at("aim");
            config.aim.aimbot_hotkey = get_or(aim, "aimbot_hotkey", config.aim.aimbot_hotkey);
            config.aim.triggerbot_hotkey = get_or(aim, "triggerbot_hotkey", config.aim.triggerbot_hotkey);
            if (aim.contains("global")) from_json(aim.at("global"), config.aim.global);
            if (aim.contains("weapons")) {
                for (auto it = aim.at("weapons").begin(); it != aim.at("weapons").end(); ++it) {
                    if (auto weapon = weapon_from_token(it.key())) {
                        WeaponConfig wc;
                        from_json(it.value(), wc);
                        config.aim.weapons[*weapon] = wc;
                    }
                }
            }
        }
        if (j.contains("hud")) {
            const json& hud = j.at("hud");
            config.hud.draw_fov_circle = get_or(hud, "draw_fov_circle", config.hud.draw_fov_circle);
            config.hud.draw_skeleton = get_or(hud, "draw_skeleton", config.hud.draw_skeleton);
            config.hud.draw_boxes = get_or(hud, "draw_boxes", config.hud.draw_boxes);
            config.hud.draw_health_bar = get_or(hud, "draw_health_bar", config.hud.draw_health_bar);
            config.hud.draw_armor_bar = get_or(hud, "draw_armor_bar", config.hud.draw_armor_bar);
            config.hud.draw_names = get_or(hud, "draw_names", config.hud.draw_names);
            config.hud.draw_weapon = get_or(hud, "draw_weapon", config.hud.draw_weapon);
            config.hud.draw_bomb_timer = get_or(hud, "draw_bomb_timer", config.hud.draw_bomb_timer);
            config.hud.draw_crosshair = get_or(hud, "draw_crosshair", config.hud.draw_crosshair);
        }
        if (j.contains("radar")) {
            const json& radar = j.at("radar");
            config.radar.enabled = get_or(radar, "enabled", config.radar.enabled);
            config.radar.port = get_or(radar, "port", config.radar.port);
            config.radar.follow_player = get_or(radar, "follow_player", config.radar.follow_player);
            config.radar.bind_address = get_or(radar, "bind_address", config.radar.bind_address);
        }
        if (j.contains("practice")) {
            const json& practice = j.at("practice");
            config.practice.god_mode = get_or(practice, "god_mode", config.practice.god_mode);
            config.practice.infinite_ammo = get_or(practice, "infinite_ammo", config.practice.infinite_ammo);
            config.practice.show_impact_markers =
                get_or(practice, "show_impact_markers", config.practice.show_impact_markers);
            config.practice.show_damage_numbers =
                get_or(practice, "show_damage_numbers", config.practice.show_damage_numbers);
        }
        config.accent_color = get_or(j, "accent_color", config.accent_color);
        config.fps = get_or(j, "fps", config.fps);
        config.font = get_or(j, "font", config.font);
        return config;
    } catch (const std::exception& e) {
        if (error) *error = e.what();
        return std::nullopt;
    }
}

std::string config_to_string(const Config& config) {
    return to_json(config).dump(2);
}

bool apply_config_override(Config& config, const std::string& assignment, std::string* error) {
    auto eq = assignment.find('=');
    if (eq == std::string::npos) {
        if (error) *error = "expected key=value";
        return false;
    }
    std::string key = assignment.substr(0, eq);
    std::string value = assignment.substr(eq + 1);
    auto set_float = [&](float& target) {
        target = std::stof(value);
        return true;
    };
    auto set_bool = [&](bool& target) {
        target = (value == "1" || value == "true" || value == "on");
        return true;
    };
    auto set_int = [&](int& target) {
        target = std::stoi(value);
        return true;
    };

    try {
        if (key == "aim.global.aimbot.fov") return set_float(config.aim.global.aimbot.fov);
        if (key == "aim.global.aimbot.smooth") return set_float(config.aim.global.aimbot.smooth);
        if (key == "aim.global.aimbot.inertia") return set_float(config.aim.global.aimbot.inertia);
        if (key == "aim.global.aimbot.enabled") return set_bool(config.aim.global.aimbot.enabled);
        if (key == "aim.global.aimbot.visibility_check") return set_bool(config.aim.global.aimbot.visibility_check);
        if (key == "aim.global.aimbot.penetration_check") return set_bool(config.aim.global.aimbot.penetration_check);
        if (key == "aim.global.aimbot.distance_adjusted_fov") return set_bool(config.aim.global.aimbot.distance_adjusted_fov);
        if (key == "aim.global.aimbot.flash_check") return set_bool(config.aim.global.aimbot.flash_check);
        if (key == "aim.global.aimbot.target_friendlies") return set_bool(config.aim.global.aimbot.target_friendlies);
        if (key == "aim.global.aimbot.prediction_time") return set_float(config.aim.global.aimbot.prediction_time);
        if (key == "aim.global.aimbot.start_bullet") return set_int(config.aim.global.aimbot.start_bullet);
        if (key == "aim.global.aimbot.minimum_penetration_damage") {
            config.aim.global.aimbot.minimum_penetration_damage = static_cast<uint32_t>(std::stoi(value));
            return true;
        }
        if (key == "aim.global.triggerbot.velocity_check") return set_bool(config.aim.global.triggerbot.velocity_check);
        if (key == "aim.global.triggerbot.velocity_threshold") return set_float(config.aim.global.triggerbot.velocity_threshold);
        if (key == "hud.draw_boxes") return set_bool(config.hud.draw_boxes);
        if (key == "hud.draw_skeleton") return set_bool(config.hud.draw_skeleton);
        if (key == "aim.global.aimbot.targeting_mode") {
            config.aim.global.aimbot.targeting_mode =
                value == "distance" ? TargetingMode::Distance : TargetingMode::Fov;
            return true;
        }
        if (key == "aim.global.rcs.enabled") return set_bool(config.aim.global.rcs.enabled);
        if (key == "aim.global.triggerbot.enabled") return set_bool(config.aim.global.triggerbot.enabled);
        if (key == "aim.global.triggerbot.head_only") return set_bool(config.aim.global.triggerbot.head_only);
        if (key == "radar.enabled") return set_bool(config.radar.enabled);
        if (key == "radar.port") {
            config.radar.port = static_cast<uint16_t>(std::stoi(value));
            return true;
        }
        if (key == "fps") return set_int(config.fps);
        if (key == "practice.god_mode") return set_bool(config.practice.god_mode);
    } catch (const std::exception& e) {
        if (error) *error = std::string("bad value for ") + key + ": " + e.what();
        return false;
    }
    if (error) *error = "unknown config key: " + key;
    return false;
}

}  // namespace lvx
