// Copyright (C) 2026 The Star Bound Authors
// This file is part of Star Bound, based on Antares. Distributed under the LGPL+. See COPYING.

#include "game/control-groups.hpp"

#include <algorithm>

namespace antares {

void ControlGroups::clear() {
    for (auto& g : _groups) {
        g.clear();
    }
}

void ControlGroups::save(int group, const std::vector<GroupEntry>& ships) {
    if ((group >= 0) && (group < kCount)) {
        _groups[group] = add({}, ships);  // no duplicates
    }
}

std::vector<GroupEntry> ControlGroups::recall(int group, const Valid& valid) {
    if ((group < 0) || (group >= kCount)) {
        return {};
    }
    auto& g = _groups[group];
    g.erase(std::remove_if(g.begin(), g.end(), [&valid](const GroupEntry& e) { return !valid(e); }),
            g.end());
    return g;
}

std::vector<GroupEntry> ControlGroups::peek(int group, const Valid& valid) const {
    std::vector<GroupEntry> ships;
    if ((group >= 0) && (group < kCount)) {
        for (const auto& e : _groups[group]) {
            if (valid(e)) {
                ships.push_back(e);
            }
        }
    }
    return ships;
}

std::vector<GroupEntry> ControlGroups::add(
        std::vector<GroupEntry> selection, const std::vector<GroupEntry>& more) {
    for (const auto& e : more) {
        if (std::find(selection.begin(), selection.end(), e) == selection.end()) {
            selection.push_back(e);
        }
    }
    return selection;
}

}  // namespace antares
