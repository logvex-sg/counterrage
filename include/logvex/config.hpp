#pragma once

#include <map>
#include <optional>
#include <string>
#include <vector>

#include <glm/glm.hpp>
#include <nlohmann/json.hpp>

#include "logvex/types.hpp"
#include "logvex/weapon.hpp"

namespace lvx {

// Reference: deadlocked `cheat/src/config/aim.rs` KeyMode.
enum class KeyMode { Hold, Toggle };

// Reference: deadlocked `cheat/src/config/aim.rs` TargetingMode.
enum class TargetingMode { Fov, Distance };

const char* key_mode_name(KeyMode mode);
const char* targeting_mode_name(TargetingMode mode);

// Reference: deadlocked `cheat/src/config/aim.rs` AimbotConfig.
struct AimbotConfig {
    bool enable_override = false;
    bool enabled = true;
    KeyMode mode = KeyMode::Hold;
    bool target_friendlies = false;
    bool distance_adjusted_fov = true;
    int start_bullet = 0;
    bool visibility_check = true;
    bool flash_check = true;
    float fov = 2.5f;
    float smooth = 5.0f;
    float inertia = 1.0f;
    float prediction_time = 0.05f;
    // Reference default bone priority: head, then down the spine.
    std::vector<Bone> bones = {Bone::Head,    Bone::Neck,   Bone::Spine4, Bone::Spine3,
                               Bone::Spine2,  Bone::Spine1, Bone::Hip};
    TargetingMode targeting_mode = TargetingMode::Fov;
    bool penetration_check = false;
    uint32_t minimum_penetration_damage = 50;
};

// Reference: deadlocked `cheat/src/config/aim.rs` RcsConfig.
struct RcsConfig {
    bool enable_override = false;
    bool enabled = false;
    glm::vec2 strength{0.5f, 0.5f};
};

// Reference: deadlocked `cheat/src/config/aim.rs` TriggerbotConfig.
struct TriggerbotConfig {
    bool enable_override = false;
    bool enabled = false;
    uint64_t delay_min = 100;
    uint64_t delay_max = 200;
    uint64_t shot_duration = 200;
    KeyMode mode = KeyMode::Hold;
    bool flash_check = true;
    bool scope_check = true;
    bool velocity_check = true;
    float velocity_threshold = 100.0f;
    bool head_only = false;
    bool penetration_check = false;
    uint32_t minimum_penetration_damage = 50;
};

struct WeaponConfig {
    AimbotConfig aimbot{};
    RcsConfig rcs{};
    TriggerbotConfig triggerbot{};

    static WeaponConfig enabled(bool on);
};

struct GrenadeAlignConfig {
    bool enabled = false;
    int hotkey = 0;
    float fov = 10.0f;
    float smooth = 25.0f;
};

// Reference: deadlocked `cheat/src/config/aim.rs` AimConfig.
struct AimConfig {
    int aimbot_hotkey = 5;    // Mouse5
    int triggerbot_hotkey = 4;  // Mouse4
    GrenadeAlignConfig grenade_align{};
    WeaponConfig global = WeaponConfig::enabled(true);
    std::map<Weapon, WeaponConfig> weapons;
};

// Reference: deadlocked `cheat/src/config/hud.rs` (subset relevant to practice).
struct HudConfig {
    bool draw_fov_circle = true;
    bool draw_skeleton = true;
    bool draw_boxes = true;
    bool draw_health_bar = true;
    bool draw_armor_bar = true;
    bool draw_names = true;
    bool draw_weapon = true;
    bool draw_bomb_timer = true;
    bool draw_crosshair = true;
    glm::vec4 box_color{1.0f, 0.3f, 0.3f, 1.0f};
};

// Reference: deadlocked `cheat/src/config/radar.rs`.
struct RadarConfig {
    bool enabled = false;
    uint16_t port = 6347;
    bool follow_player = true;
    std::string bind_address = "127.0.0.1";
};

struct PracticeConfig {
    bool god_mode = false;
    bool infinite_ammo = true;
    bool show_impact_markers = true;
    bool show_damage_numbers = true;
};

// Reference: deadlocked `cheat/src/config/mod.rs` Config.
struct Config {
    AimConfig aim{};
    HudConfig hud{};
    RadarConfig radar{};
    PracticeConfig practice{};
    std::string accent_color = "#3b82f6";
    int fps = 120;
    std::string font = "FiraSans";

    static Config defaults();
};

std::string config_dir();
std::string default_config_path();

// JSON (de)serialisation. The reference uses TOML; JSON keeps the same key
// names so a converted file round-trips. Files use the ".json" extension.
bool save_config(const Config& config, const std::string& path, std::string* error = nullptr);
std::optional<Config> load_config(const std::string& path, std::string* error = nullptr);

// Writes a human-readable dump (used by --dump-config).
std::string config_to_string(const Config& config);

// Applies a "key=value" override such as "aim.global.aimbot.fov=5.0".
bool apply_config_override(Config& config, const std::string& assignment, std::string* error);

}  // namespace lvx
