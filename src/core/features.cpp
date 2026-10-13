#include "logvex/features.hpp"

#include <algorithm>
#include <cmath>

#include "logvex/ballistics.hpp"
#include "logvex/math.hpp"

namespace lvx {

const WeaponConfig& effective_weapon_config(const Config& config, Weapon weapon) {
    auto it = config.aim.weapons.find(weapon);
    if (it != config.aim.weapons.end()) {
        return it->second;
    }
    return config.aim.global;
}

const AimbotConfig& effective_aimbot(const Config& config, Weapon weapon) {
    const WeaponConfig& wc = effective_weapon_config(config, weapon);
    if (wc.aimbot.enable_override) return wc.aimbot;
    return config.aim.global.aimbot;
}

const TriggerbotConfig& effective_triggerbot(const Config& config, Weapon weapon) {
    const WeaponConfig& wc = effective_weapon_config(config, weapon);
    if (wc.triggerbot.enable_override) return wc.triggerbot;
    return config.aim.global.triggerbot;
}

const RcsConfig& effective_rcs(const Config& config, Weapon weapon) {
    const WeaponConfig& wc = effective_weapon_config(config, weapon);
    if (wc.rcs.enable_override) return wc.rcs;
    return config.aim.global.rcs;
}

Target find_target(const World& world, const Config& config) {
    Target target;
    const Player* local = world.local_player();
    if (!local || !local->is_alive() || !local->is_playing()) {
        return target;
    }

    const AimbotConfig& cfg = effective_aimbot(config, local->weapon);
    if (cfg.bones.empty()) {
        return target;
    }

    WeaponClass local_class = weapon_class(local->weapon);
    glm::vec2 punch = local->aim_punch * 2.0f;
    if (local_class == WeaponClass::Sniper) {
        punch = {0.0f, 0.0f};
    }
    // The reference removes the punch from the aim direction so the crosshair
    // lands where the recoil is actually pointing.
    glm::vec2 effective_view = local->view_angles - punch;
    glm::vec3 eye = local->eye_position();

    float best_metric = 0.0f;
    bool have_best = false;

    for (const auto& player : world.players) {
        if (player.index == local->index || !player.is_alive()) continue;
        bool friendly = player.team == local->team;
        if (!world.config.free_for_all && !cfg.target_friendlies && friendly) {
            continue;
        }

        for (Bone bone : cfg.bones) {
            const Hitbox* hitbox = player.hitbox_for(bone);
            if (!hitbox) continue;
            glm::vec3 predicted = hitbox->center +
                                  player.velocity * std::clamp(cfg.prediction_time, 0.0f, 0.25f);

            bool visible = !cfg.visibility_check || has_line_of_sight(world, eye, predicted);
            bool penetrable = false;
            if (!visible && cfg.penetration_check) {
                if (auto stats = weapon_stats(local->weapon)) {
                    if (auto dmg = estimate_penetration_damage(world, eye, predicted, *stats)) {
                        penetrable = *dmg >= static_cast<float>(cfg.minimum_penetration_damage);
                    }
                }
            }
            if (!visible && !penetrable) continue;

            float distance = glm::length(predicted - eye);
            glm::vec2 aim_angles = math::angles_from_vector(glm::normalize(predicted - eye));
            float fov = math::angles_to_fov(effective_view, aim_angles);
            float fov_limit = cfg.fov * (cfg.distance_adjusted_fov ? math::distance_scale(distance)
                                                                   : 1.0f);
            if (!std::isfinite(fov) || fov > fov_limit) continue;

            float metric = cfg.targeting_mode == TargetingMode::Fov ? fov : distance;
            bool select = !have_best ||
                          (cfg.targeting_mode == TargetingMode::Fov ? fov < best_metric
                                                                    : distance < best_metric);
            if (select) {
                have_best = true;
                best_metric = metric;
                target.player_index = static_cast<int>(
                    &player - world.players.data());
                target.angle = aim_angles;
                target.distance = distance;
                target.bone = bone;
                target.fov = fov;
                target.penetrated = penetrable && !visible;
            }
        }
    }
    return target;
}

glm::vec2 compute_aim_step(const World& world, const Config& config, const Target& target,
                           float dt, bool active) {
    if (!active || !target.valid()) {
        return {0.0f, 0.0f};
    }
    const Player* local = world.local_player();
    if (!local) return {0.0f, 0.0f};

    const AimbotConfig& cfg = effective_aimbot(config, local->weapon);
    if (cfg.start_bullet > 0 && local->shots_fired < cfg.start_bullet) {
        return {0.0f, 0.0f};
    }

    glm::vec2 delta{math::angle_delta(local->view_angles.x, target.angle.x),
                    math::angle_delta(local->view_angles.y, target.angle.y)};

    // Reference: deadlocked aimbot smoothing + inertia. Larger `smooth` means a
    // slower, smoother approach; `inertia` damps the tail of the motion.
    float smooth = std::max(cfg.smooth, 1.0f);
    float factor = std::clamp((1.0f - std::exp(-dt / (smooth * 0.02f))) * cfg.inertia, 0.0f, 1.0f);
    glm::vec2 step = delta * factor;
    step.x = math::soft_clamp_acceleration(step.x, 12.0f, 0.05f);
    step.y = math::soft_clamp_acceleration(step.y, 12.0f, 0.05f);
    return step;
}

glm::vec2 compute_rcs_delta(const World& world, const Config& config, bool active) {
    const Player* local = world.local_player();
    if (!local || !active) return {0.0f, 0.0f};
    const RcsConfig& cfg = effective_rcs(config, local->weapon);
    if (!cfg.enabled) return {0.0f, 0.0f};
    // Pull the view down/against the recoil punch, scaled by strength.
    return -local->aim_punch * cfg.strength;
}

bool triggerbot_should_fire(const World& world, const Config& config, const Target& target,
                            uint64_t now_ms, bool active) {
    if (!active || !target.valid()) return false;
    const Player* local = world.local_player();
    if (!local || !local->is_alive()) return false;

    const TriggerbotConfig& cfg = effective_triggerbot(config, local->weapon);
    if (cfg.head_only && target.bone != Bone::Head) return false;
    if (cfg.scope_check && weapon_class(local->weapon) == WeaponClass::Sniper && !local->scoped) {
        return false;
    }
    if (cfg.velocity_check && glm::length(local->velocity) > cfg.velocity_threshold) {
        return false;
    }
    if (cfg.flash_check && local->flashed) {
        return false;
    }
    // Delay gate: a deterministic hash of the target + time keeps the delay in
    // the configured [min, max] range without wall-clock jitter.
    uint64_t span = cfg.delay_max >= cfg.delay_min ? cfg.delay_max - cfg.delay_min : 0;
    uint64_t offset = span == 0 ? 0 : (now_ms + target.player_index * 37) % (span + 1);
    uint64_t delay = cfg.delay_min + offset;
    return now_ms >= delay;
}

void apply_no_flash(World& world, float max_alpha) {
    for (auto& player : world.players) {
        if (player.flash_alpha > max_alpha) {
            player.flash_alpha = max_alpha;
        }
        player.flashed = player.flash_alpha > 0.5f;
    }
}

void apply_fov_changer(World& world, float fov) {
    world.config.fov_override = std::clamp(fov, 20.0f, 140.0f);
}

}  // namespace lvx
