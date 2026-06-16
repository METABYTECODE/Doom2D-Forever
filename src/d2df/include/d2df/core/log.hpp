#pragma once

#include <spdlog/spdlog.h>

namespace d2df {

inline void init_logging() {
    spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] %v");
    spdlog::set_level(spdlog::level::debug);
}

} // namespace d2df
