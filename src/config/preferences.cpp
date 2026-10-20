// Copyright (C) 1997, 1999-2001, 2008 Nathan Lamont
// Copyright (C) 2008-2017 The Antares Authors
//
// This file is part of Antares, a tactical space combat game.
//
// Antares is free software: you can redistribute it and/or modify it
// under the terms of the Lesser GNU General Public License as published
// by the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Antares is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with Antares.  If not, see http://www.gnu.org/licenses/

#include "config/preferences.hpp"

#include <algorithm>
#include <sfz/sfz.hpp>

#include "config/dirs.hpp"
#include "config/keys.hpp"
#include "data/resource.hpp"
#include "game/globals.hpp"
#include "game/sys.hpp"
#include "lang/defines.hpp"

using sfz::range;
using std::max;
using std::min;
using std::unique_ptr;

namespace antares {

template <typename T>
static T clamp(T value, T lower, T upper) {
    return min(upper, max(lower, value));
}

Preferences::Preferences() {
    controls_preset = ControlPreset::CLASSIC;
    apply_preset(controls_preset, bindings);

    play_idle_music    = true;
    play_music_in_game = false;
    speech_on          = false;

    volume = 7;

    fullscreen  = false;
    window_size = {640, 480};
    ui_scale    = 0;

    enhanced_graphics = true;
}

int migrate_unreachable_defaults(Preferences* p) {
    static const struct {
        int key_num;
        Key old_default;
    } kMoved[] = {
            {kSelectFriendKeyNum, Key::N_CLEAR},
            {kSelectFoeKeyNum, Key::N_EQUALS},
            {kScaleHostileKeyNum, Key::HELP},
    };
    ActionBindings classic[ACTION_COUNT];
    apply_preset(ControlPreset::CLASSIC, classic);
    int moved = 0;
    for (const auto& m : kMoved) {
        Binding& primary = p->bindings[m.key_num].slot[kPrimarySlot];
        if (primary != Binding::key(m.old_default)) {
            continue;
        }
        const Binding new_key = classic[m.key_num].slot[kPrimarySlot];
        bool          taken   = false;
        for (int i : antares::actions_for(p->bindings, new_key)) {
            taken = taken || (i != m.key_num);
        }
        if (!taken) {
            primary = new_key;
            ++moved;
        }
    }
    return moved;
}

Key Preferences::key(int action) const {
    if ((action < 0) || (action >= ACTION_COUNT)) {
        return Key::NONE;
    }
    for (int s : {kPrimarySlot, kSecondarySlot}) {
        const Binding& b = bindings[action].slot[s];
        if (b.device == Binding::Device::KEY) {
            return Key(b.code);
        }
    }
    return Key::NONE;
}

bool Preferences::bindings_match_preset() const {
    ActionBindings preset[ACTION_COUNT];
    apply_preset(controls_preset, preset);
    for (int a = 0; a < ACTION_COUNT; ++a) {
        for (int s = 0; s < kBindingSlots; ++s) {
            if (bindings[a].slot[s] != preset[a].slot[s]) {
                return false;
            }
        }
    }
    return true;
}

Preferences Preferences::copy() const {
    Preferences copy;
    for (int a = 0; a < ACTION_COUNT; ++a) {
        copy.bindings[a] = bindings[a];
    }
    copy.controls_preset    = controls_preset;
    copy.play_idle_music    = play_idle_music;
    copy.play_music_in_game = play_music_in_game;
    copy.speech_on          = speech_on;
    copy.volume             = volume;
    copy.fullscreen         = fullscreen;
    copy.window_size        = window_size;
    copy.ui_scale           = ui_scale;
    copy.enhanced_graphics  = enhanced_graphics;
    return copy;
}

PrefsDriver::PrefsDriver() {
    if (sys.prefs) {
        throw std::runtime_error("PrefsDriver is a singleton");
    }
    sys.prefs = this;
}

PrefsDriver::~PrefsDriver() { sys.prefs = NULL; }

void PrefsDriver::set_key(size_t index, Key key) {
    set_binding(index, kPrimarySlot, Binding::key(key));
}

void PrefsDriver::set_binding(int action, BindingSlot slot, const Binding& b) {
    Preferences p(get().copy());
    p.bindings[action].slot[slot] = b;
    set(p);
}

void PrefsDriver::set_controls_preset(ControlPreset preset) {
    Preferences p(get().copy());
    p.controls_preset = preset;
    apply_preset(preset, p.bindings);
    set(p);
}

void PrefsDriver::set_play_idle_music(bool on) {
    Preferences p(get().copy());
    p.play_idle_music = on;
    set(p);
}

void PrefsDriver::set_play_music_in_game(bool on) {
    Preferences p(get().copy());
    p.play_music_in_game = on;
    set(p);
}

void PrefsDriver::set_speech_on(bool on) {
    Preferences p(get().copy());
    p.speech_on = on;
    set(p);
}

void PrefsDriver::set_volume(int volume) {
    Preferences p(get().copy());
    p.volume = volume;
    set(p);
}

void PrefsDriver::set_fullscreen(bool on) {
    Preferences p(get().copy());
    p.fullscreen = on;
    set(p);
}

void PrefsDriver::set_window_size(Size size) {
    Preferences p(get().copy());
    p.window_size = size;
    set(p);
}

void PrefsDriver::set_ui_scale(int scale) {
    Preferences p(get().copy());
    p.ui_scale = clamp(scale, 0, 4);
    set(p);
}

void PrefsDriver::set_enhanced_graphics(bool on) {
    Preferences p(get().copy());
    p.enhanced_graphics = on;
    set(p);
}

NullPrefsDriver::NullPrefsDriver() {}

NullPrefsDriver::NullPrefsDriver(Preferences defaults) : _saved(defaults.copy()) {}

const Preferences& NullPrefsDriver::get() const { return _saved; }

void NullPrefsDriver::set(const Preferences& prefs) { _saved = prefs.copy(); }

}  // namespace antares
