// Copyright (C) 2026 The Star Bound Authors
// This file is part of Star Bound, based on Antares. Distributed under the LGPL+. See COPYING.

#ifndef ANTARES_DATA_ARENA_HPP_
#define ANTARES_DATA_ARENA_HPP_

#include <map>
#include <pn/string>
#include <vector>

#include "data/level.hpp"

namespace antares {

// Arena battles (docs/arena-investigation.md): solo levels in Star Bound's own folder (arena/,
// next to data/ and art/), so data/ is never edited. They are parsed with the campaign's level
// reader but kept in plug.arena, out of plug.levels and plug.chapters, so the campaign is
// unchanged. The ships they name come from the factory data as usual.
struct ArenaLevels {
    std::map<pn::string, Level> levels;    // by file name without ".pn" (e.g. "skirmish")
    std::vector<pn::string>     problems;  // one line per file that was skipped, and why
};

// Reads every *.pn file in `dir`. A file that doesn't parse, or isn't a playable arena battle
// (arena_level_problems), is skipped and reported, so one bad file never stops the game. A
// missing folder gives nothing.
ArenaLevels load_arena_levels(pn::string_view dir);

// What an arena battle needs, as messages (empty if it has everything):
//   * a solo level with no chapter (so it can never join the campaign);
//   * one human player and at least one cpu player;
//   * a flagship owned by the human player;
//   * a win for the human player, and a win for a cpu player (the human's loss);
//   * any "ships" condition starting disabled: ship counts are 0 until the first think, so an
//     enabled one fires at once (docs/arena-investigation.md §6).
std::vector<pn::string> arena_level_problems(const Level& level);

}  // namespace antares

#endif  // ANTARES_DATA_ARENA_HPP_
