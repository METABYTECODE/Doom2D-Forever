#pragma once

namespace rivet::audio {

/// Placeholder audio backend interface.
class AudioSystem {
public:
    virtual ~AudioSystem() = default;
    virtual void update() {}
};

} // namespace rivet::audio
