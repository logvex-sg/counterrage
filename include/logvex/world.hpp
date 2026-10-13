#pragma once

#include <array>
#include <optional>
#include <string>
#include <vector>

#include <glm/glm.hpp>

#include "logvex/types.hpp"
#include "logvex/weapon.hpp"

namespace lvx {

struct Hitbox {
    Bone bone = Bone::Head;
    glm::vec3 center{0.0f};
    glm::vec3 half_extents{0.05f};  // axis-aligned box half sizes
    int damage_scale = 100;         // percent of base weapon damage
};

struct Player {
    uint32_t index = 0;
    uint64_t steam_id = 0;
    std::string name;
    Team team = Team::Unknown;
    LifeState life = LifeState::Dead;
    int health = 100;
    int max_health = 100;
    int armor = 0;
    bool has_helmet = false;
    bool has_defuser = false;
    bool has_bomb = false;
    Weapon weapon = Weapon::None;
    int ammo = 0;
    int ammo_reserve = 0;
    int money = 800;
    bool visible = true;      // computed per-frame by the simulation
    bool flashed = false;
    float flash_alpha = 0.0f;
    bool scoped = false;

    // Motion state (used for aim prediction and velocity checks).
    glm::vec3 position{0.0f};
    glm::vec3 velocity{0.0f};
    glm::vec2 view_angles{0.0f};   // (pitch, yaw) degrees
    float yaw = 0.0f;

    // Last computed recoil punch (degrees) applied to the local view.
    glm::vec2 aim_punch{0.0f};
    int shots_fired = 0;

    std::vector<Hitbox> hitboxes;

    glm::vec3 eye_position() const;
    bool is_alive() const { return life == LifeState::Alive && health > 0; }
    bool is_playing() const { return team == Team::Terrorist || team == Team::CounterTerrorist; }

    const Hitbox* hitbox_for(Bone bone) const;
};

// Read-only geometry used for hitscan: walls are axis-aligned boxes.
struct Wall {
    glm::vec3 min{0.0f};
    glm::vec3 max{0.0f};
    // Reference: deadlocked parser/bvh material damage_modifier.
    float damage_modifier = 0.5f;
};

struct WorldConfig {
    std::string map_name = "de_dust2";
    glm::vec2 window_size{1280.0f, 720.0f};
    bool free_for_all = false;
    // When false the bots never shoot back, so an aim/practice range stays usable.
    bool bots_engage = true;
    float gravity = 800.0f;
    int bomb_site_index = 0;
    glm::vec3 bomb_site_position{0.0f};
    float fov_override = 90.0f;  // simulated camera FOV (practice FOV changer)
};

// The complete simulation state. This mirrors deadlocked `shared/src/data.rs`
// Data struct: players, friendlies, entities, bomb, map metadata, view matrix.
struct World {
    WorldConfig config{};
    bool in_game = true;
    std::vector<Player> players;
    std::vector<EntityInfo> entities;
    std::vector<Wall> walls;
    BombData bomb{};
    std::string map_name;
    std::string map_build_date;
    glm::mat4 view_matrix{1.0f};
    glm::vec2 view_angles{0.0f};
    glm::vec3 eye_position{0.0f};

    // Index into `players` of the local player, or -1.
    int local_index = -1;

    const Player* local_player() const;
    Player* local_player();

    const Player* find_by_index(uint32_t index) const;
    Player* find_by_index(uint32_t index);
};

}  // namespace lvx
