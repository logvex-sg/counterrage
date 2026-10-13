#pragma once

#include <optional>

#include <glm/glm.hpp>

namespace lvx::math {

// Reference: deadlocked `cheat/src/math.rs` angles_from_vector().
// CS2 view angles are (pitch, yaw) in degrees, pitch positive looking down.
glm::vec2 angles_from_vector(const glm::vec3& forward);

// Reference: deadlocked `cheat/src/math.rs` angles_to_fov().
float angles_to_fov(const glm::vec2& view_angles, const glm::vec2& aim_angles);

// Reference: deadlocked `cheat/src/math.rs` vec2_clamp().
void vec2_clamp(glm::vec2& v);

// Reference: deadlocked `cheat/src/math.rs` soft_clamp_acceleration().
float soft_clamp_acceleration(float accel, float max_accel, float decay_rate);

// Reference: deadlocked `cheat/src/math.rs` weighted_average_component() /
// compute_max_acceleration_component().
float weighted_average_component(const float* values, int count);
float compute_max_acceleration(const float* values, int count, float multiplier, float lo, float hi,
                               float fallback);

// World -> screen using a CS2-style row-major view-projection matrix.
// Returns std::nullopt when the point is behind the camera or off-screen.
std::optional<glm::vec2> world_to_screen(const glm::mat4& view_matrix, const glm::vec3& position,
                                         const glm::vec2& window_size);

// Same transform but never rejects; used for overlay edge clamping.
glm::vec2 world_to_screen_normalized(const glm::mat4& view_matrix, const glm::vec3& position,
                                     const glm::vec2& window_size);

// Distance-scaled FOV multiplier used by the aimbot (smaller target => tighter FOV).
float distance_scale(float distance);

// Shortest signed delta between two angles in degrees.
float angle_delta(float from, float to);

}  // namespace lvx::math
