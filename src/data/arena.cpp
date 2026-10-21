// Copyright (C) 2026 The Star Bound Authors
// This file is part of Star Bound, based on Antares. Distributed under the LGPL+. See COPYING.

#include "data/arena.hpp"

#include <exception>
#include <pn/input>
#include <sfz/sfz.hpp>

#include "data/action.hpp"
#include "data/condition.hpp"
#include "data/initial.hpp"

namespace antares {

namespace {

class ArenaLister : public sfz::TreeWalker {
  public:
    explicit ArenaLister(std::vector<pn::string>* paths) : _paths(paths) {}

    void file(pn::string_view name, const sfz::Stat& st) const override {
        if ((name.size() > 3) && (name.substr(name.size() - 3) == ".pn")) {
            _paths->push_back(name.copy());
        }
    }
    void pre_directory(pn::string_view name, const sfz::Stat& st) const override {}
    void cycle_directory(pn::string_view name, const sfz::Stat& st) const override {}
    void post_directory(pn::string_view name, const sfz::Stat& st) const override {}
    void symlink(pn::string_view name, const sfz::Stat& st) const override {}
    void broken_symlink(pn::string_view name, const sfz::Stat& st) const override {}
    void other(pn::string_view name, const sfz::Stat& st) const override {}

  private:
    std::vector<pn::string>* _paths;
};

// The innermost message of a nested exception, prefixed by the outer ones.
pn::string describe(const std::exception& e) {
    pn::string s = e.what();
    try {
        std::rethrow_if_nested(e);
    } catch (const std::exception& inner) {
        s += pn::format(": {0}", describe(inner));
    } catch (...) {
    }
    return s;
}

}  // namespace

ArenaLevels load_arena_levels(pn::string_view dir) {
    ArenaLevels out;
    if (!sfz::path::isdir(dir)) {
        return out;
    }
    std::vector<pn::string> paths;
    sfz::walk(dir, sfz::WALK_PHYSICAL, ArenaLister(&paths));
    for (const pn::string& path : paths) {
        pn::string_view file = path;
        file                 = file.substr(dir.size() + 1);
        const pn::string name = file.substr(0, file.size() - 3).copy();
        try {
            pn::value  x;
            pn_error_t e;
            pn::input  in{path, pn::text};
            if (!pn::parse(in, &x, &e)) {
                throw std::runtime_error(
                        pn::format("{0}:{1}: {2}", e.lineno, e.column, pn_strerror(e.code))
                                .c_str());
            }
            Level level = ::antares::level(x);
            auto  why   = arena_level_problems(level);
            if (!why.empty()) {
                out.problems.push_back(pn::format("arena/{0}: {1}", file, why[0]));
                continue;
            }
            out.levels.emplace(name.copy(), std::move(level));
        } catch (const std::exception& e) {
            out.problems.push_back(pn::format("arena/{0}: {1}", file, describe(e)));
        }
    }
    return out;
}

std::vector<pn::string> arena_level_problems(const Level& level) {
    std::vector<pn::string> problems;
    if (level.type() != Level::Type::SOLO) {
        problems.push_back("not a solo level");
        return problems;
    }
    if (level.base.chapter.has_value()) {
        problems.push_back("has a chapter (arena battles stay out of the campaign)");
    }

    int human = -1, cpus = 0;
    for (int i = 0; i < level.solo.players.size(); ++i) {
        if (level.solo.players[i].type == LevelBase::PlayerType::HUMAN) {
            if (human >= 0) {
                problems.push_back("more than one human player");
            }
            human = i;
        } else {
            ++cpus;
        }
    }
    if (human < 0) {
        problems.push_back("no human player");
    }
    if (cpus == 0) {
        problems.push_back("no cpu player");
    }

    bool flagship = false;
    for (const Initial& i : level.base.initials) {
        flagship = flagship || (i.flagship.value_or(false) && i.owner.has_value() &&
                                (i.owner->number() == human));
    }
    if (!flagship) {
        problems.push_back("no flagship for the human player");
    }

    // A loss: a win for a cpu player, or no_ships (the engine ends the battle with it when the
    // flagship is lost and no ship is left to take over).
    bool win = false, lose = level.solo.no_ships.has_value();
    for (const Condition& c : level.base.conditions) {
        if ((c.when.type() == ConditionWhen::Type::SHIPS) && !c.disabled.value_or(false)) {
            problems.push_back("a \"ships\" condition starts enabled (it would fire at once)");
        }
        for (const Action& a : c.action) {
            if ((a.type() != Action::Type::WIN) || !a.win.player.has_value()) {
                continue;
            }
            const bool human_wins = (a.win.player->number() == human);
            win                   = win || human_wins;
            lose                  = lose || !human_wins;
        }
    }
    if (!win) {
        problems.push_back("no win for the human player");
    }
    if (!lose) {
        problems.push_back("no loss (a win for a cpu player, or no_ships)");
    }
    return problems;
}

}  // namespace antares
