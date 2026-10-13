#include "logvex/types.hpp"

namespace lvx {

const char* bone_name(Bone bone) {
    switch (bone) {
        case Bone::Hip: return "Hip";
        case Bone::Spine1: return "Spine1";
        case Bone::Spine2: return "Spine2";
        case Bone::Spine3: return "Spine3";
        case Bone::Spine4: return "Spine4";
        case Bone::Neck: return "Neck";
        case Bone::Head: return "Head";
        case Bone::LeftShoulder: return "LeftShoulder";
        case Bone::LeftElbow: return "LeftElbow";
        case Bone::LeftHand: return "LeftHand";
        case Bone::RightShoulder: return "RightShoulder";
        case Bone::RightElbow: return "RightElbow";
        case Bone::RightHand: return "RightHand";
        case Bone::LeftHip: return "LeftHip";
        case Bone::LeftKnee: return "LeftKnee";
        case Bone::LeftFoot: return "LeftFoot";
        case Bone::RightHip: return "RightHip";
        case Bone::RightKnee: return "RightKnee";
        case Bone::RightFoot: return "RightFoot";
    }
    return "Hip";
}

const std::vector<Bone>& all_bones() {
    static const std::vector<Bone> list = {
        Bone::Hip,        Bone::Spine1,     Bone::Spine2,       Bone::Spine3,
        Bone::Spine4,     Bone::Neck,       Bone::Head,         Bone::LeftShoulder,
        Bone::LeftElbow,  Bone::LeftHand,   Bone::RightShoulder, Bone::RightElbow,
        Bone::RightHand,  Bone::LeftHip,    Bone::LeftKnee,     Bone::LeftFoot,
        Bone::RightHip,   Bone::RightKnee,  Bone::RightFoot,
    };
    return list;
}

const char* team_name(Team team) {
    switch (team) {
        case Team::Unknown: return "Unknown";
        case Team::Spectator: return "Spectator";
        case Team::Terrorist: return "Terrorist";
        case Team::CounterTerrorist: return "Counter-Terrorist";
    }
    return "Unknown";
}

const char* entity_kind_name(EntityKind kind) {
    switch (kind) {
        case EntityKind::Player: return "Player";
        case EntityKind::Weapon: return "Weapon";
        case EntityKind::Inferno: return "Inferno";
        case EntityKind::Molotov: return "Molotov";
        case EntityKind::Smoke: return "Smoke";
        case EntityKind::Flashbang: return "Flashbang";
        case EntityKind::HeGrenade: return "HE";
        case EntityKind::Decoy: return "Decoy";
        case EntityKind::Chicken: return "Chicken";
    }
    return "Unknown";
}

}  // namespace lvx
