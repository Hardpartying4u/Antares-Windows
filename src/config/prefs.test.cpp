// Copyright (C) 2026 The Star Bound Authors
// This file is part of Star Bound, based on Antares. Distributed under the LGPL+. See COPYING.
//
// Classic key defaults: every action reachable on a PC keyboard, and saved configs from before
// the change keep working.

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

}  // namespace
}  // namespace antares
