// Copyright (C) 2026 The Star Bound Authors
// This file is part of Star Bound, based on Antares. Distributed under the LGPL+. See COPYING.
//
// Which folder a sprite sheet or picture is read from in each Graphics mode and with --art /
// --no-art, and which @2x/@4x picture or font image each screen scale picks.

#include <gmock/gmock.h>
#include <pn/output>
#include <sfz/sfz.hpp>

#include "config/dirs.hpp"
#include "config/preferences.hpp"
#include "data/resource.hpp"
#include "drawing/color.hpp"
#include "drawing/pix-map.hpp"

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


// A test art folder with an @2x of the "credit" picture (exactly 2x its 160x36) and an @4x of the
// wrong size, which must be skipped.
const char kPictureArt[] = "out/test/art-fixture";

void write_png(pn::string_view path, Size size) {
    ArrayPixMap pix(size);
    pix.fill(RgbColor::white());
    pn::output out{path, pn::binary};
    pix.encode(out);
}

struct PictureFixture {
    PictureFixture() {
        sfz::makedirs(pn::format("{0}/pictures", kPictureArt), 0755);
        write_png(pn::format("{0}/pictures/credit@2x.png", kPictureArt), {320, 72});
        write_png(pn::format("{0}/pictures/credit@4x.png", kPictureArt), {600, 100});
    }
};

TEST_F(ArtTest, PictureScales) {
    PictureFixture f;
    Mode           m(true, kPictureArt, false);
    EXPECT_EQ(Resource::hidpi_path("pictures/credit", 1), "pictures/credit.png");
    EXPECT_EQ(Resource::hidpi_path("pictures/credit", 2), "pictures/credit@2x.png");
    EXPECT_EQ(resource_source("pictures/credit@2x.png"), "art");
    // 3 -> @4x, @2x, 1x: the @4x is the wrong size, so @2x.
    EXPECT_EQ(Resource::hidpi_path("pictures/credit", 3), "pictures/credit@2x.png");
    EXPECT_EQ(Resource::hidpi_path("pictures/credit", 4), "pictures/credit@2x.png");
}

TEST_F(ArtTest, PicturesOriginalUsesDataOnly) {
    PictureFixture f;
    Preferences    p;
    p.enhanced_graphics = false;
    NullPrefsDriver prefs(p);
    set_art_path(kPictureArt);  // the art folder exists, but Graphics: Original hides it
    for (int scale : {1, 2, 3, 4}) {
        pn::string path = Resource::hidpi_path("pictures/credit", scale);
        EXPECT_EQ(path, "pictures/credit.png") << "scale " << scale;
        EXPECT_EQ(resource_source(path), "data") << "scale " << scale;
    }
    set_art_path("");
}

TEST_F(ArtTest, PictureArtOptionOverridesOriginal) {
    PictureFixture f;
    Mode           m(false, kPictureArt, false);
    EXPECT_EQ(Resource::hidpi_path("pictures/credit", 2), "pictures/credit@2x.png");
}

TEST_F(ArtTest, FontScales) {
    // data/ has 1x and @2x font images. Fonts are pixel art: each scale uses one that divides it, so
    // every font pixel is drawn the same size (UI 3x: 1x three times, not @2x one and a half times).
    Mode m(true, "", true);
    EXPECT_EQ(Resource::hidpi_path("fonts/button", 1, true), "fonts/button.png");
    EXPECT_EQ(Resource::hidpi_path("fonts/button", 2, true), "fonts/button@2x.png");
    EXPECT_EQ(Resource::hidpi_path("fonts/button", 3, true), "fonts/button.png");
    EXPECT_EQ(Resource::hidpi_path("fonts/button", 4, true), "fonts/button@2x.png");
}

}  // namespace
}  // namespace antares
