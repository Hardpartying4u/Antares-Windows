// Copyright (C) 2026 The Star Bound Authors
// This file is part of Star Bound, based on Antares. Distributed under the LGPL+. See COPYING.
//
// Arena battles (docs/arena-investigation.md): the shipped arena/ files load and are playable, and
// the loader skips and reports files that aren't.

#include <gmock/gmock.h>
#include <cstdio>
#include <pn/output>
#include <sfz/sfz.hpp>

#include "data/arena.hpp"
#include "data/initial.hpp"

namespace antares {
namespace {

using ArenaTest = ::testing::Test;

// Every file in arena/ (run from the repo root, as the tests are) loads as a playable battle, and
// every ship it names is in the factory data.
TEST_F(ArenaTest, ShippedBattlesLoad) {
    ArenaLevels arena = load_arena_levels("arena");
    for (const auto& p : arena.problems) {
        ADD_FAILURE() << p.c_str();
    }
    ASSERT_TRUE(arena.levels.find("skirmish") != arena.levels.end());
    for (const auto& kv : arena.levels) {
        EXPECT_TRUE(arena_level_problems(kv.second).empty()) << kv.first.c_str();
        for (const Initial& i : kv.second.base.initials) {
            const pn::string path = pn::format("data/objects/{0}.pn", i.base.name);
            EXPECT_TRUE(sfz::path::isfile(path)) << kv.first.c_str() << ": " << path.c_str();
        }
    }
}

const char kArenaDir[] = "out/test/arena";

void write(pn::string_view name, pn::string_view text) {
    sfz::makedirs(kArenaDir, 0755);
    pn::output out{pn::format("{0}/{1}.pn", kArenaDir, name), pn::text};
    out.format("{0}", text);
}

// A small battle: a flagship against one gunship, won when the gunship's side has no ships and
// lost when the flagship dies. Each flag takes away one thing an arena battle needs.
pn::string battle(bool chapter, bool flagship, bool ships_disabled) {
    return pn::format(
            "type: \"solo\"\n{0}title: \"Test\"\n"
            "players:\n"
            "\t*\ttype: \"human\"\n\t\trace: \"ish\"\n\t\tname: \"Us\"\n"
            "\t*\ttype: \"cpu\"\n\t\trace: \"gai\"\n\t\tname: \"Them\"\n"
            "initials:\n"
            "\t*\tbase: \"ish/hvd\"\n\t\towner: 0\n\t\tat: {{x: 0, y: 0}}\n{1}"
            "\t*\tbase: \"gai/gunship\"\n\t\towner: 1\n\t\tat: {{x: 0, y: -2000}}\n"
            "conditions:\n"
            "\t*\t{2}when: {{type: \"ships\", op: \"eq\", player: 1, value: 0}}\n"
            "\t\taction:\n\t\t\t*\t{{type: \"win\", player: 0, text: \"Won.\"}}\n"
            "\t*\twhen: {{type: \"destroyed\", op: \"eq\", object: {{initial: 0}}, value: true}}\n"
            "\t\taction:\n\t\t\t*\t{{type: \"win\", player: 1, text: \"Lost.\"}}\n"
            "par: {{time: \"1m\", kills: 1, losses: 0}}\n",
            chapter ? "chapter: 90\n" : "", flagship ? "\t\tflagship: true\n" : "",
            ships_disabled ? "disabled: true\n\t\t" : "");
}

// The loss can instead be the level's no_ships text (the arena rule since 2026-10-21: losing the
// flagship passes control on, and the battle is lost when no ship is left). With neither, the
// battle is skipped.
TEST_F(ArenaTest, NoShipsIsALoss) {
    const char kDir[] = "out/test/arena-loss";
    sfz::makedirs(kDir, 0755);
    // The good battle, cut before its flagship-destroyed loss.
    const pn::string      full = battle(false, true, true);
    const pn::string_view cut  = "\t*\twhen: {type: \"destroyed\"";
    pn::string_view       h    = full;
    int                   at  = -1;
    for (int i = 0; i + cut.size() <= h.size(); ++i) {
        if (h.substr(i, cut.size()) == cut) {
            at = i;
            break;
        }
    }
    ASSERT_GE(at, 0);
    const pn::string base = h.substr(0, at).copy();
    const pn::string par  = "par: {time: \"1m\", kills: 1, losses: 0}\n";
    for (const char* stale : {"no-ships", "no-loss"}) {
        std::remove(pn::format("{0}/{1}.pn", kDir, stale).c_str());
    }
    {
        pn::output out{pn::format("{0}/no-ships.pn", kDir), pn::text};
        out.format("{0}{1}no_ships: \"Your fleet was destroyed.\"\n", base, par);
    }
    {
        pn::output out{pn::format("{0}/no-loss.pn", kDir), pn::text};
        out.format("{0}{1}", base, par);
    }
    ArenaLevels arena = load_arena_levels(kDir);
    EXPECT_TRUE(arena.levels.find("no-ships") != arena.levels.end());
    ASSERT_EQ(arena.problems.size(), 1);
    EXPECT_THAT(arena.problems[0].c_str(), ::testing::HasSubstr("arena/no-loss.pn: no loss"));
}

// A good file loads; a file that doesn't parse, and files missing what an arena battle needs, are
// skipped with a reason.
TEST_F(ArenaTest, BadFilesSkipped) {
    for (const char* stale : {"good", "broken", "chapter", "enabled-ships", "no-flagship"}) {
        std::remove(pn::format("{0}/{1}.pn", kArenaDir, stale).c_str());
    }
    write("good", battle(false, true, true));
    write("broken", "type: \"solo\"\nplayers: [\n");
    write("chapter", battle(true, true, true));
    write("enabled-ships", battle(false, true, false));
    write("no-flagship", battle(false, false, true));

    ArenaLevels arena = load_arena_levels(kArenaDir);
    pn::string  all;
    for (const auto& p : arena.problems) {
        all += pn::format("{0}\n", p);
    }
    EXPECT_EQ(arena.levels.size(), 1) << all.c_str();
    EXPECT_TRUE(arena.levels.find("good") != arena.levels.end());
    ASSERT_EQ(arena.problems.size(), 4) << all.c_str();
    EXPECT_THAT(all.c_str(), ::testing::HasSubstr("arena/broken.pn: "));
    EXPECT_THAT(all.c_str(), ::testing::HasSubstr("arena/chapter.pn: has a chapter"));
    EXPECT_THAT(
            all.c_str(),
            ::testing::HasSubstr("arena/enabled-ships.pn: a \"ships\" condition starts enabled"));
    EXPECT_THAT(
            all.c_str(), ::testing::HasSubstr("arena/no-flagship.pn: no flagship for the human"));
}

TEST_F(ArenaTest, MissingFolderIsEmpty) {
    ArenaLevels arena = load_arena_levels("out/test/no-such-arena");
    EXPECT_TRUE(arena.levels.empty());
    EXPECT_TRUE(arena.problems.empty());
}

}  // namespace
}  // namespace antares
