#include "logvex/weapon.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <string>

namespace lvx {
namespace {

std::string normalise(std::string_view token) {
    std::string out;
    out.reserve(token.size());
    for (char c : token) {
        if (c == '-' || c == ' ' || c == '_') continue;
        out.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
    }
    return out;
}

struct WeaponRow {
    Weapon weapon;
    const char* name;   // Display name (reference Weapon::Display)
    const char* token;  // snake_case token (reference serde rename_all)
    WeaponClass cls;
    WeaponStats stats;
};

// Reference: deadlocked `shared/src/weapon.rs` + CS2 weapon script values read
// through the schema in `cheat/src/cs2/offsets.rs` (m_nDamage, m_flPenetration,
// m_flRange, m_flRangeModifier, m_flArmorRatio, m_flCycleTime).
const auto kWeapons = std::to_array<WeaponRow>({
    {Weapon::None, "?", "none", WeaponClass::Unknown, {}},
    // pistols
    {Weapon::CZ75, "CZ75-Auto", "cz75", WeaponClass::Pistol,
     {31, 0.77f, 1.0f, 8192.0f, 0.99f, 0.075f, 0.9f, 12, true, false}},
    {Weapon::DesertEagle, "Desert Eagle", "desert_eagle", WeaponClass::Pistol,
     {63, 0.93f, 1.0f, 8192.0f, 0.99f, 0.225f, 1.6f, 7, false, false}},
    {Weapon::DualBeretras, "Dual Berettas", "dual_berettas", WeaponClass::Pistol,
     {38, 0.72f, 1.0f, 8192.0f, 0.99f, 0.12f, 1.1f, 30, true, false}},
    {Weapon::FiveSeven, "Five-SeveN", "five_seven", WeaponClass::Pistol,
     {32, 0.77f, 1.0f, 8192.0f, 0.99f, 0.15f, 0.9f, 20, false, false}},
    {Weapon::Glock, "Glock-18", "glock", WeaponClass::Pistol,
     {30, 0.47f, 1.0f, 8192.0f, 0.99f, 0.15f, 0.8f, 20, false, false}},
    {Weapon::P2000, "P2000", "p2000", WeaponClass::Pistol,
     {35, 0.50f, 1.0f, 8192.0f, 0.99f, 0.17f, 0.9f, 13, false, false}},
    {Weapon::P250, "P250", "p250", WeaponClass::Pistol,
     {38, 0.64f, 1.0f, 8192.0f, 0.99f, 0.15f, 1.0f, 13, false, false}},
    {Weapon::Revolver, "R8 Revolver", "revolver", WeaponClass::Pistol,
     {86, 0.93f, 1.0f, 8192.0f, 0.99f, 0.25f, 1.8f, 8, false, false}},
    {Weapon::Tec9, "Tec-9", "tec9", WeaponClass::Pistol,
     {33, 0.81f, 1.0f, 8192.0f, 0.99f, 0.12f, 1.0f, 18, true, false}},
    {Weapon::Usp, "USP-S", "usp", WeaponClass::Pistol,
     {35, 0.50f, 1.0f, 8192.0f, 0.99f, 0.17f, 0.9f, 12, false, false}},
    // smg
    {Weapon::MAC10, "MAC-10", "mac10", WeaponClass::Smg,
     {29, 0.48f, 1.0f, 8192.0f, 0.99f, 0.075f, 1.2f, 30, true, false}},
    {Weapon::MP5, "MP5-SD", "mp5", WeaponClass::Smg,
     {27, 0.61f, 1.0f, 8192.0f, 0.99f, 0.08f, 1.0f, 30, true, false}},
    {Weapon::MP7, "MP7", "mp7", WeaponClass::Smg,
     {29, 0.62f, 1.0f, 8192.0f, 0.99f, 0.08f, 1.0f, 30, true, false}},
    {Weapon::MP9, "MP9", "mp9", WeaponClass::Smg,
     {26, 0.60f, 1.0f, 8192.0f, 0.99f, 0.07f, 1.0f, 30, true, false}},
    {Weapon::P90, "P90", "p90", WeaponClass::Smg,
     {26, 0.69f, 1.0f, 8192.0f, 0.99f, 0.07f, 1.0f, 50, true, false}},
    {Weapon::Bizon, "PP-Bizon", "bizon", WeaponClass::Smg,
     {27, 0.52f, 1.0f, 8192.0f, 0.99f, 0.08f, 1.0f, 64, true, false}},
    {Weapon::UMP45, "UMP-45", "ump45", WeaponClass::Smg,
     {35, 0.65f, 1.0f, 8192.0f, 0.99f, 0.09f, 1.1f, 25, true, false}},
    // shotguns
    {Weapon::Mag7, "Mag-7", "mag7", WeaponClass::Shotgun,
     {30, 0.55f, 1.0f, 3000.0f, 0.70f, 0.85f, 2.2f, 5, false, false}},
    {Weapon::Nova, "Nova", "nova", WeaponClass::Shotgun,
     {26, 0.50f, 1.0f, 3000.0f, 0.70f, 0.85f, 2.0f, 8, false, false}},
    {Weapon::SawedOff, "Sawed-Off", "sawed_off", WeaponClass::Shotgun,
     {32, 0.55f, 1.0f, 3000.0f, 0.70f, 0.85f, 2.2f, 7, false, false}},
    {Weapon::XM1014, "XM1014", "xm1014", WeaponClass::Shotgun,
     {20, 0.50f, 1.0f, 3000.0f, 0.70f, 0.25f, 1.8f, 7, false, false}},
    // lmg
    {Weapon::M249, "M249", "m249", WeaponClass::Heavy,
     {32, 0.80f, 1.0f, 8192.0f, 0.97f, 0.08f, 1.4f, 100, true, false}},
    {Weapon::Negev, "Negev", "negev", WeaponClass::Heavy,
     {35, 0.75f, 1.0f, 8192.0f, 0.97f, 0.07f, 1.4f, 150, true, false}},
    // assault rifles
    {Weapon::AK47, "AK-47", "ak47", WeaponClass::Rifle,
     {36, 0.775f, 2.0f, 8192.0f, 0.98f, 0.1f, 1.9f, 30, true, false}},
    {Weapon::Aug, "AUG", "aug", WeaponClass::Rifle,
     {28, 0.90f, 2.0f, 8192.0f, 0.98f, 0.09f, 1.4f, 30, true, false}},
    {Weapon::Famas, "FAMAS", "famas", WeaponClass::Rifle,
     {30, 0.70f, 2.0f, 8192.0f, 0.98f, 0.09f, 1.5f, 25, true, false}},
    {Weapon::Galil, "Galil AR", "galil", WeaponClass::Rifle,
     {30, 0.775f, 2.0f, 8192.0f, 0.98f, 0.09f, 1.6f, 35, true, false}},
    {Weapon::M4A1S, "M4A1-S", "m4a1s", WeaponClass::Rifle,
     {38, 0.70f, 2.0f, 8192.0f, 0.99f, 0.09f, 1.3f, 20, true, false}},
    {Weapon::M4A4, "M4A4", "m4a4", WeaponClass::Rifle,
     {33, 0.70f, 2.0f, 8192.0f, 0.98f, 0.09f, 1.4f, 30, true, false}},
    {Weapon::SG553, "SG 553", "sg553", WeaponClass::Rifle,
     {30, 1.00f, 2.0f, 8192.0f, 0.98f, 0.09f, 1.5f, 30, true, false}},
    // snipers
    {Weapon::Awp, "AWP", "awp", WeaponClass::Sniper,
     {115, 0.975f, 3.0f, 8192.0f, 0.99f, 1.46f, 3.0f, 5, false, true}},
    {Weapon::G3SG1, "G3SG1", "g3sg1", WeaponClass::Sniper,
     {80, 0.975f, 2.5f, 8192.0f, 0.99f, 0.25f, 2.6f, 20, false, true}},
    {Weapon::SCAR20, "SCAR-20", "scar20", WeaponClass::Sniper,
     {80, 0.975f, 2.5f, 8192.0f, 0.99f, 0.25f, 2.6f, 20, false, true}},
    {Weapon::SSG08, "SSG 08", "ssg08", WeaponClass::Sniper,
     {88, 0.85f, 2.5f, 8192.0f, 0.99f, 1.2f, 2.4f, 10, false, true}},
    // knives
    {Weapon::KnifeCT, "Knife", "knife_ct", WeaponClass::Knife, {55, 1.0f, 0.0f, 100.0f, 1.0f, 0.4f, 0.0f, 0, false, false}},
    {Weapon::KnifeT, "Knife", "knife_t", WeaponClass::Knife, {55, 1.0f, 0.0f, 100.0f, 1.0f, 0.4f, 0.0f, 0, false, false}},
    {Weapon::KnifeBayonet, "Bayonet", "knife_bayonet", WeaponClass::Knife, {55, 1.0f, 0.0f, 100.0f, 1.0f, 0.4f, 0.0f, 0, false, false}},
    {Weapon::KnifeBowie, "Bowie Knife", "knife_bowie", WeaponClass::Knife, {55, 1.0f, 0.0f, 100.0f, 1.0f, 0.4f, 0.0f, 0, false, false}},
    {Weapon::KnifeButterfly, "Butterfly Knife", "knife_butterfly", WeaponClass::Knife, {55, 1.0f, 0.0f, 100.0f, 1.0f, 0.4f, 0.0f, 0, false, false}},
    {Weapon::KnifeClassic, "Classic Knife", "knife_classic", WeaponClass::Knife, {55, 1.0f, 0.0f, 100.0f, 1.0f, 0.4f, 0.0f, 0, false, false}},
    {Weapon::KnifeFalchion, "Falchion Knife", "knife_falchion", WeaponClass::Knife, {55, 1.0f, 0.0f, 100.0f, 1.0f, 0.4f, 0.0f, 0, false, false}},
    {Weapon::KnifeFlip, "Flip Knife", "knife_flip", WeaponClass::Knife, {55, 1.0f, 0.0f, 100.0f, 1.0f, 0.4f, 0.0f, 0, false, false}},
    {Weapon::KnifeGut, "Gut Knife", "knife_gut", WeaponClass::Knife, {55, 1.0f, 0.0f, 100.0f, 1.0f, 0.4f, 0.0f, 0, false, false}},
    {Weapon::KnifeHuntsman, "Huntsman Knife", "knife_huntsman", WeaponClass::Knife, {55, 1.0f, 0.0f, 100.0f, 1.0f, 0.4f, 0.0f, 0, false, false}},
    {Weapon::KnifeKarambit, "Karambit", "knife_karambit", WeaponClass::Knife, {55, 1.0f, 0.0f, 100.0f, 1.0f, 0.4f, 0.0f, 0, false, false}},
    {Weapon::KnifeKukri, "Kukri Knife", "knife_kukri", WeaponClass::Knife, {55, 1.0f, 0.0f, 100.0f, 1.0f, 0.4f, 0.0f, 0, false, false}},
    {Weapon::KnifeM9Bayonet, "M9 Bayonet", "knife_m9_bayonet", WeaponClass::Knife, {55, 1.0f, 0.0f, 100.0f, 1.0f, 0.4f, 0.0f, 0, false, false}},
    {Weapon::KnifeNavaja, "Navaja Knife", "knife_navaja", WeaponClass::Knife, {55, 1.0f, 0.0f, 100.0f, 1.0f, 0.4f, 0.0f, 0, false, false}},
    {Weapon::KnifeNomad, "Nomad Knife", "knife_nomad", WeaponClass::Knife, {55, 1.0f, 0.0f, 100.0f, 1.0f, 0.4f, 0.0f, 0, false, false}},
    {Weapon::KnifeParacord, "Paracord Knife", "knife_paracord", WeaponClass::Knife, {55, 1.0f, 0.0f, 100.0f, 1.0f, 0.4f, 0.0f, 0, false, false}},
    {Weapon::KnifeShadowDaggers, "Shadow Daggers", "knife_shadow_daggers", WeaponClass::Knife, {55, 1.0f, 0.0f, 100.0f, 1.0f, 0.4f, 0.0f, 0, false, false}},
    {Weapon::KnifeSkeleton, "Skeleton Knife", "knife_skeleton", WeaponClass::Knife, {55, 1.0f, 0.0f, 100.0f, 1.0f, 0.4f, 0.0f, 0, false, false}},
    {Weapon::KnifeStiletto, "Stiletto Knife", "knife_stiletto", WeaponClass::Knife, {55, 1.0f, 0.0f, 100.0f, 1.0f, 0.4f, 0.0f, 0, false, false}},
    {Weapon::KnifeSurvival, "Survival Knife", "knife_survival", WeaponClass::Knife, {55, 1.0f, 0.0f, 100.0f, 1.0f, 0.4f, 0.0f, 0, false, false}},
    {Weapon::KnifeTalon, "Talon Knife", "knife_talon", WeaponClass::Knife, {55, 1.0f, 0.0f, 100.0f, 1.0f, 0.4f, 0.0f, 0, false, false}},
    {Weapon::KnifeUrsus, "Ursus Knife", "knife_ursus", WeaponClass::Knife, {55, 1.0f, 0.0f, 100.0f, 1.0f, 0.4f, 0.0f, 0, false, false}},
    {Weapon::KnifeGold, "Golden Knife", "knife_gold", WeaponClass::Knife, {55, 1.0f, 0.0f, 100.0f, 1.0f, 0.4f, 0.0f, 0, false, false}},
    // grenades
    {Weapon::Flashbang, "Flashbang", "flashbang", WeaponClass::Grenade, {1, 0.0f, 0.0f, 0.0f, 1.0f, 0.5f, 0.0f, 1, false, false}},
    {Weapon::HE, "HE Grenade", "he", WeaponClass::Grenade, {98, 0.0f, 0.0f, 0.0f, 1.0f, 0.5f, 0.0f, 1, false, false}},
    {Weapon::Smoke, "Smoke Grenade", "smoke", WeaponClass::Grenade, {1, 0.0f, 0.0f, 0.0f, 1.0f, 0.5f, 0.0f, 1, false, false}},
    {Weapon::Molotov, "Molotov", "molotov", WeaponClass::Grenade, {1, 0.0f, 0.0f, 0.0f, 1.0f, 0.5f, 0.0f, 1, false, false}},
    {Weapon::Decoy, "Decoy Grenade", "decoy", WeaponClass::Grenade, {1, 0.0f, 0.0f, 0.0f, 1.0f, 0.5f, 0.0f, 1, false, false}},
    {Weapon::Incendiary, "Incendiary Grenade", "incendiary", WeaponClass::Grenade, {1, 0.0f, 0.0f, 0.0f, 1.0f, 0.5f, 0.0f, 1, false, false}},
    // misc
    {Weapon::Taser, "Zeus x27", "taser", WeaponClass::Utility, {200, 1.0f, 0.0f, 300.0f, 1.0f, 1.0f, 0.0f, 1, false, false}},
    {Weapon::C4, "C4 Explosive", "c4", WeaponClass::Utility, {0, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1, false, false}},
    {Weapon::Healthshot, "Med-Shot", "healthshot", WeaponClass::Utility, {0, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1, false, false}},
});

const WeaponRow* row_for(Weapon weapon) {
    for (const auto& row : kWeapons) {
        if (row.weapon == weapon) return &row;
    }
    return nullptr;
}

}  // namespace

const char* weapon_class_name(WeaponClass cls) {
    switch (cls) {
        case WeaponClass::Unknown: return "Unknown";
        case WeaponClass::Knife: return "Knife";
        case WeaponClass::Pistol: return "Pistol";
        case WeaponClass::Smg: return "SMG";
        case WeaponClass::Heavy: return "Heavy";
        case WeaponClass::Shotgun: return "Shotgun";
        case WeaponClass::Rifle: return "Rifle";
        case WeaponClass::Sniper: return "Sniper";
        case WeaponClass::Grenade: return "Grenade";
        case WeaponClass::Utility: return "Utility";
    }
    return "Unknown";
}

Weapon weapon_from_index(uint16_t index) {
    // Reference: deadlocked Weapon::from_index().
    switch (index) {
        case 1: return Weapon::DesertEagle;
        case 2: return Weapon::DualBeretras;
        case 3: return Weapon::FiveSeven;
        case 4: return Weapon::Glock;
        case 7: return Weapon::AK47;
        case 8: return Weapon::Aug;
        case 9: return Weapon::Awp;
        case 10: return Weapon::Famas;
        case 11: return Weapon::G3SG1;
        case 13: return Weapon::Galil;
        case 14: return Weapon::M249;
        case 16: return Weapon::M4A4;
        case 17: return Weapon::MAC10;
        case 19: return Weapon::P90;
        case 23: return Weapon::MP5;
        case 24: return Weapon::UMP45;
        case 25: return Weapon::XM1014;
        case 26: return Weapon::Bizon;
        case 27: return Weapon::Mag7;
        case 28: return Weapon::Negev;
        case 29: return Weapon::SawedOff;
        case 30: return Weapon::Tec9;
        case 31: return Weapon::Taser;
        case 32: return Weapon::P2000;
        case 33: return Weapon::MP7;
        case 34: return Weapon::MP9;
        case 35: return Weapon::Nova;
        case 36: return Weapon::P250;
        case 38: return Weapon::SCAR20;
        case 39: return Weapon::SG553;
        case 40: return Weapon::SSG08;
        case 41: return Weapon::KnifeGold;
        case 42: return Weapon::KnifeCT;
        case 43: return Weapon::Flashbang;
        case 44: return Weapon::HE;
        case 45: return Weapon::Smoke;
        case 46: return Weapon::Molotov;
        case 47: return Weapon::Decoy;
        case 48: return Weapon::Incendiary;
        case 49: return Weapon::C4;
        case 57: return Weapon::Healthshot;
        case 59: return Weapon::KnifeT;
        case 60: return Weapon::M4A1S;
        case 61: return Weapon::Usp;
        case 63: return Weapon::CZ75;
        case 64: return Weapon::Revolver;
        case 503: return Weapon::KnifeClassic;
        case 505: return Weapon::KnifeFlip;
        case 506: return Weapon::KnifeGut;
        case 507: return Weapon::KnifeKarambit;
        case 508: return Weapon::KnifeM9Bayonet;
        case 509: return Weapon::KnifeHuntsman;
        case 512: return Weapon::KnifeFalchion;
        case 514: return Weapon::KnifeBowie;
        case 515: return Weapon::KnifeButterfly;
        case 516: return Weapon::KnifeShadowDaggers;
        case 517: return Weapon::KnifeParacord;
        case 518: return Weapon::KnifeSurvival;
        case 519: return Weapon::KnifeUrsus;
        case 520: return Weapon::KnifeNavaja;
        case 521: return Weapon::KnifeNomad;
        case 522: return Weapon::KnifeStiletto;
        case 523: return Weapon::KnifeTalon;
        case 524: return Weapon::KnifeSkeleton;
        default: return Weapon::None;
    }
}

std::string_view weapon_name(Weapon weapon) {
    const WeaponRow* row = row_for(weapon);
    return row ? row->name : "?";
}

WeaponClass weapon_class(Weapon weapon) {
    const WeaponRow* row = row_for(weapon);
    return row ? row->cls : WeaponClass::Unknown;
}

std::string_view weapon_icon(Weapon weapon) {
    const WeaponRow* row = row_for(weapon);
    if (!row) return "?";
    // ASCII glyph map for the headless/CLI radar and tests.
    switch (weapon) {
        case Weapon::AK47: return "AK";
        case Weapon::M4A1S: return "M4S";
        case Weapon::M4A4: return "M4";
        case Weapon::Awp: return "AWP";
        case Weapon::DesertEagle: return "DE";
        case Weapon::Glock: return "GLK";
        case Weapon::Usp: return "USP";
        case Weapon::C4: return "C4";
        case Weapon::HE: return "HE";
        case Weapon::Flashbang: return "FLS";
        case Weapon::Smoke: return "SMK";
        case Weapon::Molotov: return "MOL";
        case Weapon::Incendiary: return "INC";
        case Weapon::Taser: return "ZEUS";
        case Weapon::KnifeCT:
        case Weapon::KnifeT: return "KNF";
        default: return row->token[0] == '\0' ? "?" : row->token;
    }
}

std::optional<WeaponStats> weapon_stats(Weapon weapon) {
    const WeaponRow* row = row_for(weapon);
    if (!row || weapon == Weapon::None) {
        return std::nullopt;
    }
    return row->stats;
}

const std::vector<Weapon>& all_weapons() {
    static const std::vector<Weapon> list = [] {
        std::vector<Weapon> out;
        out.reserve(kWeapons.size());
        for (const auto& row : kWeapons) {
            if (row.weapon != Weapon::None) out.push_back(row.weapon);
        }
        return out;
    }();
    return list;
}

std::optional<Weapon> weapon_from_token(std::string_view token) {
    const std::string key = normalise(token);
    for (const auto& row : kWeapons) {
        if (row.weapon == Weapon::None) continue;
        if (normalise(row.token) == key || normalise(row.name) == key) {
            return row.weapon;
        }
    }
    return std::nullopt;
}

}  // namespace lvx
