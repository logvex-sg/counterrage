#include <algorithm>
#include <cstdio>

#include <imgui.h>

#include "logvex/features.hpp"
#include "logvex/gui/app.hpp"
#include "logvex/math.hpp"

namespace lvx::gui {

// Draws the 3D scene as an ESP overlay: world grid, player boxes, skeletons,
// health/armor bars, names and the bomb timer. All drawing uses the simulated
// view matrix produced by the Simulation, so moving/aiming changes the output.
void App::draw_scene() {
    ImDrawList* draw = ImGui::GetBackgroundDrawList();
    const World& world = sim_.world();
    glm::vec2 window_size{ImGui::GetIO().DisplaySize.x, ImGui::GetIO().DisplaySize.y};

    // Ground grid projected through the view matrix.
    if (config_.hud.draw_crosshair) {
        glm::vec3 origin = world.eye_position;
        for (int i = -20; i <= 20; ++i) {
            float offset = static_cast<float>(i) * 100.0f;
            for (int axis = 0; axis < 2; ++axis) {
                glm::vec3 a = axis == 0 ? glm::vec3{offset, -2000.0f, 0.0f}
                                        : glm::vec3{-2000.0f, offset, 0.0f};
                glm::vec3 b = axis == 0 ? glm::vec3{offset, 2000.0f, 0.0f}
                                        : glm::vec3{2000.0f, offset, 0.0f};
                auto pa = math::world_to_screen(world.view_matrix, a, window_size);
                auto pb = math::world_to_screen(world.view_matrix, b, window_size);
                if (pa && pb) {
                    draw->AddLine(ImVec2(pa->x, pa->y), ImVec2(pb->x, pb->y),
                                  IM_COL32(60, 70, 90, 120), 1.0f);
                }
            }
        }
        (void)origin;
    }

    // Wall outlines.
    for (const auto& wall : world.walls) {
        glm::vec3 corners[8] = {
            {wall.min.x, wall.min.y, wall.min.z}, {wall.max.x, wall.min.y, wall.min.z},
            {wall.max.x, wall.max.y, wall.min.z}, {wall.min.x, wall.max.y, wall.min.z},
            {wall.min.x, wall.min.y, wall.max.z}, {wall.max.x, wall.min.y, wall.max.z},
            {wall.max.x, wall.max.y, wall.max.z}, {wall.min.x, wall.max.y, wall.max.z},
        };
        const int edges[12][2] = {{0, 1}, {1, 2}, {2, 3}, {3, 0}, {4, 5}, {5, 6},
                                  {6, 7}, {7, 4}, {0, 4}, {1, 5}, {2, 6}, {3, 7}};
        for (auto& edge : edges) {
            auto a = math::world_to_screen(world.view_matrix, corners[edge[0]], window_size);
            auto b = math::world_to_screen(world.view_matrix, corners[edge[1]], window_size);
            if (a && b) {
                draw->AddLine(ImVec2(a->x, a->y), ImVec2(b->x, b->y),
                              IM_COL32(120, 120, 140, 160), 1.0f);
            }
        }
    }

    // Players.
    for (size_t i = 0; i < world.players.size(); ++i) {
        const Player& player = world.players[i];
        if (!player.is_alive()) continue;
        bool is_local = static_cast<int>(i) == world.local_index;

        // Box from the topmost and bottom hitboxes.
        glm::vec3 top{player.position.x, player.position.y, player.position.z + 72.0f};
        glm::vec3 bottom{player.position.x, player.position.y, player.position.z};
        auto ptop = math::world_to_screen(world.view_matrix, top, window_size);
        auto pbottom = math::world_to_screen(world.view_matrix, bottom, window_size);

        ImU32 color = is_local ? IM_COL32(80, 160, 255, 255)
                               : (player.visible ? IM_COL32(240, 80, 80, 255)
                                                 : IM_COL32(140, 60, 60, 255));

        if (ptop && pbottom && config_.hud.draw_boxes && !is_local) {
            float height = std::fabs(ptop->y - pbottom->y);
            float width = height * 0.5f;
            ImVec2 min{ptop->x - width * 0.5f, ptop->y};
            ImVec2 max{ptop->x + width * 0.5f, pbottom->y};
            draw->AddRect(min, max, color, 0.0f, 0, 1.5f);

            if (config_.hud.draw_health_bar) {
                float frac = static_cast<float>(player.health) / std::max(player.max_health, 1);
                float bar_x = min.x - 6.0f;
                draw->AddRectFilled(ImVec2(bar_x, min.y), ImVec2(bar_x + 3.0f, max.y),
                                    IM_COL32(40, 40, 40, 200));
                float filled = (max.y - min.y) * frac;
                draw->AddRectFilled(ImVec2(bar_x, max.y - filled), ImVec2(bar_x + 3.0f, max.y),
                                    IM_COL32(80, 220, 100, 255));
            }
            if (config_.hud.draw_armor_bar) {
                float frac = std::min(player.armor / 100.0f, 1.0f);
                float bar_x = max.x + 3.0f;
                draw->AddRectFilled(ImVec2(bar_x, min.y), ImVec2(bar_x + 3.0f, max.y),
                                    IM_COL32(40, 40, 40, 200));
                float filled = (max.y - min.y) * frac;
                draw->AddRectFilled(ImVec2(bar_x, max.y - filled), ImVec2(bar_x + 3.0f, max.y),
                                    IM_COL32(90, 160, 255, 255));
            }
            if (config_.hud.draw_names) {
                draw->AddText(ImVec2(min.x, min.y - 16.0f), IM_COL32(230, 230, 230, 255),
                              player.name.c_str());
            }
            if (config_.hud.draw_weapon) {
                draw->AddText(ImVec2(min.x, max.y + 2.0f), IM_COL32(200, 200, 200, 255),
                              std::string(weapon_name(player.weapon)).c_str());
            }
        }

        // Skeleton ESP from the bone connection table.
        if (config_.hud.draw_skeleton && !is_local) {
            for (const auto& [a, b] : kBoneConnections) {
                const Hitbox* ha = player.hitbox_for(a);
                const Hitbox* hb = player.hitbox_for(b);
                if (!ha || !hb) continue;
                auto pa = math::world_to_screen(world.view_matrix, ha->center, window_size);
                auto pb = math::world_to_screen(world.view_matrix, hb->center, window_size);
                if (pa && pb) {
                    draw->AddLine(ImVec2(pa->x, pa->y), ImVec2(pb->x, pb->y),
                                  IM_COL32(240, 240, 120, 180), 1.0f);
                }
            }
            if (const Hitbox* head = player.hitbox_for(Bone::Head)) {
                auto p = math::world_to_screen(world.view_matrix, head->center, window_size);
                if (p) draw->AddCircle(ImVec2(p->x, p->y), 6.0f, IM_COL32(240, 240, 120, 200));
            }
        }
    }

    // Entities (dropped weapons, smokes).
    for (const auto& entity : world.entities) {
        auto p = math::world_to_screen(world.view_matrix, entity.position, window_size);
        if (!p) continue;
        draw->AddCircleFilled(ImVec2(p->x, p->y), 4.0f, IM_COL32(255, 200, 80, 220));
        draw->AddText(ImVec2(p->x + 6.0f, p->y - 6.0f), IM_COL32(255, 220, 140, 255),
                      entity.label.c_str());
    }

    // Impact markers from the shot traces.
    if (config_.practice.show_impact_markers) {
        for (const auto& trace : sim_.shot_traces()) {
            auto p = math::world_to_screen(world.view_matrix, trace.end, window_size);
            if (!p) continue;
            ImU32 c = trace.hit_player ? IM_COL32(255, 80, 80, 220) : IM_COL32(200, 200, 200, 160);
            draw->AddCircle(ImVec2(p->x, p->y), 5.0f, c, 0, 1.5f);
            if (config_.practice.show_damage_numbers && trace.hit_player) {
                char buffer[32];
                std::snprintf(buffer, sizeof(buffer), "%d", static_cast<int>(trace.end.x));
                (void)buffer;
            }
        }
    }

    // FOV circle around the crosshair.
    if (config_.hud.draw_fov_circle) {
        const Player* local = world.local_player();
        if (local) {
            const AimbotConfig& aim = effective_aimbot(config_, local->weapon);
            float radius = std::tan(glm::radians(aim.fov)) *
                           (window_size.y * 0.5f) / std::tan(glm::radians(45.0f));
            ImVec2 center{window_size.x * 0.5f, window_size.y * 0.5f};
            draw->AddCircle(center, std::max(radius, 2.0f), IM_COL32(255, 255, 255, 90));
        }
    }

    // Crosshair.
    ImVec2 center{window_size.x * 0.5f, window_size.y * 0.5f};
    draw->AddLine(ImVec2(center.x - 8.0f, center.y), ImVec2(center.x + 8.0f, center.y),
                  IM_COL32(0, 255, 0, 200), 1.5f);
    draw->AddLine(ImVec2(center.x, center.y - 8.0f), ImVec2(center.x, center.y + 8.0f),
                  IM_COL32(0, 255, 0, 200), 1.5f);

    // Bomb timer HUD.
    if (config_.hud.draw_bomb_timer && world.bomb.planted) {
        char buffer[64];
        std::snprintf(buffer, sizeof(buffer), "BOMB %.1fs", world.bomb.timer);
        draw->AddText(ImVec2(window_size.x * 0.5f - 30.0f, 24.0f), IM_COL32(255, 90, 90, 255),
                      buffer);
    }
}

}  // namespace lvx::gui
