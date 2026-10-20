// Copyright (C) 2026 The Star Bound Authors
// This file is part of Star Bound, based on Antares. Distributed under the LGPL+. See COPYING.

#include "config/bindings.hpp"

#include <pn/map>

namespace antares {

namespace {

using Pad = Gamepad::Button;

// Config tokens for gamepad buttons. Fixed here rather than taken from the data strings, so a
// config reads the same whatever data is installed.
const struct {
    Pad         button;
    const char* token;
} kPadTokens[] = {
        {Pad::A, "a"},        {Pad::B, "b"},         {Pad::X, "x"},        {Pad::Y, "y"},
        {Pad::LB, "lb"},      {Pad::RB, "rb"},       {Pad::LSB, "ls"},     {Pad::RSB, "rs"},
        {Pad::START, "start"}, {Pad::BACK, "back"},  {Pad::VENDOR, "guide"}, {Pad::UP, "up"},
        {Pad::DOWN, "down"},  {Pad::LEFT, "left"},   {Pad::RIGHT, "right"}, {Pad::LT, "lt"},
        {Pad::RT, "rt"},
};

const char* const kActionConfigNames[ACTION_COUNT] = {
        "ship accel",      "ship decel",     "ship ccw",    "ship cw",      "fire 1",
        "fire 2",          "fire s",         "warp",

        "select friendly", "select hostile", "select base", "target",       "order",
        "zoom in",         "zoom out",

        "comp up",         "comp down",      "comp accept", "comp cancel",

        "transfer",        "zoom 1:1",       "zoom 1:2",    "zoom 1:4",     "zoom 1:16",
        "zoom hostile",    "zoom object",    "zoom all",    "next message", "help",
        "volume down",     "volume up",      "game music",  "net settings", "fast motion",

        "hotkey 1",        "hotkey 2",       "hotkey 3",    "hotkey 4",     "hotkey 5",
        "hotkey 6",        "hotkey 7",       "hotkey 8",    "hotkey 9",     "hotkey 10",

        "attack move",     "patrol",
};

// The presets, as data: for each action its primary, secondary and gamepad binding.
struct PresetRow {
    int     action;
    Binding primary, secondary, gamepad;
};

Binding K(Key::Value k) { return Binding::key(Key(k)); }
Binding P(Pad b) { return Binding::gamepad(b); }
const Binding kNone      = Binding::none();
const Binding kWheelUp   = Binding::wheel(+1);
const Binding kWheelDown = Binding::wheel(-1);

// Ares' layout, with the PC keys chosen on 2026-10-20 for the three it had on keys PC keyboards
// lack, the wheel zoom of 2026-10-20 and the Ares gamepad mapping. Attack move and patrol are
// unbound: they belong to the Modern mode. The fire keys are X and Z, just above where Ares had
// them (Option and Command, Left Alt and the Windows key on a PC): Left Alt with Tab (warp) opens
// Windows' app switcher and the Windows key opens the Start menu, both taking the game out of focus.
const PresetRow kClassic[] = {
        {kUpKeyNum, K(Key::N8), kNone, P(Pad::A)},
        {kDownKeyNum, K(Key::N5), kNone, P(Pad::B)},
        {kLeftKeyNum, K(Key::N4), kNone, kNone},
        {kRightKeyNum, K(Key::N6), kNone, kNone},
        {kOneKeyNum, K(Key::X), kNone, P(Pad::RT)},
        {kTwoKeyNum, K(Key::Z), kNone, P(Pad::RT)},
        {kEnterKeyNum, K(Key::SPACE), kNone, P(Pad::LT)},
        {kWarpKeyNum, K(Key::TAB), kNone, P(Pad::LSB)},
        {kSelectFriendKeyNum, K(Key::N_PERIOD), kNone, kNone},
        {kSelectFoeKeyNum, K(Key::N_TIMES), kNone, kNone},
        {kSelectBaseKeyNum, K(Key::N_DIVIDE), kNone, kNone},
        {kDestinationKeyNum, K(Key::SHIFT), kNone, kNone},
        {kOrderKeyNum, K(Key::CONTROL), kNone, kNone},
        {kZoomInKeyNum, K(Key::N_PLUS), kWheelUp, P(Pad::Y)},
        {kZoomOutKeyNum, K(Key::N_MINUS), kWheelDown, P(Pad::X)},
        {kCompUpKeyNum, K(Key::UP_ARROW), kNone, P(Pad::UP)},
        {kCompDownKeyNum, K(Key::DOWN_ARROW), kNone, P(Pad::DOWN)},
        {kCompAcceptKeyNum, K(Key::RIGHT_ARROW), kNone, P(Pad::RIGHT)},
        {kCompCancelKeyNum, K(Key::LEFT_ARROW), kNone, P(Pad::LEFT)},
        {kTransferKeyNum, K(Key::F8), kNone, kNone},
        {kScale121KeyNum, K(Key::F9), kNone, kNone},
        {kScale122KeyNum, K(Key::F10), kNone, kNone},
        {kScale124KeyNum, K(Key::F11), kNone, kNone},
        {kScale1216KeyNum, K(Key::F12), kNone, kNone},
        {kScaleHostileKeyNum, K(Key::END), kNone, kNone},
        {kScaleObjectKeyNum, K(Key::HOME), kNone, kNone},
        {kScaleAllKeyNum, K(Key::PAGE_UP), kNone, kNone},
        {kMessageNextKeyNum, K(Key::BACKSPACE), kNone, P(Pad::BACK)},
        {kHelpKeyNum, K(Key::F1), kNone, kNone},
        {kVolumeDownKeyNum, K(Key::F2), kNone, kNone},
        {kVolumeUpKeyNum, K(Key::F3), kNone, kNone},
        {kActionMusicKeyNum, K(Key::F4), kNone, kNone},
        {kNetSettingsKeyNum, K(Key::F5), kNone, kNone},
        {kFastMotionKeyNum, K(Key::F6), kNone, kNone},
        {kHotKey1Num, K(Key::K1), kNone, kNone},
        {kHotKey2Num, K(Key::K2), kNone, kNone},
        {kHotKey3Num, K(Key::K3), kNone, kNone},
        {kHotKey4Num, K(Key::K4), kNone, kNone},
        {kHotKey5Num, K(Key::K5), kNone, kNone},
        {kHotKey6Num, K(Key::K6), kNone, kNone},
        {kHotKey7Num, K(Key::K7), kNone, kNone},
        {kHotKey8Num, K(Key::K8), kNone, kNone},
        {kHotKey9Num, K(Key::K9), kNone, kNone},
        {kHotKey10Num, K(Key::K0), kNone, kNone},
        {kAttackMoveKeyNum, kNone, kNone, kNone},
        {kPatrolKeyNum, kNone, kNone, kNone},
};

// Modern (Ares-style): the right hand steers on the arrow keys, like Ares' keypad 8-4-5-6; everything
// else is under the left hand (fingers on A S D F, thumb on Space). Fire on Space, F and D; select
// on Q, E and R; order on S; the minicomputer on the column T, G, B with V as cancel (mirroring the
// arrows); attack move A (then click), patrol W. Ctrl, Alt and the Windows key are left free: Ctrl
// is the Modern group and attack-move modifier, and Alt+Tab, Alt+Enter and the Windows key belong
// to Windows. The zoom shortcuts stay on the keys above the arrows and the function keys.
const PresetRow kModernAres[] = {
        {kUpKeyNum, K(Key::UP_ARROW), kNone, P(Pad::A)},
        {kDownKeyNum, K(Key::DOWN_ARROW), kNone, P(Pad::B)},
        {kLeftKeyNum, K(Key::LEFT_ARROW), kNone, kNone},
        {kRightKeyNum, K(Key::RIGHT_ARROW), kNone, kNone},
        {kOneKeyNum, K(Key::SPACE), kNone, P(Pad::RT)},
        {kTwoKeyNum, K(Key::F), kNone, P(Pad::RT)},
        {kEnterKeyNum, K(Key::D), kNone, P(Pad::LT)},
        {kWarpKeyNum, K(Key::TAB), kNone, P(Pad::LSB)},
        {kSelectFriendKeyNum, K(Key::Q), kNone, kNone},
        {kSelectFoeKeyNum, K(Key::E), kNone, kNone},
        {kSelectBaseKeyNum, K(Key::R), kNone, kNone},
        {kDestinationKeyNum, K(Key::SHIFT), kNone, kNone},
        {kOrderKeyNum, K(Key::S), kNone, kNone},
        {kZoomInKeyNum, K(Key::X), kWheelUp, P(Pad::Y)},
        {kZoomOutKeyNum, K(Key::Z), kWheelDown, P(Pad::X)},
        {kCompUpKeyNum, K(Key::T), kNone, P(Pad::UP)},
        {kCompDownKeyNum, K(Key::G), kNone, P(Pad::DOWN)},
        {kCompAcceptKeyNum, K(Key::B), kNone, P(Pad::RIGHT)},
        {kCompCancelKeyNum, K(Key::V), kNone, P(Pad::LEFT)},
        {kTransferKeyNum, K(Key::C), kNone, kNone},
        {kScale121KeyNum, K(Key::F9), kNone, kNone},
        {kScale122KeyNum, K(Key::F10), kNone, kNone},
        {kScale124KeyNum, K(Key::F11), kNone, kNone},
        {kScale1216KeyNum, K(Key::F12), kNone, kNone},
        {kScaleHostileKeyNum, K(Key::END), kNone, kNone},
        {kScaleObjectKeyNum, K(Key::HOME), kNone, kNone},
        {kScaleAllKeyNum, K(Key::PAGE_UP), kNone, kNone},
        {kMessageNextKeyNum, K(Key::BACKTICK), kNone, P(Pad::BACK)},
        {kHelpKeyNum, K(Key::F1), kNone, kNone},
        {kVolumeDownKeyNum, K(Key::F2), kNone, kNone},
        {kVolumeUpKeyNum, K(Key::F3), kNone, kNone},
        {kActionMusicKeyNum, K(Key::F4), kNone, kNone},
        {kNetSettingsKeyNum, K(Key::F5), kNone, kNone},
        {kFastMotionKeyNum, K(Key::F6), kNone, kNone},
        {kHotKey1Num, K(Key::K1), kNone, kNone},
        {kHotKey2Num, K(Key::K2), kNone, kNone},
        {kHotKey3Num, K(Key::K3), kNone, kNone},
        {kHotKey4Num, K(Key::K4), kNone, kNone},
        {kHotKey5Num, K(Key::K5), kNone, kNone},
        {kHotKey6Num, K(Key::K6), kNone, kNone},
        {kHotKey7Num, K(Key::K7), kNone, kNone},
        {kHotKey8Num, K(Key::K8), kNone, kNone},
        {kHotKey9Num, K(Key::K9), kNone, kNone},
        {kHotKey10Num, K(Key::K0), kNone, kNone},
        {kAttackMoveKeyNum, K(Key::A), kNone, kNone},
        {kPatrolKeyNum, K(Key::W), kNone, kNone},
};

// Modern (WASD): flight on WASD, weapons and selection around it, zoom on the main row and the
// wheel; the rest as Classic. Attack move on G (Ctrl+right-click will also give it), patrol on T,
// so the mouse hand never needs the keyboard for them.
const PresetRow kModernWasd[] = {
        {kUpKeyNum, K(Key::W), kNone, P(Pad::A)},
        {kDownKeyNum, K(Key::S), kNone, P(Pad::B)},
        {kLeftKeyNum, K(Key::A), kNone, kNone},
        {kRightKeyNum, K(Key::D), kNone, kNone},
        {kOneKeyNum, K(Key::SPACE), kNone, P(Pad::RT)},
        {kTwoKeyNum, K(Key::F), kNone, P(Pad::RT)},
        {kEnterKeyNum, K(Key::R), kNone, P(Pad::LT)},
        {kWarpKeyNum, K(Key::TAB), kNone, P(Pad::LSB)},
        {kSelectFriendKeyNum, K(Key::Q), kNone, kNone},
        {kSelectFoeKeyNum, K(Key::E), kNone, kNone},
        {kSelectBaseKeyNum, K(Key::B), kNone, kNone},
        {kDestinationKeyNum, K(Key::SHIFT), kNone, kNone},
        {kOrderKeyNum, K(Key::CONTROL), kNone, kNone},
        {kZoomInKeyNum, K(Key::EQUALS), kWheelUp, P(Pad::Y)},
        {kZoomOutKeyNum, K(Key::MINUS), kWheelDown, P(Pad::X)},
        {kCompUpKeyNum, K(Key::UP_ARROW), kNone, P(Pad::UP)},
        {kCompDownKeyNum, K(Key::DOWN_ARROW), kNone, P(Pad::DOWN)},
        {kCompAcceptKeyNum, K(Key::RIGHT_ARROW), kNone, P(Pad::RIGHT)},
        {kCompCancelKeyNum, K(Key::LEFT_ARROW), kNone, P(Pad::LEFT)},
        {kTransferKeyNum, K(Key::F8), kNone, kNone},
        {kScale121KeyNum, K(Key::F9), kNone, kNone},
        {kScale122KeyNum, K(Key::F10), kNone, kNone},
        {kScale124KeyNum, K(Key::F11), kNone, kNone},
        {kScale1216KeyNum, K(Key::F12), kNone, kNone},
        {kScaleHostileKeyNum, K(Key::END), kNone, kNone},
        {kScaleObjectKeyNum, K(Key::HOME), kNone, kNone},
        {kScaleAllKeyNum, K(Key::PAGE_UP), kNone, kNone},
        {kMessageNextKeyNum, K(Key::BACKSPACE), kNone, P(Pad::BACK)},
        {kHelpKeyNum, K(Key::F1), kNone, kNone},
        {kVolumeDownKeyNum, K(Key::F2), kNone, kNone},
        {kVolumeUpKeyNum, K(Key::F3), kNone, kNone},
        {kActionMusicKeyNum, K(Key::F4), kNone, kNone},
        {kNetSettingsKeyNum, K(Key::F5), kNone, kNone},
        {kFastMotionKeyNum, K(Key::F6), kNone, kNone},
        {kHotKey1Num, K(Key::K1), kNone, kNone},
        {kHotKey2Num, K(Key::K2), kNone, kNone},
        {kHotKey3Num, K(Key::K3), kNone, kNone},
        {kHotKey4Num, K(Key::K4), kNone, kNone},
        {kHotKey5Num, K(Key::K5), kNone, kNone},
        {kHotKey6Num, K(Key::K6), kNone, kNone},
        {kHotKey7Num, K(Key::K7), kNone, kNone},
        {kHotKey8Num, K(Key::K8), kNone, kNone},
        {kHotKey9Num, K(Key::K9), kNone, kNone},
        {kHotKey10Num, K(Key::K0), kNone, kNone},
        {kAttackMoveKeyNum, K(Key::G), kNone, kNone},
        {kPatrolKeyNum, K(Key::T), kNone, kNone},
};

static_assert(sizeof(kClassic) / sizeof(kClassic[0]) == ACTION_COUNT, "Classic covers every action");
static_assert(
        sizeof(kModernAres) / sizeof(kModernAres[0]) == ACTION_COUNT, "Ares-style covers every action");
static_assert(
        sizeof(kModernWasd) / sizeof(kModernWasd[0]) == ACTION_COUNT, "WASD covers every action");

bool is_weapon(int action) {
    return (action == kOneKeyNum) || (action == kTwoKeyNum) || (action == kEnterKeyNum);
}

}  // namespace

Binding Binding::key(Key k) {
    Binding b;
    if (k != Key::NONE) {
        b.device = Device::KEY;
        b.code   = k.value();
    }
    return b;
}

Binding Binding::mouse(int button) {
    Binding b;
    b.device = Device::MOUSE;
    b.code   = button;
    return b;
}

Binding Binding::wheel(int direction) {
    Binding b;
    b.device = Device::WHEEL;
    b.code   = (direction > 0) ? +1 : -1;
    return b;
}

Binding Binding::gamepad(Gamepad::Button button) {
    Binding b;
    if (button != Gamepad::Button::NONE) {
        b.device = Device::GAMEPAD;
        b.code   = static_cast<int>(button);
    }
    return b;
}

bool Binding::reserved() const {
    switch (device) {
        case Device::KEY:
            return (code == Key::ESCAPE) || (code == Key::RETURN) || (code == Key::CAPS_LOCK);
        case Device::MOUSE: return (code == 0) || (code == 1);
        case Device::GAMEPAD:
            return (code == static_cast<int>(Pad::LB)) || (code == static_cast<int>(Pad::RB)) ||
                   (code == static_cast<int>(Pad::START));
        default: return false;
    }
}

pn::string Binding::name() const {
    switch (device) {
        case Device::NONE: return "";
        case Device::KEY: return Key(code).long_name().copy();
        case Device::MOUSE: return pn::format("MOUSE {0}", code + 1);
        case Device::WHEEL: return (code > 0) ? "WHEEL UP" : "WHEEL DOWN";
        case Device::GAMEPAD: {
            pn::string out;
            if (Gamepad::name(static_cast<Pad>(code), out)) {
                return out;
            }
            for (const auto& t : kPadTokens) {
                if (static_cast<int>(t.button) == code) {
                    return pn::string{t.token};
                }
            }
            return "";
        }
    }
    return "";
}

bool slot_accepts(BindingSlot slot, const Binding& b) {
    if (b.empty()) {
        return true;
    }
    if (b.reserved()) {
        return false;
    }
    if (slot == kGamepadSlot) {
        return b.device == Binding::Device::GAMEPAD;
    }
    return b.device != Binding::Device::GAMEPAD;
}

pn::string_view preset_id(ControlPreset p) {
    switch (p) {
        case ControlPreset::MODERN_ARES: return "modern-ares";
        case ControlPreset::MODERN_WASD: return "modern-wasd";
        default: return "classic";
    }
}

pn::string_view preset_label(ControlPreset p) {
    switch (p) {
        case ControlPreset::MODERN_ARES: return "MODERN: ARES";
        case ControlPreset::MODERN_WASD: return "MODERN: WASD";
        default: return "CLASSIC";
    }
}

sfz::optional<ControlPreset> preset_with_id(pn::string_view id) {
    sfz::optional<ControlPreset> found;
    for (ControlPreset p : kControlPresets) {
        if (preset_id(p) == id) {
            found.emplace(p);
        }
    }
    if (id == "modern") {  // saved 2026-10-20, before the Modern presets split: the WASD layout
        found.emplace(ControlPreset::MODERN_WASD);
    }
    return found;
}

void apply_preset(ControlPreset p, ActionBindings out[ACTION_COUNT]) {
    const PresetRow* rows = kClassic;
    switch (p) {
        case ControlPreset::MODERN_ARES: rows = kModernAres; break;
        case ControlPreset::MODERN_WASD: rows = kModernWasd; break;
        default: break;
    }
    for (int i = 0; i < ACTION_COUNT; ++i) {
        const PresetRow& r                = rows[i];
        out[r.action].slot[kPrimarySlot]   = r.primary;
        out[r.action].slot[kSecondarySlot] = r.secondary;
        out[r.action].slot[kGamepadSlot]   = r.gamepad;
    }
}

bool may_share(int a, int b) { return is_weapon(a) && is_weapon(b); }

std::vector<std::pair<int, int>> find_clashes(const ActionBindings bindings[ACTION_COUNT]) {
    std::vector<std::pair<int, int>> clashes;
    for (int a = 0; a < ACTION_COUNT; ++a) {
        for (int b = a + 1; b < ACTION_COUNT; ++b) {
            if (may_share(a, b)) {
                continue;
            }
            bool clash = false;
            for (const Binding& x : bindings[a].slot) {
                for (const Binding& y : bindings[b].slot) {
                    clash = clash || (!x.empty() && (x == y));
                }
            }
            if (clash) {
                clashes.emplace_back(a, b);
            }
        }
    }
    return clashes;
}

std::vector<int> actions_for(const ActionBindings bindings[ACTION_COUNT], const Binding& b) {
    std::vector<int> actions;
    if (b.empty()) {
        return actions;
    }
    for (int a = 0; a < ACTION_COUNT; ++a) {
        for (const Binding& x : bindings[a].slot) {
            if (x == b) {
                actions.push_back(a);
                break;
            }
        }
    }
    return actions;
}

const ActionGroup kActionGroups[6] = {
        {"SHIP", kUpKeyNum, kSelectFriendKeyNum},
        {"COMMAND", kSelectFriendKeyNum, kTransferKeyNum},
        {"SHORTCUTS", kTransferKeyNum, kHelpKeyNum},
        {"UTILITY", kHelpKeyNum, kFirstHotKeyNum},
        {"HOT KEYS", kFirstHotKeyNum, KEY_COUNT},
        {"MODERN", KEY_COUNT, ACTION_COUNT},
};

pn::string_view action_config_name(int action) {
    return ((0 <= action) && (action < ACTION_COUNT)) ? kActionConfigNames[action] : "";
}

pn::value binding_to_value(const Binding& b) {
    switch (b.device) {
        case Binding::Device::NONE: return pn::value{};
        case Binding::Device::KEY: return pn::map{{"key", b.code}};
        case Binding::Device::MOUSE: return pn::map{{"mouse", b.code}};
        case Binding::Device::WHEEL: return pn::map{{"wheel", (b.code > 0) ? "up" : "down"}};
        case Binding::Device::GAMEPAD:
            for (const auto& t : kPadTokens) {
                if (static_cast<int>(t.button) == b.code) {
                    return pn::map{{"gamepad", t.token}};
                }
            }
            return pn::value{};
    }
    return pn::value{};
}

Binding binding_from_value(pn::value_cref x) {
    if (!x.is_map()) {
        return Binding::none();
    }
    pn::map_cref m = x.as_map();
    if (m.get("key").is_int()) {
        return Binding::key(Key(static_cast<int>(m.get("key").as_int())));
    } else if (m.get("mouse").is_int()) {
        return Binding::mouse(static_cast<int>(m.get("mouse").as_int()));
    } else if (m.get("wheel").is_string()) {
        pn::string_view dir = m.get("wheel").as_string();
        if (dir == "up") {
            return Binding::wheel(+1);
        } else if (dir == "down") {
            return Binding::wheel(-1);
        }
    } else if (m.get("gamepad").is_string()) {
        pn::string_view token = m.get("gamepad").as_string();
        for (const auto& t : kPadTokens) {
            if (token == t.token) {
                return Binding::gamepad(t.button);
            }
        }
    }
    return Binding::none();
}

}  // namespace antares
