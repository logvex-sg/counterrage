#pragma once

#include <cstdint>
#include <random>
#include <string>
#include <vector>

#include <glm/glm.hpp>

#include "logvex/world.hpp"

namespace lvx {

// Input frame the GUI / CLI feeds into the simulation. Mirrors the subset of
// CS2 input the reference reads (movement, look, attack, jump, crouch, use).
struct InputState {
    glm::vec2 move{0.0f};      // x = strafe, y = forward
    glm::vec2 look_delta{0.0f};// mouse delta in degrees
    float move_speed = 250.0f; // units/s
    bool attack = false;       // primary fire held
    bool jump = false;
    bool crouch = false;
    bool walk = false;         // shift (quiet movement)
    bool scoped = false;
    bool reload = false;
    bool use = false;
};

struct HitEvent {
    uint64_t sequence = 0;
    uint32_t attacker = 0;
    uint32_t victim = 0;
    Bone bone = Bone::Head;
    float damage = 0.0f;
    int remaining_health = 0;
    bool killed = false;
    bool penetrated = false;
};

struct ShotTrace {
    glm::vec3 origin{0.0f};
    glm::vec3 end{0.0f};
    glm::vec3 direction{0.0f};
    uint32_t shooter = 0;
    bool hit_player = false;
    uint32_t victim = 0;
    Bone bone = Bone::Head;
    bool penetrated = false;
};

// Deterministic offline practice simulation. There is no game process, memory
// access or network input: everything below runs on synthetic geometry, which
// is what makes the practice features reproducible and safe to test.
class Simulation {
public:
    explicit Simulation(uint64_t seed = 1337);

    World& world() { return world_; }
    const World& world() const { return world_; }

    void set_world(World world);
    void reset(uint64_t seed);

    // Loads a named built-in scenario ("dust2_practice", "aim_arena",
    // "wallbang_lane"). Returns false for an unknown name.
    bool load_scenario(const std::string& name);

    // Advance the simulation by dt seconds.
    void step(const InputState& input, float dt);

    // The local player fires one hitscan shot (used by triggerbot / manual fire).
    ShotTrace fire_local(const glm::vec3& aim_override = glm::vec3(0.0f), bool has_override = false);

    // Feed a synthetic hit to a remote bot so they return fire.
    void bot_fire(uint32_t bot_index);

    const std::vector<HitEvent>& hit_events() const { return hit_events_; }
    const std::vector<ShotTrace>& shot_traces() const { return shot_traces_; }
    uint64_t tick() const { return tick_; }
    float time() const { return time_; }

    // Reference: deadlocked Compute::distance_scale().
    float distance_scale(float distance) const;

    // Direction (unit) toward a target point from the local eye.
    glm::vec3 aim_direction(const glm::vec3& target) const;

private:
    void rebuild_view_matrix();
    void recompute_visibility();
    void update_recoil(float dt);
    void update_bots(float dt);
    void step_movement(const InputState& input, float dt);

    World world_{};
    uint64_t seed_ = 1337;
    std::mt19937 rng_{1337};
    uint64_t tick_ = 0;
    float time_ = 0.0f;
    float fire_cooldown_ = 0.0f;
    uint64_t hit_sequence_ = 0;
    std::vector<HitEvent> hit_events_;
    std::vector<ShotTrace> shot_traces_;
};

}  // namespace lvx
