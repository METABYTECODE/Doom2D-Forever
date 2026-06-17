#include <d2df/debug/debug_ui.hpp>

#include <SDL.h>
#include <imgui.h>
#include <imgui_impl_sdl2.h>
#include <imgui_impl_sdlrenderer2.h>
#include <spdlog/spdlog.h>

#include <d2df/debug/panels/collision_debug_panel.hpp>
#include <d2df/debug/panels/perf_debug_panel.hpp>
#include <d2df/debug/panels/tiles_debug_panel.hpp>

namespace d2df::debug {

DebugUi::DebugUi() {
    panels_.push_back(std::make_unique<PerfDebugPanel>());
    panels_.push_back(std::make_unique<CollisionDebugPanel>());
    panels_.push_back(std::make_unique<TilesDebugPanel>());
}

DebugUi::~DebugUi() {
    shutdown();
}

bool DebugUi::init(SDL_Window* window, SDL_Renderer* renderer) {
    if (initialized_) {
        return true;
    }

    window_ = window;
    renderer_ = renderer;

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    ImGui::StyleColorsDark();

    if (!ImGui_ImplSDL2_InitForSDLRenderer(window_, renderer_)) {
        spdlog::error("DebugUi: ImGui_ImplSDL2_InitForSDLRenderer failed");
        ImGui::DestroyContext();
        return false;
    }
    if (!ImGui_ImplSDLRenderer2_Init(renderer_)) {
        spdlog::error("DebugUi: ImGui_ImplSDLRenderer2_Init failed");
        ImGui_ImplSDL2_Shutdown();
        ImGui::DestroyContext();
        return false;
    }

    initialized_ = true;
    spdlog::info("DebugUi: ImGui ready (Insert toggles menu)");
    return true;
}

void DebugUi::shutdown() {
    if (!initialized_) {
        return;
    }

    ImGui_ImplSDLRenderer2_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    initialized_ = false;
    window_ = nullptr;
    renderer_ = nullptr;
}

void DebugUi::toggle_menu() {
    context_.menu_visible = !context_.menu_visible;
}

bool DebugUi::wants_capture_keyboard() const {
    if (!initialized_ || !context_.menu_visible) {
        return false;
    }
    return ImGui::GetIO().WantCaptureKeyboard;
}

bool DebugUi::wants_capture_mouse() const {
    if (!initialized_ || !context_.menu_visible) {
        return false;
    }
    return ImGui::GetIO().WantCaptureMouse;
}

void DebugUi::process_event(const SDL_Event& event) {
    if (!initialized_) {
        return;
    }
    ImGui_ImplSDL2_ProcessEvent(&event);
}

void DebugUi::begin_frame() {
    if (!initialized_) {
        return;
    }

    ImGui_ImplSDLRenderer2_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();
}

void DebugUi::draw_panels() {
    if (!initialized_ || !context_.menu_visible) {
        return;
    }

    ImGui::SetNextWindowSize(ImVec2(320.0f, 420.0f), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowPos(ImVec2(16.0f, 16.0f), ImGuiCond_FirstUseEver);

    if (ImGui::Begin("D2DF Debug", &context_.menu_visible, ImGuiWindowFlags_MenuBar)) {
        if (ImGui::BeginMenuBar()) {
            if (ImGui::BeginMenu("View")) {
                if (ImGui::MenuItem("Close", "Insert")) {
                    context_.menu_visible = false;
                }
                ImGui::EndMenu();
            }
            ImGui::EndMenuBar();
        }

        if (ImGui::BeginTabBar("DebugPanels")) {
            for (const auto& panel : panels_) {
                if (ImGui::BeginTabItem(panel->title())) {
                    panel->draw_ui(context_);
                    ImGui::EndTabItem();
                }
            }
            ImGui::EndTabBar();
        }
    }
    ImGui::End();
}

void DebugUi::draw_world(SDL_Renderer* renderer, const render::Camera2D& camera, int viewport_w,
                         int viewport_h, const DebugWorldView& world) {
    if (!context_.any_world_overlay()) {
        return;
    }

    for (const auto& panel : panels_) {
        panel->draw_world(renderer, camera, viewport_w, viewport_h, world, context_);
    }
}

void DebugUi::render() {
    if (!initialized_) {
        return;
    }

    draw_panels();
    ImGui::Render();
    ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(), renderer_);
}

} // namespace d2df::debug
