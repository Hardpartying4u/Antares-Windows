// Copyright (C) 2026 The Star Bound Authors
// This file is part of Star Bound, based on Antares. Distributed under the LGPL+. See COPYING.

#include "ui/screens/arena.hpp"

#include <algorithm>
#include <map>
#include <pn/output>

#include "data/base-object.hpp"
#include "data/initial.hpp"
#include "data/interface.hpp"
#include "data/level.hpp"
#include "data/plugin.hpp"
#include "data/resource.hpp"
#include "drawing/color.hpp"
#include "drawing/interface.hpp"
#include "game/sys.hpp"
#include "sound/fx.hpp"
#include "video/driver.hpp"
#include "video/transitions.hpp"

namespace antares {

namespace {

const char kBeginButton[]    = "begin";
const char kCancelButton[]   = "cancel";
const char kListRect[]       = "rect";          // the data's level-name box: the battle list
const char kFrameRect[]      = "select-level";  // the data's "Select Level" box, relabelled
const int  kRowHeight        = 44;  // a title line and two fleet lines
const int  kRowPad           = 4;
const Rect kFrameBounds      = {54, 105, 587, 346};

// Each kind of ship in `owner`'s fleet with how many there are, in the order they first appear,
// by the ship's long name ("Cruiser"); the flagship first.
std::vector<std::pair<pn::string, int>> fleet(const Level& level, int owner) {
    std::vector<std::pair<pn::string, int>> ships;
    for (bool flagship : {true, false}) {
        for (const Initial& i : level.base.initials) {
            if (!i.owner.has_value() || (i.owner->number() != owner) ||
                (i.flagship.value_or(false) != flagship)) {
                continue;
            }
            pn::string name = i.base.name.copy();
            try {
                name = Resource::object(i.base.name).long_name.copy();
            } catch (...) {
            }
            auto it = std::find_if(ships.begin(), ships.end(), [&](const auto& s) {
                return s.first == name;
            });
            if (it == ships.end()) {
                ships.emplace_back(std::move(name), 1);
            } else {
                ++it->second;
            }
        }
    }
    return ships;
}

}  // namespace

pn::string ArenaScreen::fleet_line(const std::vector<std::pair<pn::string, int>>& ships) {
    pn::string line;
    for (const auto& s : ships) {
        line += pn::format(
                "{0}{1}", line.empty() ? "" : ", ",
                (s.second == 1) ? s.first.copy() : pn::format("{0} {1}s", s.second, s.first));
    }
    return line;
}

ArenaScreen::ArenaScreen(bool* cancelled, int* index, const Level** level)
        : InterfaceScreen("select-level", {0, 0, 640, 480}),
          _cancelled(cancelled),
          _index(index),
          _level(level) {
    remove_widget("previous");
    remove_widget("next");
    remove_widget(kFrameRect);
    BoxRectData frame;
    frame.id     = kFrameRect;
    frame.type   = WidgetDataBase::RECT;
    frame.bounds = kFrameBounds;
    frame.label.emplace("Arena");
    frame.hue = Hue::AQUA;
    add_widget(WidgetData{std::move(frame)});

    for (const auto& kv : plug.arena) {
        const Level& l = kv.second;
        int          human = 0, cpu = 1;
        for (int p = 0; p < l.solo.players.size(); ++p) {
            if (l.solo.players[p].type == LevelBase::PlayerType::HUMAN) {
                human = p;
            } else {
                cpu = p;
            }
        }
        _rows.push_back(Row{&l, l.base.name.copy(),
                            pn::format("You: {0}", fleet_line(fleet(l, human))),
                            pn::format("Enemy: {0}", fleet_line(fleet(l, cpu)))});
    }
    *_index = std::max(0, std::min<int>(*_index, int(_rows.size()) - 1));
    *_level = _rows.empty() ? nullptr : _rows[*_index].level;

    button(kBeginButton)
            ->bind({
                    [this] {
                        _state      = FADING_OUT;
                        *_cancelled = false;
                        stack()->push(new ColorFade(
                                ColorFade::TO_COLOR, RgbColor::black(), secs(1), false, NULL));
                    },
                    [this] { return !_rows.empty(); },
            });
    button(kCancelButton)->bind({[this] {
        *_cancelled = true;
        stack()->pop(this);
    }});
}

void ArenaScreen::become_front() {
    switch (_state) {
        case SELECTING: InterfaceScreen::become_front(); break;
        case FADING_OUT: stack()->pop(this); break;
    }
}

void ArenaScreen::select(int index) {
    if ((index < 0) || (index >= _rows.size()) || (index == *_index)) {
        return;
    }
    *_index = index;
    *_level = _rows[index].level;
    sys.sound.select();
}

Rect ArenaScreen::row_bounds(int index) const {
    Rect r = widget(kListRect)->inner_bounds();
    r.inset(kRowPad, kRowPad);
    r.top += index * kRowHeight;
    r.bottom = r.top + kRowHeight - 2;
    const Point off = offset();
    r.offset(off.h, off.v);
    return r;
}

void ArenaScreen::key_down(const KeyDownEvent& event) {
    if (_state == SELECTING) {
        switch (event.key().value()) {
            case Key::UP_ARROW: select(*_index - 1); return;
            case Key::DOWN_ARROW: select(*_index + 1); return;
            default: break;
        }
    }
    InterfaceScreen::key_down(event);
}

void ArenaScreen::gamepad_button_down(const GamepadButtonDownEvent& event) {
    if (_state == SELECTING) {
        switch (event.button) {
            case Gamepad::Button::UP: select(*_index - 1); return;
            case Gamepad::Button::DOWN: select(*_index + 1); return;
            default: break;
        }
    }
    InterfaceScreen::gamepad_button_down(event);
}

void ArenaScreen::mouse_down(const MouseDownEvent& event) {
    if (_state == SELECTING) {
        for (int i = 0; i < _rows.size(); ++i) {
            if (row_bounds(i).contains(event.where())) {
                select(i);
                return;
            }
        }
    }
    InterfaceScreen::mouse_down(event);
}

// Each row: the battle's title, then both fleets; the selected row on a dark aqua bar.
void ArenaScreen::overlay() const {
    if (_rows.empty()) {
        draw_text_in_rect(
                row_bounds(0), "No arena battles found.", InterfaceStyle::LARGE, Hue::AQUA);
        return;
    }
    const Rect list = widget(kListRect)->inner_bounds();
    for (int i = 0; i < _rows.size(); ++i) {
        Rect r = row_bounds(i);
        if (r.bottom > list.bottom + offset().v) {
            break;  // more battles than fit: a later step adds scrolling
        }
        const bool selected = (i == *_index);
        if (selected) {
            Rects().fill(r, GetRGBTranslateColorShade(Hue::AQUA, DARKER));
        }
        Rect line = r;
        line.inset(4, 2);
        line.bottom = line.top + 13;
        draw_text_in_rect(
                line, _rows[i].title, InterfaceStyle::LARGE,
                selected ? Hue::GOLD : Hue::AQUA);
        line.offset(0, 14);
        draw_text_in_rect(line, _rows[i].ours, InterfaceStyle::SMALL, Hue::PALE_GREEN);
        line.offset(0, 13);
        draw_text_in_rect(line, _rows[i].theirs, InterfaceStyle::SMALL, Hue::SALMON);
    }
}

}  // namespace antares
