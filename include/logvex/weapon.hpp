#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace lvx {

// Reference: deadlocked `shared/src/weapon_class.rs`.
enum class WeaponClass { Unknown, Knife, Pistol, Smg, Heavy, Shotgun, Rifle, Sniper, Grenade, Utility };

const char* weapon_class_name(WeaponClass cls);

// Reference: deadlocked `shared/src/weapon.rs` Weapon enum. Names and ids match
// the reference so config files and radar payloads stay compatible.
enum class Weapon {
    None = 0,
    CZ75,
    DesertEagle,
    DualBeretras,
    FiveSeven,
    Glock,
    P2000,
    P250,
    Revolver,
    Tec9,
    Usp,
    MAC10,
    MP5,
    MP7,
    MP9,
    P90,
    Bizon,
    UMP45,
    Mag7,
    Nova,
    SawedOff,
    XM1014,
    M249,
    Negev,
    AK47,
    Aug,
    Famas,
    Galil,
    M4A1S,
    M4A4,
    SG553,
    Awp,
    G3SG1,
    SCAR20,
    SSG08,
    KnifeCT,
    KnifeT,
    KnifeBayonet,
    KnifeBowie,
    KnifeButterfly,
    KnifeClassic,
    KnifeFalchion,
    KnifeFlip,
    KnifeGut,
    KnifeHuntsman,
    KnifeKarambit,
    KnifeKukri,
    KnifeM9Bayonet,
    KnifeNavaja,
    KnifeNomad,
    KnifeParacord,
    KnifeShadowDaggers,
    KnifeSkeleton,
    KnifeStiletto,
    KnifeSurvival,
    KnifeTalon,
    KnifeUrsus,
    KnifeGold,
    Flashbang,
    HE,
    Smoke,
    Molotov,
    Decoy,
    Incendiary,
    Taser,
    C4,
    Healthshot,
};

// Weapon id -> Weapon. Reference: deadlocked Weapon::from_index().
Weapon weapon_from_index(uint16_t index);

// Reference: deadlocked Weapon::to_string() (Display impl).
std::string_view weapon_name(Weapon weapon);

// Reference: deadlocked Weapon::weapon_class().
WeaponClass weapon_class(Weapon weapon);

// Reference: deadlocked Weapon::to_icon(); kept as an ASCII fallback glyph so
// the headless/CLI and tests need no icon font.
std::string_view weapon_icon(Weapon weapon);

// Ballistics / handling data used by the simulator. Values mirror the CS2
// weapon script fields the reference reads through the schema
// (CCSWeaponBaseVData: m_nDamage, m_flPenetration, m_flRange,
// m_flRangeModifier, m_flArmorRatio, m_flCycleTime).
struct WeaponStats {
    int damage = 0;
    float armor_penetration = 1.0f;
    float penetration = 1.0f;
    float range = 8192.0f;
    float range_modifier = 0.99f;
    float cycle_time = 0.1f;   // seconds between shots
    float recoil_magnitude = 0.0f;
    int max_clip = 0;
    bool automatic = false;
    bool is_sniper = false;
};

std::optional<WeaponStats> weapon_stats(Weapon weapon);

const std::vector<Weapon>& all_weapons();

// Parses a config / CLI token ("ak47", "AK-47", "deagle") to a Weapon.
std::optional<Weapon> weapon_from_token(std::string_view token);

}  // namespace lvx
