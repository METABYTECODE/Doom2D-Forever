#pragma once

#include <string_view>

namespace rivet::render {

enum class RendererBackend {
    Sdl,
    OpenGl,
};

[[nodiscard]] std::string_view to_string(RendererBackend backend);
[[nodiscard]] RendererBackend parse_renderer_backend(std::string_view value);
[[nodiscard]] bool is_renderer_backend_available(RendererBackend backend);

} // namespace rivet::render
