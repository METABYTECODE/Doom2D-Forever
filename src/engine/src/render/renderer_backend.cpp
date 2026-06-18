#include <rivet/render/renderer_backend.hpp>

namespace rivet::render {

std::string_view to_string(const RendererBackend backend) {
    switch (backend) {
    case RendererBackend::Sdl:
        return "sdl";
    case RendererBackend::OpenGl:
        return "opengl";
    }
    return "sdl";
}

RendererBackend parse_renderer_backend(const std::string_view value) {
    if (value == "opengl" || value == "gl") {
        return RendererBackend::OpenGl;
    }
    return RendererBackend::Sdl;
}

bool is_renderer_backend_available(const RendererBackend backend) {
    switch (backend) {
    case RendererBackend::Sdl:
        return true;
    case RendererBackend::OpenGl:
        return false;
    }
    return false;
}

} // namespace rivet::render
