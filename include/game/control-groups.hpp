// Copyright (C) 2026 The Star Bound Authors
// This file is part of Star Bound, based on Antares. Distributed under the LGPL+. See COPYING.

#ifndef ANTARES_GAME_CONTROL_GROUPS_HPP_
#define ANTARES_GAME_CONTROL_GROUPS_HPP_

#include <stdint.h>
#include <functional>
#include <vector>

#include "math/geometry.hpp"

namespace antares {

// The Modern mode's control groups (docs/modern-controls.md step E): ten groups of the player's
// ships, saved with Ctrl+number and recalled with the number. A ship is kept as its object number
// and id, so a destroyed ship whose slot is reused doesn't come back as a different one. Ships
// that died, were captured or became locked are dropped whenever a group is read.
struct GroupEntry {
    int32_t number;
    int32_t id;
};
inline bool operator==(const GroupEntry& a, const GroupEntry& b) {
    return (a.number == b.number) && (a.id == b.id);
}

class ControlGroups {
  public:
    static const int kCount = 10;
    using Valid             = std::function<bool(const GroupEntry&)>;

    void clear();
    void save(int group, const std::vector<GroupEntry>& ships);

    // The group's ships that are still `valid`, in saved order; the others are dropped for good.
    std::vector<GroupEntry> recall(int group, const Valid& valid);
    // The same without dropping anything (for drawing).
    std::vector<GroupEntry> peek(int group, const Valid& valid) const;

    // `selection` with `more` appended, skipping ships already in it.
    static std::vector<GroupEntry> add(
            std::vector<GroupEntry> selection, const std::vector<GroupEntry>& more);

  private:
    std::vector<GroupEntry> _groups[kCount];
};

// A patrolling ship (docs/modern-controls.md step F): it flies between `a` (where it was ordered)
// and `b` (the clicked point), heading for `b` when `to_b`. Kept in the game state.
struct PatrolRecord {
    int32_t number;
    int32_t id;
    Point   a, b;
    bool    to_b;
};

}  // namespace antares

#endif  // ANTARES_GAME_CONTROL_GROUPS_HPP_
