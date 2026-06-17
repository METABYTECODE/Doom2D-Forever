#include <d2df/debug/panels/perf_debug_panel.hpp>

#include <imgui.h>

#include <d2df/core/perf_stats.hpp>

namespace d2df::debug {

void PerfDebugPanel::draw_ui(DebugContext& context) {
    const auto& stats = core::PerfStats::instance();

    ImGui::Checkbox("FPS overlay", &context.perf.show_overlay);
    ImGui::Separator();

    ImGui::Text("FPS: %.1f", stats.fps());
    ImGui::Text("Frame: %.2f ms", stats.frame_ms());
    ImGui::Text("Sim steps/frame: %u", stats.sim_steps_last_frame());

    ImGui::Separator();
    ImGui::TextUnformatted("Region avg / max (ms, 120 frames)");

    if (ImGui::BeginTable("PerfRegions", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
        ImGui::TableSetupColumn("Region");
        ImGui::TableSetupColumn("Avg");
        ImGui::TableSetupColumn("Max");
        ImGui::TableHeadersRow();

        for (std::size_t i = 0; i < static_cast<std::size_t>(core::PerfRegion::Count); ++i) {
            const auto region = static_cast<core::PerfRegion>(i);
            if (region == core::PerfRegion::Frame) {
                continue;
            }
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::TextUnformatted(core::perf_region_name(region));
            ImGui::TableNextColumn();
            ImGui::Text("%.2f", stats.region_avg_ms(region));
            ImGui::TableNextColumn();
            ImGui::Text("%.2f", stats.region_max_ms(region));
        }
        ImGui::EndTable();
    }

    ImGui::Separator();
    ImGui::Text("Textures: %zu", context.perf.textures_cached);
    ImGui::Text("SFX chunks: %zu", context.perf.sfx_chunks);
    ImGui::Text("Music tracks: %zu", context.perf.music_tracks);
    ImGui::Text("Projectiles: %zu", context.perf.projectiles);
    ImGui::Text("Items: %zu", context.perf.items);
    ImGui::Text("Monsters: %zu", context.perf.monsters);
    ImGui::Text("Effects: %zu", context.perf.effects);
}

void PerfDebugPanel::draw_world(SDL_Renderer* renderer, const render::Camera2D& camera,
                                int viewport_w, int viewport_h, const DebugWorldView& world,
                                const DebugContext& context) const {
    (void)renderer;
    (void)camera;
    (void)viewport_w;
    (void)viewport_h;
    (void)world;
    (void)context;
}

} // namespace d2df::debug
