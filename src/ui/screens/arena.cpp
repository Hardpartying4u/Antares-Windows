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
const Rect kListBounds       = {70, 125, 570, 321};  // taller than the data's box: four rows

// Where a battle sits on the difficulty ladder, from the end of its title: "Skirmish (Easy)".
// Unknown difficulties go last.
int difficulty_rank(pn::string_view title) {
    static const char* const kLadder[] = {"(Easy)", "(Medium)", "(Hard)", "(Very Hard)"};
    while (!title.empty() && ((title.data()[title.size() - 1] == '\n') ||
                              (title.data()[title.size() - 1] == ' '))) {
        title = title.substr(0, title.size() - 1);
    }
    for (int i = 0; i < 4; ++i) {
        const pn::string_view d = kLadder[i];
        if ((title.size() >= d.size()) && (title.substr(title.size() - d.size()) == d)) {
            return i;
        }
    }
    return 4;
}

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

// "Enemy: Heavy Destroyer, 4 Gunships" for the given ships, cut to one line `width` wide: ships
// that don't fit are summed up as "+N more".
pn::string ArenaScreen::fleet_line(
        pn::string_view prefix, const std::vector<std::pair<pn::string, int>>& ships, int width) {
    auto one_line = [width](pn::string_view text) {
        return GetInterfaceTextHeightFromWidth(text, InterfaceStyle::SMALL, width) <=
               GetInterfaceTextHeightFromWidth("X", InterfaceStyle::SMALL, width);
    };
    pn::string line = prefix.copy();
    int        rest = 0;
    for (const auto& s : ships) {
        rest += s.second;
    }
    for (int i = 0; i < ships.size(); ++i) {
        const auto& s    = ships[i];
        pn::string  next = pn::format(
                "{0}{1}{2}", line, (i == 0) ? "" : ", ",
                (s.second == 1) ? s.first.copy() : pn::format("{0} {1}s", s.second, s.first));
        const int  left = rest - s.second;
        pn::string with = (left > 0) ? pn::format("{0}, +{1} more", next, left) : next.copy();
        if ((left > 0) ? !one_line(with) : !one_line(next)) {
            return pn::format("{0}{1}+{2} more", line, (i == 0) ? "" : ", ", rest);
        }
        line = std::move(next);
        rest = left;
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
    remove_widget(kListRect);
    BoxRectData list;
    list.id     = kListRect;
    list.type   = WidgetDataBase::RECT;
    list.bounds = kListBounds;
    list.hue    = Hue::AQUA;
    list.style  = InterfaceStyle::SMALL;
    add_widget(WidgetData{std::move(list)});

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
        const int width = row_bounds(0).width() - 8;  // as drawn in overlay()
        _rows.push_back(Row{&l, l.base.name.copy(), fleet_line("You: ", fleet(l, human), width),
                            fleet_line("Enemy: ", fleet(l, cpu), width)});
    }
    // Easy to Very Hard; the same difficulty by title.
    std::stable_sort(_rows.begin(), _rows.end(), [](const Row& a, const Row& b) {
        const int ra = difficulty_rank(a.title), rb = difficulty_rank(b.title);
        return (ra != rb) ? (ra < rb) : (a.title < b.title);
    });
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
