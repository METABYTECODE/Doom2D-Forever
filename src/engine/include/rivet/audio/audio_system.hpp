#pragma once

#include <filesystem>
#include <memory>

namespace rivet::audio {

/// SDL_mixer-backed audio service: music + one-shot SFX from filesystem paths.
class AudioSystem {
public:
    static constexpr int kLoopChannels = 8;
    static constexpr int kTotalChannels = 16;

    AudioSystem();
    ~AudioSystem();

    AudioSystem(const AudioSystem&) = delete;
    AudioSystem& operator=(const AudioSystem&) = delete;

    bool init();
    void shutdown();

    [[nodiscard]] bool enabled() const;

    void play_music(const std::filesystem::path& path, int loops = -1);
    void stop_music();

    void play_sfx(const std::filesystem::path& path, int volume = -1);

    void set_music_volume(float normalized);
    void set_sfx_volume(float normalized);

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace rivet::audio
