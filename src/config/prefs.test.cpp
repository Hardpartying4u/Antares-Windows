// Copyright (C) 2026 The Star Bound Authors
// This file is part of Star Bound, based on Antares. Distributed under the LGPL+. See COPYING.
//
// Classic key defaults: every action reachable on a PC keyboard, and saved configs from before
// the change keep working. A saved volume outside 0..8 is brought back into range on load.

#include <gmock/gmock.h>
#include <pn/output>
#include <sfz/sfz.hpp>

#include "config/file-prefs-driver.hpp"
#include "config/keys.hpp"
#include "config/preferences.hpp"

namespace antares {
namespace {

using PrefsTest = ::testing::Test;

// Keys a PC keyboard can't produce through GLFW (Keypad Clear and Help have no GLFW key; most PC
// keypads have no "=").
bool unreachable_on_pc(Key k) {
    return (k == Key::NONE) || (k == Key::N_CLEAR) || (k == Key::HELP) || (k == Key::N_EQUALS);
}

TEST_F(PrefsTest, DefaultsReachableAndDistinct) {
    const Preferences p;
    for (int i = 0; i < KEY_COUNT; ++i) {
        EXPECT_FALSE(unreachable_on_pc(p.keys[i])) << "action " << i;
        for (int j = i + 1; j < KEY_COUNT; ++j) {
            EXPECT_NE(p.keys[i], p.keys[j]) << "actions " << i << " and " << j;
        }
    }
    EXPECT_EQ(p.keys[kSelectFriendKeyNum], Key::N_PERIOD);
    EXPECT_EQ(p.keys[kSelectFoeKeyNum], Key::N_TIMES);
    EXPECT_EQ(p.keys[kScaleHostileKeyNum], Key::END);
}

// A saved config with Ares' old defaults, as configs written before the change have.
Preferences old_saved() {
    Preferences p;
    p.keys[kSelectFriendKeyNum] = Key::N_CLEAR;
    p.keys[kSelectFoeKeyNum]    = Key::N_EQUALS;
    p.keys[kScaleHostileKeyNum] = Key::HELP;
    return p;
}

TEST_F(PrefsTest, MigrateOldDefaults) {
    Preferences p = old_saved();
    EXPECT_EQ(migrate_unreachable_defaults(&p), 3);
    EXPECT_EQ(p.keys[kSelectFriendKeyNum], Key::N_PERIOD);
    EXPECT_EQ(p.keys[kSelectFoeKeyNum], Key::N_TIMES);
    EXPECT_EQ(p.keys[kScaleHostileKeyNum], Key::END);
}

TEST_F(PrefsTest, MigrateKeepsChosenKeys) {
    Preferences p = old_saved();
    p.keys[kSelectFriendKeyNum] = Key::F;  // rebound by the player: kept
    EXPECT_EQ(migrate_unreachable_defaults(&p), 2);
    EXPECT_EQ(p.keys[kSelectFriendKeyNum], Key::F);
}

TEST_F(PrefsTest, MigrateNeverCreatesAConflict) {
    Preferences p = old_saved();
    p.keys[kOneKeyNum] = Key::END;  // the new default is already used by another action
    EXPECT_EQ(migrate_unreachable_defaults(&p), 2);
    EXPECT_EQ(p.keys[kScaleHostileKeyNum], Key::HELP);  // left as saved
}

const char kConfig[] = "out/test/prefs-old.pn";

TEST_F(PrefsTest, OldConfigFileLoads) {
    sfz::makedirs("out/test", 0755);
    {
        // The keys section as a config from before the change writes it: the old defaults, plus
        // one key the player rebound (fire 1 on Q).
        pn::output out{kConfig, pn::text};
        out.format(
                "keys: {{\"select friendly\": 83, \"select hostile\": 103, \"zoom hostile\": 117, "
                "\"fire 1\": 20}}\n");
    }
    FilePrefsDriver prefs(kConfig);
    EXPECT_EQ(prefs.key(kSelectFriendKeyNum), Key::N_PERIOD);
    EXPECT_EQ(prefs.key(kSelectFoeKeyNum), Key::N_TIMES);
    EXPECT_EQ(prefs.key(kScaleHostileKeyNum), Key::END);
    EXPECT_EQ(prefs.key(kOneKeyNum).value(), 20);  // the player's own binding is kept
}

// Writes a config whose sound section has the given volume, with other settings that must survive.
static void write_config_with_volume(int volume) {
    sfz::makedirs("out/test", 0755);
    pn::output out{kConfig, pn::text};
    out.format(
            "sound: {{volume: {0}, speech: false, \"idle music\": false, \"game music\": true}}\n"
            "keys: {{\"fire 1\": 20, \"volume down\": 59}}\n"
            "video: {{fullscreen: true, window: {{width: 5120, height: 1351}}, \"ui scale\": 3, "
            "\"enhanced graphics\": false}}\n",
            volume);
}

TEST_F(PrefsTest, VolumeAboveRangeIsClamped) {
    write_config_with_volume(23);  // as an earlier build's uncapped F3 could save
    FilePrefsDriver prefs(kConfig);
    EXPECT_EQ(prefs.volume(), 8);
    // The rest of the config is kept as saved.
    EXPECT_FALSE(prefs.speech_on());
    EXPECT_FALSE(prefs.play_idle_music());
    EXPECT_TRUE(prefs.play_music_in_game());
    EXPECT_EQ(prefs.key(kOneKeyNum).value(), 20);
    EXPECT_EQ(prefs.key(kVolumeDownKeyNum).value(), 59);
    EXPECT_TRUE(prefs.fullscreen());
    EXPECT_EQ(prefs.window_size().width, 5120);
    EXPECT_EQ(prefs.window_size().height, 1351);
    EXPECT_EQ(prefs.ui_scale(), 3);
    EXPECT_FALSE(prefs.enhanced_graphics());
}

TEST_F(PrefsTest, VolumeBelowRangeIsClamped) {
    write_config_with_volume(-3);
    FilePrefsDriver prefs(kConfig);
    EXPECT_EQ(prefs.volume(), 0);
}

TEST_F(PrefsTest, VolumeInRangeIsKept) {
    for (int v : {0, 5, 8}) {
        write_config_with_volume(v);
        FilePrefsDriver prefs(kConfig);
        EXPECT_EQ(prefs.volume(), v);
    }
}

}  // namespace
}  // namespace antares
