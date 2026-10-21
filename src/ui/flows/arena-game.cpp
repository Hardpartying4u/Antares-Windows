// Copyright (C) 2026 The Star Bound Authors
// This file is part of Star Bound, based on Antares. Distributed under the LGPL+. See COPYING.

#include "ui/flows/arena-game.hpp"

#include "data/level.hpp"
#include "ui/screens/arena.hpp"

namespace antares {

ArenaGame::ArenaGame() : _game_result(NO_GAME) {}

ArenaGame::~ArenaGame() {}

void ArenaGame::become_front() {
    switch (_state) {
        case PICK:
            _state = PICKING;
            stack()->push(new ArenaScreen(&_cancelled, &_index, &_level));
            break;

        case PICKING:
            if (_cancelled || !_level) {
                _state = QUIT;
                stack()->pop(this);
                break;
            }
            _state       = PLAYING;
            _game_result = NO_GAME;
            stack()->push(new MainPlay(*_level, false, &_input_source, true, &_game_result));
            break;

        case PLAYING:
            switch (_game_result) {
                case RESTART_GAME:
                    _game_result = NO_GAME;
                    stack()->push(new MainPlay(*_level, false, &_input_source, true, &_game_result));
                    break;

                default:  // a win (after its debriefing), or Quit after a loss or from Esc
                    _state = PICK;
                    become_front();
                    break;
            }
            break;

        case QUIT: stack()->pop(this); break;
    }
}

}  // namespace antares
