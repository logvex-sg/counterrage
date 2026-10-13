#include "test.hpp"

#include "logvex/features.hpp"
#include "logvex/math.hpp"
#include "logvex/scenarios.hpp"
#include "logvex/simulation.hpp"

using namespace lvx;

TEST(find_target_ignores_friendlies) {
    Simulation sim(11);
    sim.load_scenario("aim_arena");
    Config config = Config::defaults();
    config.aim.global.aimbot.fov = 30.0f;
    config.aim.global.aimbot.distance_adjusted_fov = false;
    Target target = find_target(sim.world(), config);
    CHECK(target.valid());
    if (target.valid()) {
        const Player& picked = sim.world().players[static_cast<size_t>(target.player_index)];
        CHECK(picked.team == Team::Terrorist);
    }
}

TEST(find_target_respects_fov_limit) {
    Simulation sim(12);
    sim.load_scenario("aim_arena");
    Config config = Config::defaults();
    config.aim.global.aimbot.fov = 0.05f;
    config.aim.global.aimbot.distance_adjusted_fov = false;
    // Turn far away from every bot.
    sim.world().local_player()->view_angles = {0.0f, -90.0f};
    Target target = find_target(sim.world(), config);
    CHECK(!target.valid());
}

TEST(aim_step_converges_toward_target) {
    Simulation sim(13);
    sim.load_scenario("aim_arena");
    Config config = Config::defaults();
    // Large FOV so the bot is acquired immediately; disable the distance
    // scaling and velocity prediction so the target point is static.
    config.aim.global.aimbot.fov = 90.0f;
    config.aim.global.aimbot.smooth = 2.0f;
    config.aim.global.aimbot.distance_adjusted_fov = false;
    config.aim.global.aimbot.prediction_time = 0.0f;
    Player* local = sim.world().local_player();
    // Offset the view so there is a non-zero angle to converge from.
    local->view_angles = {0.0f, 70.0f};
    Target target = find_target(sim.world(), config);
    CHECK(target.valid());
    if (!target.valid()) return;
    float start_fov = target.fov;
    CHECK(start_fov > 1.0f);
    for (int i = 0; i < 400; ++i) {
        target = find_target(sim.world(), config);
        if (!target.valid()) break;
        glm::vec2 step = compute_aim_step(sim.world(), config, target, 1.0f / 128.0f, true);
        local->view_angles += step;
        math::vec2_clamp(local->view_angles);
    }
    Target after = find_target(sim.world(), config);
    CHECK(after.valid());
    if (after.valid()) {
        CHECK(after.fov < start_fov);
        CHECK(after.fov < 0.5f);
    }
}

TEST(rcs_opposes_recoil) {
    Simulation sim(14);
    sim.load_scenario("aim_arena");
    Config config = Config::defaults();
    config.aim.global.rcs.enabled = true;
    sim.world().local_player()->aim_punch = {2.0f, 1.0f};
    glm::vec2 delta = compute_rcs_delta(sim.world(), config, true);
    CHECK(delta.x < 0.0f);
    CHECK(delta.y < 0.0f);
}

TEST(triggerbot_gates_on_velocity) {
    Simulation sim(15);
    sim.load_scenario("movement_range");
    Config config = Config::defaults();
    config.aim.global.triggerbot.enabled = true;
    config.aim.global.triggerbot.velocity_check = true;
    config.aim.global.triggerbot.velocity_threshold = 10.0f;
    sim.world().local_player()->velocity = {200.0f, 0.0f, 0.0f};
    Target target;
    target.player_index = 1;
    target.bone = Bone::Head;
    CHECK(!triggerbot_should_fire(sim.world(), config, target, 100000, true));
}

TEST(triggerbot_fires_when_still_and_delay_elapsed) {
    Simulation sim(16);
    sim.load_scenario("aim_arena");
    Config config = Config::defaults();
    config.aim.global.triggerbot.enabled = true;
    config.aim.global.triggerbot.delay_min = 50;
    config.aim.global.triggerbot.delay_max = 60;
    config.aim.global.triggerbot.velocity_check = true;
    sim.world().local_player()->velocity = {0.0f, 0.0f, 0.0f};
    Target target;
    target.player_index = 1;
    target.bone = Bone::Head;
    CHECK(triggerbot_should_fire(sim.world(), config, target, 100000, true));
}

TEST(per_weapon_override_wins) {
    Config config = Config::defaults();
    config.aim.global.aimbot.fov = 3.0f;
    config.aim.weapons[Weapon::AK47].aimbot.enable_override = true;
    config.aim.weapons[Weapon::AK47].aimbot.fov = 9.0f;
    CHECK_NEAR(effective_aimbot(config, Weapon::AK47).fov, 9.0f, 0.001);
    CHECK_NEAR(effective_aimbot(config, Weapon::Awp).fov, 3.0f, 0.001);
}

TEST(fov_changer_clamps) {
    World world = make_scenario("aim_arena").value();
    apply_fov_changer(world, 5.0f);
    CHECK(world.config.fov_override >= 20.0f);
    apply_fov_changer(world, 500.0f);
    CHECK(world.config.fov_override <= 140.0f);
}
