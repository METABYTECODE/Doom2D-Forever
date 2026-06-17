#include <d2df/core/resource_audit.hpp>

#include <spdlog/spdlog.h>

namespace d2df::core {

void log_resource_snapshot(const ResourceSnapshot& snapshot, const std::string& context) {
  spdlog::info(
      "Resource audit [{}]: textures={} sfx_chunks={} music={} projectiles={} items={} "
      "monsters={} effects={}",
      context, snapshot.textures_cached, snapshot.sfx_chunks, snapshot.music_tracks,
      snapshot.projectiles, snapshot.items, snapshot.monsters, snapshot.effects);

  if (snapshot.has_runtime_allocations()) {
    spdlog::warn("Resource audit [{}]: cached GPU/audio assets still loaded (expected until "
                 "shutdown completes)",
                 context);
  }
}

} // namespace d2df::core
