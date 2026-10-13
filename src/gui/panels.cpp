#include <algorithm>
#include <cstdio>
#include <string>
#include <vector>

#include <GLFW/glfw3.h>
#include <imgui.h>

#include "logvex/diagnostics.hpp"
#include "logvex/features.hpp"
#include "logvex/gui/app.hpp"
#include "logvex/math.hpp"
#include "logvex/scenarios.hpp"
#include "logvex/weapon.hpp"

namespace lvx::gui {

void App::draw_menu_bar() {
    if (!ImGui::BeginMainMenuBar()) return;

    if (ImGui::BeginMenu("File")) {
        if (ImGui::MenuItem("Save config")) save_config_now();
        if (ImGui::MenuItem("Load config")) load_config_now();
        ImGui::Separator();
        if (ImGui::MenuItem("Reset scenario")) {
            sim_.set_world(make_scenario(scenarios()[static_cast<size_t>(scenario_index_)].name)
                               .value());
            set_status("scenario reset");
        }
        ImGui::Separator();
        if (ImGui::MenuItem("Quit")) glfwWindowShouldClose(window_);
        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Scenario")) {
        for (size_t i = 0; i < scenarios().size(); ++i) {
            bool selected = static_cast<int>(i) == scenario_index_;
            if (ImGui::MenuItem(scenarios()[i].name.c_str(), nullptr, selected)) {
                scenario_index_ = static_cast<int>(i);
                sim_.set_world(make_scenario(scenarios()[i].name).value());
                set_status("loaded scenario " + scenarios()[i].name);
            }
        }
        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Radar")) {
        if (ImGui::MenuItem("Start server", nullptr, radar_running_)) start_radar();
        if (ImGui::MenuItem("Stop server")) stop_radar();
        ImGui::EndMenu();
    }

    ImGui::EndMainMenuBar();
}

void App::draw_panels() {
    ImGui::SetNextWindowPos(ImVec2(16.0f, 32.0f), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(430.0f, 620.0f), ImGuiCond_FirstUseEver);
    ImGui::Begin("Logvex CS2 Practice");

    if (ImGui::BeginTabBar("tabs")) {
        Player* local = sim_.world().local_player();

        if (ImGui::BeginTabItem("Aimbot")) {
            AimbotConfig& aim = config_.aim.global.aimbot;
            ImGui::Checkbox("Enabled", &aim.enabled);
            ImGui::SameLine();
            const char* modes[] = {"Hold", "Toggle"};
            int mode = aim.mode == KeyMode::Toggle ? 1 : 0;
            if (ImGui::Combo("Mode", &mode, modes, 2)) {
                aim.mode = mode == 1 ? KeyMode::Toggle : KeyMode::Hold;
            }
            ImGui::SliderFloat("FOV", &aim.fov, 0.1f, 40.0f);
            ImGui::SliderFloat("Smooth", &aim.smooth, 1.0f, 30.0f);
            ImGui::SliderFloat("Inertia", &aim.inertia, 0.1f, 3.0f);
            ImGui::SliderFloat("Prediction", &aim.prediction_time, 0.0f, 0.25f);
            ImGui::SliderInt("Start bullet", &aim.start_bullet, 0, 10);
            ImGui::Checkbox("Distance adjusted FOV", &aim.distance_adjusted_fov);
            ImGui::Checkbox("Visibility check", &aim.visibility_check);
            ImGui::Checkbox("Flash check", &aim.flash_check);
            ImGui::Checkbox("Target friendlies", &aim.target_friendlies);
            ImGui::Checkbox("Penetration check", &aim.penetration_check);
            if (aim.penetration_check) {
                int min_dmg = static_cast<int>(aim.minimum_penetration_damage);
                if (ImGui::SliderInt("Min pen damage", &min_dmg, 1, 200)) {
                    aim.minimum_penetration_damage = static_cast<uint32_t>(min_dmg);
                }
            }
            const char* targeting[] = {"FOV", "Distance"};
            int tm = aim.targeting_mode == TargetingMode::Distance ? 1 : 0;
            if (ImGui::Combo("Targeting", &tm, targeting, 2)) {
                aim.targeting_mode = tm == 1 ? TargetingMode::Distance : TargetingMode::Fov;
            }
            ImGui::SeparatorText("Bones");
            for (Bone bone : all_bones()) {
                bool enabled = std::find(aim.bones.begin(), aim.bones.end(), bone) != aim.bones.end();
                if (ImGui::Checkbox(bone_name(bone), &enabled)) {
                    if (enabled) {
                        aim.bones.push_back(bone);
                    } else {
                        aim.bones.erase(std::remove(aim.bones.begin(), aim.bones.end(), bone),
                                        aim.bones.end());
                    }
                }
                ImGui::SameLine();
                if ((ImGui::GetCursorPosX() > 200.0f)) ImGui::NewLine();
            }
            ImGui::NewLine();

            if (local) {
                ImGui::Text("Local weapon: %s", std::string(weapon_name(local->weapon)).c_str());
                WeaponConfig& wc = config_.aim.weapons[local->weapon];
                if (ImGui::Checkbox("Per-weapon override", &wc.aimbot.enable_override)) {
                    wc.aimbot = config_.aim.global.aimbot;
                    wc.aimbot.enable_override = true;
                }
                if (wc.aimbot.enable_override) {
                    ImGui::SliderFloat("Override FOV", &wc.aimbot.fov, 0.1f, 40.0f);
                    ImGui::SliderFloat("Override smooth", &wc.aimbot.smooth, 1.0f, 30.0f);
                }
            }
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Triggerbot")) {
            TriggerbotConfig& trig = config_.aim.global.triggerbot;
            ImGui::Checkbox("Enabled", &trig.enabled);
            const char* modes[] = {"Hold", "Toggle"};
            int mode = trig.mode == KeyMode::Toggle ? 1 : 0;
            if (ImGui::Combo("Mode", &mode, modes, 2)) {
                trig.mode = mode == 1 ? KeyMode::Toggle : KeyMode::Hold;
            }
            int dmin = static_cast<int>(trig.delay_min);
            int dmax = static_cast<int>(trig.delay_max);
            if (ImGui::SliderInt("Delay min (ms)", &dmin, 0, 1000)) trig.delay_min = static_cast<uint64_t>(dmin);
            if (ImGui::SliderInt("Delay max (ms)", &dmax, 0, 1000)) trig.delay_max = static_cast<uint64_t>(dmax);
            int duration = static_cast<int>(trig.shot_duration);
            if (ImGui::SliderInt("Shot duration", &duration, 0, 1000)) trig.shot_duration = static_cast<uint64_t>(duration);
            ImGui::Checkbox("Flash check", &trig.flash_check);
            ImGui::Checkbox("Scope check", &trig.scope_check);
            ImGui::Checkbox("Velocity check", &trig.velocity_check);
            ImGui::SliderFloat("Velocity threshold", &trig.velocity_threshold, 0.0f, 300.0f);
            ImGui::Checkbox("Head only", &trig.head_only);
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("RCS")) {
            RcsConfig& rcs = config_.aim.global.rcs;
            ImGui::Checkbox("Enabled", &rcs.enabled);
            ImGui::SliderFloat("Strength X", &rcs.strength.x, 0.0f, 2.0f);
            ImGui::SliderFloat("Strength Y", &rcs.strength.y, 0.0f, 2.0f);
            if (local) ImGui::Text("Recoil punch: %.2f / %.2f", local->aim_punch.x, local->aim_punch.y);
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("HUD")) {
            ImGui::Checkbox("FOV circle", &config_.hud.draw_fov_circle);
            ImGui::Checkbox("Boxes", &config_.hud.draw_boxes);
            ImGui::Checkbox("Skeleton", &config_.hud.draw_skeleton);
            ImGui::Checkbox("Health bar", &config_.hud.draw_health_bar);
            ImGui::Checkbox("Armor bar", &config_.hud.draw_armor_bar);
            ImGui::Checkbox("Names", &config_.hud.draw_names);
            ImGui::Checkbox("Weapon", &config_.hud.draw_weapon);
            ImGui::Checkbox("Bomb timer", &config_.hud.draw_bomb_timer);
            ImGui::Checkbox("Crosshair / grid", &config_.hud.draw_crosshair);
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Practice")) {
            ImGui::Checkbox("God mode", &config_.practice.god_mode);
            ImGui::Checkbox("Infinite ammo", &config_.practice.infinite_ammo);
            ImGui::Checkbox("Impact markers", &config_.practice.show_impact_markers);
            ImGui::Checkbox("Damage numbers", &config_.practice.show_damage_numbers);
            if (local) {
                ImGui::SliderInt("Health", &local->health, 0, 100);
                ImGui::SliderInt("Armor", &local->armor, 0, 100);
                ImGui::SliderFloat("Flash alpha", &local->flash_alpha, 0.0f, 1.0f);
                ImGui::Text("Ammo: %d / %d", local->ammo, local->ammo_reserve);
                if (ImGui::Button("Reload")) {
                    if (auto stats = weapon_stats(local->weapon)) {
                        int need = stats->max_clip - local->ammo;
                        int take = std::min(need, local->ammo_reserve);
                        local->ammo += take;
                        local->ammo_reserve -= take;
                        set_status("reloaded");
                    }
                }
                ImGui::SameLine();
                if (ImGui::Button("Fire")) {
                    ShotTrace trace = sim_.fire_local();
                    set_status(trace.hit_player ? "shot hit" : "shot fired");
                }
            }
            ImGui::Separator();
            if (ImGui::Button("Reset all bots")) {
                sim_.set_world(make_scenario(scenarios()[static_cast<size_t>(scenario_index_)].name)
                                   .value());
                set_status("bots reset");
            }
            ImGui::SameLine();
            if (ImGui::Button("Reset config")) {
                config_ = Config::defaults();
                set_status("config reset to defaults");
            }
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Radar")) {
            ImGui::Checkbox("Enabled", &config_.radar.enabled);
            int port = static_cast<int>(config_.radar.port);
            if (ImGui::InputInt("Port", &port)) {
                config_.radar.port = static_cast<uint16_t>(std::clamp(port, 1, 65535));
            }
            char bind[64];
            std::snprintf(bind, sizeof(bind), "%s", config_.radar.bind_address.c_str());
            if (ImGui::InputText("Bind address", bind, sizeof(bind))) {
                config_.radar.bind_address = bind;
            }
            if (radar_running_) {
                ImGui::TextColored(ImVec4(0.4f, 0.9f, 0.4f, 1.0f), "Listening on %s:%u",
                                   config_.radar.bind_address.c_str(), radar_port_);
                if (ImGui::Button("Stop")) stop_radar();
            } else {
                if (ImGui::Button("Start")) start_radar();
            }
            ImGui::TextWrapped("Open http://%s:%u/ in a browser for the live radar.",
                               config_.radar.bind_address.c_str(), config_.radar.port);
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Diagnostics")) {
            static EnvironmentReport report = detect_environment();
            if (ImGui::Button("Re-scan environment")) report = detect_environment();
            ImGui::TextWrapped("%s", format_environment_report(report).c_str());
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Stats")) {
            ImGui::Text("FPS: %.1f", fps_);
            ImGui::Text("Tick: %llu", static_cast<unsigned long long>(sim_.tick()));
            ImGui::Text("Time: %.2fs", sim_.time());
            ImGui::Text("Players: %zu", sim_.world().players.size());
            ImGui::Text("Shots: %zu", sim_.shot_traces().size());
            ImGui::Text("Hits: %zu", sim_.hit_events().size());
            ImGui::Text("Target: %s", target_.valid() ? bone_name(target_.bone) : "none");
            if (target_.valid()) {
                ImGui::Text("Target FOV: %.2f  distance: %.0f", target_.fov, target_.distance);
            }
            ImGui::Text("Aimbot active: %s", aimbot_active_ ? "yes" : "no");
            ImGui::Text("Triggerbot active: %s", triggerbot_active_ ? "yes" : "no");
            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }
    ImGui::End();
}

void App::draw_status_bar() {
    ImGuiIO& io = ImGui::GetIO();
    ImGui::SetNextWindowPos(ImVec2(0.0f, io.DisplaySize.y - 28.0f));
    ImGui::SetNextWindowSize(ImVec2(io.DisplaySize.x, 28.0f));
    ImGui::Begin("##status", nullptr,
                 ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
                     ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBringToFrontOnFocus);
    if (status_timer_ > 0.0f) {
        ImGui::Text("%s", status_message_.c_str());
    } else {
        ImGui::Text("RMB look | LMB fire | WASD move | Shift walk | Ctrl crouch | R reload | "
                    "Z aimbot | X triggerbot | 1-9 weapon");
    }
    ImGui::SameLine(ImGui::GetWindowWidth() - 160.0f);
    ImGui::Text("fps %.0f  tick %llu", fps_, static_cast<unsigned long long>(sim_.tick()));
    ImGui::End();
}

}  // namespace lvx::gui
