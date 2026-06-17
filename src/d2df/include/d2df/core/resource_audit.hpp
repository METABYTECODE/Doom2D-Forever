#pragma once

#include <cstddef>
#include <string>

namespace d2df::core {

struct ResourceSnapshot {
    std::size_t textures_cached = 0;
    std::size_t sfx_chunks = 0;
    std::size_t music_tracks = 0;
    std::size_t projectiles = 0;
    std::size_t items = 0;
    std::size_t monsters = 0;
    std::size_t effects = 0;

    [[nodiscard]] bool has_runtime_allocations() const {
        return textures_cached > 0 || sfx_chunks > 0 || music_tracks > 0;
    }
};

void log_resource_snapshot(const ResourceSnapshot& snapshot, const std::string& context);

} // namespace d2df::core
