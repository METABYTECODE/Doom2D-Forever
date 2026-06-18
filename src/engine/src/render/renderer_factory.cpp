#include <stdexcept>
#include <string>

#include <rivet/render/renderer_factory.hpp>
#include <rivet/render/sdl_renderer.hpp>

namespace rivet::render {

std::shared_ptr<IRenderer> create_renderer(
    const RendererBackend backend,
    const RendererCreateInfo& create_info) {
    if (!is_renderer_backend_available(backend)) {
        throw std::runtime_error(
            std::string("Renderer backend is not available: ") + std::string(to_string(backend)));
    }

    switch (backend) {
    case RendererBackend::Sdl:
        if (create_info.window == nullptr) {
            throw std::runtime_error("SDL renderer requires a valid platform window");
        }
        return std::make_shared<SdlRenderer>(create_info.window);
    case RendererBackend::OpenGl:
        break;
    }

    throw std::runtime_error(
        std::string("Renderer backend is not available: ") + std::string(to_string(backend)));
}

} // namespace rivet::render
