#pragma once

#include <optional>

#include <glm/glm.hpp>

#include "logvex/world.hpp"

namespace lvx {

struct RayHit {
    bool hit = false;
    float t = 0.0f;          // distance along the ray to the entry point
    glm::vec3 point{0.0f};
    int wall_index = -1;     // -1 when the hit is a player hitbox
    int player_index = -1;
    Bone bone = Bone::Head;
};

// Slab test against an axis-aligned box. `tmin` is the entry distance.
bool ray_aabb(const glm::vec3& origin, const glm::vec3& dir, const glm::vec3& box_min,
              const glm::vec3& box_max, float& tmin);

// First wall intersection along a normalised ray, or std::nullopt.
std::optional<RayHit> raycast_walls(const World& world, const glm::vec3& origin,
                                    const glm::vec3& dir, float max_distance);

// First player-hitbox intersection. `ignore_index` skips the local player.
std::optional<RayHit> raycast_players(const World& world, const glm::vec3& origin,
                                      const glm::vec3& dir, float max_distance,
                                      int ignore_index = -1);

// True when nothing blocks the segment origin -> target (world visibility).
bool has_line_of_sight(const World& world, const glm::vec3& origin, const glm::vec3& target);

// Reference: deadlocked `cheat/src/parser/bvh.rs` estimate_penetration_damage().
// Estimates the damage a shot at `target` inflicts after penetrating walls,
// using the weapon penetration data. Returns std::nullopt when no shot lands.
std::optional<float> estimate_penetration_damage(const World& world, const glm::vec3& origin,
                                                 const glm::vec3& target, const WeaponStats& stats);

// Reference: deadlocked damage calculation, simplified to hitscan:
// base damage, hitbox scale, distance falloff, armor absorption.
float compute_damage(const WeaponStats& stats, float distance, Bone bone, int armor,
                     bool has_helmet);

}  // namespace lvx
