// Copyright (C) 2026 The Star Bound Authors
// This file is part of Star Bound, based on Antares. Distributed under the LGPL+. See COPYING.

#ifndef ANTARES_CONFIG_BINDINGS_HPP_
#define ANTARES_CONFIG_BINDINGS_HPP_

#include <pn/string>
#include <pn/value>
#include <sfz/sfz.hpp>
#include <utility>
#include <vector>

#include "config/gamepad.hpp"
#include "config/keys.hpp"

namespace antares {

// One input that can trigger an action: a key, a mouse button, a wheel direction or a gamepad
// button.
struct Binding {
    enum class Device { NONE, KEY, MOUSE, WHEEL, GAMEPAD };

    Device device = Device::NONE;
    int    code   = 0;  // KEY: USB key code; MOUSE: button index (0 left, 1 right, 2 middle, ...);
                        // WHEEL: +1 up (away from the player), -1 down; GAMEPAD: Gamepad::Button.

    static Binding none() { return Binding{}; }
    static Binding key(Key k);
    static Binding mouse(int button);
    static Binding wheel(int direction);
    static Binding gamepad(Gamepad::Button button);

    bool empty() const { return device == Device::NONE; }

    // Inputs the game keeps for itself: Esc, Return and Caps Lock (pause, menus, text entry); the
    // left and right mouse buttons (select and target); the gamepad's bumpers (select and target
    // modes) and Start (the Play Again menu). They can't be bound.
    bool reserved() const;

    // For display, e.g. "KEYPAD 8", "MOUSE 3", "WHEEL UP", "RT". Uses the game's key and gamepad
    // names, so it needs the data strings loaded.
    pn::string name() const;
};

inline bool operator==(const Binding& a, const Binding& b) {
    return (a.device == b.device) && (a.empty() || (a.code == b.code));
}
inline bool operator!=(const Binding& a, const Binding& b) { return !(a == b); }

// Each action has two keyboard-and-mouse bindings (primary, secondary: a key, a mouse button or a
// wheel direction) and one gamepad binding.
enum BindingSlot { kPrimarySlot = 0, kSecondarySlot = 1, kGamepadSlot = 2, kBindingSlots = 3 };

struct ActionBindings {
    Binding slot[kBindingSlots];
};

// Whether a binding may go in a slot: keyboard, mouse and wheel in primary and secondary, gamepad
// buttons in the gamepad slot; never a reserved input.
bool slot_accepts(BindingSlot slot, const Binding& b);

// The built-in sets of bindings:
//  * Classic: Ares' layout (with the PC keys of 2026-10-20).
//  * Modern (Ares-style), the default Modern layout: the arrow keys steer (the PC equivalent of
//    Ares' keypad 8-4-5-6); every fire and action key is under the left hand.
//  * Modern (WASD): flies on W A S D, leaving the right hand on the mouse.
enum class ControlPreset { CLASSIC, MODERN_ARES, MODERN_WASD };
const ControlPreset kControlPresets[] = {
        ControlPreset::CLASSIC, ControlPreset::MODERN_ARES, ControlPreset::MODERN_WASD};

// Config ids "classic", "modern-ares", "modern-wasd". preset_with_id() also reads "modern", the
// one Modern preset of 2026-10-20, which had the WASD layout.
pn::string_view              preset_id(ControlPreset p);
pn::string_view              preset_label(ControlPreset p);  // "CLASSIC", "MODERN: ARES", ...
sfz::optional<ControlPreset> preset_with_id(pn::string_view id);
void apply_preset(ControlPreset p, ActionBindings out[ACTION_COUNT]);

// Two actions clash when they share an input, except that the weapon actions (fire 1, fire 2,
// special) may share one: Ares' gamepad fires both weapons with RT. Returns each clashing pair
// once, lower action first.
bool                             may_share(int a, int b);
std::vector<std::pair<int, int>> find_clashes(const ActionBindings bindings[ACTION_COUNT]);

// The actions an input triggers, in action order.
std::vector<int> actions_for(const ActionBindings bindings[ACTION_COUNT], const Binding& b);

// The actions in groups, as the Key Controls screen shows them, a page each: Ship, Command,
// Shortcuts, Utility, Hot Keys (the original 44, as Ares grouped them) and Modern (the added ones).
struct ActionGroup {
    const char* label;
    int         begin, end;  // actions [begin, end)
};
extern const ActionGroup kActionGroups[6];
const int                kMaxGroupSize = 11;  // rows on a Key Controls page

// The config file's name for each action ("ship accel", ... "attack move", "patrol").
pn::string_view action_config_name(int action);

// Config file form: null, {key: 96}, {mouse: 2}, {wheel: "up"}, or a gamepad token ("rt").
pn::value binding_to_value(const Binding& b);
Binding   binding_from_value(pn::value_cref x);

}  // namespace antares

#endif  // ANTARES_CONFIG_BINDINGS_HPP_
