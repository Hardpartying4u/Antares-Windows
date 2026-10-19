// Copyright (C) 2026 The Star Bound Authors
// This file is part of Star Bound, based on Antares. Distributed under the LGPL+. See COPYING.

#include "ui/gamepad-poller.hpp"

#include <cmath>

namespace antares {

namespace {

// Standard-layout buttons, in GLFW order, as the game's buttons (Guide has no use in the game).
const Gamepad::Button kButtonMap[GamepadPoller::kButtons] = {
        Gamepad::Button::A,     Gamepad::Button::B,     Gamepad::Button::X,
        Gamepad::Button::Y,     Gamepad::Button::LB,    Gamepad::Button::RB,
        Gamepad::Button::BACK,  Gamepad::Button::START, Gamepad::Button::NONE,
        Gamepad::Button::LSB,   Gamepad::Button::RSB,   Gamepad::Button::UP,
        Gamepad::Button::RIGHT, Gamepad::Button::DOWN,  Gamepad::Button::LEFT,
};

const float kStickOn       = 0.60f;
const float kStickOff      = 0.45f;
const float kTriggerOn     = 0.75f;  // of the trigger's travel
const float kTriggerOff    = 0.40f;
const int   kStickSteps    = 32;
const float kPi            = 3.14159265358979f;

}  // namespace

void GamepadPoller::stick(
        Gamepad::Stick which, float x, float y, Stick* s, wall_time now,
        std::vector<std::unique_ptr<Event>>* out) {
    const float r      = std::sqrt(x * x + y * y);
    const bool  active = s->active ? (r >= kStickOff) : (r >= kStickOn);
    int         dir    = s->direction;
    if (active) {
        // The game's gamepad code (PlayerShip::gamepad_stick) adds 180 degrees to the vector it
        // gets, so it wants the opposite of the stick's direction on screen (x right, y down).
        float a = std::atan2(-y, -x);
        if (a < 0) {
            a += 2 * kPi;
        }
        dir = static_cast<int>(std::lround(a / (2 * kPi) * kStickSteps)) % kStickSteps;
    }
    if ((active == s->active) && (!active || (dir == s->direction))) {
        return;
    }
    s->active    = active;
    s->direction = dir;
    if (active) {
        const float a = dir * 2 * kPi / kStickSteps;
        out->emplace_back(new GamepadStickEvent(now, which, std::cos(a), std::sin(a)));
    } else {
        out->emplace_back(new GamepadStickEvent(now, which, 0.0, 0.0));
    }
}

std::vector<std::unique_ptr<Event>> GamepadPoller::update(
        const State& state, bool connected, wall_time now) {
    State s = connected ? state : State{};
    std::vector<std::unique_ptr<Event>> out;
    for (int i = 0; i < kButtons; ++i) {
        if ((s.buttons[i] == _buttons[i]) || (kButtonMap[i] == Gamepad::Button::NONE)) {
            _buttons[i] = s.buttons[i];
            continue;
        }
        _buttons[i] = s.buttons[i];
        if (s.buttons[i]) {
            out.emplace_back(new GamepadButtonDownEvent(now, kButtonMap[i]));
        } else {
            out.emplace_back(new GamepadButtonUpEvent(now, kButtonMap[i]));
        }
    }
    const Gamepad::Button kTriggerButtons[2] = {Gamepad::Button::LT, Gamepad::Button::RT};
    for (int i = 0; i < 2; ++i) {
        const float travel = (s.axes[4 + i] + 1) / 2;
        const bool  down   = _triggers[i] ? (travel > kTriggerOff) : (travel > kTriggerOn);
        if (down != _triggers[i]) {
            _triggers[i] = down;
            if (down) {
                out.emplace_back(new GamepadButtonDownEvent(now, kTriggerButtons[i]));
            } else {
                out.emplace_back(new GamepadButtonUpEvent(now, kTriggerButtons[i]));
            }
        }
    }
    stick(Gamepad::Stick::LS, s.axes[0], s.axes[1], &_sticks[0], now, &out);
    stick(Gamepad::Stick::RS, s.axes[2], s.axes[3], &_sticks[1], now, &out);
    return out;
}

}  // namespace antares
