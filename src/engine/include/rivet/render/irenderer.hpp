#pragma once

#include <cstdint>
#include <filesystem>
#include <optional>
#include <span>
#include <string_view>

#include <rivet/render/camera2d.hpp>
#include <rivet/render/color.hpp>
#include <rivet/render/rect.hpp>
#include <rivet/resources/texture.hpp>

namespace rivet::render {

/// Backend-agnostic render API. SDL / Vulkan / OpenGL implementations register via services.
class IRenderer {
public:
    virtual ~IRenderer() = default;

    [[nodiscard]] virtual std::string_view backend_name() const = 0;

    virtual void begin_frame() = 0;
    virtual void end_frame() = 0;
    virtual void clear(const Color& color) = 0;
    virtual void draw_filled_rect(const Rect& rect, const Color& color) = 0;

    virtual void set_camera(const Camera2D& camera) = 0;
    [[nodiscard]] virtual const Camera2D& camera() const = 0;

    [[nodiscard]] virtual resources::TextureHandle load_texture(const std::filesystem::path& path) = 0;
    [[nodiscard]] virtual resources::TextureHandle create_texture_rgba(
        std::span<const std::uint8_t> pixels,
        int width,
        int height) = 0;
    virtual void unload_texture(resources::TextureHandle handle) = 0;
    virtual void draw_texture(
        resources::TextureHandle handle,
        const Rect& dest,
        const Rect& source = {},
        bool flip_horizontal = false) = 0;
    [[nodiscard]] virtual std::optional<resources::TextureInfo> texture_info(
        resources::TextureHandle handle) const = 0;
    virtual void render(float interpolation_alpha) { (void)interpolation_alpha; }
};

} // namespace rivet::render
