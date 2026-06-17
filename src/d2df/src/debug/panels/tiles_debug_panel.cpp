#include <d2df/debug/panels/tiles_debug_panel.hpp>

#include <imgui.h>

#include <d2df/debug/debug_draw.hpp>
#include <d2df/map/panel_types.hpp>

namespace d2df::debug {
namespace {

bool panel_matches(const map::MapPanel& panel, const DebugContext::TileFlags& flags) {
    if ((panel.flags & map::PANEL_FLAG_HIDE) != 0) {
        return false;
    }

    const auto type = panel.type;
    if (flags.solid &&
        ((type & map::PANEL_WALL) != 0 || (type & map::PANEL_CLOSEDOOR) != 0 ||
         (type & map::PANEL_BLOCKMON) != 0)) {
        if ((type & (map::PANEL_CLOSEDOOR | map::PANEL_OPENDOOR)) != 0 && !panel.enabled) {
            return true;
        }
        if ((type & map::PANEL_WALL) != 0 || (type & map::PANEL_BLOCKMON) != 0) {
            return true;
        }
        if ((type & map::PANEL_CLOSEDOOR) != 0 && panel.enabled) {
            return true;
        }
    }

    if (flags.steps && (type & map::PANEL_STEP) != 0) {
        return true;
    }
    if (flags.water && (type & map::PANEL_WATER) != 0) {
        return true;
    }
    if (flags.acid && ((type & map::PANEL_ACID1) != 0 || (type & map::PANEL_ACID2) != 0)) {
        return true;
    }
    if (flags.doors && ((type & map::PANEL_CLOSEDOOR) != 0 || (type & map::PANEL_OPENDOOR) != 0)) {
        return true;
    }
    if (flags.lifts &&
        ((type & map::PANEL_LIFTUP) != 0 || (type & map::PANEL_LIFTDOWN) != 0 ||
         (type & map::PANEL_LIFTLEFT) != 0 || (type & map::PANEL_LIFTRIGHT) != 0)) {
        return true;
    }

    return false;
}

void draw_panel_overlay(SDL_Renderer* renderer, const render::Camera2D& camera, int viewport_w,
                        int viewport_h, const map::MapPanel& panel) {
    const auto type = panel.type;
    std::uint8_t r = 180;
    std::uint8_t g = 180;
    std::uint8_t b = 180;

    if ((type & map::PANEL_WALL) != 0 || (type & map::PANEL_BLOCKMON) != 0) {
        r = 220;
        g = 64;
        b = 64;
    } else if ((type & map::PANEL_STEP) != 0) {
        r = 220;
        g = 180;
        b = 64;
    } else if ((type & map::PANEL_WATER) != 0) {
        r = 64;
        g = 140;
        b = 220;
    } else if ((type & map::PANEL_ACID1) != 0 || (type & map::PANEL_ACID2) != 0) {
        r = 64;
        g = 220;
        b = 64;
    } else if ((type & map::PANEL_CLOSEDOOR) != 0 || (type & map::PANEL_OPENDOOR) != 0) {
        r = 180;
        g = 64;
        b = 220;
    } else if ((type & map::PANEL_LIFTUP) != 0 || (type & map::PANEL_LIFTDOWN) != 0 ||
               (type & map::PANEL_LIFTLEFT) != 0 || (type & map::PANEL_LIFTRIGHT) != 0) {
        r = 220;
        g = 140;
        b = 64;
    }

    const float x = static_cast<float>(panel.position.x);
    const float y = static_cast<float>(panel.position.y);
    const float w = static_cast<float>(panel.size.width);
    const float h = static_cast<float>(panel.size.height);
    draw_world_rect_fill(renderer, camera, viewport_w, viewport_h, x, y, w, h, r, g, b, 48);
}

} // namespace

void TilesDebugPanel::draw_ui(DebugContext& context) {
    ImGui::TextUnformatted("Panel collision layers");
    ImGui::Separator();
    ImGui::Checkbox("Solid (walls, blockmon, closed doors)", &context.tiles.solid);
    ImGui::Checkbox("Steps", &context.tiles.steps);
    ImGui::Checkbox("Water", &context.tiles.water);
    ImGui::Checkbox("Acid", &context.tiles.acid);
    ImGui::Checkbox("Doors", &context.tiles.doors);
    ImGui::Checkbox("Lifts", &context.tiles.lifts);
}

void TilesDebugPanel::draw_world(SDL_Renderer* renderer, const render::Camera2D& camera,
                                 int viewport_w, int viewport_h, const DebugWorldView& world,
                                 const DebugContext& context) const {
    const auto& panels = world.collision.panels();
    for (const auto& panel : panels) {
        if (!panel_matches(panel, context.tiles)) {
            continue;
        }
        draw_panel_overlay(renderer, camera, viewport_w, viewport_h, panel);
    }
}

} // namespace d2df::debug
