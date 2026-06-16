#include <d2df/audio/sound_system.hpp>

#include <SDL_mixer.h>
#include <spdlog/spdlog.h>

#include <d2df/resources/asset_database.hpp>

namespace d2df::audio {

SoundSystem::SoundSystem() = default;

SoundSystem::~SoundSystem() {
    shutdown();
}

bool SoundSystem::init() {
    if (enabled_) {
        return true;
    }
    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) != 0) {
        spdlog::warn("SoundSystem: Mix_OpenAudio failed: {}", Mix_GetError());
        return false;
    }
    Mix_AllocateChannels(16);
    enabled_ = true;
    spdlog::info("SoundSystem: SDL_mixer initialized");
    return true;
}

void SoundSystem::shutdown() {
    stop_music();
    for (auto& [id, chunk] : chunks_) {
        (void)id;
        if (chunk != nullptr) {
            Mix_FreeChunk(chunk);
        }
    }
    chunks_.clear();
    for (auto& [id, track] : music_) {
        (void)id;
        if (track != nullptr) {
            Mix_FreeMusic(track);
        }
    }
    music_.clear();
    if (enabled_) {
        Mix_CloseAudio();
        enabled_ = false;
    }
}

void SoundSystem::set_content_root(std::filesystem::path content_root) {
    content_root_ = std::move(content_root);
}

void SoundSystem::bind_assets(const resources::AssetDatabase& assets) {
    assets_ = &assets;
}

Mix_Chunk* SoundSystem::load_chunk(const char* asset_id) {
    if (!enabled_ || assets_ == nullptr || asset_id == nullptr) {
        return nullptr;
    }

    const std::string key(asset_id);
    const auto existing = chunks_.find(key);
    if (existing != chunks_.end()) {
        return existing->second;
    }

    const auto* entry = assets_->get(asset_id);
    if (entry == nullptr) {
        return nullptr;
    }

    const auto path = content_root_ / entry->path;
    Mix_Chunk* chunk = Mix_LoadWAV(path.string().c_str());
    if (chunk == nullptr) {
        spdlog::debug("SoundSystem: failed to load {} ({})", asset_id, Mix_GetError());
        return nullptr;
    }

    chunks_.emplace(key, chunk);
    return chunk;
}

Mix_Music* SoundSystem::load_music(const char* asset_id) {
    if (!enabled_ || assets_ == nullptr || asset_id == nullptr) {
        return nullptr;
    }

    const std::string key(asset_id);
    const auto existing = music_.find(key);
    if (existing != music_.end()) {
        return existing->second;
    }

    const auto* entry = assets_->get(asset_id);
    if (entry == nullptr) {
        spdlog::debug("SoundSystem: music asset not found: {}", asset_id);
        return nullptr;
    }

    const auto path = content_root_ / entry->path;
    Mix_Music* track = Mix_LoadMUS(path.string().c_str());
    if (track == nullptr) {
        spdlog::debug("SoundSystem: failed to load music {} ({})", asset_id, Mix_GetError());
        return nullptr;
    }

    music_.emplace(key, track);
    return track;
}

void SoundSystem::play(const char* asset_id) {
    Mix_Chunk* chunk = load_chunk(asset_id);
    if (chunk == nullptr) {
        return;
    }
    Mix_PlayChannel(-1, chunk, 0);
}

void SoundSystem::play_music(const char* asset_id, int loops) {
    if (!enabled_ || asset_id == nullptr || asset_id[0] == '\0') {
        return;
    }

    if (active_music_id_ == asset_id && Mix_PlayingMusic() != 0) {
        return;
    }

    Mix_Music* track = load_music(asset_id);
    if (track == nullptr) {
        return;
    }

    if (Mix_PlayMusic(track, loops) != 0) {
        spdlog::debug("SoundSystem: Mix_PlayMusic failed for {} ({})", asset_id, Mix_GetError());
        return;
    }

    active_music_id_ = asset_id;
    spdlog::info("SoundSystem: playing music {}", asset_id);
}

void SoundSystem::stop_music() {
    if (!enabled_) {
        active_music_id_.clear();
        return;
    }
    Mix_HaltMusic();
    active_music_id_.clear();
}

} // namespace d2df::audio
