// Copyright (C) 2026 The Star Bound Authors
// This file is part of Star Bound, based on Antares. Distributed under the LGPL+. See COPYING.
//
// Which folder a sprite sheet is read from in each Graphics mode and with --art / --no-art.

#include <gmock/gmock.h>
#include <pn/output>

#include "config/dirs.hpp"
#include "config/preferences.hpp"
#include "data/resource.hpp"

namespace antares {
namespace {

// The Human cruiser has a 1x sheet in both art/ and data/; the Ishiman heavy destroyer's @2x sheet
// exists only in art/.
const char kBoth[]    = "sprites/uns/cruiser/image.png";
const char kArtOnly[] = "sprites/ish/hvd/image@2x.png";

// Sets up one mode as the game would: the Graphics preference, then the command-line options.
struct Mode {
    Mode(bool enhanced, pn::string_view art_option, bool no_art) {
        Preferences p;
        p.enhanced_graphics = enhanced;
        prefs.reset(new NullPrefsDriver(p));
        configure_art(art_option, no_art);
    }
    ~Mode() { set_art_path(""); }
    std::unique_ptr<NullPrefsDriver> prefs;
};

void report(pn::string_view mode) {
    pn::out.format(
            "{0}: {1} -> {2}, {3} -> {4}\n", mode, kBoth, resource_source(kBoth), kArtOnly,
            resource_source(kArtOnly).empty() ? pn::string_view{"(none)"} : resource_source(kArtOnly));
}

using ArtTest = ::testing::Test;

TEST_F(ArtTest, Enhanced) {
    Mode m(true, "", false);
    report("Enhanced");
    EXPECT_EQ(resource_source(kBoth), "art");
    EXPECT_EQ(resource_source(kArtOnly), "art");
}

TEST_F(ArtTest, Original) {
    Mode m(false, "", false);
    report("Original");
    EXPECT_EQ(resource_source(kBoth), "data");
    EXPECT_EQ(resource_source(kArtOnly), "");
}

TEST_F(ArtTest, NoArtOption) {
    Mode m(true, "", true);
    report("Enhanced + --no-art");
    EXPECT_EQ(resource_source(kBoth), "data");
    EXPECT_EQ(resource_source(kArtOnly), "");
}

TEST_F(ArtTest, ArtOption) {
    Mode m(true, "art", false);
    report("Enhanced + --art art");
    EXPECT_EQ(resource_source(kBoth), "art");
    EXPECT_EQ(resource_source(kArtOnly), "art");
}

TEST_F(ArtTest, ArtOptionOverridesOriginal) {
    Mode m(false, "art", false);
    report("Original + --art art");
    EXPECT_EQ(resource_source(kBoth), "art");
    EXPECT_EQ(resource_source(kArtOnly), "art");
}

TEST_F(ArtTest, ArtOptionMissingFolder) {
    Preferences p;
    NullPrefsDriver prefs(p);
    EXPECT_THROW(configure_art("no-such-art-folder", false), std::runtime_error);
}

}  // namespace
}  // namespace antares
