#include "logvex/gui/app.hpp"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cstdio>
#include <cstdlib>

#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include "logvex/log.hpp"
#include "logvex/math.hpp"
#include "logvex/radar_server.hpp"
#include "logvex/scenarios.hpp"
#include "logvex/weapon.hpp"

namespace lvx::gui {
namespace {

radar::StateFeed g_feed;
radar::Server* g_radar_server = nullptr;

// GLFW callbacks write into a file-scope pointer to the App's input frame.
InputFrame* g_input = nullptr;

void glfw_key_cb(GLFWwindow*, int key, int, int action, int) {
    if (!g_input || key < 0 || key >= 512) return;
    if (action == GLFW_PRESS) g_input->keys[static_cast<size_t>(key)] = true;
    if (action == GLFW_RELEASE) g_input->keys[static_cast<size_t>(key)] = false;
}

void glfw_mouse_button_cb(GLFWwindow*, int button, int action, int) {
    if (!g_input || button < 0 || button >= 8) return;
    if (action == GLFW_PRESS) g_input->mouse[static_cast<size_t>(button)] = true;
    if (action == GLFW_RELEASE) g_input->mouse[static_cast<size_t>(button)] = false;
}

void glfw_cursor_cb(GLFWwindow*, double x, double y) {
    static double last_x = x;
    static double last_y = y;
    if (!g_input) return;
    if (g_input->cursor_captured) {
        g_input->mouse_delta += glm::vec2{static_cast<float>(x - last_x),
                                          static_cast<float>(last_y - y)};
    }
    last_x = x;
    last_y = y;
}

void glfw_scroll_cb(GLFWwindow*, double, double y) {
    if (g_input) g_input->scroll += static_cast<float>(y);
}

}  // namespace

App::App() = default;

App::~App() {
    stop_radar();
    if (window_) {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
        glfwDestroyWindow(window_);
        window_ = nullptr;
    }
    glfwTerminate();
}

bool App::init(int width, int height, const std::string& title) {
    if (!glfwInit()) {
        LVX_LOG_ERROR("glfwInit failed (no display?)");
        return false;
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    window_ = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);
    if (!window_) {
        LVX_LOG_ERROR("glfwCreateWindow failed");
        return false;
    }
    glfwMakeContextCurrent(window_);
    glfwSwapInterval(1);

    g_input = &input_;
    glfwSetWindowUserPointer(window_, this);
    glfwSetKeyCallback(window_, glfw_key_cb);
    glfwSetMouseButtonCallback(window_, glfw_mouse_button_cb);
    glfwSetCursorPosCallback(window_, glfw_cursor_cb);
    glfwSetScrollCallback(window_, glfw_scroll_cb);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window_, true);
    ImGui_ImplOpenGL3_Init("#version 330 core");

    config_ = Config::defaults();
    sim_.set_world(make_scenario(scenarios()[static_cast<size_t>(scenario_index_)].name).value());
    last_time_ = glfwGetTime();
    LVX_LOG_INFO("GUI initialised (" + std::to_string(width) + "x" + std::to_string(height) + ")");
    return true;
}

void App::set_status(const std::string& message) {
    status_message_ = message;
    status_timer_ = 3.0f;
    LVX_LOG_INFO(message);
}

void App::save_config_now() {
    std::string error;
    std::string path = default_config_path();
    if (save_config(config_, path, &error)) {
        set_status("saved config to " + path);
    } else {
        set_status("save failed: " + error);
    }
}

void App::load_config_now() {
    std::string error;
    std::string path = default_config_path();
    auto loaded = load_config(path, &error);
    if (loaded) {
        config_ = *loaded;
        set_status("loaded config from " + path);
    } else {
        set_status("load failed: " + error);
    }
}

void App::start_radar() {
    if (radar_running_) return;
    g_radar_server = new radar::Server(config_.radar.port, config_.radar.bind_address, &g_feed);
    if (g_radar_server->start()) {
        radar_running_ = true;
        radar_port_ = g_radar_server->port();
        set_status("radar server listening on " + config_.radar.bind_address + ":" +
                   std::to_string(radar_port_));
    } else {
        delete g_radar_server;
        g_radar_server = nullptr;
        set_status("radar server failed to bind port " + std::to_string(config_.radar.port));
    }
}

void App::stop_radar() {
    if (g_radar_server) {
        g_radar_server->stop();
        delete g_radar_server;
        g_radar_server = nullptr;
    }
    radar_running_ = false;
}

void App::handle_input(float dt) {
    if (!window_) return;
    Player* local = sim_.world().local_player();
    if (!local) return;

    // Mouse look: hold right mouse to capture the cursor.
    if (input_.mouse[GLFW_MOUSE_BUTTON_RIGHT]) {
        if (!input_.cursor_captured) {
            glfwSetInputMode(window_, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            input_.cursor_captured = true;
        }
    } else if (input_.cursor_captured) {
        glfwSetInputMode(window_, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        input_.cursor_captured = false;
    }

    InputState state;
    float forward = 0.0f;
    float strafe = 0.0f;
    if (input_.keys[GLFW_KEY_W]) forward += 1.0f;
    if (input_.keys[GLFW_KEY_S]) forward -= 1.0f;
    if (input_.keys[GLFW_KEY_A]) strafe -= 1.0f;
    if (input_.keys[GLFW_KEY_D]) strafe += 1.0f;
    state.move = {strafe, forward};
    state.walk = input_.keys[GLFW_KEY_LEFT_SHIFT];
    state.crouch = input_.keys[GLFW_KEY_LEFT_CONTROL];
    state.jump = input_.keys[GLFW_KEY_SPACE];
    state.reload = input_.keys[GLFW_KEY_R];
    state.scoped = input_.keys[GLFW_KEY_Q];
    state.attack = input_.mouse[GLFW_MOUSE_BUTTON_LEFT];
    state.look_delta = input_.mouse_delta * 0.12f;
    input_.mouse_delta = {0.0f, 0.0f};

    // Weapon selection (1-9).
    for (int i = 0; i < 9; ++i) {
        if (input_.keys[GLFW_KEY_1 + i]) {
            const auto& weapons = all_weapons();
            size_t index = static_cast<size_t>(i);
            if (index < weapons.size()) {
                selected_weapon_ = i;
                local->weapon = weapons[index];
                if (auto stats = weapon_stats(local->weapon)) {
                    local->ammo = stats->max_clip;
                    local->ammo_reserve = stats->max_clip * 3;
                }
            }
        }
    }

    // Aimbot / triggerbot activation: mouse buttons (reference binds) plus
    // keyboard fallbacks for testing (Z / X toggle).
    const AimbotConfig& aim = effective_aimbot(config_, local->weapon);
    const TriggerbotConfig& trig = effective_triggerbot(config_, local->weapon);

    // While a script drives the app, the script owns the active flags, but the
    // simulation still advances with neutral input so time and bots progress.
    if (script_mode_) {
        sim_.step(state, dt);
        return;
    }

    bool aim_key = input_.mouse[4] || input_.keys[GLFW_KEY_Z];
    bool trig_key = input_.mouse[3] || input_.keys[GLFW_KEY_X];

    if (aim.mode == KeyMode::Toggle) {
        static bool z_prev = false;
        if (input_.keys[GLFW_KEY_Z] && !z_prev) aimbot_toggle_state_ = !aimbot_toggle_state_;
        z_prev = input_.keys[GLFW_KEY_Z];
        aimbot_active_ = aimbot_toggle_state_;
    } else {
        aimbot_active_ = aim_key;
    }
    if (trig.mode == KeyMode::Toggle) {
        static bool x_prev = false;
        if (input_.keys[GLFW_KEY_X] && !x_prev) triggerbot_toggle_state_ = !triggerbot_toggle_state_;
        x_prev = input_.keys[GLFW_KEY_X];
        triggerbot_active_ = triggerbot_toggle_state_;
    } else {
        triggerbot_active_ = trig_key;
    }

    sim_.step(state, dt);

    // Recoil control runs after the step so it counteracts the fresh punch.
    if (config_.aim.global.rcs.enabled) {
        glm::vec2 rcs = compute_rcs_delta(sim_.world(), config_, true);
        if (local) local->view_angles += rcs * dt * 10.0f;
    }
}

void App::update_features(float dt) {
    Player* local = sim_.world().local_player();
    if (!local) return;

    target_ = find_target(sim_.world(), config_);

    if (aimbot_active_) {
        glm::vec2 step = compute_aim_step(sim_.world(), config_, target_, dt, target_.valid());
        local->view_angles += step;
        math::vec2_clamp(local->view_angles);
    }

    if (triggerbot_active_ && target_.valid()) {
        uint64_t now_ms = static_cast<uint64_t>(sim_.time() * 1000.0f);
        if (triggerbot_should_fire(sim_.world(), config_, target_, now_ms, true)) {
            sim_.fire_local();
            trigger_armed_ms_ = now_ms;
        }
    }

    // Flash decays over time so the flash check is meaningful.
    if (local->flash_alpha > 0.0f) {
        local->flash_alpha = std::max(0.0f, local->flash_alpha - dt * 0.5f);
        local->flashed = local->flash_alpha > 0.5f;
    }

    g_feed.publish(sim_.world());
}

void App::draw_frame() {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    draw_menu_bar();
    draw_scene();      // background draw list: 3D grid + ESP overlay
    draw_panels();
    draw_status_bar();

    ImGui::Render();
    int display_w = 0;
    int display_h = 0;
    glfwGetFramebufferSize(window_, &display_w, &display_h);
    glViewport(0, 0, display_w, display_h);
    glClearColor(0.05f, 0.06f, 0.08f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    glfwSwapBuffers(window_);
}

void App::run_one_frame(float dt) {
    handle_input(dt);
    update_features(dt);
    draw_frame();
    if (status_timer_ > 0.0f) status_timer_ -= dt;
    frame_count_++;
}

int App::run(int max_frames) {
    if (!window_) return 1;
    while (!glfwWindowShouldClose(window_)) {
        glfwPollEvents();
        double now = glfwGetTime();
        float dt = static_cast<float>(now - last_time_);
        last_time_ = now;
        dt = std::clamp(dt, 0.0005f, 0.1f);
        fps_ = fps_ * 0.9f + (1.0f / dt) * 0.1f;

        run_one_frame(dt);

        if (max_frames > 0 && frame_count_ >= max_frames) {
            break;
        }
        if (input_.keys[GLFW_KEY_ESCAPE] && !input_.cursor_captured) {
            break;
        }
    }
    return 0;
}

bool App::capture_screenshot(const std::string& path) {
    if (!window_) return false;
    int w = 0;
    int h = 0;
    glfwGetFramebufferSize(window_, &w, &h);
    if (w <= 0 || h <= 0) return false;
    std::vector<unsigned char> pixels(static_cast<size_t>(w) * h * 3);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadPixels(0, 0, w, h, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());

    // OpenGL origin is bottom-left; flip to top-left for the image.
    std::string ppm_path = path + ".ppm";
    FILE* file = std::fopen(ppm_path.c_str(), "wb");
    if (!file) return false;
    std::fprintf(file, "P6\n%d %d\n255\n", w, h);
    for (int y = h - 1; y >= 0; --y) {
        std::fwrite(pixels.data() + static_cast<size_t>(y) * w * 3, 1,
                    static_cast<size_t>(w) * 3, file);
    }
    std::fclose(file);
    // Convert to PNG when ImageMagick is available; otherwise keep the PPM.
    std::string cmd = "convert " + ppm_path + " " + path + " 2>/dev/null";
    int rc = std::system(cmd.c_str());
    if (rc == 0) {
        std::remove(ppm_path.c_str());
    }
    return true;
}

int App::run_script(const std::vector<std::string>& commands) {
    if (!window_) return static_cast<int>(commands.size());
    script_mode_ = true;
    int failures = 0;
    auto fail = [&](const std::string& why) {
        std::fprintf(stderr, "  [script FAIL] %s\n", why.c_str());
        failures++;
    };
    auto ok = [&](const std::string& what) {
        std::printf("  [script ok]   %s\n", what.c_str());
    };

    Player* local = sim_.world().local_player();
    float start_y = local ? local->position.y : 0.0f;

    for (const std::string& raw : commands) {
        std::string line = raw;
        // Strip comments and whitespace.
        auto hash = line.find('#');
        if (hash != std::string::npos) line = line.substr(0, hash);
        while (!line.empty() && std::isspace(static_cast<unsigned char>(line.front()))) line.erase(0, 1);
        while (!line.empty() && std::isspace(static_cast<unsigned char>(line.back()))) line.pop_back();
        if (line.empty()) continue;

        std::string cmd = line;
        std::string arg;
        auto space = line.find(' ');
        if (space != std::string::npos) {
            cmd = line.substr(0, space);
            arg = line.substr(space + 1);
        }

        local = sim_.world().local_player();

        if (cmd == "scenario") {
            auto world = make_scenario(arg);
            if (!world) {
                fail("unknown scenario " + arg);
            } else {
                sim_.set_world(std::move(*world));
                for (size_t i = 0; i < scenarios().size(); ++i) {
                    if (scenarios()[i].name == arg) scenario_index_ = static_cast<int>(i);
                }
                run_one_frame(1.0f / 128.0f);
                ok("scenario -> " + arg + " (" + std::to_string(sim_.world().players.size()) + " players)");
            }
        } else if (cmd == "frames") {
            int count = std::stoi(arg);
            for (int i = 0; i < count; ++i) run_one_frame(1.0f / 128.0f);
            ok("advanced " + arg + " frames (tick=" + std::to_string(sim_.tick()) + ")");
        } else if (cmd == "set") {
            std::string error;
            if (apply_config_override(config_, arg, &error)) {
                ok("set " + arg);
            } else {
                fail("set " + arg + ": " + error);
            }
        } else if (cmd == "hud") {
            bool value = arg.find("=on") != std::string::npos;
            std::string key = arg.substr(0, arg.find('='));
            if (key == "boxes") config_.hud.draw_boxes = value;
            else if (key == "skeleton") config_.hud.draw_skeleton = value;
            else if (key == "fov_circle") config_.hud.draw_fov_circle = value;
            else if (key == "health_bar") config_.hud.draw_health_bar = value;
            else if (key == "armor_bar") config_.hud.draw_armor_bar = value;
            else if (key == "names") config_.hud.draw_names = value;
            else if (key == "weapon") config_.hud.draw_weapon = value;
            else if (key == "bomb_timer") config_.hud.draw_bomb_timer = value;
            else if (key == "crosshair") config_.hud.draw_crosshair = value;
            else {
                fail("unknown hud key " + key);
                continue;
            }
            ok("hud " + arg);
        } else if (cmd == "select_weapon") {
            auto weapon = weapon_from_token(arg);
            if (!weapon || !local) {
                fail("select_weapon " + arg);
            } else {
                local->weapon = *weapon;
                if (auto stats = weapon_stats(*weapon)) {
                    local->ammo = stats->max_clip;
                    local->ammo_reserve = stats->max_clip * 3;
                }
                ok("weapon -> " + arg);
            }
        } else if (cmd == "aimbot") {
            aimbot_active_ = (arg == "on");
            ok("aimbot " + arg);
        } else if (cmd == "triggerbot") {
            triggerbot_active_ = (arg == "on");
            ok("triggerbot " + arg);
        } else if (cmd == "expect_lock") {
            bool want = arg == "on";
            bool have = target_.valid();
            if (want != have) fail("expect_lock " + arg + " (got " + (have ? "locked" : "none") + ")");
            else ok("lock state = " + arg);
        } else if (cmd == "expect_moved") {
            float dy = local ? local->position.y - start_y : 0.0f;
            if (dy <= 0.0f) fail("expect_moved (dy=" + std::to_string(dy) + ")");
            else ok("player moved dy=" + std::to_string(dy));
        } else if (cmd == "move") {
            float seconds = std::stof(arg);
            InputState state;
            state.move = {0.0f, 1.0f};
            int steps = static_cast<int>(seconds * 128.0f);
            for (int i = 0; i < steps; ++i) {
                sim_.step(state, 1.0f / 128.0f);
            }
            run_one_frame(1.0f / 128.0f);
            ok("moved forward for " + arg + "s");
        } else if (cmd == "expect_visible") {
            int index = std::stoi(arg);
            const Player* player = sim_.world().find_by_index(static_cast<uint32_t>(index));
            if (!player || !player->visible) fail("expect_visible " + arg);
            else ok("player " + arg + " visible");
        } else if (cmd == "expect_not_visible") {
            int index = std::stoi(arg);
            const Player* player = sim_.world().find_by_index(static_cast<uint32_t>(index));
            if (!player || player->visible) fail("expect_not_visible " + arg);
            else ok("player " + arg + " not visible");
        } else if (cmd == "fire") {
            ShotTrace trace = sim_.fire_local();
            ok(std::string("fire -> ") + (trace.hit_player ? "hit" : "miss"));
        } else if (cmd == "save_config") {
            std::string error;
            if (save_config(config_, default_config_path(), &error)) ok("save_config");
            else fail("save_config: " + error);
        } else if (cmd == "load_config") {
            std::string error;
            auto loaded = load_config(default_config_path(), &error);
            if (loaded) {
                config_ = *loaded;
                ok("load_config");
            } else {
                fail("load_config: " + error);
            }
        } else if (cmd == "expect_config_fov") {
            float want = std::stof(arg);
            if (std::fabs(config_.aim.global.aimbot.fov - want) > 0.01f) {
                fail("expect_config_fov " + arg + " (got " + std::to_string(config_.aim.global.aimbot.fov) + ")");
            } else {
                ok("config fov = " + arg);
            }
        } else if (cmd == "radar_start") {
            start_radar();
            if (radar_running_) ok("radar started on port " + std::to_string(radar_port_));
            else fail("radar_start");
        } else if (cmd == "radar_stop") {
            stop_radar();
            ok("radar stopped");
        } else if (cmd == "screenshot") {
            if (capture_screenshot(arg)) ok("screenshot -> " + arg);
            else fail("screenshot " + arg);
        } else {
            fail("unknown command: " + cmd);
        }
    }
    std::printf("script finished: %d failures\n", failures);
    return failures;
}

}  // namespace lvx::gui
