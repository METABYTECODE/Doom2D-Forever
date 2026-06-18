#pragma once

struct SDL_Renderer;
struct SDL_Texture;
struct SDL_Window;
union SDL_Event;

#include <filesystem>
#include <functional>
#include <optional>
#include <span>
#include <unordered_map>

#include <rivet/render/camera2d.hpp>
#include <rivet/render/irenderer.hpp>
#include <rivet/resources/texture.hpp>

namespace rivet::render {

/// SDL_Renderer backend. Uses SDL only as a renderer implementation, not as platform.
class SdlRenderer final : public IRenderer {
public:
    explicit SdlRenderer(SDL_Window* window);
    ~SdlRenderer() override;

    SdlRenderer(const SdlRenderer&) = delete;
    SdlRenderer& operator=(const SdlRenderer&) = delete;

    [[nodiscard]] std::string_view backend_name() const override { return "sdl"; }
    [[nodiscard]] SDL_Renderer* native_renderer() const { return renderer_; }

    void begin_frame() override;
    void end_frame() override;
    void clear(const Color& color) override;
    void draw_filled_rect(const Rect& rect, const Color& color) override;

    void set_camera(const Camera2D& camera) override;
    [[nodiscard]] const Camera2D& camera() const override { return camera_; }

    [[nodiscard]] resources::TextureHandle load_texture(const std::filesystem::path& path) override;
    [[nodiscard]] resources::TextureHandle create_texture_rgba(
        std::span<const std::uint8_t> pixels,
        int width,
        int height) override;
    void unload_texture(resources::TextureHandle handle) override;
    void draw_texture(
        resources::TextureHandle handle,
        const Rect& dest,
        const Rect& source = {}) override;
    [[nodiscard]] std::optional<resources::TextureInfo> texture_info(
        resources::TextureHandle handle) const override;

private:
    [[nodiscard]] Rect apply_camera(const Rect& world_rect) const;

    SDL_Renderer* renderer_ = nullptr;
    Camera2D camera_;
    resources::TextureHandle next_texture_handle_ = 1;
    std::unordered_map<resources::TextureHandle, SDL_Texture*> textures_;
    std::unordered_map<resources::TextureHandle, resources::TextureInfo> texture_info_;
};

} // namespace rivet::render
