#include "test.hpp"

#include "logvex/scenarios.hpp"
#include "logvex/simulation.hpp"

using namespace lvx;

TEST(simulation_loads_scenarios) {
    Simulation sim(1);
    CHECK(sim.load_scenario("aim_arena"));
    CHECK(sim.world().players.size() == 6);
    CHECK(sim.load_scenario("dust2_practice"));
    CHECK(sim.world().bomb.planted);
    CHECK(!sim.load_scenario("does_not_exist"));
}

TEST(simulation_is_deterministic) {
    Simulation a(99);
    Simulation b(99);
    a.load_scenario("aim_arena");
    b.load_scenario("aim_arena");
    InputState input;
    input.look_delta = {0.0f, 0.3f};
    input.attack = true;
    for (int i = 0; i < 120; ++i) {
        a.step(input, 1.0f / 128.0f);
        b.step(input, 1.0f / 128.0f);
    }
    CHECK(a.tick() == b.tick());
    CHECK(a.hit_events().size() == b.hit_events().size());
    if (!a.hit_events().empty() && !b.hit_events().empty()) {
        CHECK_NEAR(a.hit_events().back().damage, b.hit_events().back().damage, 0.001);
    }
}

TEST(firing_consumes_ammo_and_produces_trace) {
    Simulation sim(3);
    sim.load_scenario("aim_arena");
    const Player* local = sim.world().local_player();
    int before = local->ammo;
    ShotTrace trace = sim.fire_local();
    CHECK(!sim.shot_traces().empty());
    CHECK(sim.world().local_player()->ammo == before - 1);
    CHECK(sim.world().local_player()->shots_fired == 1);
    (void)trace;
}

TEST(fire_toward_bot_damages_it) {
    Simulation sim(5);
    sim.load_scenario("aim_arena");
    const Player* local = sim.world().local_player();
    glm::vec3 eye = local->eye_position();
    const Player* bot = sim.world().find_by_index(2);
    int before = bot->health;
    glm::vec3 aim = bot->hitbox_for(Bone::Head)->center - eye;
    ShotTrace trace = sim.fire_local(aim, true);
    CHECK(trace.hit_player);
    const Player* after = sim.world().find_by_index(2);
    CHECK(after->health < before);
}

TEST(kill_sets_life_state_dead) {
    Simulation sim(6);
    sim.load_scenario("aim_arena");
    const Player* local = sim.world().local_player();
    glm::vec3 eye = local->eye_position();
    for (int i = 0; i < 20; ++i) {
        const Player* bot = sim.world().find_by_index(2);
        if (!bot->is_alive()) break;
        glm::vec3 aim = bot->hitbox_for(Bone::Head)->center - eye;
        sim.fire_local(aim, true);
        // Reset the fire cooldown so shots are not gated by cycle time.
        sim.step(InputState{}, 0.2f);
    }
    const Player* bot = sim.world().find_by_index(2);
    CHECK(!bot->is_alive());
    CHECK(bot->health == 0);
}

TEST(bomb_timer_counts_down) {
    Simulation sim(7);
    sim.load_scenario("dust2_practice");
    float before = sim.world().bomb.timer;
    for (int i = 0; i < 128; ++i) sim.step(InputState{}, 1.0f / 128.0f);
    CHECK(sim.world().bomb.timer < before);
}

TEST(movement_changes_local_position) {
    Simulation sim(8);
    sim.load_scenario("aim_arena");
    const Player* local = sim.world().local_player();
    float y0 = local->position.y;
    InputState input;
    input.move = {0.0f, 1.0f};  // forward
    for (int i = 0; i < 64; ++i) sim.step(input, 1.0f / 128.0f);
    CHECK(sim.world().local_player()->position.y > y0);
}
