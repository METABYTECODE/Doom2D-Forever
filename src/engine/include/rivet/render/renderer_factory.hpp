#pragma once

#include <memory>

#include <rivet/render/irenderer.hpp>
#include <rivet/render/renderer_backend.hpp>

struct SDL_Window;

namespace rivet::render {

struct RendererCreateInfo {
    SDL_Window* window = nullptr;
};

[[nodiscard]] std::shared_ptr<IRenderer> create_renderer(
    RendererBackend backend,
    const RendererCreateInfo& create_info);

} // namespace rivet::render
