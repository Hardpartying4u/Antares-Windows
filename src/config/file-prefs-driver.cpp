// Copyright (C) 1997, 1999-2001, 2008 Nathan Lamont
// Copyright (C) 2015-2017 The Antares Authors
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

#include "config/file-prefs-driver.hpp"

#include <fcntl.h>

#include <algorithm>

#include <pn/input>
#include <pn/map>
#include <pn/output>
#include <sfz/sfz.hpp>

#include "config/dirs.hpp"
#include "config/keys.hpp"
#include "sound/fx.hpp"

using sfz::makedirs;
using sfz::range;
using sfz::path::dirname;

namespace antares {

// Config versions:
//  1. (no version field) "keys": one key per original action. Loads as the Classic preset with
//     those keys.
//  2. (2026-10-20) bindings for every action (two keyboard-and-mouse and one gamepad) and the
//     selected preset, under "controls". Still writes "keys", so an older build can read the keys
//     it knows.
//  3. (2026-10-20) the same as 2, written after Classic's fire keys moved to X and Z. Versions 1 and
//     2 have the fire-key move applied on load (migrate_classic_fire_keys).
//  4. (2026-10-21) the same as 3, written after Modern: Ares took Classic's bindings. Versions 1 to 3
//     on Modern: Ares with its old layout untouched get the new one on load (migrate_modern_ares).
static const int64_t kConfigVersion = 4;

// Modern: Ares' own layout before version 4, where it differed from Classic: primary keys only (its
// secondary and gamepad bindings were Classic's).
static const struct {
    int        action;
    Key::Value key;
} kModernAresBefore4[] = {
        {kUpKeyNum, Key::UP_ARROW},        {kDownKeyNum, Key::DOWN_ARROW},
        {kLeftKeyNum, Key::LEFT_ARROW},    {kRightKeyNum, Key::RIGHT_ARROW},
        {kOneKeyNum, Key::SPACE},          {kTwoKeyNum, Key::F},
        {kEnterKeyNum, Key::D},            {kSelectFriendKeyNum, Key::Q},
        {kSelectFoeKeyNum, Key::E},        {kSelectBaseKeyNum, Key::R},
        {kOrderKeyNum, Key::S},            {kZoomInKeyNum, Key::X},
        {kZoomOutKeyNum, Key::Z},          {kCompUpKeyNum, Key::T},
        {kCompDownKeyNum, Key::G},         {kCompAcceptKeyNum, Key::B},
        {kCompCancelKeyNum, Key::V},       {kTransferKeyNum, Key::C},
        {kMessageNextKeyNum, Key::BACKTICK}, {kAttackMoveKeyNum, Key::A},
};

// A Modern: Ares config whose bindings are still the old layout gets the new one (Classic's). One
// the player customised is kept as it is: it shows as changed, and Reset to Defaults gives the new
// layout.
static void migrate_modern_ares(Preferences* p) {
    ActionBindings before[ACTION_COUNT];
    apply_preset(ControlPreset::CLASSIC, before);
    for (const auto& r : kModernAresBefore4) {
        before[r.action].slot[kPrimarySlot] = Binding::key(Key(r.key));
    }
    for (int a = 0; a < ACTION_COUNT; ++a) {
        for (int s = 0; s < kBindingSlots; ++s) {
            if (p->bindings[a].slot[s] != before[a].slot[s]) {
                return;
            }
        }
    }
    apply_preset(ControlPreset::MODERN_ARES, p->bindings);
}

static const char* const kSlotNames[kBindingSlots] = {"primary", "secondary", "gamepad"};

static void set(bool& v, pn::value_cref x) {
    if (x.is_bool()) {
        v = x.as_bool();
    }
}

static void set(int& v, pn::value_cref x) {
    if (x.is_int()) {
        v = x.as_int();
    }
}

static void set(int16_t& v, pn::value_cref x) {
    if (x.is_int()) {
        v = x.as_int();
    }
}

static void set(Key& v, pn::value_cref x) {
    if (x.is_int()) {
        v = Key(x.as_int());
    }
}

static void set(Size& s, pn::value_cref x) {
    if (x.is_map()) {
        set(s.width, x.as_map().get("width"));
        set(s.height, x.as_map().get("height"));
    }
}

FilePrefsDriver::FilePrefsDriver(pn::string_view path) : _path(path.copy()) {
    using ::antares::set;

    pn::value x;
    pn::input in = pn::input{_path, pn::text};
    if (!in || !pn::parse(in, &x, nullptr)) {
        return;
    }
    pn::map_cref m = x.as_map();

    pn::map_cref sound = m.get("sound").as_map();
    set(_current.volume, sound.get("volume"));
    // Volume is 0..8 (the sound drivers play at volume / 8). Earlier builds let F3 go past 8, so a
    // saved config can hold more; bring it back into range instead of amplifying.
    _current.volume = std::max<int16_t>(0, std::min<int16_t>(kMaxVolumePreference, _current.volume));
    set(_current.speech_on, sound.get("speech"));
    set(_current.play_idle_music, sound.get("idle music"));
    set(_current.play_music_in_game, sound.get("game music"));

    const int64_t version = m.get("version").is_int() ? m.get("version").as_int() : 1;
    if (m.get("controls").is_map()) {
        pn::map_cref controls = m.get("controls").as_map();
        if (controls.get("preset").is_string()) {
            auto preset = preset_with_id(controls.get("preset").as_string());
            if (preset.has_value()) {
                _current.controls_preset = *preset;
                apply_preset(*preset, _current.bindings);  // for any action the file lacks
            }
        }
        pn::map_cref bindings = controls.get("bindings").as_map();
        for (int a = 0; a < ACTION_COUNT; ++a) {
            if (!bindings.has(action_config_name(a))) {
                continue;
            }
            pn::map_cref entry = bindings.get(action_config_name(a)).as_map();
            for (int s = 0; s < kBindingSlots; ++s) {
                if (!entry.has(kSlotNames[s])) {
                    continue;
                }
                const Binding b = binding_from_value(entry.get(kSlotNames[s]));
                if (slot_accepts(static_cast<BindingSlot>(s), b)) {
                    _current.bindings[a].slot[s] = b;
                }
            }
        }
    } else {
        pn::map_cref keys = m.get("keys").as_map();
        for (int a = 0; a < KEY_COUNT; ++a) {
            pn::value_cref k = keys.get(action_config_name(a));
            if (k.is_int()) {
                const Binding b = Binding::key(Key(static_cast<int>(k.as_int())));
                if (slot_accepts(kPrimarySlot, b)) {
                    _current.bindings[a].slot[kPrimarySlot] = b;
                }
            }
        }
        migrate_unreachable_defaults(&_current);
    }
    if ((version < 3) && (_current.controls_preset == ControlPreset::CLASSIC)) {
        // Left Alt and the Windows key were Classic's defaults; on a Modern preset they are a
        // player's own choice.
        migrate_classic_fire_keys(&_current);
    }
    if ((version < 4) && (_current.controls_preset == ControlPreset::MODERN_ARES)) {
        migrate_modern_ares(&_current);
    }

    pn::map_cref video = m.get("video").as_map();
    set(_current.fullscreen, video.get("fullscreen"));
    set(_current.window_size, video.get("window"));
    set(_current.ui_scale, video.get("ui scale"));
    if ((_current.ui_scale < 0) || (_current.ui_scale > 4)) {
        _current.ui_scale = 0;
    }
    set(_current.enhanced_graphics, video.get("enhanced graphics"));
}

void FilePrefsDriver::set(const Preferences& p) {
    _current = p.copy();

    pn::map keys;
    for (int a = 0; a < KEY_COUNT; ++a) {
        keys[action_config_name(a)] = p.key(a).value();
    }
    pn::map bindings;
    for (int a = 0; a < ACTION_COUNT; ++a) {
        pn::map entry;
        for (int s = 0; s < kBindingSlots; ++s) {
            entry[kSlotNames[s]] = binding_to_value(p.bindings[a].slot[s]);
        }
        bindings[action_config_name(a)] = std::move(entry);
    }

    makedirs(dirname(_path), 0755);
    pn::output output = pn::output{_path, pn::text};
    output.dump(pn::map{
            {"version", kConfigVersion},
            {"sound",
             pn::map{
                     {"volume", p.volume},
                     {"speech", p.speech_on},
                     {"idle music", p.play_idle_music},
                     {"game music", p.play_music_in_game},
             }},
            {"keys", std::move(keys)},
            {"controls",
             pn::map{
                     {"preset", preset_id(p.controls_preset).copy()},
                     {"bindings", std::move(bindings)},
             }},
            {"video",
             pn::map{
                     {"fullscreen", p.fullscreen},
                     {"window",
                      pn::map{
                              {"width", p.window_size.width},
                              {"height", p.window_size.height},
                      }},
                     {"ui scale", p.ui_scale},
                     {"enhanced graphics", p.enhanced_graphics},
             }},
    });
}

}  // namespace antares
