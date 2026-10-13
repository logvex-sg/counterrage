#include "logvex/ballistics.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

namespace lvx {
namespace {

// Slab test returning entry and exit distances along a normalised ray.
bool slab(const glm::vec3& origin, const glm::vec3& dir, const glm::vec3& box_min,
          const glm::vec3& box_max, float& t_entry, float& t_exit) {
    float tmin = -std::numeric_limits<float>::infinity();
    float tmax = std::numeric_limits<float>::infinity();
    for (int axis = 0; axis < 3; ++axis) {
        float o = origin[axis];
        float d = dir[axis];
        float lo = box_min[axis];
        float hi = box_max[axis];
        if (std::fabs(d) < 1e-8f) {
            if (o < lo || o > hi) return false;
            continue;
        }
        float t1 = (lo - o) / d;
        float t2 = (hi - o) / d;
        if (t1 > t2) std::swap(t1, t2);
        tmin = std::max(tmin, t1);
        tmax = std::min(tmax, t2);
        if (tmin > tmax) return false;
    }
    t_entry = tmin;
    t_exit = tmax;
    return true;
}

int hitbox_scale(Bone bone) {
    switch (bone) {
        case Bone::Head: return 400;
        case Bone::Neck: return 100;
        case Bone::Hip: return 75;
        default: return 100;
    }
}

}  // namespace

bool ray_aabb(const glm::vec3& origin, const glm::vec3& dir, const glm::vec3& box_min,
              const glm::vec3& box_max, float& tmin) {
    float t_entry = 0.0f;
    float t_exit = 0.0f;
    if (!slab(origin, dir, box_min, box_max, t_entry, t_exit)) {
        return false;
    }
    if (t_exit < 0.0f) {
        return false;
    }
    tmin = std::max(t_entry, 0.0f);
    return true;
}

std::optional<RayHit> raycast_walls(const World& world, const glm::vec3& origin,
                                    const glm::vec3& dir, float max_distance) {
    RayHit best;
    best.t = max_distance;
    bool found = false;
    for (size_t i = 0; i < world.walls.size(); ++i) {
        float t_entry = 0.0f;
        float t_exit = 0.0f;
        if (!slab(origin, dir, world.walls[i].min, world.walls[i].max, t_entry, t_exit)) {
            continue;
        }
        if (t_exit < 0.0f || t_entry > max_distance) {
            continue;
        }
        float t = std::max(t_entry, 0.0f);
        if (t < best.t) {
            best.hit = true;
            best.t = t;
            best.point = origin + dir * t;
            best.wall_index = static_cast<int>(i);
            found = true;
        }
    }
    if (!found) return std::nullopt;
    return best;
}

std::optional<RayHit> raycast_players(const World& world, const glm::vec3& origin,
                                      const glm::vec3& dir, float max_distance, int ignore_index) {
    RayHit best;
    best.t = max_distance;
    bool found = false;
    for (size_t pi = 0; pi < world.players.size(); ++pi) {
        const Player& player = world.players[pi];
        if (static_cast<int>(pi) == ignore_index || !player.is_alive()) {
            continue;
        }
        for (const auto& hitbox : player.hitboxes) {
            glm::vec3 bmin = hitbox.center - hitbox.half_extents;
            glm::vec3 bmax = hitbox.center + hitbox.half_extents;
            float t_entry = 0.0f;
            float t_exit = 0.0f;
            if (!slab(origin, dir, bmin, bmax, t_entry, t_exit)) {
                continue;
            }
            if (t_exit < 0.0f || t_entry > max_distance) {
                continue;
            }
            float t = std::max(t_entry, 0.0f);
            if (t < best.t) {
                best.hit = true;
                best.t = t;
                best.point = origin + dir * t;
                best.player_index = static_cast<int>(pi);
                best.bone = hitbox.bone;
                best.wall_index = -1;
                found = true;
            }
        }
    }
    if (!found) return std::nullopt;
    return best;
}

bool has_line_of_sight(const World& world, const glm::vec3& origin, const glm::vec3& target) {
    glm::vec3 delta = target - origin;
    float distance = glm::length(delta);
    if (distance < 1e-4f) {
        return true;
    }
    glm::vec3 dir = delta / distance;
    auto wall = raycast_walls(world, origin, dir, distance);
    return !wall.has_value();
}

std::optional<float> estimate_penetration_damage(const World& world, const glm::vec3& origin,
                                                 const glm::vec3& target, const WeaponStats& stats) {
    glm::vec3 delta = target - origin;
    float distance = glm::length(delta);
    if (distance < 1e-4f || stats.damage <= 0 || stats.penetration <= 0.0f) {
        return std::nullopt;
    }
    glm::vec3 dir = delta / distance;

    // Collect wall segments (entry/exit) the ray passes through, ordered.
    struct Segment {
        float entry;
        float exit;
        float damage_modifier;
    };
    std::vector<Segment> segments;
    for (const auto& wall : world.walls) {
        float t_entry = 0.0f;
        float t_exit = 0.0f;
        if (!slab(origin, dir, wall.min, wall.max, t_entry, t_exit)) continue;
        if (t_exit < 0.0f || t_entry > distance) continue;
        segments.push_back({std::max(t_entry, 0.0f), std::min(t_exit, distance), wall.damage_modifier});
    }
    std::sort(segments.begin(), segments.end(),
              [](const Segment& a, const Segment& b) { return a.entry < b.entry; });

    // Reference: deadlocked parser/bvh.rs estimate_penetration_damage().
    // Each wall costs thickness from a penetration budget and retains a share
    // of the damage set by the material's damage_modifier (wood ~0.8,
    // concrete ~0.25).
    const float range_modifier = std::max(stats.range_modifier, 0.0001f);
    float damage = static_cast<float>(stats.damage);
    float budget = stats.penetration * 300.0f;
    float previous_distance = 0.0f;

    for (const auto& seg : segments) {
        float thickness = std::max(seg.exit - seg.entry, 0.0f);
        if (thickness > budget) {
            return std::nullopt;
        }
        budget -= thickness;

        float retain = std::clamp(1.0f - thickness / (stats.penetration * 200.0f), 0.05f, 1.0f);
        damage *= retain * seg.damage_modifier;
        if (damage <= 1.0f) {
            return std::nullopt;
        }
        damage *= std::pow(range_modifier, thickness / 500.0f);
        previous_distance = seg.exit;
    }

    damage *= std::pow(range_modifier, std::max(distance - previous_distance, 0.0f) / 500.0f);
    if (damage <= 0.0f) {
        return std::nullopt;
    }
    return damage;
}

float compute_damage(const WeaponStats& stats, float distance, Bone bone, int armor,
                     bool has_helmet) {
    float damage = static_cast<float>(stats.damage) * (static_cast<float>(hitbox_scale(bone)) / 100.0f);
    float range_modifier = std::max(stats.range_modifier, 0.0001f);
    damage *= std::pow(range_modifier, distance / 500.0f);

    if (armor > 0) {
        bool head = bone == Bone::Head;
        if (!head || has_helmet) {
            damage *= stats.armor_penetration;
        }
    }
    return std::max(damage, 0.0f);
}

}  // namespace lvx
