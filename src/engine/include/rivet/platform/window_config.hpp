#pragma once

#include <string>

namespace rivet::platform {

struct WindowConfig {
    std::string title = "D2DF Engine";
    int width = 800;
    int height = 600;
    bool resizable = true;
};

} // namespace rivet::platform
