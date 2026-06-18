#pragma once

#include <spdlog/spdlog.h>

namespace rivet::core {

inline void init_logging() {
    spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] %v");
    spdlog::set_level(spdlog::level::debug);
}

} // namespace rivet::core
