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

#ifndef ANTARES_CONFIG_PREFERENCES_HPP_
#define ANTARES_CONFIG_PREFERENCES_HPP_

#include <pn/string>

#include "config/bindings.hpp"
#include "config/keys.hpp"
#include "math/geometry.hpp"

namespace antares {

struct Preferences {
    Preferences();
    Preferences copy() const;

    // What triggers each action (two keyboard-and-mouse bindings and a gamepad button), and the
    // preset they were last set from.
    ActionBindings bindings[ACTION_COUNT];
    ControlPreset  controls_preset;
    bool    play_idle_music;
    bool    play_music_in_game;
    bool    speech_on;
    int16_t volume;
    bool    fullscreen;
    Size    window_size;
    int16_t ui_scale;  // 0: automatic; 1-4: fixed integer UI scale.
    bool    enhanced_graphics;  // true: use the art folder (Enhanced); false: Original.

    // The action's first key binding (primary, else secondary), or Key::NONE. Text that names an
    // action's key (tutorials, labels, the minicomputer) uses this.
    Key key(int action) const;

    // Whether the bindings are exactly those of the selected preset.
    bool bindings_match_preset() const;
};

// Saved configs from before 2026-10-20 store Ares' defaults for Select friendly (Keypad Clear),
// Select hostile (Keypad =) and Zoom to nearest hostile (Help), keys PC keyboards don't have.
// Moves each primary binding still on that old default to the Classic default, unless another
// action already uses the new key. Other bindings are left as saved. Returns how many were moved.
int migrate_unreachable_defaults(Preferences* p);

class PrefsDriver {
  public:
    PrefsDriver();
    virtual ~PrefsDriver();

    virtual const Preferences& get() const                   = 0;
    virtual void               set(const Preferences& prefs) = 0;

    Key            key(size_t index) const { return get().key(index); }
    const Binding& binding(int action, BindingSlot slot) const {
        return get().bindings[action].slot[slot];
    }
    std::vector<int> actions_for(const Binding& b) const {
        return antares::actions_for(get().bindings, b);
    }
    ControlPreset controls_preset() const { return get().controls_preset; }
    bool play_idle_music() const { return get().play_idle_music; }
    bool play_music_in_game() const { return get().play_music_in_game; }
    bool speech_on() const { return get().speech_on; }
    int  volume() const { return get().volume; }
    int  fullscreen() const { return get().fullscreen; }
    Size window_size() const { return get().window_size; }
    int  ui_scale() const { return get().ui_scale; }
    bool enhanced_graphics() const { return get().enhanced_graphics; }

    void set_key(size_t index, Key key);  // as the primary binding
    void set_binding(int action, BindingSlot slot, const Binding& b);
    void set_controls_preset(ControlPreset p);  // selects the preset and applies its bindings
    void set_play_idle_music(bool on);
    void set_play_music_in_game(bool on);
    void set_speech_on(bool on);
    void set_volume(int volume);
    void set_fullscreen(bool on);
    void set_window_size(Size size);
    void set_ui_scale(int scale);
    void set_enhanced_graphics(bool on);

    static PrefsDriver* driver();
};

class NullPrefsDriver : public PrefsDriver {
  public:
    NullPrefsDriver();
    NullPrefsDriver(Preferences defaults);

    virtual const Preferences& get() const;
    virtual void               set(const Preferences& prefs);

  private:
    Preferences _saved;
};

}  // namespace antares

#endif  // ANTARES_CONFIG_PREFERENCES_HPP_
