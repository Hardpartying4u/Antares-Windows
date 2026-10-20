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

#include "ui/screens/options.hpp"

#include <string.h>

#include <algorithm>
#include <pn/output>

#include "config/keys.hpp"
#include "config/ledger.hpp"
#include "config/preferences.hpp"
#include "data/interface.hpp"
#include "data/resource.hpp"
#include "drawing/color.hpp"
#include "drawing/styled-text.hpp"
#include "drawing/text.hpp"
#include "game/level.hpp"
#include "game/main.hpp"
#include "game/time.hpp"
#include "sound/driver.hpp"
#include "sound/fx.hpp"
#include "sound/music.hpp"
#include "ui/card.hpp"
#include "ui/interface-handling.hpp"
#include "video/driver.hpp"

using std::make_pair;
using std::pair;
using std::vector;

namespace antares {

static constexpr char kCancelButton[]       = "cancel";
static constexpr char kDoneButton[]         = "done";
static constexpr char kKeyControlsButton[]  = "key-controls";
static constexpr char kVideoControlButton[] = "video-control";
static constexpr char kSoundControlButton[] = "options";

static constexpr char kGameMusicButton[] = "music-during-action";
static constexpr char kIdleMusicButton[] = "music-during-interlude";

static constexpr char kVolUpButton[]   = "volume-up";
static constexpr char kVolDownButton[] = "volume-down";
static constexpr char kVolRect[]       = "volume";

static constexpr char kFullscreenButton[] = "fullscreen";

static constexpr char kConflictTextRect[] = "conflict";

OptionsScreen::OptionsScreen() : _state(SOUND_CONTROL), _revert(sys.prefs->get().copy()) {}

void OptionsScreen::become_front() {
    switch (_state) {
        case SOUND_CONTROL: stack()->push(new SoundControlScreen(&_state)); break;

        case KEY_CONTROL: stack()->push(new KeyControlScreen(&_state)); break;

        case VIDEO_CONTROL: stack()->push(new VideoControlScreen(&_state)); break;

        case ACCEPT: stack()->pop(this); break;

        case CANCEL:
            sys.prefs->set(_revert);
            sys.music.sync();  // TODO(sfiera): do this in driver.
            stack()->pop(this);
            break;
    }
}

SoundControlScreen::SoundControlScreen(OptionsScreen::State* state)
        : InterfaceScreen("options/sound", {0, 0, 640, 480}), _state(state) {
    checkbox(kIdleMusicButton)
            ->bind({
                    [] { return sys.prefs->play_idle_music(); },
                    [](bool on) {
                        sys.prefs->set_play_idle_music(on);
                        sys.music.sync();
                    },
            });

    checkbox(kGameMusicButton)
            ->bind({
                    [] { return sys.prefs->play_music_in_game(); },
                    [](bool on) { sys.prefs->set_play_music_in_game(on); },
            });

    button(kVolUpButton)
            ->bind({
                    [] {
                        sys.prefs->set_volume(sys.prefs->volume() + 1);
                        sys.audio->set_global_volume(sys.prefs->volume());
                    },
                    [] { return sys.prefs->volume() < kMaxVolumePreference; },
            });

    button(kVolDownButton)
            ->bind({
                    [] {
                        sys.prefs->set_volume(sys.prefs->volume() - 1);
                        sys.audio->set_global_volume(sys.prefs->volume());
                    },
                    [] { return sys.prefs->volume() > 0; },
            });

    button(kDoneButton)
            ->bind({
                    [this] {
                        *_state = OptionsScreen::ACCEPT;
                        stack()->pop(this);
                    },
            });

    button(kCancelButton)
            ->bind({
                    [this] {
                        *_state = OptionsScreen::CANCEL;
                        stack()->pop(this);
                    },
            });

    button(kVideoControlButton)
            ->bind({
                    [this] {
                        *_state = OptionsScreen::VIDEO_CONTROL;
                        stack()->pop(this);
                    },
            });

    button(kKeyControlsButton)
            ->bind({
                    [this] {
                        *_state = OptionsScreen::KEY_CONTROL;
                        stack()->pop(this);
                    },
            });
}

SoundControlScreen::~SoundControlScreen() {}

void SoundControlScreen::overlay() const {
    const int volume = sys.prefs->volume();
    Rect      bounds = widget(kVolRect)->inner_bounds();
    Point     off    = offset();
    bounds.offset(off.h, off.v);

    const int notch_width  = bounds.width() / kMaxVolumePreference;
    const int notch_height = bounds.height() - 4;
    Rect      notch_bounds(0, 0, notch_width * kMaxVolumePreference, notch_height);
    notch_bounds.center_in(bounds);

    Rect notch(
            notch_bounds.left, notch_bounds.top, notch_bounds.left + notch_width,
            notch_bounds.bottom);
    notch.inset(3, 6);

    Rects rects;
    for (int i = 0; i < volume; ++i) {
        const RgbColor color = GetRGBTranslateColorShade(Hue::PALE_PURPLE, 2 * (i + 1));
        rects.fill(notch, color);
        notch.offset(notch_width, 0);
    }
}

// UI scale choices, shown as a column of checkboxes that act like radio buttons. They are added
// in code rather than in data/interfaces/options/video.pn, which belongs to the separately
// licensed data repository.
static const struct {
    const char* id;
    const char* label;
    int         scale;  // 0: automatic
} kUiScaleChoices[] = {
        {"ui-scale-auto", "UI Scale: Auto", 0}, {"ui-scale-1", "UI Scale: 1x", 1},
        {"ui-scale-2", "UI Scale: 2x", 2},      {"ui-scale-3", "UI Scale: 3x", 3},
        {"ui-scale-4", "UI Scale: 4x", 4},
};

// Graphics choices: Enhanced uses the art folder (upscaled and remastered sprites), Original uses
// only the original art. Sprites are loaded when a level starts, so the change shows from the next
// level on. Shown below Fullscreen, like the UI scale choices.
static const struct {
    const char* id;
    const char* label;
    bool        enhanced;
} kGraphicsChoices[] = {
        {"graphics-enhanced", "Graphics: Enhanced", true},
        {"graphics-original", "Graphics: Original", false},
};

VideoControlScreen::VideoControlScreen(OptionsScreen::State* state)
        : InterfaceScreen("options/video", {0, 0, 640, 480}), _state(state) {
    checkbox(kFullscreenButton)
            ->bind({
                    [] { return sys.prefs->fullscreen(); },
                    [](bool on) { sys.prefs->set_fullscreen(on); },
            });

    int top = 111;  // aligned with the Fullscreen checkbox
    for (const auto& choice : kUiScaleChoices) {
        CheckboxButtonData data;
        data.id     = choice.id;
        data.type   = WidgetDataBase::CHECKBOX;
        data.bounds = {340, top, 540, top + 10};
        data.label  = choice.label;
        data.hue    = Hue::PALE_PURPLE;
        data.style  = InterfaceStyle::LARGE;
        top += 33;

        const int scale = choice.scale;
        dynamic_cast<CheckboxButton*>(add_widget(WidgetData{std::move(data)}))
                ->bind({
                        [scale] { return sys.prefs->ui_scale() == scale; },
                        [scale](bool on) {
                            if (on) {
                                sys.prefs->set_ui_scale(scale);
                            }
                        },
                });
    }

    top = 144;  // below the Fullscreen checkbox
    for (const auto& choice : kGraphicsChoices) {
        CheckboxButtonData data;
        data.id     = choice.id;
        data.type   = WidgetDataBase::CHECKBOX;
        data.bounds = {100, top, 300, top + 10};
        data.label  = choice.label;
        data.hue    = Hue::PALE_PURPLE;
        data.style  = InterfaceStyle::LARGE;
        top += 33;

        const bool enhanced = choice.enhanced;
        dynamic_cast<CheckboxButton*>(add_widget(WidgetData{std::move(data)}))
                ->bind({
                        [enhanced] { return sys.prefs->enhanced_graphics() == enhanced; },
                        [enhanced](bool on) {
                            if (on) {
                                sys.prefs->set_enhanced_graphics(enhanced);
                            }
                        },
                });
    }

    button(kDoneButton)
            ->bind({
                    [this] {
                        *_state = OptionsScreen::ACCEPT;
                        stack()->pop(this);
                    },
            });

    button(kCancelButton)
            ->bind({
                    [this] {
                        *_state = OptionsScreen::CANCEL;
                        stack()->pop(this);
                    },
            });

    button(kKeyControlsButton)
            ->bind({
                    [this] {
                        *_state = OptionsScreen::KEY_CONTROL;
                        stack()->pop(this);
                    },
            });

    button(kSoundControlButton)
            ->bind({
                    [this] {
                        *_state = OptionsScreen::SOUND_CONTROL;
                        stack()->pop(this);
                    },
            });
}

VideoControlScreen::~VideoControlScreen() {}

void VideoControlScreen::overlay() const {}

static const usecs kFlashTime = ticks(12);

// The rebinding screen (M2 stage 5). It keeps the Key Controls frame and buttons from
// data/interfaces/options/keys.pn (Cancel, Options, Done and the conflict line), which belongs to
// the separately licensed data, and builds the rest in code: page and preset choices, and for each
// action on the page its primary, secondary and gamepad binding.
namespace {

const ActionGroup* const kPages     = kActionGroups;
const int                kPageCount = sizeof(kActionGroups) / sizeof(kActionGroups[0]);

const int         kPresetLeft                 = 132;  // after "PRESET:" and the first box
const int         kRowTop                     = 122;
const int         kRowStep                    = 16;
const int         kLabelLeft                  = 60;
const int         kCellLeft[kBindingSlots]    = {250, 362, 474};  // labels fit 25 characters
const int         kCellWidth                  = 104;
const char* const kSlotNames[kBindingSlots]   = {"PRIMARY", "SECONDARY", "GAMEPAD"};
const char* const kAddedActionNames[]         = {"Attack Move"};
static_assert(KEY_COUNT + 1 == ACTION_COUNT, "a name for each added action");

pn::string cell_id(int action, int slot) { return pn::format("cell-{0}-{1}", action, slot); }

pn::string binding_label(const Binding& b) { return b.empty() ? pn::string{"-"} : b.name(); }

}  // namespace

static constexpr char kControlNameStrings[] = "controls/long";

KeyControlScreen::KeyControlScreen(OptionsScreen::State* state)
        : InterfaceScreen("options/keys", {0, 0, 640, 480}),
          _state(state),
          _action_names(Resource::strings(kControlNameStrings)) {
    remove_widget("tabs");          // one fixed button per key: replaced by the pages below
    remove_widget("instructions");  // drawn in overlay(), with the capture prompt

    button(kDoneButton)
            ->bind({
                    [this] {
                        *_state = OptionsScreen::ACCEPT;
                        stack()->pop(this);
                    },
                    [this] { return _clashes.empty(); },
            });

    button(kCancelButton)
            ->bind({
                    [this] {
                        *_state = OptionsScreen::CANCEL;
                        stack()->pop(this);
                    },
            });

    button(kSoundControlButton)
            ->bind({
                    [this] {
                        *_state = OptionsScreen::SOUND_CONTROL;
                        stack()->pop(this);
                    },
                    [this] { return _clashes.empty(); },
            });

    // Pages, as radio-like checkboxes along the top, each as wide as its label needs.
    int left = 56;
    for (int i = 0; i < kPageCount; ++i) {
        const int width = 24 + 8 * static_cast<int>(strlen(kPages[i].label));
        CheckboxButtonData data;
        data.id     = pn::format("page-{0}", i);
        data.type   = WidgetDataBase::CHECKBOX;
        data.bounds = {left, 62, left + width, 72};
        data.label  = kPages[i].label;
        data.hue    = Hue::AQUA;
        data.style  = InterfaceStyle::SMALL;
        left += width + 11;
        dynamic_cast<CheckboxButton*>(add_widget(WidgetData{std::move(data)}))
                ->bind({
                        [this, i] { return _page == i; },
                        [this, i](bool on) {
                            if (on) {
                                _page      = i;
                                _capturing = false;
                                _selected  = -1;
                                request_rebuild();
                            }
                        },
                });
    }

    // The presets, after a "PRESET:" label drawn in overlay(). Choosing one replaces every binding
    // with that preset's; Cancel restores the bindings from when Options was opened.
    left = kPresetLeft;
    for (ControlPreset preset : kControlPresets) {
        const int width = 24 + 8 * static_cast<int>(preset_label(preset).size());
        CheckboxButtonData data;
        data.id     = pn::format("preset-{0}", preset_id(preset));
        data.type   = WidgetDataBase::CHECKBOX;
        data.bounds = {left, 84, left + width, 94};
        data.label  = preset_label(preset).copy();
        data.hue    = Hue::PALE_PURPLE;
        data.style  = InterfaceStyle::SMALL;
        left += width + 11;
        dynamic_cast<CheckboxButton*>(add_widget(WidgetData{std::move(data)}))
                ->bind({
                        [preset] { return sys.prefs->controls_preset() == preset; },
                        [this, preset](bool on) {
                            if (on) {
                                sys.prefs->set_controls_preset(preset);
                                bindings_changed();
                            }
                        },
                });
    }

    // Reset to Defaults: the selected preset's bindings. Between Cancel and Options.
    {
        PlainButtonData data;
        data.id     = "reset-keys";
        data.type   = WidgetDataBase::BUTTON;
        data.bounds = {146, 427, 386, 437};
        data.label  = "RESET TO DEFAULTS";
        data.hue    = Hue::PALE_PURPLE;
        data.style  = InterfaceStyle::SMALL;
        dynamic_cast<PlainButton*>(add_widget(WidgetData{std::move(data)}))
                ->bind({
                        [this] {
                            sys.prefs->set_controls_preset(sys.prefs->controls_preset());
                            bindings_changed();
                        },
                });
    }

    // Clear: empties the selected binding.
    {
        PlainButtonData data;
        data.id     = "clear-binding";
        data.type   = WidgetDataBase::BUTTON;
        data.bounds = {474, 404, 578, 414};
        data.label  = "CLEAR";
        data.hue    = Hue::PALE_PURPLE;
        data.style  = InterfaceStyle::SMALL;
        dynamic_cast<PlainButton*>(add_widget(WidgetData{std::move(data)}))
                ->bind({
                        [this] {
                            sys.prefs->set_binding(
                                    _selected, static_cast<BindingSlot>(_selected_slot),
                                    Binding::none());
                            bindings_changed();
                        },
                        [this] { return _selected >= 0; },
                });
    }

    update_clashes();
    rebuild();
}

KeyControlScreen::~KeyControlScreen() {}

pn::string_view KeyControlScreen::action_name(int action) const {
    if (action < KEY_COUNT) {
        return _action_names.at(action);
    }
    return kAddedActionNames[action - KEY_COUNT];
}

// The binding cells of the current page. Rebuilt after a change rather than from a cell's own
// action (a widget can't be removed while it runs).
void KeyControlScreen::rebuild() {
    _rebuild_pending = false;
    for (int a = 0; a < ACTION_COUNT; ++a) {
        for (int s = 0; s < kBindingSlots; ++s) {
            remove_widget(cell_id(a, s));
        }
    }
    for (int a = kPages[_page].begin; a < kPages[_page].end; ++a) {
        const int top = kRowTop + (a - kPages[_page].begin) * kRowStep;
        for (int s = 0; s < kBindingSlots; ++s) {
            PlainButtonData data;
            data.id     = cell_id(a, s);
            data.type   = WidgetDataBase::BUTTON;
            data.bounds = {kCellLeft[s], top, kCellLeft[s] + kCellWidth, top + 10};
            data.label  = binding_label(sys.prefs->binding(a, static_cast<BindingSlot>(s)));
            data.hue    = Hue::AQUA;
            data.style  = InterfaceStyle::SMALL;
            dynamic_cast<PlainButton*>(add_widget(WidgetData{std::move(data)}))
                    ->bind({[this, a, s] {
                        _selected      = a;
                        _selected_slot = s;
                        _capturing     = true;
                        adjust_interface();
                    }});
        }
    }
    adjust_interface();
}

void KeyControlScreen::request_rebuild() { _rebuild_pending = true; }

void KeyControlScreen::bindings_changed() {
    _capturing = false;
    update_clashes();
    request_rebuild();
}

// Puts an input on the selected binding, if that slot takes it (keys, mouse buttons and the wheel
// in primary and secondary, gamepad buttons in the gamepad slot; never a reserved input).
void KeyControlScreen::assign(const Binding& b) {
    if (!slot_accepts(static_cast<BindingSlot>(_selected_slot), b) || b.empty()) {
        sys.sound.warning();
        return;
    }
    sys.prefs->set_binding(_selected, static_cast<BindingSlot>(_selected_slot), b);
    bindings_changed();
}

void KeyControlScreen::key_down(const KeyDownEvent& event) {
    if (!_capturing) {
        InterfaceScreen::key_down(event);
        return;
    }
    if (event.key() == Key::ESCAPE) {
        _capturing = false;
        adjust_interface();
        return;
    }
    assign(Binding::key(event.key()));
}

void KeyControlScreen::key_up(const KeyUpEvent& event) {
    if (!_capturing) {
        InterfaceScreen::key_up(event);
    }
}

void KeyControlScreen::mouse_down(const MouseDownEvent& event) {
    if (_capturing) {
        if (event.button() >= 2) {
            assign(Binding::mouse(event.button()));
            return;
        } else if (event.button() == 1) {
            sys.sound.warning();  // right button: kept for targeting
            return;
        }
        _capturing = false;  // a left click elsewhere ends the capture (the selection stays)
        adjust_interface();
    }
    InterfaceScreen::mouse_down(event);
}

void KeyControlScreen::mouse_wheel(const MouseWheelEvent& event) {
    if (_capturing && (event.steps() != 0)) {
        assign(Binding::wheel(event.steps()));
    }
}

void KeyControlScreen::gamepad_button_down(const GamepadButtonDownEvent& event) {
    if (_capturing) {
        assign(Binding::gamepad(event.button));
        return;
    }
    InterfaceScreen::gamepad_button_down(event);
}

void KeyControlScreen::gamepad_button_up(const GamepadButtonUpEvent& event) {
    if (!_capturing) {
        InterfaceScreen::gamepad_button_up(event);
    }
}

bool KeyControlScreen::next_timer(wall_time& time) {
    if (_rebuild_pending) {
        time = now();
        return true;
    }
    if (_next_flash > wall_time()) {
        time = _next_flash;
        return true;
    }
    return false;
}

void KeyControlScreen::fire_timer() {
    if (_rebuild_pending) {
        rebuild();
    }
    if (_next_flash > wall_time()) {
        wall_time now = antares::now();
        while (_next_flash < now) {
            _next_flash += kFlashTime;
            _flashed_on = !_flashed_on;
        }
    }
    adjust_interface();
}

// Cell highlights: the selected cell is active while it waits for an input; cells in a clash flash
// gold.
void KeyControlScreen::adjust_interface() {
    for (int a = kPages[_page].begin; a < kPages[_page].end; ++a) {
        for (int s = 0; s < kBindingSlots; ++s) {
            PlainButton* cell = button(cell_id(a, s));
            if (!cell) {
                continue;
            }
            cell->hue() = Hue::AQUA;
            if (_capturing && (a == _selected) && (s == _selected_slot)) {
                cell->activate();
            } else {
                cell->deactivate();
            }
        }
    }
    if (!_flashed_on) {
        return;
    }
    const auto& b = sys.prefs->get().bindings;
    for (const auto& c : _clashes) {
        for (int s = 0; s < kBindingSlots; ++s) {
            for (int t = 0; t < kBindingSlots; ++t) {
                if (!b[c.first].slot[s].empty() && (b[c.first].slot[s] == b[c.second].slot[t])) {
                    for (const auto& cell : {cell_id(c.first, s), cell_id(c.second, t)}) {
                        if (PlainButton* w = button(cell)) {
                            w->hue() = Hue::GOLD;
                        }
                    }
                }
            }
        }
    }
}

void KeyControlScreen::update_clashes() {
    _clashes = find_clashes(sys.prefs->get().bindings);
    if (_clashes.empty()) {
        _next_flash = wall_time();
        _flashed_on = false;
    } else if (_next_flash == wall_time()) {
        _next_flash = now() + kFlashTime;
        _flashed_on = true;
    }
}

void KeyControlScreen::overlay() const {
    const Point off = offset();
    auto text = [&off](Rect r, pn::string_view s, Hue hue) {
        r.offset(off.h, off.v);
        draw_text_in_rect(r, s, InterfaceStyle::SMALL, hue);
    };

    text({56, 84, 112, 94}, "PRESET:", Hue::PALE_PURPLE);  // checkbox boxes draw left of bounds
    if (!sys.prefs->get().bindings_match_preset()) {
        text({478, 84, 584, 94}, "(CHANGED)", Hue::PALE_PURPLE);
    }
    for (int s = 0; s < kBindingSlots; ++s) {
        text({kCellLeft[s], 106, kCellLeft[s] + kCellWidth, 116}, kSlotNames[s], Hue::TAN);
    }
    for (int a = kPages[_page].begin; a < kPages[_page].end; ++a) {
        const int top = kRowTop + (a - kPages[_page].begin) * kRowStep;
        text({kLabelLeft, top, kCellLeft[0] - 6, top + 10}, action_name(a), Hue::AQUA);
    }

    if (!_clashes.empty()) {
        const int  a = _clashes[0].first;
        const int  b = _clashes[0].second;
        pn::string message =
                pn::format("{0} and {1} use the same input", action_name(a), action_name(b));
        if (_clashes.size() > 1) {
            message = pn::format("{0} ({1} more clashes)", message, _clashes.size() - 1);
        }
        const TextRect& box    = dynamic_cast<const TextRect&>(*widget(kConflictTextRect));
        Rect            bounds = box.inner_bounds();
        bounds.offset(off.h, off.v);
        draw_text_in_rect(bounds, message, box.style(), box.hue());
    }

    if (_capturing) {
        text({44, 351, 599, 400},
             pn::format(
                     "PRESS THE INPUT FOR {0} ({1}): {2}. ESC CANCELS.", action_name(_selected),
                     kSlotNames[_selected_slot],
                     (_selected_slot == kGamepadSlot)
                             ? "A GAMEPAD BUTTON"
                             : "A KEY, THE MIDDLE OR A SIDE MOUSE BUTTON, OR THE WHEEL"),
             Hue::GOLD);
    } else {
        text({44, 351, 599, 400},
             "CLICK A BINDING, THEN PRESS A KEY, MOUSE BUTTON, WHEEL OR GAMEPAD BUTTON. CLEAR "
             "EMPTIES THE SELECTED BINDING. ESC, RETURN, CAPS LOCK, THE LEFT AND RIGHT MOUSE "
             "BUTTONS, AND THE GAMEPAD'S BUMPERS AND START ARE PRE-ASSIGNED.",
             Hue::TAN);
    }
}

}  // namespace antares
