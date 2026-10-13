#pragma once

#include <array>
#include <atomic>
#include <string>
#include <thread>
#include <vector>

#include "logvex/config.hpp"
#include "logvex/features.hpp"
#include "logvex/simulation.hpp"

struct GLFWwindow;

namespace lvx::gui {

// Keyboard/mouse state captured from GLFW. Mirrors the subset of CS2 input the
// reference reads through its input layer.
struct InputFrame {
    std::array<bool, 512> keys{};
    std::array<bool, 8> mouse{};
    glm::vec2 mouse_delta{0.0f};
    float scroll = 0.0f;
    bool cursor_captured = false;
};

// The interactive trainer window. Owns the GLFW window, the ImGui context and
// the deterministic simulation. Every control is wired to real state: HUD
// toggles change what the overlay draws, key binds drive the aimbot/triggerbot,
// scenario buttons rebuild the world, and config save/load touches disk.
class App {
public:
    App();
    ~App();

    // Returns false when the window/GL context could not be created.
    bool init(int width, int height, const std::string& title);

    // Runs the main loop for at most `max_frames` frames (0 = until closed).
    // Returns the process exit code.
    int run(int max_frames = 0);

    // Executes a list of scripted control commands (see scripts/gui_demo.txt).
    // Each command drives the exact same code paths the ImGui widgets use, so
    // the script verifies GUI behaviour deterministically without a human.
    // Returns the number of failed commands.
    int run_script(const std::vector<std::string>& commands);

    // Writes the current framebuffer to a PNG (via ImageMagick) or PPM file.
    bool capture_screenshot(const std::string& path);

    Config& config() { return config_; }
    Simulation& simulation() { return sim_; }

private:
    void handle_input(float dt);
    void update_features(float dt);
    void draw_frame();
    void draw_scene();
    void draw_overlay();
    void draw_panels();
    void draw_menu_bar();
    void draw_status_bar();
    void start_radar();
    void stop_radar();
    void save_config_now();
    void load_config_now();
    void set_status(const std::string& message);
    void run_one_frame(float dt);

    GLFWwindow* window_ = nullptr;
    InputFrame input_{};
    Config config_ = Config::defaults();
    Simulation sim_{1337};
    Target target_{};

    bool script_mode_ = false;  // disables key-driven state while a script runs
    bool aimbot_active_ = false;
    bool triggerbot_active_ = false;
    bool aimbot_toggle_state_ = false;
    bool triggerbot_toggle_state_ = false;
    uint64_t trigger_armed_ms_ = 0;
    int frame_count_ = 0;
    double last_time_ = 0.0;
    float fps_ = 0.0f;
    int selected_weapon_ = 0;
    int scenario_index_ = 0;
    std::string status_message_ = "ready";
    float status_timer_ = 0.0f;

    // Radar server runs on its own thread.
    std::thread radar_thread_;
    std::atomic<bool> radar_running_{false};
    std::atomic<bool> radar_stop_{false};
    uint16_t radar_port_ = 0;
};

}  // namespace lvx::gui
