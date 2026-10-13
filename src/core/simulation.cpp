#include "logvex/simulation.hpp"

#include <algorithm>
#include <cmath>
#include <random>

#include <glm/gtc/matrix_transform.hpp>

#include "logvex/ballistics.hpp"
#include "logvex/log.hpp"
#include "logvex/math.hpp"
#include "logvex/scenarios.hpp"

namespace lvx {
namespace {

constexpr float kPi = 3.14159265358979f;

glm::vec3 forward_from_angles(const glm::vec2& angles) {
    float pitch = angles.x * kPi / 180.0f;
    float yaw = angles.y * kPi / 180.0f;
    return {std::cos(pitch) * std::cos(yaw), std::cos(pitch) * std::sin(yaw), -std::sin(pitch)};
}

}  // namespace

Simulation::Simulation(uint64_t seed) : seed_(seed) { reset(seed); }

void Simulation::reset(uint64_t seed) {
    seed_ = seed;
    rng_.seed(static_cast<uint32_t>(seed));
    tick_ = 0;
    time_ = 0.0f;
    fire_cooldown_ = 0.0f;
    hit_sequence_ = 0;
    hit_events_.clear();
    shot_traces_.clear();
    world_ = make_scenario("aim_arena").value_or(World{});
    rebuild_view_matrix();
    recompute_visibility();
}

void Simulation::set_world(World world) {
    world_ = std::move(world);
    rebuild_view_matrix();
    recompute_visibility();
}

bool Simulation::load_scenario(const std::string& name) {
    auto scenario = make_scenario(name);
    if (!scenario.has_value()) {
        LVX_LOG_WARN("unknown scenario requested: " + name);
        return false;
    }
    set_world(std::move(*scenario));
    tick_ = 0;
    time_ = 0.0f;
    return true;
}

void Simulation::rebuild_view_matrix() {
    Player* local = world_.local_player();
    if (!local) {
        world_.view_matrix = glm::mat4(1.0f);
        return;
    }
    world_.eye_position = local->eye_position();
    world_.view_angles = local->view_angles;

    glm::vec3 forward = forward_from_angles(local->view_angles);
    glm::vec3 eye = local->eye_position();
    glm::mat4 view = glm::lookAt(eye, eye + forward, glm::vec3{0.0f, 0.0f, 1.0f});
    glm::mat4 projection = glm::perspective(glm::radians(world_.config.fov_override),
                                            world_.config.window_size.x /
                                                std::max(world_.config.window_size.y, 1.0f),
                                            1.0f, 16384.0f);
    world_.view_matrix = projection * view;
}

void Simulation::recompute_visibility() {
    const Player* local = world_.local_player();
    if (!local) return;
    glm::vec3 eye = local->eye_position();
    for (auto& player : world_.players) {
        if (player.index == local->index || !player.is_alive()) {
            player.visible = false;
            continue;
        }
        const Hitbox* head = player.hitbox_for(Bone::Head);
        glm::vec3 point = head ? head->center : player.position + glm::vec3{0.0f, 0.0f, 64.0f};
        player.visible = has_line_of_sight(world_, eye, point);
    }
}

void Simulation::update_recoil(float dt) {
    Player* local = world_.local_player();
    if (!local) return;
    // Reference: deadlocked aim_punch handling (recoil decays to zero).
    local->aim_punch *= std::max(0.0f, 1.0f - dt * 6.0f);
    if (glm::length(local->aim_punch) < 0.001f) {
        local->aim_punch = {0.0f, 0.0f};
    }
}

void Simulation::step_movement(const InputState& input, float dt) {
    Player* local = world_.local_player();
    if (!local || !local->is_alive()) return;

    float yaw = local->view_angles.y * kPi / 180.0f;
    glm::vec3 forward{std::cos(yaw), std::sin(yaw), 0.0f};
    glm::vec3 right{-std::sin(yaw), std::cos(yaw), 0.0f};

    glm::vec3 wish = forward * input.move.y + right * input.move.x;
    if (glm::length(wish) > 1.0f) {
        wish = glm::normalize(wish);
    }
    float speed = input.move_speed * (input.walk ? 0.52f : 1.0f);
    glm::vec3 velocity = wish * speed;
    local->velocity = velocity;
    glm::vec3 old_feet = local->position;
    local->position += velocity * dt;
    local->position.z = input.crouch ? -18.0f : 0.0f;  // crouch lowers the eye line
    // Keep the hitbox rig attached to the new feet position.
    glm::vec3 delta = local->position - old_feet;
    for (auto& hitbox : local->hitboxes) {
        hitbox.center += delta;
    }
}

void Simulation::update_bots(float dt) {
    Player* local = world_.local_player();
    for (auto& bot : world_.players) {
        if (bot.index == (local ? local->index : 0) || !bot.is_alive()) continue;
        // Simple strafing patrol so prediction and velocity checks have motion.
        float phase = std::sin(time_ * 0.8f + static_cast<float>(bot.index));
        glm::vec3 velocity{phase * 90.0f, 0.0f, 0.0f};
        bot.velocity = velocity;
        glm::vec3 new_feet = bot.position + velocity * dt;
        glm::vec3 delta = new_feet - bot.position;
        bot.position = new_feet;
        for (auto& hitbox : bot.hitboxes) {
            hitbox.center += delta;
        }
        // Bots face the local player.
        if (local) {
            glm::vec3 to_local = local->eye_position() - bot.eye_position();
            bot.view_angles = math::angles_from_vector(glm::normalize(to_local));
            bot.yaw = bot.view_angles.y;
        }
        // Occasional return fire when the local player is exposed.
        if (world_.config.bots_engage && local && local->is_alive() && bot.visible &&
            std::uniform_real_distribution<float>(0.0f, 1.0f)(rng_) < dt * 0.5f) {
            bot_fire(bot.index);
        }
    }
}

void Simulation::step(const InputState& input, float dt) {
    dt = std::clamp(dt, 0.0f, 0.1f);
    tick_++;
    time_ += dt;

    Player* local = world_.local_player();
    if (local) {
        local->view_angles += input.look_delta;
        math::vec2_clamp(local->view_angles);
        local->yaw = local->view_angles.y;
        if (input.scoped) local->scoped = true;
        if (input.reload) {
            if (auto stats = weapon_stats(local->weapon)) {
                int need = stats->max_clip - local->ammo;
                int take = std::min(need, local->ammo_reserve);
                local->ammo += take;
                local->ammo_reserve -= take;
            }
        }
    }

    step_movement(input, dt);
    update_recoil(dt);
    update_bots(dt);

    if (world_.bomb.planted) {
        world_.bomb.timer = std::max(0.0f, world_.bomb.timer - dt);
    }

    if (fire_cooldown_ > 0.0f) {
        fire_cooldown_ -= dt;
    }

    rebuild_view_matrix();
    recompute_visibility();

    // Automatic weapons keep firing while attack is held.
    if (input.attack && local && local->is_alive() && fire_cooldown_ <= 0.0f) {
        fire_local();
    }
}

ShotTrace Simulation::fire_local(const glm::vec3& aim_override, bool has_override) {
    ShotTrace trace;
    Player* local = world_.local_player();
    if (!local || !local->is_alive()) {
        return trace;
    }
    auto stats_opt = weapon_stats(local->weapon);
    if (!stats_opt.has_value() || stats_opt->damage <= 0) {
        return trace;
    }
    const WeaponStats& stats = *stats_opt;

    glm::vec3 origin = local->eye_position();
    glm::vec3 dir = has_override ? glm::normalize(aim_override)
                                 : forward_from_angles(local->view_angles);
    trace.origin = origin;
    trace.direction = dir;
    trace.shooter = local->index;

    constexpr float kMaxDistance = 16384.0f;
    auto player_hit = raycast_players(world_, origin, dir, kMaxDistance, world_.local_index);
    auto wall_hit = raycast_walls(world_, origin, dir, kMaxDistance);

    bool blocked = false;
    if (wall_hit.has_value()) {
        if (!player_hit.has_value() || wall_hit->t < player_hit->t) {
            blocked = true;
        }
    }

    float end_t = kMaxDistance;
    if (player_hit.has_value() && !blocked) {
        end_t = player_hit->t;
        trace.hit_player = true;
        trace.victim = world_.players[static_cast<size_t>(player_hit->player_index)].index;
        trace.bone = player_hit->bone;
    } else if (wall_hit.has_value()) {
        end_t = wall_hit->t;
    }
    trace.end = origin + dir * end_t;

    // Consume ammo and apply recoil (infinite ammo is a practice toggle).
    if (local->ammo > 0) {
        local->ammo--;
    }
    local->shots_fired++;
    local->aim_punch += glm::vec2{stats.recoil_magnitude, stats.recoil_magnitude * 0.4f} *
                        (stats.is_sniper ? 1.2f : 1.0f);
    fire_cooldown_ = stats.cycle_time;

    if (trace.hit_player) {
        Player* victim = world_.find_by_index(trace.victim);
        if (victim) {
            float distance = glm::length(trace.end - origin);
            float damage = compute_damage(stats, distance, trace.bone, victim->armor,
                                          victim->has_helmet);
            // Try to apply as penetration when a wall stands before the target.
            bool penetrated = false;
            if (blocked) {
                if (auto pen = estimate_penetration_damage(world_, origin, trace.end, stats)) {
                    damage = *pen;
                    penetrated = true;
                }
            }
            victim->health = std::max(0, victim->health - static_cast<int>(std::round(damage)));
            HitEvent event;
            event.sequence = ++hit_sequence_;
            event.attacker = local->index;
            event.victim = victim->index;
            event.bone = trace.bone;
            event.damage = damage;
            event.remaining_health = victim->health;
            event.penetrated = penetrated;
            if (victim->health <= 0) {
                victim->life = LifeState::Dead;
                event.killed = true;
            }
            hit_events_.push_back(event);
        }
    }
    shot_traces_.push_back(trace);
    if (shot_traces_.size() > 512) {
        shot_traces_.erase(shot_traces_.begin(), shot_traces_.begin() + 256);
    }
    return trace;
}

void Simulation::bot_fire(uint32_t bot_index) {
    Player* bot = world_.find_by_index(bot_index);
    Player* local = world_.local_player();
    if (!bot || !local || !bot->is_alive() || !local->is_alive()) {
        return;
    }
    auto stats_opt = weapon_stats(bot->weapon);
    if (!stats_opt.has_value() || stats_opt->damage <= 0) return;
    const WeaponStats& stats = *stats_opt;

    glm::vec3 origin = bot->eye_position();
    glm::vec3 target = local->eye_position();
    glm::vec3 delta = target - origin;
    float distance = glm::length(delta);
    if (distance < 1e-3f) return;
    glm::vec3 dir = delta / distance;
    if (!has_line_of_sight(world_, origin, target)) {
        return;
    }

    ShotTrace trace;
    trace.origin = origin;
    trace.end = target;
    trace.direction = dir;
    trace.shooter = bot->index;
    trace.hit_player = true;
    trace.victim = local->index;
    trace.bone = Bone::Spine3;
    shot_traces_.push_back(trace);

    float damage = compute_damage(stats, distance, trace.bone, local->armor, local->has_helmet);
    local->health = std::max(0, local->health - static_cast<int>(std::round(damage)));
    HitEvent event;
    event.sequence = ++hit_sequence_;
    event.attacker = bot->index;
    event.victim = local->index;
    event.bone = trace.bone;
    event.damage = damage;
    event.remaining_health = local->health;
    if (local->health <= 0) {
        local->life = LifeState::Dead;
        event.killed = true;
    }
    hit_events_.push_back(event);
}

float Simulation::distance_scale(float distance) const { return math::distance_scale(distance); }

glm::vec3 Simulation::aim_direction(const glm::vec3& target) const {
    const Player* local = world_.local_player();
    if (!local) return {0.0f, 0.0f, 0.0f};
    glm::vec3 delta = target - local->eye_position();
    float len = glm::length(delta);
    return len > 1e-5f ? delta / len : glm::vec3{0.0f, 0.0f, 0.0f};
}

}  // namespace lvx
