#include "test.hpp"

#include "logvex/math.hpp"

using namespace lvx;

TEST(angles_from_vector_axes) {
    glm::vec2 a = math::angles_from_vector(glm::vec3{1.0f, 0.0f, 0.0f});
    CHECK_NEAR(a.x, 0.0f, 0.001);
    CHECK_NEAR(a.y, 0.0f, 0.001);

    glm::vec2 b = math::angles_from_vector(glm::vec3{0.0f, 1.0f, 0.0f});
    CHECK_NEAR(b.y, 90.0f, 0.001);

    glm::vec2 c = math::angles_from_vector(glm::vec3{0.0f, 0.0f, -1.0f});
    CHECK_NEAR(c.x, 90.0f, 0.001);  // looking straight down
}

TEST(angles_to_fov_basic) {
    CHECK_NEAR(math::angles_to_fov({0.0f, 0.0f}, {0.0f, 0.0f}), 0.0f, 0.001);
    CHECK_NEAR(math::angles_to_fov({0.0f, 0.0f}, {0.0f, 3.0f}), 3.0f, 0.001);
    CHECK_NEAR(math::angles_to_fov({4.0f, 0.0f}, {0.0f, 0.0f}), 4.0f, 0.001);
}

TEST(angles_to_fov_wraps_yaw) {
    // 179 and -179 are 2 degrees apart.
    CHECK_NEAR(math::angles_to_fov({0.0f, 179.0f}, {0.0f, -179.0f}), 2.0f, 0.001);
}

TEST(vec2_clamp_limits_pitch_and_wraps_yaw) {
    glm::vec2 v{120.0f, 400.0f};
    math::vec2_clamp(v);
    CHECK(v.x <= 89.0f && v.x >= -89.0f);
    CHECK(v.y <= 180.0f && v.y > -180.0f);
}

TEST(angle_delta_shortest_path) {
    CHECK_NEAR(math::angle_delta(179.0f, -179.0f), 2.0f, 0.001);
    CHECK_NEAR(math::angle_delta(-179.0f, 179.0f), -2.0f, 0.001);
    CHECK_NEAR(math::angle_delta(10.0f, 10.0f), 0.0f, 0.001);
}

TEST(soft_clamp_passes_small_values) {
    CHECK_NEAR(math::soft_clamp_acceleration(1.0f, 5.0f, 0.1f), 1.0f, 0.001);
    float clamped = math::soft_clamp_acceleration(50.0f, 5.0f, 0.1f);
    CHECK(clamped > 5.0f);  // soft: exceeds but is damped
    CHECK(clamped < 50.0f);
}

TEST(world_to_screen_projects_centre) {
    glm::mat4 proj(0.0f);
    proj[0][0] = 1.0f;
    proj[1][1] = 1.0f;
    proj[2][2] = 1.0f;
    proj[3][3] = 1.0f;
    auto result = math::world_to_screen(proj, glm::vec3{0.0f, 0.0f, 0.0f}, {800.0f, 600.0f});
    CHECK(result.has_value());
    if (result) {
        CHECK_NEAR(result->x, 400.0f, 1.0);
        CHECK_NEAR(result->y, 300.0f, 1.0);
    }
}

TEST(distance_scale_is_monotonic) {
    CHECK_NEAR(math::distance_scale(100.0f), 1.0f, 0.001);
    CHECK(math::distance_scale(2048.0f) < math::distance_scale(1024.0f));
    CHECK(math::distance_scale(100000.0f) >= 0.4f);
}
