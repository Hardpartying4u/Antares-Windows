// Copyright (C) 2026 The Star Bound Authors
// This file is part of Star Bound, based on Antares. Distributed under the LGPL+. See COPYING.

#ifndef ANTARES_UI_FLOWS_ARENA_GAME_HPP_
#define ANTARES_UI_FLOWS_ARENA_GAME_HPP_

#include "game/input-source.hpp"
#include "game/main.hpp"
#include "ui/card.hpp"

namespace antares {

// Arena (docs/arena-investigation.md §4): the battle picker, then the battle, then back to the
// picker; Cancel on the picker returns to the main menu. A battle is a normal solo level played
// with MainPlay, so the control preset, the Modern mode and the group bar apply as in the
// campaign, and the game's own result screens are used:
//   * a win: its message and the debriefing table (time, losses, kills, score against par);
//   * a loss: its message, then Restart / Quit;
//   * Esc in battle: Resume / Restart / Quit.
// Restart replays the battle; Quit and a finished win return to the picker. Nothing is saved:
// no campaign progress, no unlocked chapters.
class ArenaGame : public Card {
  public:
    ArenaGame();
    ~ArenaGame();

    void become_front() override;

  private:
    enum State { PICK, PICKING, PLAYING, QUIT };
    State _state = PICK;

    bool            _cancelled = false;
    int             _index     = 0;
    const Level*    _level     = nullptr;
    GameResult      _game_result;
    RealInputSource _input_source;
};

}  // namespace antares

#endif  // ANTARES_UI_FLOWS_ARENA_GAME_HPP_
