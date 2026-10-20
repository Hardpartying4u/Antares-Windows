// Copyright (C) 1997, 1999-2001, 2008 Nathan Lamont
// Copyright (C) 2008-2017 The Antares Authors
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

#include "data/replay.hpp"

#include <stdio.h>

#include <fstream>
#include <functional>
#include <pn/output>
#include <string>
#include <sfz/sfz.hpp>

#include "config/dirs.hpp"
#include "config/ledger.hpp"
#include "config/preferences.hpp"
#include "data/plugin.hpp"
#include "data/resource.hpp"
#include "drawing/color.hpp"
#include "drawing/pix-map.hpp"
#include "game/admiral.hpp"
#include "game/cheat.hpp"
#include "game/cursor.hpp"
#include "game/globals.hpp"
#include "game/input-source.hpp"
#include "game/instruments.hpp"
#include "game/labels.hpp"
#include "game/level.hpp"
#include "game/main.hpp"
#include "game/messages.hpp"
#include "game/modern-orders.hpp"
#include "game/motion.hpp"
#include "game/space-object.hpp"
#include "game/sys.hpp"
#include "game/vector.hpp"
#include "lang/exception.hpp"
#include "math/random.hpp"
#include "math/rotation.hpp"
#include "sound/driver.hpp"
#include "sound/music.hpp"
#include "ui/card.hpp"
#include "ui/interface-handling.hpp"
#include "ui/screens/debriefing.hpp"
#include "video/driver.hpp"
#include "video/offscreen-driver.hpp"
#include "video/text-driver.hpp"

using std::unique_ptr;

namespace args = sfz::args;
namespace path = sfz::path;

namespace antares {
namespace {

// A test step run when the input queue delivers it to the player's ship (at its major tick).
class StepEvent : public Event {
  public:
    StepEvent(std::function<void(EventReceiver*)> step)
            : Event(wall_time()), _step(std::move(step)) {}
    void send(EventReceiver* receiver) const override { _step(receiver); }

  private:
    std::function<void(EventReceiver*)> _step;
};

// The level's Nth initial object, if it exists now.
Handle<SpaceObject> initial(int n) {
    if ((n >= 0) && (n < g.initials.size()) && g.initials[n].get() && g.initials[n]->active &&
        (g.initials[n]->id == g.initial_ids[n])) {
        return g.initials[n];
    }
    return SpaceObject::none();
}

pn::string describe(Handle<SpaceObject> o) {
    if (!o.get()) {
        return "nothing";
    }
    if (o->destObject.get()) {
        return pn::format(
                "{0} #{1} heading for {2} #{3}", o->long_name(), o.number(),
                o->destObject->long_name(), o->destObject.number());
    }
    return pn::format(
            "{0} #{1} heading for {2}, {3}", o->long_name(), o.number(), o->destinationLocation.h,
            o->destinationLocation.v);
}

class ReplayMaster : public Card {
  public:
    ReplayMaster(pn::input_view in, const sfz::optional<pn::string>& output_path)
            : _state(NEW),
              _replay_data(in),
              _random_seed(_replay_data.global_seed),
              _game_result(NO_GAME),
              _input_source(&_replay_data) {
        if (output_path.has_value()) {
            _output_path.emplace(output_path->copy());
        }
    }

    virtual void become_front() {
        switch (_state) {
            case NEW:
                _state = REPLAY;
                init();
                Randomize(4);  // For the decision to replay intro.
                _game_result  = NO_GAME;
                g.random.seed = _random_seed;
                stack()->push(new MainPlay(
                        *Level::get(_replay_data.chapter_id), true, &_input_source, false,
                        &_game_result));
                break;

            case REPLAY:
                if (_orders_log.has_value()) {
                    // Where the selected ship ended up, so the test shows the order took effect.
                    _orders_log->format(
                            "{0}\tend\t{1}\n", g.time.time_since_epoch().count(),
                            describe(Handle<Admiral>(0)->control()));
                    modern_order_log = nullptr;
                }
                if (_output_path.has_value()) {
                    pn::string path = pn::format("{0}/debriefing.txt", *_output_path);
                    sfz::makedirs(path::dirname(path), 0755);
                    pn::output outcome{path, pn::text};
                    if (g.victory_text.has_value()) {
                        outcome.write(*g.victory_text);
                        if (_game_result == WIN_GAME) {
                            outcome.write("\n");
                            Handle<Admiral> player(0);
                            pn::string      text = DebriefingScreen::build_score_text(
                                    g.time, g.level->solo.par.time, GetAdmiralLoss(player),
                                    g.level->solo.par.losses, GetAdmiralKill(player),
                                    g.level->solo.par.kills);
                            outcome.write(text);
                            outcome.write("\n");
                        }
                    }
                }
                stack()->pop(this);
                break;
        }
    }

    // Test input the replay file doesn't hold, one per line: "TICK key ACTION" (an action number,
    // pressed for one major tick), "TICK lclick X Y" or "TICK rclick X Y" (a click at logical
    // pixels), "TICK ldrag X1 Y1 X2 Y2" (a drag box), "TICK keydown N" / "TICK keyup N" (hold an
    // action, e.g. 11, the target modifier, for a Shift+drag), "TICK log-orderable" (log where each
    // ship the player could order is drawn), "TICK log-selection" (log the selected ships and
    // where they are heading), "TICK select-initial N" (select the level's Nth initial object, as a left-click on
    // it would), "TICK rclick-initial N" (a right-click where initial N is drawn at that tick), or
    // "TICK script-target A B" (what a level's "target" action does: initial A ordered to initial
    // B, its static destination cleared for the order). TICK is in game ticks, rounded up to a
    // major tick. Orders the Modern mode gives, and each scripted order's result, are logged to
    // OUTPUT/orders.log.
    void inject(pn::string_view path) {
        std::ifstream in(path.copy().c_str());
        std::string   l;
        while (std::getline(in, l)) {
            long long tick, a = 0, b = 0;
            char      word[16];
            if ((l.empty() || (l[0] == '#')) ||
                (sscanf(l.c_str(), "%lld %15s %lld %lld", &tick, word, &a, &b) < 2)) {
                continue;
            }
            tick           = ((tick + 2) / 3) * 3;
            game_ticks at  = game_ticks(ticks(tick));
            game_ticks off = game_ticks(ticks(tick + 3));
            std::string w(word);
            if (w == "ldrag") {
                // "TICK ldrag X1 Y1 X2 Y2": left button down at (X1, Y1), up a major tick later at
                // (X2, Y2): a drag box.
                long long x1 = 0, y1 = 0, x2 = 0, y2 = 0;
                sscanf(l.c_str(), "%*lld %*15s %lld %lld %lld %lld", &x1, &y1, &x2, &y2);
                _input_source.add(
                        at, unique_ptr<Event>(new MouseDownEvent(
                                    wall_time(), 0, 1, Point(int32_t(x1), int32_t(y1)))));
                _input_source.add(
                        off, unique_ptr<Event>(new MouseUpEvent(
                                     wall_time(), 0, Point(int32_t(x2), int32_t(y2)))));
            } else if (w == "keydown") {
                _input_source.add(at, unique_ptr<Event>(new ActionDownEvent(wall_time(), int(a))));
            } else if (w == "keyup") {
                _input_source.add(at, unique_ptr<Event>(new ActionUpEvent(wall_time(), int(a))));
            } else if (w == "log-orderable") {
                // Where each ship the player could order is drawn (for writing ldrag lines).
                _input_source.add(at, unique_ptr<Event>(new StepEvent([this](EventReceiver*) {
                    if (!_orders_log.has_value()) {
                        return;
                    }
                    for (auto o : SpaceObject::all()) {
                        if ((o->active == kObjectInUse) && o->sprite.get() &&
                            group_selectable(
                                    o->attributes, o->owner == g.admiral,
                                    o == g.admiral->flagship())) {
                            _orders_log->format(
                                    "{0}\torderable\t{1} #{2}\tat {3}, {4}\n",
                                    g.time.time_since_epoch().count(), o->long_name(), o.number(),
                                    o->sprite->where.h, o->sprite->where.v);
                        }
                    }
                })));
            } else if (w == "log-selection") {
                // Each selected ship and where it is heading.
                _input_source.add(at, unique_ptr<Event>(new StepEvent([this](EventReceiver*) {
                    if (!_orders_log.has_value()) {
                        return;
                    }
                    auto ships = modern_selection();
                    _orders_log->format(
                            "{0}\tselection\t{1} ships\n", g.time.time_since_epoch().count(),
                            ships.size());
                    for (auto o : ships) {
                        _orders_log->format(
                                "{0}\t  selected\t{1}\n", g.time.time_since_epoch().count(),
                                describe(o));
                    }
                })));
            } else if (w == "select-initial") {
                const int n = int(a);
                _input_source.add(at, unique_ptr<Event>(new StepEvent([n](EventReceiver*) {
                    if (initial(n).get()) {
                        g.admiral->set_control(initial(n));
                    }
                })));
            } else if (w == "rclick-initial") {
                const int n = int(a);
                _input_source.add(at, unique_ptr<Event>(new StepEvent([n](EventReceiver* r) {
                    auto o = initial(n);
                    if (o.get() && o->sprite.get()) {
                        const Point where = o->sprite->where;
                        r->mouse_down(MouseDownEvent(wall_time(), 1, 1, where));
                        r->mouse_up(MouseUpEvent(wall_time(), 1, where));
                    }
                })));
            } else if (w == "script-target") {
                const int s = int(a), d = int(b);
                _input_source.add(at, unique_ptr<Event>(new StepEvent([this, s, d](EventReceiver*) {
                    auto subject = initial(s), direct = initial(d);
                    if (!subject.get() || !direct.get()) {
                        return;
                    }
                    // As apply(const TargetAction&) in src/game/action.cpp.
                    uint32_t save = subject->attributes;
                    subject->attributes &= ~kStaticDestination;
                    OverrideObjectDestination(subject, direct);
                    subject->attributes = save;
                    if (_orders_log.has_value()) {
                        _orders_log->format(
                                "{0}\tscript target\t{1}\n", g.time.time_since_epoch().count(),
                                describe(subject));
                    }
                })));
            } else if (w == "key") {
                _input_source.add(at, unique_ptr<Event>(new ActionDownEvent(wall_time(), int(a))));
                _input_source.add(off, unique_ptr<Event>(new ActionUpEvent(wall_time(), int(a))));
            } else if ((w == "lclick") || (w == "rclick")) {
                const int button = (w == "rclick") ? 1 : 0;
                _input_source.add(
                        at, unique_ptr<Event>(new MouseDownEvent(wall_time(), button, 1, Point(int32_t(a), int32_t(b)))));
                _input_source.add(
                        off, unique_ptr<Event>(new MouseUpEvent(wall_time(), button, Point(int32_t(a), int32_t(b)))));
            }
        }
        if (_output_path.has_value()) {
            sfz::makedirs(*_output_path, 0755);
            _orders_log.emplace(pn::format("{0}/orders.log", *_output_path), pn::text);
            modern_order_log = [this](pn::string_view s) { _orders_log->write(s); };
        }
    }

  private:
    void init();

    enum State {
        NEW,
        REPLAY,
    };
    State _state;

    sfz::optional<pn::string> _output_path;
    ReplayData                _replay_data;
    const int32_t             _random_seed;
    GameResult                _game_result;
    ReplayInputSource         _input_source;
    sfz::optional<pn::output> _orders_log;
};

void ReplayMaster::init() {
    init_globals();

    sys.audio->set_global_volume(8);  // Max volume.

    sys_init();
    Label::init();
    Messages::init();
    InstrumentInit();
    SpriteHandlingInit();
    PluginInit(sfz::nullopt);
    SpaceObjectHandlingInit();  // MUST be after PluginInit()
    Admiral::init();
    Vectors::init();
}

void usage(pn::output_view out, pn::string_view progname, int retcode) {
    out.format(
            "usage: {0} [OPTIONS]"
            "\n"
            "\n  Plays a replay into a set of images and a log of sounds"
            "\n"
            "\n  arguments:"
            "\n    replay              an Antares replay script"
            "\n"
            "\n  options:"
            "\n    -o, --output=OUTPUT  place output in this directory"
            "\n    -i, --interval=INTERVAL"
            "\n                         take one screenshot per this many ticks (default: 60)"
            "\n    -w, --width=WIDTH    screen width (default: 640)"
            "\n    -h, --height=HEIGHT  screen height (default: 480)"
            "\n    -t, --text           produce text output"
            "\n    -s, --smoke          run as smoke text"
            "\n        --opengl=2.0|3.2 select OpenGL version (default: 3.2)"
            "\n        --modern         play with the Modern control mode on"
            "\n        --inject=FILE    add clicks and key actions from FILE (see ReplayMaster)"
            "\n        --help           display this help screen"
            "\n",
            progname);
    exit(retcode);
}

void main(int argc, char* const* argv) {
    args::callbacks callbacks;

    sfz::optional<pn::string> replay_path;
    callbacks.argument = [&replay_path](pn::string_view arg) {
        if (!replay_path.has_value()) {
            replay_path.emplace(arg.copy());
        } else {
            return false;
        }
        return true;
    };

    sfz::optional<pn::string> output_dir;
    int                       interval     = 60;
    int                       width        = 640;
    int                       height       = 480;
    bool                      text         = false;
    bool                      smoke        = false;
    sfz::optional<pn::string> inject_path;
    std::pair<int, int>       gl_version   = {3, 2};
    pn::string_view           glsl_version = "330 core";
    callbacks.short_option = [&](pn::rune opt, const args::callbacks::get_value_f& get_value) {
        switch (opt.value()) {
            case 'o': output_dir.emplace(get_value().copy()); return true;
            case 'i': sfz::args::integer_option(get_value(), &interval); return true;
            case 'w': sfz::args::integer_option(get_value(), &width); return true;
            case 'h': sfz::args::integer_option(get_value(), &height); return true;
            case 't': text = true; return true;
            case 's': smoke = true; return true;
            default: return false;
        }
    };

    callbacks.long_option = [&](pn::string_view                     opt,
                                const args::callbacks::get_value_f& get_value) {
        if (opt == "output") {
            return callbacks.short_option(pn::rune{'o'}, get_value);
        } else if (opt == "interval") {
            return callbacks.short_option(pn::rune{'i'}, get_value);
        } else if (opt == "width") {
            return callbacks.short_option(pn::rune{'w'}, get_value);
        } else if (opt == "height") {
            return callbacks.short_option(pn::rune{'h'}, get_value);
        } else if (opt == "text") {
            return callbacks.short_option(pn::rune{'t'}, get_value);
        } else if (opt == "smoke") {
            return callbacks.short_option(pn::rune{'s'}, get_value);
        } else if (opt == "opengl") {
            if (get_value() == "2.0") {
                gl_version   = {2, 0};
                glsl_version = "110";
            } else if (get_value() == "3.2") {
                gl_version   = {3, 2};
                glsl_version = "330 core";
            } else {
                throw std::runtime_error("invalid OpenGL version");
            }
            return true;
        } else if (opt == "modern") {
            modern_controls_in_replays = true;
            return true;
        } else if (opt == "inject") {
            inject_path.emplace(get_value().copy());
            return true;
        } else if (opt == "help") {
            usage(pn::out, sfz::path::basename(argv[0]), 0);
            return true;
        } else {
            return false;
        }
    };

    args::parse(argc - 1, argv + 1, callbacks);
    if (!replay_path.has_value()) {
        throw std::runtime_error("missing required argument 'replay'");
    }

    if (output_dir.has_value()) {
        sfz::makedirs(*output_dir, 0755);
    }

    Preferences preferences;
    preferences.play_music_in_game = true;
    // Replays play back actions, so bindings don't change the game, but tutorial messages name the
    // player's keys. Use the fire keys these replays were recorded with (Ares' Option and Command,
    // Classic's defaults until 2026-10-20) so the text matches the expected output in test/, which
    // is upstream's test data.
    preferences.bindings[kOneKeyNum].slot[kPrimarySlot] = Binding::key(Key::OPTION);
    preferences.bindings[kTwoKeyNum].slot[kPrimarySlot] = Binding::key(Key::COMMAND);
    NullPrefsDriver prefs(preferences.copy());

    EventScheduler scheduler;
    scheduler.schedule_event(unique_ptr<Event>(new MouseMoveEvent(wall_time(), Point(320, 240))));
    // TODO(sfiera): add recurring snapshots to OffscreenVideoDriver.
    for (int64_t i = 1; i < 72000; i += interval) {
        scheduler.schedule_snapshot(i);
    }

    unique_ptr<SoundDriver> sound;
    if (!smoke && output_dir.has_value()) {
        pn::string out = pn::format("{0}/sound.log", *output_dir);
        sound.reset(new LogSoundDriver(out));
    } else {
        sound.reset(new NullSoundDriver);
    }
    NullLedger ledger;

    pn::input replay_file{*replay_path, pn::binary};
    auto      master = [&]() {
        auto m = new ReplayMaster(replay_file, output_dir);
        if (inject_path.has_value()) {
            m->inject(*inject_path);
        }
        return m;
    };
    if (smoke) {
        TextVideoDriver video({width, height}, sfz::optional<pn::string>());
        video.loop(master(), scheduler);
    } else if (text) {
        TextVideoDriver video({width, height}, output_dir);
        video.loop(master(), scheduler);
    } else {
#ifndef _WIN32
        OffscreenVideoDriver video({width, height}, 1, gl_version, glsl_version, output_dir);
        video.loop(master(), scheduler);
#endif
    }
}

}  // namespace
}  // namespace antares

int main(int argc, char* const* argv) { return antares::wrap_main(antares::main, argc, argv); }
