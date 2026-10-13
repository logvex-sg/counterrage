#include "logvex/scenarios.hpp"

#include <cmath>
#include <optional>

namespace lvx {
namespace {

// Builds a CS2-proportioned humanoid hitbox set anchored at `feet`. Dimensions
// are in game units (1 unit ~= 1.9 cm) and follow the reference bone layout.
void add_human_hitboxes(Player& player, const glm::vec3& feet, float yaw_deg) {
    float yaw = yaw_deg * 3.14159265358979f / 180.0f;
    float cs = std::cos(yaw);
    float sn = std::sin(yaw);
    auto rotate = [&](float fx, float fy, float fz) {
        return glm::vec3{feet.x + fx * cs - fy * sn, feet.y + fx * sn + fy * cs, feet.z + fz};
    };

    auto push = [&](Bone bone, const glm::vec3& center, const glm::vec3& half, int scale) {
        player.hitboxes.push_back(Hitbox{bone, center, half, scale});
    };

    // Head/neck (critical hitboxes).
    push(Bone::Head, rotate(0.0f, 0.0f, 64.0f), {4.0f, 4.0f, 4.5f}, 400);
    push(Bone::Neck, rotate(0.0f, 0.0f, 56.0f), {3.0f, 3.0f, 3.0f}, 100);
    // Spine chain.
    push(Bone::Spine4, rotate(0.0f, 0.0f, 50.0f), {6.0f, 4.0f, 5.0f}, 100);
    push(Bone::Spine3, rotate(0.0f, 0.0f, 43.0f), {6.5f, 4.5f, 5.0f}, 100);
    push(Bone::Spine2, rotate(0.0f, 0.0f, 36.0f), {6.5f, 4.5f, 5.0f}, 100);
    push(Bone::Spine1, rotate(0.0f, 0.0f, 29.0f), {6.5f, 4.5f, 5.0f}, 100);
    push(Bone::Hip, rotate(0.0f, 0.0f, 22.0f), {7.0f, 5.0f, 5.0f}, 75);
    // Arms.
    push(Bone::LeftShoulder, rotate(-7.0f, 0.0f, 52.0f), {3.0f, 3.0f, 3.0f}, 75);
    push(Bone::LeftElbow, rotate(-10.0f, 0.0f, 42.0f), {2.5f, 2.5f, 3.0f}, 75);
    push(Bone::LeftHand, rotate(-11.0f, 0.0f, 33.0f), {2.0f, 2.0f, 2.5f}, 75);
    push(Bone::RightShoulder, rotate(7.0f, 0.0f, 52.0f), {3.0f, 3.0f, 3.0f}, 75);
    push(Bone::RightElbow, rotate(10.0f, 0.0f, 42.0f), {2.5f, 2.5f, 3.0f}, 75);
    push(Bone::RightHand, rotate(11.0f, 0.0f, 33.0f), {2.0f, 2.0f, 2.5f}, 75);
    // Legs.
    push(Bone::LeftHip, rotate(-4.0f, 0.0f, 20.0f), {3.0f, 3.0f, 4.0f}, 75);
    push(Bone::LeftKnee, rotate(-4.0f, 0.0f, 11.0f), {2.5f, 2.5f, 3.0f}, 75);
    push(Bone::LeftFoot, rotate(-4.0f, 0.0f, 3.0f), {3.0f, 4.0f, 3.0f}, 75);
    push(Bone::RightHip, rotate(4.0f, 0.0f, 20.0f), {3.0f, 3.0f, 4.0f}, 75);
    push(Bone::RightKnee, rotate(4.0f, 0.0f, 11.0f), {2.5f, 2.5f, 3.0f}, 75);
    push(Bone::RightFoot, rotate(4.0f, 0.0f, 3.0f), {3.0f, 4.0f, 3.0f}, 75);
}

Player make_player(uint32_t index, const char* name, Team team, const glm::vec3& feet,
                   Weapon weapon, float yaw_deg = 0.0f) {
    Player player;
    player.index = index;
    player.steam_id = 76561198000000000ull + index;
    player.name = name;
    player.team = team;
    player.life = LifeState::Alive;
    player.health = 100;
    player.max_health = 100;
    player.armor = 100;
    player.has_helmet = true;
    player.weapon = weapon;
    player.position = feet;
    player.yaw = yaw_deg;
    player.view_angles = {0.0f, yaw_deg};
    if (auto stats = weapon_stats(weapon)) {
        player.ammo = stats->max_clip;
        player.ammo_reserve = stats->max_clip * 3;
    }
    add_human_hitboxes(player, feet, yaw_deg);
    return player;
}

Wall make_wall(const glm::vec3& min, const glm::vec3& max, float damage_modifier) {
    return Wall{min, max, damage_modifier};
}

}  // namespace

const std::vector<ScenarioInfo>& scenarios() {
    static const std::vector<ScenarioInfo> list = {
        {"aim_arena", "Flat range with five bots spread across the FOV"},
        {"dust2_practice", "Simplified mid/doors geometry with cover and a bombsite"},
        {"wallbang_lane", "Corridor with a penetrable wall and bots behind it"},
        {"movement_range", "Open ground for movement/velocity triggerbot checks"},
    };
    return list;
}

std::optional<World> make_scenario(const std::string& name) {
    World world;
    world.config.map_name = name;
    world.map_name = name;
    world.map_build_date = "synthetic-2024-01-01";
    world.config.window_size = {1280.0f, 720.0f};

    if (name == "aim_arena") {
        // A practice range: bots hold still and do not shoot back.
        world.config.bots_engage = false;
        world.players.push_back(make_player(1, "you", Team::CounterTerrorist, {0.0f, 0.0f, 0.0f},
                                            Weapon::AK47, 90.0f));
        world.local_index = 0;
        world.players.push_back(make_player(2, "bot_close", Team::Terrorist, {0.0f, 400.0f, 0.0f},
                                            Weapon::AK47, 180.0f));
        world.players.push_back(make_player(3, "bot_mid", Team::Terrorist, {120.0f, 700.0f, 0.0f},
                                            Weapon::AK47, 180.0f));
        world.players.push_back(make_player(4, "bot_left", Team::Terrorist, {-260.0f, 900.0f, 0.0f},
                                            Weapon::Awp, 180.0f));
        world.players.push_back(make_player(5, "bot_far", Team::Terrorist, {0.0f, 1500.0f, 0.0f},
                                            Weapon::M4A4, 180.0f));
        world.players.push_back(make_player(6, "teammate", Team::CounterTerrorist,
                                            {-200.0f, 600.0f, 0.0f}, Weapon::M4A1S, 180.0f));
        return world;
    }

    if (name == "wallbang_lane") {
        world.players.push_back(make_player(1, "you", Team::CounterTerrorist, {0.0f, 0.0f, 0.0f},
                                            Weapon::AK47, 90.0f));
        world.local_index = 0;
        // Thin penetrable wooden wall (damage_modifier 0.8) directly ahead.
        world.walls.push_back(make_wall({-200.0f, 500.0f, 0.0f}, {200.0f, 516.0f, 160.0f}, 0.8f));
        world.players.push_back(make_player(2, "behind_wall", Team::Terrorist,
                                            {0.0f, 640.0f, 0.0f}, Weapon::AK47, 180.0f));
        // Thick concrete wall (damage_modifier 0.25) further ahead.
        world.walls.push_back(make_wall({-200.0f, 900.0f, 0.0f}, {200.0f, 950.0f, 160.0f}, 0.25f));
        world.players.push_back(make_player(3, "behind_concrete", Team::Terrorist,
                                            {0.0f, 1100.0f, 0.0f}, Weapon::AK47, 180.0f));
        return world;
    }

    if (name == "movement_range") {
        world.players.push_back(make_player(1, "you", Team::CounterTerrorist, {0.0f, 0.0f, 0.0f},
                                            Weapon::AK47, 90.0f));
        world.local_index = 0;
        Player moving = make_player(2, "runner", Team::Terrorist, {-400.0f, 600.0f, 0.0f},
                                    Weapon::Glock, 180.0f);
        moving.velocity = {200.0f, 0.0f, 0.0f};
        world.players.push_back(moving);
        return world;
    }

    if (name == "dust2_practice") {
        world.config.bomb_site_position = {0.0f, 1200.0f, 0.0f};
        world.players.push_back(make_player(1, "you", Team::CounterTerrorist, {0.0f, 0.0f, 0.0f},
                                            Weapon::M4A4, 90.0f));
        world.local_index = 0;
        // Mid doors: two pillars leaving a gap, like the reference mid geometry.
        world.walls.push_back(make_wall({-400.0f, 800.0f, 0.0f}, {-180.0f, 860.0f, 160.0f}, 0.5f));
        world.walls.push_back(make_wall({180.0f, 800.0f, 0.0f}, {400.0f, 860.0f, 160.0f}, 0.5f));
        world.players.push_back(make_player(2, "catwalk", Team::Terrorist, {-60.0f, 1100.0f, 0.0f},
                                            Weapon::AK47, 180.0f));
        world.players.push_back(make_player(3, "long_door", Team::Terrorist, {300.0f, 1400.0f, 0.0f},
                                            Weapon::Awp, 200.0f));
        world.players.push_back(make_player(4, "mid_cover", Team::Terrorist, {0.0f, 900.0f, 0.0f},
                                            Weapon::M4A4, 180.0f));
        // Bomb plant state for the timer HUD.
        world.bomb.planted = true;
        world.bomb.timer = 40.0f;
        world.bomb.position = {0.0f, 1200.0f, 0.0f};
        // A dropped weapon and a smoke for the entity overlay.
        EntityInfo dropped;
        dropped.kind = EntityKind::Weapon;
        dropped.entity = 500;
        dropped.position = {-150.0f, 500.0f, 0.0f};
        dropped.label = "AK-47";
        dropped.weapon.weapon_name = "AK-47";
        world.entities.push_back(dropped);

        EntityInfo smoke;
        smoke.kind = EntityKind::Smoke;
        smoke.entity = 501;
        smoke.position = {100.0f, 1000.0f, 0.0f};
        smoke.label = "Smoke";
        smoke.grenade = GrenadeInfo{501, smoke.position, "Smoke"};
        world.entities.push_back(smoke);
        return world;
    }

    return std::nullopt;
}

}  // namespace lvx
