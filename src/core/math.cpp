#include "logvex/math.hpp"

#include <algorithm>
#include <cmath>

namespace lvx::math {

glm::vec2 angles_from_vector(const glm::vec3& forward) {
    float yaw = std::atan2(forward.y, forward.x) * 180.0f / 3.14159265358979f;
    float horizontal = std::sqrt(forward.x * forward.x + forward.y * forward.y);
    float pitch = std::atan2(-forward.z, horizontal) * 180.0f / 3.14159265358979f;
    return {pitch, yaw};
}

float angle_delta(float from, float to) {
    float delta = to - from;
    while (delta > 180.0f) delta -= 360.0f;
    while (delta < -180.0f) delta += 360.0f;
    return delta;
}

float angles_to_fov(const glm::vec2& view_angles, const glm::vec2& aim_angles) {
    float pitch = std::fmod(view_angles.x - aim_angles.x + 180.0f, 360.0f);
    if (pitch < 0.0f) pitch += 360.0f;
    pitch -= 180.0f;
    float yaw = std::fmod(view_angles.y - aim_angles.y + 180.0f, 360.0f);
    if (yaw < 0.0f) yaw += 360.0f;
    yaw -= 180.0f;
    return std::sqrt(pitch * pitch + yaw * yaw);
}

void vec2_clamp(glm::vec2& v) {
    if (v.x > 89.0f && v.x <= 180.0f) v.x = 89.0f;
    if (v.x > 180.0f) v.x -= 360.0f;
    if (v.x < -89.0f) v.x = -89.0f;
    v.y = std::fmod(v.y + 180.0f, 360.0f);
    if (v.y < 0.0f) v.y += 360.0f;
    v.y -= 180.0f;
}

float soft_clamp_acceleration(float accel, float max_accel, float decay_rate) {
    if (std::fabs(accel) <= max_accel) {
        return accel;
    }
    float excess = std::fabs(accel) - max_accel;
    float sign = accel < 0.0f ? -1.0f : 1.0f;
    return sign * (max_accel + excess * std::exp(-excess * decay_rate));
}

float weighted_average_component(const float* values, int count) {
    if (count <= 0) {
        return 0.0f;
    }
    float sum = 0.0f;
    float weight_sum = 0.0f;
    for (int i = 0; i < count; ++i) {
        float weight = 1.0f + static_cast<float>(i) * 0.15f;
        sum += values[i] * weight;
        weight_sum += weight;
    }
    return weight_sum > 0.0f ? sum / weight_sum : 0.0f;
}

float compute_max_acceleration(const float* values, int count, float multiplier, float lo, float hi,
                               float fallback) {
    if (count < 3) {
        return fallback;
    }
    float value = weighted_average_component(values, count) * multiplier;
    return std::clamp(value, lo, hi);
}

std::optional<glm::vec2> world_to_screen(const glm::mat4& view_matrix, const glm::vec3& position,
                                         const glm::vec2& window_size) {
    glm::vec4 clip = view_matrix * glm::vec4(position, 1.0f);
    if (clip.w < 0.0001f) {
        return std::nullopt;
    }
    glm::vec3 ndc = glm::vec3(clip) / clip.w;
    glm::vec2 half = window_size * 0.5f;
    glm::vec2 screen{half.x + 0.5f * ndc.x * window_size.x + 0.5f,
                     half.y - 0.5f * ndc.y * window_size.y + 0.5f};
    if (screen.x < 0.0f || screen.x > window_size.x || screen.y < 0.0f ||
        screen.y > window_size.y) {
        return std::nullopt;
    }
    return screen;
}

glm::vec2 world_to_screen_normalized(const glm::mat4& view_matrix, const glm::vec3& position,
                                     const glm::vec2& window_size) {
    glm::vec4 clip = view_matrix * glm::vec4(position, 1.0f);
    glm::vec3 ndc{0.0f};
    if (clip.w > 0.0f) {
        ndc = glm::vec3(clip) / clip.w;
    }
    glm::vec2 half = window_size * 0.5f;
    return {half.x + 0.5f * ndc.x * window_size.x + 0.5f,
            half.y - 0.5f * ndc.y * window_size.y + 0.5f};
}

float distance_scale(float distance) {
    // Reference: deadlocked Compute::distance_scale(). Closer targets keep the
    // full FOV, distant targets get a proportionally tighter cone.
    const float min_scale = 0.4f;
    const float reference_distance = 1024.0f;
    if (distance <= reference_distance) {
        return 1.0f;
    }
    float scale = reference_distance / distance;
    return std::clamp(scale, min_scale, 1.0f);
}

}  // namespace lvx::math
