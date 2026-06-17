#include <d2df/debug/panels/collision_debug_panel.hpp>

#include <imgui.h>

#include <d2df/debug/debug_draw.hpp>
#include <d2df/sim/projectile_system.hpp>

namespace d2df::debug {

void CollisionDebugPanel::draw_ui(DebugContext& context) {
    ImGui::TextUnformatted("Hitbox overlays");
    ImGui::Separator();
    ImGui::Checkbox("Player", &context.collision.player);
    ImGui::Checkbox("Monsters", &context.collision.monsters);
    ImGui::Checkbox("Projectiles", &context.collision.projectiles);
}

void CollisionDebugPanel::draw_world(SDL_Renderer* renderer, const render::Camera2D& camera,
                                     int viewport_w, int viewport_h, const DebugWorldView& world,
                                     const DebugContext& context) const {
    if (context.collision.player) {
        const float alpha = world.render_alpha;
        const float x = world.player.render_x(alpha);
        const float y = world.player.render_y(alpha);
        draw_world_rect_outline(renderer, camera, viewport_w, viewport_h, x, y, world.player.width,
                                world.player.height, 64, 220, 96, 230);
    }

    if (context.collision.monsters) {
        for (const auto& target : world.targets) {
            if (!target.alive() && !target.is_corpse()) {
                continue;
            }

            const float alpha = world.render_alpha;
            const float x = target.is_corpse() ? target.x : target.render_x(alpha);
            const float y = target.is_corpse() ? target.y : target.render_y(alpha);
            draw_world_rect_outline(renderer, camera, viewport_w, viewport_h, x, y, target.width,
                                    target.height, 220, 96, 64, 220);
        }
    }

    if (context.collision.projectiles) {
        for (const auto& projectile : world.projectiles.projectiles()) {
            if (!projectile.active) {
                continue;
            }
            draw_world_rect_outline(renderer, camera, viewport_w, viewport_h, projectile.x,
                                    projectile.y, projectile.width, projectile.height, 220, 220,
                                    64, 200);
        }
    }
}

} // namespace d2df::debug
