#include "test.hpp"

#include "logvex/ballistics.hpp"
#include "logvex/scenarios.hpp"

using namespace lvx;

TEST(ray_aabb_hits_and_misses) {
    float t = 0.0f;
    CHECK(ray_aabb(glm::vec3{-10.0f, 0.0f, 0.0f}, glm::vec3{1.0f, 0.0f, 0.0f},
                   glm::vec3{-1.0f, -1.0f, -1.0f}, glm::vec3{1.0f, 1.0f, 1.0f}, t));
    CHECK_NEAR(t, 9.0f, 0.01);
    CHECK(!ray_aabb(glm::vec3{-10.0f, 5.0f, 0.0f}, glm::vec3{1.0f, 0.0f, 0.0f},
                    glm::vec3{-1.0f, -1.0f, -1.0f}, glm::vec3{1.0f, 1.0f, 1.0f}, t));
}

TEST(wall_blocks_line_of_sight) {
    World world = make_scenario("wallbang_lane").value();
    const Player* local = world.local_player();
    CHECK(local != nullptr);
    glm::vec3 eye = local->eye_position();
    // The bot behind the concrete wall is not visible.
    const Player* behind = world.find_by_index(3);
    CHECK(behind != nullptr);
    if (behind) {
        glm::vec3 point = behind->hitbox_for(Bone::Spine3)->center;
        CHECK(!has_line_of_sight(world, eye, point));
    }
}

TEST(raycast_players_finds_nearest_hitbox) {
    World world = make_scenario("aim_arena").value();
    const Player* local = world.local_player();
    glm::vec3 eye = local->eye_position();
    const Player* bot = world.find_by_index(2);
    glm::vec3 point = bot->hitbox_for(Bone::Head)->center;
    glm::vec3 dir = glm::normalize(point - eye);
    auto hit = raycast_players(world, eye, dir, 16384.0f, world.local_index);
    CHECK(hit.has_value());
    if (hit) CHECK(hit->bone == Bone::Head);
}

TEST(penetration_damage_drops_with_wall) {
    World world = make_scenario("wallbang_lane").value();
    const Player* local = world.local_player();
    glm::vec3 eye = local->eye_position();
    auto ak = weapon_stats(Weapon::AK47).value();

    const Player* soft = world.find_by_index(2);  // behind thin wooden wall
    const Player* hard = world.find_by_index(3);  // behind thick concrete
    auto soft_dmg = estimate_penetration_damage(world, eye, soft->hitbox_for(Bone::Spine3)->center, ak);
    auto hard_dmg = estimate_penetration_damage(world, eye, hard->hitbox_for(Bone::Spine3)->center, ak);
    CHECK(soft_dmg.has_value());
    if (soft_dmg) CHECK(*soft_dmg > 0.0f);
    // The concrete wall is much harder: the shot must either fail to land or
    // do strictly less damage than through the wooden wall.
    if (soft_dmg) {
        CHECK(!hard_dmg.has_value() || *hard_dmg < *soft_dmg);
    }
}

TEST(compute_damage_headshot_multiplier) {
    auto ak = weapon_stats(Weapon::AK47).value();
    float body = compute_damage(ak, 200.0f, Bone::Spine3, 0, false);
    float head = compute_damage(ak, 200.0f, Bone::Head, 0, false);
    CHECK(head > body * 3.0f);
}

TEST(compute_damage_armor_reduces) {
    auto ak = weapon_stats(Weapon::AK47).value();
    float unarmored = compute_damage(ak, 200.0f, Bone::Spine3, 0, false);
    float armored = compute_damage(ak, 200.0f, Bone::Spine3, 100, false);
    CHECK(armored < unarmored);
}
