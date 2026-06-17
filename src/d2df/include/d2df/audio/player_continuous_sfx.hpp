#pragma once

#include <d2df/audio/sound_system.hpp>
#include <d2df/sim/player_state.hpp>
#include <d2df/sim/weapon_types.hpp>

namespace d2df::audio {

/// Stateful player sounds (chainsaw, flamethrower, jetpack).
/// Legacy uses loops=0 in SDL mixer and re-triggers samples from game logic.
class PlayerContinuousSfx {
public:
    void bind(SoundSystem* sound);

    void reset();

    void tick(const sim::PlayerState& player, const sim::PlayerCombat& combat, bool fire_held);

    void on_saw_fire(bool melee_hit);

private:
    static constexpr int kSawIdle = 0;
    static constexpr int kSawFire = 1;
    static constexpr int kSawHit = 2;
    static constexpr int kSawSelect = 3;
    static constexpr int kFlameOn = 4;
    static constexpr int kFlameWork = 5;
    static constexpr int kFlameOff = 6;
    static constexpr int kJetOn = 7;
    static constexpr int kJetFly = 8;
    static constexpr int kJetOff = 9;

    void stop_saw_sounds();
    void stop_flame_sounds();
    void stop_jet_sounds();
    void stop_all();

    void start_flame();
    void stop_flame();

    bool saw_channel_active(int channel) const;
    bool jet_channel_active(int channel) const;

    SoundSystem* sound_ = nullptr;
    sim::WeaponId last_weapon_ = sim::WeaponId::Pistol;
    bool flaming_ = false;
    bool jetpack_was_active_ = false;
};

} // namespace d2df::audio
