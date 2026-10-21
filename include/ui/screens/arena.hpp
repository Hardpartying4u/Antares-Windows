// Copyright (C) 2026 The Star Bound Authors
// This file is part of Star Bound, based on Antares. Distributed under the LGPL+. See COPYING.

#ifndef ANTARES_UI_SCREENS_ARENA_HPP_
#define ANTARES_UI_SCREENS_ARENA_HPP_

#include <pn/string>
#include <vector>

#include "ui/screen.hpp"

namespace antares {

union Level;

// The Arena battle picker (docs/arena-investigation.md §4): one row per battle in plug.arena, its
// title and both fleets, ordered Easy to Very Hard by the difficulty ending its title. Up/Down
// (or the gamepad's d-pad, or a click) picks a row; Begin (Return, gamepad A) starts it; Cancel
// (Esc, gamepad B) goes back. Built in code on the campaign's
// level-select frame (data/interfaces/select-level.pn), whose Previous/Next buttons and "Select
// Level" box are removed, so data/ is never edited.
class ArenaScreen : public InterfaceScreen {
  public:
    // `index` is the selected row, kept by the caller so the picker reopens on the last battle.
    ArenaScreen(bool* cancelled, int* index, const Level** level);

    void become_front() override;
    void key_down(const KeyDownEvent& event) override;
    void mouse_down(const MouseDownEvent& event) override;
    void gamepad_button_down(const GamepadButtonDownEvent& event) override;

    // `prefix` and the ships ("4 Cruisers"), cut to one line of small text `width` wide: ships that
    // don't fit end the line as "+N more".
    static pn::string fleet_line(
            pn::string_view prefix, const std::vector<std::pair<pn::string, int>>& ships,
            int width);

  protected:
    void overlay() const override;

  private:
    struct Row {
        const Level* level;
        pn::string   title;
        pn::string   ours;
        pn::string   theirs;
    };

    void select(int index);
    Rect row_bounds(int index) const;  // in screen coordinates

    enum State { SELECTING, FADING_OUT };
    State            _state = SELECTING;
    bool*            _cancelled;
    int*             _index;
    const Level**    _level;
    std::vector<Row> _rows;
};

}  // namespace antares

#endif  // ANTARES_UI_SCREENS_ARENA_HPP_
