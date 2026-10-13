#pragma once

#include <optional>
#include <vector>

#include <glm/glm.hpp>

#include "logvex/config.hpp"
#include "logvex/simulation.hpp"
#include "logvex/world.hpp"

namespace lvx {

// Reference: deadlocked `cheat/src/cs2/target.rs` Target.
struct Target {
    int player_index = -1;
    glm::vec2 angle{0.0f};
    float distance = 0.0f;
    Bone bone = Bone::Head;
    float fov = 0.0f;
    bool penetrated = false;

    bool valid() const { return player_index >= 0; }
    void reset() { *this = Target{}; }
};

// Reference: deadlocked `cheat/src/cs2/target.rs` find_target().
// Resolves the per-weapon config override then selects the best bone within FOV.
Target find_target(const World& world, const Config& config);

// Applies smoothing/inertia to produce the per-frame aim delta in degrees.
// Reference: deadlocked `cheat/src/cs2/features/aimbot.rs`.
glm::vec2 compute_aim_step(const World& world, const Config& config, const Target& target,
                           float dt, bool active);

// Reference: deadlocked `cheat/src/cs2/features/rcs.rs`.
glm::vec2 compute_rcs_delta(const World& world, const Config& config, bool active);

// Reference: deadlocked `cheat/src/cs2/features/triggerbot.rs`.
// Returns true when the triggerbot should fire this frame.
bool triggerbot_should_fire(const World& world, const Config& config, const Target& target,
                            uint64_t now_ms, bool active);

// Resolves the effective weapon config (per-weapon override merged with global).
const WeaponConfig& effective_weapon_config(const Config& config, Weapon weapon);
const AimbotConfig& effective_aimbot(const Config& config, Weapon weapon);
const TriggerbotConfig& effective_triggerbot(const Config& config, Weapon weapon);
const RcsConfig& effective_rcs(const Config& config, Weapon weapon);

// Reference: deadlocked `cheat/src/cs2/features/no_flash.rs` (practice scope:
// only affects the local simulated flash, never game memory).
void apply_no_flash(World& world, float max_alpha);

// Reference: deadlocked `cheat/src/cs2/features/fov_changer.rs` (practice scope:
// adjusts the simulated camera FOV used by the overlay projection).
void apply_fov_changer(World& world, float fov);

}  // namespace lvx
