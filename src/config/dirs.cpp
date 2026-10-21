// Copyright (C) 1997, 1999-2001, 2008 Nathan Lamont
// Copyright (C) 2017 The Antares Authors
//
// This file is part of Antares, a tactical space combat game.
//
// Antares is free software: you can redistribute it and/or modify it
// under the terms of the Lesser GNU General Public License as published
// by the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Antares is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with Antares.  If not, see http://www.gnu.org/licenses/

#include "config/dirs.hpp"

#include <sfz/sfz.hpp>
#include <stdexcept>

#include "config/preferences.hpp"
#include "game/sys.hpp"

namespace antares {

static sfz::optional<pn::string> app_data;
static sfz::optional<pn::string> factory_scenario;
const char kFactoryScenarioIdentifier[] = "4cab7415715aeeacf1486a352267ae82c0efb220";

pn::string_view application_path() {
    if (app_data.has_value()) {
        return *app_data;
    }
    return default_application_path();
}

void set_application_path(pn::string_view path) { app_data.emplace(path.copy()); }

pn::string_view factory_scenario_path() {
    if (factory_scenario.has_value()) {
        return *factory_scenario;
    }
    return default_factory_scenario_path();
}

void set_factory_scenario_path(pn::string_view path) { factory_scenario.emplace(path.copy()); }

static pn::string art;
static bool       art_forced = false;

// `name` in the folder that holds the application data folder.
static pn::string beside_application_data(pn::string_view name) {
    pn::string_view app = application_path();
    int             cut = -1;
    for (int i = 0; i < app.size(); ++i) {
        if ((app.data()[i] == '/') || (app.data()[i] == '\\')) {
            cut = i;
        }
    }
    return (cut < 0) ? name.copy() : pn::format("{0}/{1}", app.substr(0, cut), name);
}

pn::string default_art_path() { return beside_application_data("art"); }

pn::string default_arena_path() { return beside_application_data("arena"); }

pn::string_view art_path() {
    if (!art_forced && sys.prefs && !sys.prefs->enhanced_graphics()) {
        return "";  // Graphics: Original
    }
    return art;
}

void set_art_path(pn::string_view path, bool forced) {
    art        = path.copy();
    art_forced = forced;
}

void configure_art(pn::string_view art_option, bool no_art) {
    set_art_path("");
    if (no_art) {
        return;
    }
    const bool forced = !art_option.empty();
    pn::string dir    = forced ? art_option.copy() : default_art_path();
    if (sfz::path::isdir(dir)) {
        set_art_path(dir, forced);
    } else if (forced) {
        throw std::runtime_error(pn::format("{0}: art folder not found", dir).c_str());
    }
}

}  // namespace antares
