#pragma once

#include <filesystem>
#include <string>
#include <unordered_map>

struct Mix_Chunk;
struct Mix_Music;

namespace d2df::resources {
class AssetDatabase;
}

namespace d2df::audio {

class SoundSystem {
public:
    SoundSystem();
    ~SoundSystem();

    SoundSystem(const SoundSystem&) = delete;
    SoundSystem& operator=(const SoundSystem&) = delete;

    bool init();
    void shutdown();

    void set_content_root(std::filesystem::path content_root);
    void bind_assets(const resources::AssetDatabase& assets);

    void play(const char* asset_id);
    void play_music(const char* asset_id, int loops = -1);
    void stop_music();
    [[nodiscard]] bool enabled() const { return enabled_; }

private:
    Mix_Chunk* load_chunk(const char* asset_id);
    Mix_Music* load_music(const char* asset_id);

    std::filesystem::path content_root_;
    const resources::AssetDatabase* assets_ = nullptr;
    std::unordered_map<std::string, Mix_Chunk*> chunks_;
    std::unordered_map<std::string, Mix_Music*> music_;
    std::string active_music_id_;
    bool enabled_ = false;
};

} // namespace d2df::audio
