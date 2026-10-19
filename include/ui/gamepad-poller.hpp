// Copyright (C) 2026 The Star Bound Authors
// This file is part of Star Bound, based on Antares. Distributed under the LGPL+. See COPYING.

#ifndef ANTARES_UI_GAMEPAD_POLLER_HPP_
#define ANTARES_UI_GAMEPAD_POLLER_HPP_

#include <memory>
#include <vector>

#include "config/gamepad.hpp"
#include "math/units.hpp"
#include "ui/event.hpp"

namespace antares {

// Turns polled gamepad states into the gamepad events the game already handles (the same ones the
// macOS driver sends), for drivers that poll instead of receiving events (GLFW on Windows).
//
// The state uses the standard layout, in GLFW's order (GLFW_GAMEPAD_BUTTON_*, GLFW_GAMEPAD_AXIS_*):
// buttons A, B, X, Y, LB, RB, Back, Start, Guide, left stick, right stick, D-pad up, right, down,
// left; axes left x, left y, right x, right y (-1..1, y down), left trigger, right trigger (-1 at
// rest, 1 fully pressed).
//
// Events are sent only on change: buttons when pressed or released; triggers as the LT/RT buttons
// (pressed above 75 %, released below 40 %); sticks as a direction quantised to 32 steps, or
// centred, with a dead zone that a stick has to leave (radius 0.6) and re-enter (0.45). A stick is
// sent as the game's gamepad code reads it (PlayerShip::gamepad_stick adds 180 degrees): the
// opposite of its direction on screen, so pushing up sends (0, +1) and right sends (-1, 0).
class GamepadPoller {
  public:
    static constexpr int kButtons = 15;
    static constexpr int kAxes    = 6;

    struct State {
        bool  buttons[kButtons] = {};
        float axes[kAxes]       = {0, 0, 0, 0, -1, -1};
    };

    // Events for the change from the last state; `connected` false releases everything held.
    std::vector<std::unique_ptr<Event>> update(const State& state, bool connected, wall_time now);

  private:
    struct Stick {
        bool active    = false;
        int  direction = 0;  // 0..31 steps of the sent vector's angle
    };
    void stick(
            Gamepad::Stick which, float x, float y, Stick* s, wall_time now,
            std::vector<std::unique_ptr<Event>>* out);

    bool  _buttons[kButtons] = {};
    bool  _triggers[2]       = {};
    Stick _sticks[2];
};

}  // namespace antares

#endif  // ANTARES_UI_GAMEPAD_POLLER_HPP_
