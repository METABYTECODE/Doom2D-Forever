#include <rivet/audio/audio_system.hpp>

#include <SDL_mixer.h>
#include <SDL.h>
#include <spdlog/spdlog.h>

#include <algorithm>
#include <filesystem>
#include <string>
#include <unordered_map>

namespace rivet::audio {

namespace {

[[nodiscard]] int clamp_volume(const float normalized) {
    return std::clamp(static_cast<int>(normalized * MIX_MAX_VOLUME), 0, MIX_MAX_VOLUME);
}

[[nodiscard]] std::string path_key(const std::filesystem::path& path) {
    return path.lexically_normal().generic_string();
}

} // namespace

struct AudioSystem::Impl {
    std::unordered_map<std::string, Mix_Chunk*> chunks;
    std::unordered_map<std::string, Mix_Music*> music;
    std::string active_music_path;
    bool enabled = false;
    int music_volume = MIX_MAX_VOLUME;
    int sfx_volume = MIX_MAX_VOLUME;

    Mix_Chunk* load_chunk(const std::filesystem::path& path) {
        if (!enabled) {
            return nullptr;
        }

        const std::string key = path_key(path);
        if (const auto it = chunks.find(key); it != chunks.end()) {
            return it->second;
        }

        Mix_Chunk* chunk = Mix_LoadWAV(path.string().c_str());
        if (chunk == nullptr) {
            spdlog::debug("AudioSystem: failed to load sfx {} ({})", path.string(), Mix_GetError());
            return nullptr;
        }

        chunks.emplace(key, chunk);
        return chunk;
    }

    Mix_Music* load_music(const std::filesystem::path& path) {
        if (!enabled) {
            return nullptr;
        }

        const std::string key = path_key(path);
        if (const auto it = music.find(key); it != music.end()) {
            return it->second;
        }

        Mix_Music* track = Mix_LoadMUS(path.string().c_str());
        if (track == nullptr) {
            spdlog::debug("AudioSystem: failed to load music {} ({})", path.string(), Mix_GetError());
            return nullptr;
        }

        music.emplace(key, track);
        return track;
    }

    void clear() {
        stop_music();
        for (auto& [id, chunk] : chunks) {
            (void)id;
            if (chunk != nullptr) {
                Mix_FreeChunk(chunk);
            }
        }
        chunks.clear();
        for (auto& [id, track] : music) {
            (void)id;
            if (track != nullptr) {
                Mix_FreeMusic(track);
            }
        }
        music.clear();
    }

    void stop_music() {
        if (!enabled) {
            active_music_path.clear();
            return;
        }
        Mix_HaltMusic();
        active_music_path.clear();
    }
};

AudioSystem::AudioSystem() : impl_(std::make_unique<Impl>()) {}

AudioSystem::~AudioSystem() {
    shutdown();
}

bool AudioSystem::init() {
    if (impl_->enabled) {
        return true;
    }

    if (SDL_InitSubSystem(SDL_INIT_AUDIO) != 0) {
        spdlog::warn("AudioSystem: SDL_INIT_AUDIO failed: {}", SDL_GetError());
        return false;
    }

    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) != 0) {
        spdlog::warn("AudioSystem: Mix_OpenAudio failed: {}", Mix_GetError());
        return false;
    }

    Mix_AllocateChannels(kTotalChannels);
    Mix_VolumeMusic(impl_->music_volume);
    impl_->enabled = true;
    spdlog::info("AudioSystem: SDL_mixer ready ({} channels)", kTotalChannels);
    return true;
}

void AudioSystem::shutdown() {
    if (!impl_) {
        return;
    }

    impl_->clear();
    if (impl_->enabled) {
        Mix_CloseAudio();
        SDL_QuitSubSystem(SDL_INIT_AUDIO);
        impl_->enabled = false;
    }
}

bool AudioSystem::enabled() const {
    return impl_ && impl_->enabled;
}

void AudioSystem::play_music(const std::filesystem::path& path, const int loops) {
    if (!enabled() || path.empty()) {
        return;
    }

    const std::string key = path_key(path);
    if (impl_->active_music_path == key && Mix_PlayingMusic() != 0) {
        return;
    }

    Mix_Music* track = impl_->load_music(path);
    if (track == nullptr) {
        return;
    }

    if (Mix_PlayMusic(track, loops) != 0) {
        spdlog::warn("AudioSystem: Mix_PlayMusic failed for {} ({})", path.string(), Mix_GetError());
        return;
    }

    impl_->active_music_path = key;
    spdlog::info("AudioSystem: playing music {}", path.string());
}

void AudioSystem::stop_music() {
    if (impl_) {
        impl_->stop_music();
    }
}

void AudioSystem::play_sfx(const std::filesystem::path& path, int volume) {
    if (!enabled() || path.empty()) {
        return;
    }

    Mix_Chunk* chunk = impl_->load_chunk(path);
    if (chunk == nullptr) {
        return;
    }

    const int vol = volume < 0 ? impl_->sfx_volume : std::clamp(volume, 0, MIX_MAX_VOLUME);
    Mix_VolumeChunk(chunk, vol);

    for (int channel = kLoopChannels; channel < kTotalChannels; ++channel) {
        if (Mix_Playing(channel) == 0) {
            Mix_PlayChannel(channel, chunk, 0);
            return;
        }
    }

    Mix_PlayChannel(kLoopChannels, chunk, 0);
}

void AudioSystem::set_music_volume(const float normalized) {
    if (!impl_) {
        return;
    }
    impl_->music_volume = clamp_volume(normalized);
    if (impl_->enabled) {
        Mix_VolumeMusic(impl_->music_volume);
    }
}

void AudioSystem::set_sfx_volume(const float normalized) {
    if (!impl_) {
        return;
    }
    impl_->sfx_volume = clamp_volume(normalized);
}

} // namespace rivet::audio
