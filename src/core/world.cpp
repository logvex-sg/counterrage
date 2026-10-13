#include "logvex/world.hpp"

#include <algorithm>

namespace lvx {

glm::vec3 Player::eye_position() const {
    // CS2 eye height is 64 units standing, 46 crouched. The simulator keeps a
    // single eye height and folds crouch into `position.z`.
    return {position.x, position.y, position.z + 64.0f};
}

const Hitbox* Player::hitbox_for(Bone bone) const {
    for (const auto& hitbox : hitboxes) {
        if (hitbox.bone == bone) return &hitbox;
    }
    return nullptr;
}

const Player* World::local_player() const {
    if (local_index < 0 || local_index >= static_cast<int>(players.size())) {
        return nullptr;
    }
    return &players[static_cast<size_t>(local_index)];
}

Player* World::local_player() {
    if (local_index < 0 || local_index >= static_cast<int>(players.size())) {
        return nullptr;
    }
    return &players[static_cast<size_t>(local_index)];
}

const Player* World::find_by_index(uint32_t index) const {
    for (const auto& player : players) {
        if (player.index == index) return &player;
    }
    return nullptr;
}

Player* World::find_by_index(uint32_t index) {
    for (auto& player : players) {
        if (player.index == index) return &player;
    }
    return nullptr;
}

}  // namespace lvx
