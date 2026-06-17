#include <d2df/audio/player_continuous_sfx.hpp>

namespace d2df::audio {

void PlayerContinuousSfx::bind(SoundSystem* sound) {
    sound_ = sound;
}

void PlayerContinuousSfx::reset() {
    stop_all();
    last_weapon_ = sim::WeaponId::Pistol;
    flaming_ = false;
    jetpack_was_active_ = false;
}

void PlayerContinuousSfx::stop_saw_sounds() {
    if (sound_ == nullptr) {
        return;
    }
    sound_->stop_channel(kSawIdle);
    sound_->stop_channel(kSawFire);
    sound_->stop_channel(kSawHit);
    sound_->stop_channel(kSawSelect);
}

void PlayerContinuousSfx::stop_flame_sounds() {
    if (sound_ == nullptr) {
        return;
    }
    sound_->stop_channel(kFlameOn);
    sound_->stop_channel(kFlameWork);
    sound_->stop_channel(kFlameOff);
}

void PlayerContinuousSfx::stop_jet_sounds() {
    if (sound_ == nullptr) {
        return;
    }
    sound_->stop_channel(kJetOn);
    sound_->stop_channel(kJetFly);
    sound_->stop_channel(kJetOff);
}

void PlayerContinuousSfx::stop_all() {
    stop_saw_sounds();
    stop_flame_sounds();
    stop_jet_sounds();
}

bool PlayerContinuousSfx::saw_channel_active(int channel) const {
    return sound_ != nullptr && sound_->is_channel_playing(channel);
}

bool PlayerContinuousSfx::jet_channel_active(int channel) const {
    return sound_ != nullptr && sound_->is_channel_playing(channel);
}

void PlayerContinuousSfx::start_flame() {
    if (sound_ == nullptr) {
        flaming_ = true;
        return;
    }

    sound_->stop_channel(kFlameOff);

    if (!flaming_) {
        sound_->stop_channel(kFlameWork);
        sound_->play_channel(kFlameOn, "sfx.world.startflm", 0);
        flaming_ = true;
        return;
    }

    // Legacy FlamerOn: while already flaming, (re)start work loop when start/work are idle.
    if (!sound_->is_channel_playing(kFlameOn) && !sound_->is_channel_playing(kFlameWork)) {
        sound_->play_channel(kFlameWork, "sfx.world.workflm", 0);
    }
}

void PlayerContinuousSfx::stop_flame() {
    if (!flaming_) {
        return;
    }

    flaming_ = false;
    if (sound_ == nullptr) {
        return;
    }

    sound_->stop_channel(kFlameOn);
    sound_->stop_channel(kFlameWork);
    sound_->play_channel(kFlameOff, "sfx.world.stopflm", 0);
}

void PlayerContinuousSfx::on_saw_fire(bool melee_hit) {
    if (sound_ == nullptr) {
        return;
    }

    sound_->stop_channel(kSawSelect);
    sound_->stop_channel(kSawIdle);

    if (melee_hit) {
        sound_->play("sfx.world.hitsaw");
        return;
    }

    sound_->play("sfx.world.firesaw");
}

void PlayerContinuousSfx::tick(const sim::PlayerState& player, const sim::PlayerCombat& combat,
                               bool fire_held) {
    if (sound_ == nullptr || !sound_->enabled()) {
        return;
    }

    if (!player.alive()) {
        stop_all();
        flaming_ = false;
        jetpack_was_active_ = false;
        last_weapon_ = combat.current_weapon;
        return;
    }

    const auto weapon = combat.current_weapon;
    if (weapon != last_weapon_) {
        if (weapon == sim::WeaponId::Saw) {
            stop_saw_sounds();
            sound_->play_channel(kSawSelect, "sfx.world.selectsaw", 0);
        } else {
            stop_saw_sounds();
        }

        if (last_weapon_ == sim::WeaponId::Flamethrower) {
            stop_flame();
        }
        last_weapon_ = weapon;
    }

    const bool wants_flame =
        fire_held && weapon == sim::WeaponId::Flamethrower && combat.has_ammo_for_weapon(weapon);
    if (wants_flame) {
        start_flame();
    } else if (flaming_) {
        stop_flame();
    }

    if (weapon == sim::WeaponId::Saw) {
        if (!saw_channel_active(kSawSelect)) {
            if (!saw_channel_active(kSawIdle)) {
                sound_->play_channel(kSawIdle, "sfx.world.idlesaw", 0);
            }
        }
    } else {
        stop_saw_sounds();
    }

    const bool jet_active = player.jetpack_active();
    if (jet_active && !jetpack_was_active_) {
        sound_->stop_channel(kJetFly);
        sound_->stop_channel(kJetOff);
        sound_->play_channel(kJetOn, "sfx.world.startjetpack", 0);
    } else if (!jet_active && jetpack_was_active_) {
        sound_->stop_channel(kJetOn);
        sound_->stop_channel(kJetFly);
        sound_->play_channel(kJetOff, "sfx.world.stopjetpack", 0);
    }

    if (jet_active) {
        if (!jet_channel_active(kJetOn) && !jet_channel_active(kJetOff) &&
            !jet_channel_active(kJetFly)) {
            sound_->play_channel(kJetFly, "sfx.world.workjetpack", 0);
        }
    } else if (!jet_channel_active(kJetOff)) {
        sound_->stop_channel(kJetOn);
        sound_->stop_channel(kJetFly);
    }

    jetpack_was_active_ = jet_active;
}

} // namespace d2df::audio
