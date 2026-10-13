#include "test.hpp"

#include "logvex/config.hpp"

using namespace lvx;

TEST(defaults_populate_weapon_overrides) {
    Config config = Config::defaults();
    CHECK(!config.aim.weapons.empty());
    CHECK(config.aim.weapons.count(Weapon::AK47) == 1);
}

TEST(config_round_trip) {
    Config config = Config::defaults();
    config.aim.global.aimbot.fov = 6.25f;
    config.aim.global.aimbot.smooth = 3.5f;
    config.aim.global.rcs.enabled = true;
    config.aim.global.triggerbot.delay_min = 40;
    config.aim.global.triggerbot.delay_max = 90;
    config.hud.draw_skeleton = false;
    config.practice.god_mode = true;

    std::string path = "/tmp/lvx_test_config.json";
    std::string error;
    CHECK(save_config(config, path, &error));
    auto loaded = load_config(path, &error);
    CHECK(loaded.has_value());
    if (loaded) {
        CHECK_NEAR(loaded->aim.global.aimbot.fov, 6.25f, 0.001);
        CHECK_NEAR(loaded->aim.global.aimbot.smooth, 3.5f, 0.001);
        CHECK(loaded->aim.global.rcs.enabled);
        CHECK(loaded->aim.global.triggerbot.delay_min == 40);
        CHECK(loaded->aim.global.triggerbot.delay_max == 90);
        CHECK(!loaded->hud.draw_skeleton);
        CHECK(loaded->practice.god_mode);
    }
}

TEST(config_bones_round_trip) {
    Config config = Config::defaults();
    config.aim.global.aimbot.bones = {Bone::Head, Bone::Neck};
    std::string path = "/tmp/lvx_test_bones.json";
    CHECK(save_config(config, path));
    auto loaded = load_config(path);
    CHECK(loaded.has_value());
    if (loaded) {
        CHECK(loaded->aim.global.aimbot.bones.size() == 2);
        CHECK(loaded->aim.global.aimbot.bones[0] == Bone::Head);
        CHECK(loaded->aim.global.aimbot.bones[1] == Bone::Neck);
    }
}

TEST(apply_override_updates_values) {
    Config config = Config::defaults();
    std::string error;
    CHECK(apply_config_override(config, "aim.global.aimbot.fov=12.5", &error));
    CHECK_NEAR(config.aim.global.aimbot.fov, 12.5f, 0.001);
    CHECK(apply_config_override(config, "aim.global.rcs.enabled=true", &error));
    CHECK(config.aim.global.rcs.enabled);
    CHECK(!apply_config_override(config, "no.such.key=1", &error));
}

TEST(config_dir_uses_xdg) {
    std::string dir = config_dir();
    CHECK(!dir.empty());
}
