#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

namespace lvx {

// Reference: deadlocked `shared/src/bones.rs`. Bone ids are kept identical so
// the same head/spine/limb targeting logic transfers unchanged.
enum class Bone : uint64_t {
    Hip = 1,
    Spine1 = 2,
    Spine2 = 3,
    Spine3 = 4,
    Spine4 = 5,
    Neck = 6,
    Head = 7,
    LeftShoulder = 9,
    LeftElbow = 10,
    LeftHand = 11,
    RightShoulder = 13,
    RightElbow = 14,
    RightHand = 15,
    LeftHip = 17,
    LeftKnee = 18,
    LeftFoot = 19,
    RightHip = 20,
    RightKnee = 21,
    RightFoot = 22,
};

// Reference: deadlocked `shared/src/bones.rs` CONNECTIONS table (skeleton ESP).
inline constexpr std::pair<Bone, Bone> kBoneConnections[18] = {
    {Bone::Hip, Bone::Spine1},         {Bone::Spine1, Bone::Spine2},
    {Bone::Spine2, Bone::Spine3},      {Bone::Spine3, Bone::Spine4},
    {Bone::Spine4, Bone::Neck},        {Bone::Neck, Bone::Head},
    {Bone::Neck, Bone::LeftShoulder},  {Bone::LeftShoulder, Bone::LeftElbow},
    {Bone::LeftElbow, Bone::LeftHand}, {Bone::Neck, Bone::RightShoulder},
    {Bone::RightShoulder, Bone::RightElbow}, {Bone::RightElbow, Bone::RightHand},
    {Bone::Hip, Bone::LeftHip},        {Bone::LeftHip, Bone::LeftKnee},
    {Bone::LeftKnee, Bone::LeftFoot},  {Bone::Hip, Bone::RightHip},
    {Bone::RightHip, Bone::RightKnee}, {Bone::RightKnee, Bone::RightFoot},
};

const char* bone_name(Bone bone);
const std::vector<Bone>& all_bones();

enum class Team : int8_t { Unknown = 0, Spectator = 1, Terrorist = 2, CounterTerrorist = 3 };

const char* team_name(Team team);

enum class LifeState : uint8_t { Alive = 0, Dying = 1, Dead = 2 };

// Reference: deadlocked `shared/src/entity.rs` WeaponInfo / grenade info.
enum class EntityKind : uint8_t {
    Player,
    Weapon,
    Inferno,
    Molotov,
    Smoke,
    Flashbang,
    HeGrenade,
    Decoy,
    Chicken,
};

const char* entity_kind_name(EntityKind kind);

struct GrenadeInfo {
    uint32_t entity = 0;
    glm::vec3 position{0.0f};
    std::string name;
};

struct InfernoInfo {
    uint32_t entity = 0;
    glm::vec3 position{0.0f};
    std::vector<glm::vec3> hull;
};

struct WeaponEntityInfo {
    uint32_t entity = 0;
    glm::vec3 position{0.0f};
    std::string weapon_name;
    int ammo = 0;
    int ammo_reserve = 0;
};

struct EntityInfo {
    EntityKind kind = EntityKind::Weapon;
    uint32_t entity = 0;
    glm::vec3 position{0.0f};
    std::string label;
    GrenadeInfo grenade{};
    InfernoInfo inferno{};
    WeaponEntityInfo weapon{};
};

struct BombData {
    bool planted = false;
    float timer = 0.0f;
    bool being_defused = false;
    glm::vec3 position{0.0f};
    float defuse_remain_time = 0.0f;
};

}  // namespace lvx
