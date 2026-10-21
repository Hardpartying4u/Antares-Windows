// Copyright (C) 2026 The Star Bound Authors
// This file is part of Star Bound, based on Antares. Distributed under the LGPL+. See COPYING.
//
// The binding model: presets, clashes, what each slot accepts, and the version 2 config format
// (and loading version 1).

#include <gmock/gmock.h>
#include <cstdio>
#include <pn/input>
#include <pn/map>
#include <pn/output>
#include <sfz/sfz.hpp>

#include "config/bindings.hpp"
#include "config/file-prefs-driver.hpp"
#include "config/preferences.hpp"

namespace antares {
namespace {

using BindingsTest = ::testing::Test;
using Pad          = Gamepad::Button;

ActionBindings* preset(ControlPreset p) {
    static ActionBindings b[ACTION_COUNT];
    apply_preset(p, b);
    return b;
}

// The original action numbers are stored in replays and old configs: they must keep their names.
TEST_F(BindingsTest, OriginalActionsKeepTheirNumbers) {
    EXPECT_EQ(KEY_COUNT, 44);
    EXPECT_TRUE(action_config_name(kUpKeyNum) == pn::string_view{"ship accel"}) << "ship accel";
    EXPECT_TRUE(action_config_name(kOrderKeyNum) == pn::string_view{"order"}) << "order";
    EXPECT_TRUE(action_config_name(kFastMotionKeyNum) == pn::string_view{"fast motion"}) << "fast motion";
    EXPECT_TRUE(action_config_name(kHotKey10Num) == pn::string_view{"hotkey 10"}) << "hotkey 10";
    EXPECT_GE(kAttackMoveKeyNum, KEY_COUNT);
    EXPECT_TRUE(action_config_name(kAttackMoveKeyNum) == pn::string_view{"attack move"}) << "attack move";
    EXPECT_EQ(ACTION_COUNT, KEY_COUNT + 1);  // patrol (45) was removed on 2026-10-21
}

// Classic is today's game: the keys of 2026-10-20, the wheel on zoom, Ares' gamepad mapping, and
// nothing on the Modern commands.
TEST_F(BindingsTest, ClassicMatchesTodaysControls) {
    const ActionBindings* c = preset(ControlPreset::CLASSIC);
    const Preferences     defaults;
    for (int a = 0; a < ACTION_COUNT; ++a) {
        for (int s = 0; s < kBindingSlots; ++s) {
            EXPECT_EQ(defaults.bindings[a].slot[s], c[a].slot[s]) << "default is Classic";
        }
    }
    EXPECT_EQ(c[kUpKeyNum].slot[kPrimarySlot], Binding::key(Key::N8));
    EXPECT_EQ(c[kOneKeyNum].slot[kPrimarySlot], Binding::key(Key::X));  // not Left Alt (Alt+Tab)
    EXPECT_EQ(c[kTwoKeyNum].slot[kPrimarySlot], Binding::key(Key::Z));  // not the Windows key
    EXPECT_THAT(actions_for(c, Binding::key(Key::X)), ::testing::ElementsAre(int(kOneKeyNum)));
    EXPECT_THAT(actions_for(c, Binding::key(Key::Z)), ::testing::ElementsAre(int(kTwoKeyNum)));
    for (ControlPreset p : kControlPresets) {  // no preset binds Alt or the Windows keys any more
        ActionBindings b[ACTION_COUNT];
        apply_preset(p, b);
        for (int a = 0; a < ACTION_COUNT; ++a) {
            for (int s = 0; s < kBindingSlots; ++s) {
                for (Key::Value k : {Key::OPTION, Key::COMMAND, Key::R_OPTION, Key::R_COMMAND}) {
                    EXPECT_NE(b[a].slot[s], Binding::key(Key(k))) << a;
                }
            }
        }
    }
    EXPECT_EQ(c[kSelectFriendKeyNum].slot[kPrimarySlot], Binding::key(Key::N_PERIOD));
    EXPECT_EQ(c[kOrderKeyNum].slot[kPrimarySlot], Binding::key(Key::CONTROL));
    EXPECT_EQ(c[kVolumeDownKeyNum].slot[kPrimarySlot], Binding::key(Key::F2));
    EXPECT_EQ(c[kHotKey10Num].slot[kPrimarySlot], Binding::key(Key::K0));
    EXPECT_EQ(c[kZoomInKeyNum].slot[kSecondarySlot], Binding::wheel(+1));
    EXPECT_EQ(c[kZoomOutKeyNum].slot[kSecondarySlot], Binding::wheel(-1));

    const struct {
        int action;
        Pad button;
    } pad[] = {
            {kUpKeyNum, Pad::A},         {kDownKeyNum, Pad::B},         {kOneKeyNum, Pad::RT},
            {kTwoKeyNum, Pad::RT},       {kEnterKeyNum, Pad::LT},       {kWarpKeyNum, Pad::LSB},
            {kZoomInKeyNum, Pad::Y},     {kZoomOutKeyNum, Pad::X},      {kMessageNextKeyNum, Pad::BACK},
            {kCompUpKeyNum, Pad::UP},    {kCompDownKeyNum, Pad::DOWN},  {kCompAcceptKeyNum, Pad::RIGHT},
            {kCompCancelKeyNum, Pad::LEFT},
    };
    for (const auto& p : pad) {
        EXPECT_EQ(c[p.action].slot[kGamepadSlot], Binding::gamepad(p.button)) << p.action;
    }
    for (int s = 0; s < kBindingSlots; ++s) {
        EXPECT_TRUE(c[kAttackMoveKeyNum].slot[s].empty());
    }
}

TEST_F(BindingsTest, ModernWasdFliesOnWasd) {
    const ActionBindings* m = preset(ControlPreset::MODERN_WASD);
    EXPECT_EQ(m[kUpKeyNum].slot[kPrimarySlot], Binding::key(Key::W));
    EXPECT_EQ(m[kDownKeyNum].slot[kPrimarySlot], Binding::key(Key::S));
    EXPECT_EQ(m[kLeftKeyNum].slot[kPrimarySlot], Binding::key(Key::A));
    EXPECT_EQ(m[kRightKeyNum].slot[kPrimarySlot], Binding::key(Key::D));
    EXPECT_EQ(m[kOrderKeyNum].slot[kPrimarySlot], Binding::key(Key::CONTROL));
    EXPECT_EQ(m[kAttackMoveKeyNum].slot[kPrimarySlot], Binding::key(Key::G));
    EXPECT_EQ(m[kZoomInKeyNum].slot[kSecondarySlot], Binding::wheel(+1));
    EXPECT_EQ(m[kUpKeyNum].slot[kGamepadSlot], Binding::gamepad(Pad::A));  // same pad as Classic
}

// Modern: Ares has Classic's bindings in every slot (since 2026-10-21); only the mode differs.
TEST_F(BindingsTest, ModernAresIsClassic) {
    ActionBindings classic[ACTION_COUNT];
    apply_preset(ControlPreset::CLASSIC, classic);
    const ActionBindings* m = preset(ControlPreset::MODERN_ARES);
    for (int a = 0; a < ACTION_COUNT; ++a) {
        for (int s = 0; s < kBindingSlots; ++s) {
            EXPECT_EQ(m[a].slot[s], classic[a].slot[s]) << "action " << a << " slot " << s;
        }
    }
}

// Configs saved with the one Modern preset of stage 4 ("modern") load as Modern (WASD), the layout
// it had.
TEST_F(BindingsTest, OldModernIdIsWasd) {
    auto p = preset_with_id("modern");
    ASSERT_TRUE(p.has_value());
    EXPECT_TRUE(*p == ControlPreset::MODERN_WASD);
}

TEST_F(BindingsTest, PresetsAreValid) {
    for (ControlPreset p : kControlPresets) {
        const ActionBindings* b = preset(p);
        EXPECT_THAT(find_clashes(b), ::testing::IsEmpty()) << static_cast<int>(p);
        for (int a = 0; a < ACTION_COUNT; ++a) {
            for (int s = 0; s < kBindingSlots; ++s) {
                EXPECT_TRUE(slot_accepts(static_cast<BindingSlot>(s), b[a].slot[s]))
                        << static_cast<int>(p) << " action " << a << " slot " << s;
            }
        }
        auto found = preset_with_id(preset_id(p));
        EXPECT_TRUE(found.has_value() && (*found == p));
    }
    EXPECT_FALSE(preset_with_id("wasd").has_value());
}

TEST_F(BindingsTest, SlotsAndReservedInputs) {
    EXPECT_TRUE(slot_accepts(kPrimarySlot, Binding::key(Key::Q)));
    EXPECT_TRUE(slot_accepts(kSecondarySlot, Binding::mouse(2)));  // middle button
    EXPECT_TRUE(slot_accepts(kSecondarySlot, Binding::wheel(+1)));
    EXPECT_TRUE(slot_accepts(kGamepadSlot, Binding::gamepad(Pad::Y)));
    EXPECT_FALSE(slot_accepts(kPrimarySlot, Binding::gamepad(Pad::Y)));  // pad buttons: pad slot
    EXPECT_FALSE(slot_accepts(kGamepadSlot, Binding::key(Key::Q)));
    for (const Binding& r :
         {Binding::key(Key::ESCAPE), Binding::key(Key::RETURN), Binding::key(Key::CAPS_LOCK),
          Binding::mouse(0), Binding::mouse(1), Binding::gamepad(Pad::LB),
          Binding::gamepad(Pad::RB), Binding::gamepad(Pad::START)}) {
        EXPECT_TRUE(r.reserved());
        EXPECT_FALSE(slot_accepts(kPrimarySlot, r));
        EXPECT_FALSE(slot_accepts(kGamepadSlot, r));
    }
}

TEST_F(BindingsTest, ClashesAndSharing) {
    ActionBindings b[ACTION_COUNT];
    apply_preset(ControlPreset::CLASSIC, b);
    // Q on both thrust (secondary) and select base (primary): a clash.
    b[kUpKeyNum].slot[kSecondarySlot]        = Binding::key(Key::Q);
    b[kSelectBaseKeyNum].slot[kPrimarySlot] = Binding::key(Key::Q);
    // The weapons share RT in Classic, and may also share a key: not a clash.
    b[kOneKeyNum].slot[kSecondarySlot] = Binding::key(Key::J);
    b[kTwoKeyNum].slot[kSecondarySlot] = Binding::key(Key::J);
    EXPECT_THAT(
            find_clashes(b),
            ::testing::ElementsAre(std::make_pair(int(kUpKeyNum), int(kSelectBaseKeyNum))));

    EXPECT_THAT(
            actions_for(b, Binding::gamepad(Pad::RT)),
            ::testing::ElementsAre(int(kOneKeyNum), int(kTwoKeyNum)));
    EXPECT_THAT(actions_for(b, Binding::wheel(+1)), ::testing::ElementsAre(int(kZoomInKeyNum)));
    EXPECT_THAT(actions_for(b, Binding::none()), ::testing::IsEmpty());
}

TEST_F(BindingsTest, ConfigValues) {
    for (const Binding& x :
         {Binding::none(), Binding::key(Key::N8), Binding::mouse(4), Binding::wheel(+1),
          Binding::wheel(-1), Binding::gamepad(Pad::RT), Binding::gamepad(Pad::LSB)}) {
        pn::value v = binding_to_value(x);
        EXPECT_EQ(binding_from_value(v), x);
    }
    EXPECT_TRUE(binding_from_value(pn::value{pn::map{{"gamepad", "nonsense"}}}).empty());
    EXPECT_TRUE(binding_from_value(pn::value{"rt"}).empty());  // must be a map
}

// Every action is on exactly one Key Controls page, in order, and each page fits the screen.
TEST_F(BindingsTest, GroupsCoverEveryActionOnce) {
    int next = 0;
    for (const ActionGroup& g : kActionGroups) {
        EXPECT_EQ(g.begin, next) << g.label;
        EXPECT_LT(g.begin, g.end) << g.label;
        EXPECT_LE(g.end - g.begin, kMaxGroupSize) << g.label;
        next = g.end;
    }
    EXPECT_EQ(next, ACTION_COUNT);
}

const char kV2Config[] = "out/test/prefs-v2.pn";

// Customised bindings and the preset survive a save and load; the file still has the version 1
// "keys" section, so an older build reads the primary keys.
TEST_F(BindingsTest, Version2RoundTrip) {
    sfz::makedirs("out/test", 0755);
    std::remove(kV2Config);  // start from defaults
    {
        FilePrefsDriver prefs(kV2Config);
        prefs.set_controls_preset(ControlPreset::MODERN_WASD);
        prefs.set_binding(kOneKeyNum, kSecondarySlot, Binding::mouse(3));
        prefs.set_binding(kEnterKeyNum, kGamepadSlot, Binding::gamepad(Pad::Y));
        prefs.set_binding(kHelpKeyNum, kPrimarySlot, Binding::none());
        prefs.set_volume(5);
    }
    FilePrefsDriver prefs(kV2Config);
    EXPECT_TRUE(prefs.controls_preset() == ControlPreset::MODERN_WASD);
    EXPECT_EQ(prefs.binding(kUpKeyNum, kPrimarySlot), Binding::key(Key::W));
    EXPECT_EQ(prefs.binding(kOneKeyNum, kSecondarySlot), Binding::mouse(3));
    EXPECT_EQ(prefs.binding(kEnterKeyNum, kGamepadSlot), Binding::gamepad(Pad::Y));
    EXPECT_TRUE(prefs.binding(kHelpKeyNum, kPrimarySlot).empty());
    EXPECT_EQ(prefs.binding(kAttackMoveKeyNum, kPrimarySlot), Binding::key(Key::G));
    EXPECT_EQ(prefs.volume(), 5);
    EXPECT_FALSE(prefs.get().bindings_match_preset());

    pn::value x;
    pn::input in{kV2Config, pn::text};
    ASSERT_TRUE(pn::parse(in, &x, nullptr));
    EXPECT_EQ(x.as_map().get("version").as_int(), 4);
    EXPECT_EQ(x.as_map().get("keys").as_map().get("ship accel").as_int(), Key(Key::W).value());
    EXPECT_EQ(x.as_map().get("keys").as_map().get("help").as_int(), 0);  // no key: 0
}

// A version 1 config (only "keys") loads as Classic with its keys: the wheel and gamepad bindings
// it never had come from Classic, as they worked before.
TEST_F(BindingsTest, Version1LoadsAsClassic) {
    sfz::makedirs("out/test", 0755);
    {
        pn::output out{kV2Config, pn::text};
        out.format("sound: {{volume: 7}}\nkeys: {{\"ship accel\": 26, \"fire 1\": 20}}\n");
    }
    FilePrefsDriver prefs(kV2Config);
    EXPECT_TRUE(prefs.controls_preset() == ControlPreset::CLASSIC);
    EXPECT_EQ(prefs.binding(kUpKeyNum, kPrimarySlot), Binding::key(Key::W));
    EXPECT_EQ(prefs.binding(kOneKeyNum, kPrimarySlot), Binding::key(Key::Q));
    EXPECT_EQ(prefs.binding(kDownKeyNum, kPrimarySlot), Binding::key(Key::N5));
    EXPECT_EQ(prefs.binding(kZoomInKeyNum, kSecondarySlot), Binding::wheel(+1));
    EXPECT_EQ(prefs.binding(kUpKeyNum, kGamepadSlot), Binding::gamepad(Pad::A));
}

// A version 2 config saved with the stage 4 "modern" preset loads as Modern (WASD), keeping its
// bindings.
TEST_F(BindingsTest, Version2OldModernLoadsAsWasd) {
    sfz::makedirs("out/test", 0755);
    {
        pn::output out{kV2Config, pn::text};
        out.format(
                "version: 2\ncontrols: {{preset: \"modern\", bindings: {{"
                "\"attack move\": {{primary: {{key: 19}}}}, "
                "\"patrol\": {{primary: {{key: 23}}}}}}}}\n");  // patrol: removed, ignored
    }
    FilePrefsDriver prefs(kV2Config);
    EXPECT_TRUE(prefs.controls_preset() == ControlPreset::MODERN_WASD);
    EXPECT_EQ(prefs.binding(kUpKeyNum, kPrimarySlot), Binding::key(Key::W));
    EXPECT_EQ(prefs.binding(kAttackMoveKeyNum, kPrimarySlot), Binding::key(Key::P));  // as saved
}

// Configs saved before 2026-10-20's fire-key change (version 1 and 2) still on Classic's old fire
// keys get X and Z; anything the player set is kept.
void write_v2_classic(const char* bindings) {
    sfz::makedirs("out/test", 0755);
    pn::output out{kV2Config, pn::text};
    out.format("version: 2\ncontrols: {{preset: \"classic\", bindings: {{{0}}}}}\n", bindings);
}

TEST_F(BindingsTest, OldClassicFireKeysMove) {
    write_v2_classic(
            "\"fire 1\": {primary: {key: 226}}, \"fire 2\": {primary: {key: 227}}");
    FilePrefsDriver prefs(kV2Config);
    EXPECT_EQ(prefs.binding(kOneKeyNum, kPrimarySlot), Binding::key(Key::X));
    EXPECT_EQ(prefs.binding(kTwoKeyNum, kPrimarySlot), Binding::key(Key::Z));
}

TEST_F(BindingsTest, OldClassicFireKeysMoveFromVersion1) {
    sfz::makedirs("out/test", 0755);
    {
        pn::output out{kV2Config, pn::text};
        out.format("keys: {{\"fire 1\": 226, \"fire 2\": 227, \"warp\": 43}}\n");
    }
    FilePrefsDriver prefs(kV2Config);
    EXPECT_EQ(prefs.binding(kOneKeyNum, kPrimarySlot), Binding::key(Key::X));
    EXPECT_EQ(prefs.binding(kTwoKeyNum, kPrimarySlot), Binding::key(Key::Z));
}

TEST_F(BindingsTest, FireKeysThePlayerChoseStay) {
    // Fire 1 rebound to Q (as the developer's own config has), fire 2 still on the Windows key but
    // Z already used by select base: neither moves.
    write_v2_classic(
            "\"fire 1\": {primary: {key: 20}}, \"fire 2\": {primary: {key: 227}}, "
            "\"select base\": {primary: {key: 29}}");
    FilePrefsDriver prefs(kV2Config);
    EXPECT_EQ(prefs.binding(kOneKeyNum, kPrimarySlot), Binding::key(Key::Q));
    EXPECT_EQ(prefs.binding(kTwoKeyNum, kPrimarySlot), Binding::key(Key::COMMAND));
    EXPECT_EQ(prefs.binding(kSelectBaseKeyNum, kPrimarySlot), Binding::key(Key::Z));
}

TEST_F(BindingsTest, FireKeysMoveOnlyOnce) {
    // A version 3 config was written after the change: Left Alt there is the player's choice.
    sfz::makedirs("out/test", 0755);
    {
        pn::output out{kV2Config, pn::text};
        out.format(
                "version: 3\ncontrols: {{preset: \"classic\", bindings: {{"
                "\"fire 1\": {{primary: {{key: 226}}}}}}}}\n");
    }
    FilePrefsDriver prefs(kV2Config);
    EXPECT_EQ(prefs.binding(kOneKeyNum, kPrimarySlot), Binding::key(Key::OPTION));
}

TEST_F(BindingsTest, ModernFireKeysNotMoved) {
    sfz::makedirs("out/test", 0755);
    {
        pn::output out{kV2Config, pn::text};
        out.format(
                "version: 2\ncontrols: {{preset: \"modern-wasd\", bindings: {{"
                "\"fire 1\": {{primary: {{key: 226}}}}}}}}\n");
    }
    FilePrefsDriver prefs(kV2Config);
    EXPECT_EQ(prefs.binding(kOneKeyNum, kPrimarySlot), Binding::key(Key::OPTION));
}

// Modern: Ares' layout before version 4 (where it differed from Classic), as a config's bindings.
pn::string old_modern_ares_bindings() {
    const struct {
        int        action;
        Key::Value key;
    } rows[] = {
            {kUpKeyNum, Key::UP_ARROW},      {kDownKeyNum, Key::DOWN_ARROW},
            {kLeftKeyNum, Key::LEFT_ARROW},  {kRightKeyNum, Key::RIGHT_ARROW},
            {kOneKeyNum, Key::SPACE},        {kTwoKeyNum, Key::F},
            {kEnterKeyNum, Key::D},          {kSelectFriendKeyNum, Key::Q},
            {kSelectFoeKeyNum, Key::E},      {kSelectBaseKeyNum, Key::R},
            {kOrderKeyNum, Key::S},          {kZoomInKeyNum, Key::X},
            {kZoomOutKeyNum, Key::Z},        {kCompUpKeyNum, Key::T},
            {kCompDownKeyNum, Key::G},       {kCompAcceptKeyNum, Key::B},
            {kCompCancelKeyNum, Key::V},     {kTransferKeyNum, Key::C},
            {kMessageNextKeyNum, Key::BACKTICK}, {kAttackMoveKeyNum, Key::A},
    };
    pn::string out;
    for (const auto& r : rows) {
        out += pn::format(
                "{0}\"{1}\": {{primary: {{key: {2}}}}}", out.empty() ? "" : ", ",
                action_config_name(r.action), Key(r.key).value());
    }
    return out;
}

// A version 3 config on Modern: Ares with the old layout untouched gets the new one (Classic's).
TEST_F(BindingsTest, OldModernAresLayoutMigrates) {
    sfz::makedirs("out/test", 0755);
    {
        pn::output out{kV2Config, pn::text};
        out.format(
                "version: 3\ncontrols: {{preset: \"modern-ares\", bindings: {{{0}}}}}\n",
                old_modern_ares_bindings());
    }
    FilePrefsDriver prefs(kV2Config);
    EXPECT_TRUE(prefs.controls_preset() == ControlPreset::MODERN_ARES);
    EXPECT_TRUE(prefs.get().bindings_match_preset());
    EXPECT_EQ(prefs.binding(kUpKeyNum, kPrimarySlot), Binding::key(Key::N8));
    EXPECT_EQ(prefs.binding(kOrderKeyNum, kPrimarySlot), Binding::key(Key::CONTROL));
    EXPECT_TRUE(prefs.binding(kAttackMoveKeyNum, kPrimarySlot).empty());
}

// One the player customised keeps its bindings; Reset to Defaults then gives the new layout.
TEST_F(BindingsTest, CustomisedModernAresKept) {
    sfz::makedirs("out/test", 0755);
    {
        pn::output out{kV2Config, pn::text};
        out.format(
                "version: 3\ncontrols: {{preset: \"modern-ares\", bindings: {{{0}, "
                "\"help\": {{primary: {{key: 0}}}}}}}}\n",
                old_modern_ares_bindings());
    }
    FilePrefsDriver prefs(kV2Config);
    EXPECT_TRUE(prefs.controls_preset() == ControlPreset::MODERN_ARES);
    EXPECT_FALSE(prefs.get().bindings_match_preset());
    EXPECT_EQ(prefs.binding(kUpKeyNum, kPrimarySlot), Binding::key(Key::UP_ARROW));
    EXPECT_EQ(prefs.binding(kAttackMoveKeyNum, kPrimarySlot), Binding::key(Key::A));
    EXPECT_TRUE(prefs.binding(kHelpKeyNum, kPrimarySlot).empty());
}

// Bindings a file shouldn't hold (a reserved input, a gamepad button in a key slot) are ignored.
TEST_F(BindingsTest, Version2IgnoresInvalidBindings) {
    sfz::makedirs("out/test", 0755);
    {
        pn::output out{kV2Config, pn::text};
        out.format(
                "version: 2\ncontrols: {{preset: \"classic\", bindings: {{"
                "\"ship accel\": {{primary: {{key: 41}}}}, "
                "\"ship decel\": {{primary: {{gamepad: \"a\"}}}}, "
                "\"fire 1\": {{gamepad: {{gamepad: \"start\"}}}}}}}}\n");
    }
    FilePrefsDriver prefs(kV2Config);
    EXPECT_EQ(prefs.binding(kUpKeyNum, kPrimarySlot), Binding::key(Key::N8));     // Esc refused
    EXPECT_EQ(prefs.binding(kDownKeyNum, kPrimarySlot), Binding::key(Key::N5));   // pad in key slot
    EXPECT_EQ(prefs.binding(kOneKeyNum, kGamepadSlot), Binding::gamepad(Pad::RT));  // Start refused
}

}  // namespace
}  // namespace antares
