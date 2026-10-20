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

#ifndef ANTARES_UI_SCREENS_OPTIONS_HPP_
#define ANTARES_UI_SCREENS_OPTIONS_HPP_

#include <utility>
#include <vector>

#include "config/preferences.hpp"
#include "math/units.hpp"
#include "ui/screen.hpp"

namespace antares {

class OptionsScreen : public Card {
  public:
    enum State {
        SOUND_CONTROL,
        VIDEO_CONTROL,
        KEY_CONTROL,
        ACCEPT,
        CANCEL,
    };

    OptionsScreen();
    virtual void become_front();

  private:
    State       _state;
    Preferences _revert;
};

class SoundControlScreen : public InterfaceScreen {
  public:
    SoundControlScreen(OptionsScreen::State* state);
    ~SoundControlScreen();

    virtual void overlay() const;

  private:
    OptionsScreen::State* const _state;
};

class VideoControlScreen : public InterfaceScreen {
  public:
    VideoControlScreen(OptionsScreen::State* state);
    ~VideoControlScreen();

    virtual void overlay() const;

  private:
    OptionsScreen::State* const _state;
};

// Options > Key Controls: every action's bindings (primary, secondary, gamepad), a page at a time,
// with the preset choice, clash checking, Clear and Reset to Defaults.
class KeyControlScreen : public InterfaceScreen {
  public:
    KeyControlScreen(OptionsScreen::State* state);
    ~KeyControlScreen();

    virtual void key_down(const KeyDownEvent& event);
    virtual void key_up(const KeyUpEvent& event);
    virtual void mouse_down(const MouseDownEvent& event);
    virtual void mouse_wheel(const MouseWheelEvent& event);
    virtual void gamepad_button_down(const GamepadButtonDownEvent& event);
    virtual void gamepad_button_up(const GamepadButtonUpEvent& event);

    virtual bool next_timer(wall_time& time);
    virtual void fire_timer();

    virtual void overlay() const;

  private:
    pn::string_view action_name(int action) const;

    void rebuild();
    void request_rebuild();
    void bindings_changed();
    void assign(const Binding& b);
    void adjust_interface();
    void update_clashes();

    OptionsScreen::State* const _state;

    int  _page            = 0;
    int  _selected        = -1;  // action of the selected binding, or -1
    int  _selected_slot   = 0;
    bool _capturing       = false;  // waiting for an input for the selected binding
    bool _rebuild_pending = false;

    std::vector<std::pair<int, int>> _clashes;
    wall_time                        _next_flash;
    bool                             _flashed_on = false;

    std::vector<pn::string> _action_names;
};

}  // namespace antares

#endif  // ANTARES_UI_SCREENS_OPTIONS_HPP_
