#include "test.hpp"

#include "logvex/weapon.hpp"

using namespace lvx;

TEST(weapon_index_mapping_matches_reference) {
    CHECK(weapon_from_index(7) == Weapon::AK47);
    CHECK(weapon_from_index(9) == Weapon::Awp);
    CHECK(weapon_from_index(1) == Weapon::DesertEagle);
    CHECK(weapon_from_index(60) == Weapon::M4A1S);
    CHECK(weapon_from_index(64) == Weapon::Revolver);
    CHECK(weapon_from_index(503) == Weapon::KnifeClassic);
    CHECK(weapon_from_index(9999) == Weapon::None);
}

TEST(weapon_classification) {
    CHECK(weapon_class(Weapon::AK47) == WeaponClass::Rifle);
    CHECK(weapon_class(Weapon::Awp) == WeaponClass::Sniper);
    CHECK(weapon_class(Weapon::Glock) == WeaponClass::Pistol);
    CHECK(weapon_class(Weapon::Negev) == WeaponClass::Heavy);
    CHECK(weapon_class(Weapon::KnifeKarambit) == WeaponClass::Knife);
    CHECK(weapon_class(Weapon::HE) == WeaponClass::Grenade);
    CHECK(weapon_class(Weapon::Taser) == WeaponClass::Utility);
}

TEST(weapon_names) {
    CHECK(weapon_name(Weapon::AK47) == "AK-47");
    CHECK(weapon_name(Weapon::M4A1S) == "M4A1-S");
    CHECK(weapon_name(Weapon::DesertEagle) == "Desert Eagle");
}

TEST(weapon_token_parsing) {
    CHECK(weapon_from_token("ak47").value_or(Weapon::None) == Weapon::AK47);
    CHECK(weapon_from_token("AK-47").value_or(Weapon::None) == Weapon::AK47);
    CHECK(weapon_from_token("m4a1s").value_or(Weapon::None) == Weapon::M4A1S);
    CHECK(weapon_from_token("deagle").value_or(Weapon::None) == Weapon::None);
    CHECK(weapon_from_token("awp").value_or(Weapon::None) == Weapon::Awp);
}

TEST(weapon_stats_sanity) {
    auto ak = weapon_stats(Weapon::AK47);
    CHECK(ak.has_value());
    if (ak) {
        CHECK(ak->damage == 36);
        CHECK(ak->automatic);
        CHECK(ak->max_clip == 30);
    }
    auto awp = weapon_stats(Weapon::Awp);
    CHECK(awp.has_value());
    if (awp) CHECK(awp->is_sniper);
    CHECK(!weapon_stats(Weapon::None).has_value());
}

TEST(all_weapons_nonempty) { CHECK(all_weapons().size() >= 60); }
