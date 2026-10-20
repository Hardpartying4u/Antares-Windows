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

#include "game/player-ship.hpp"

#include <algorithm>
#include <pn/output>
#include <sfz/sfz.hpp>

#include "config/gamepad.hpp"
#include "config/keys.hpp"
#include "config/preferences.hpp"
#include "data/base-object.hpp"
#include "data/resource.hpp"
#include "drawing/color.hpp"
#include "drawing/text.hpp"
#include "game/admiral.hpp"
#include "game/cheat.hpp"
#include "game/cursor.hpp"
#include "game/globals.hpp"
#include "game/input-source.hpp"
#include "game/instruments.hpp"
#include "game/labels.hpp"
#include "game/level.hpp"
#include "game/messages.hpp"
#include "game/minicomputer.hpp"
#include "game/modern-orders.hpp"
#include "game/motion.hpp"
#include "game/non-player-ship.hpp"
#include "game/space-object.hpp"
#include "game/starfield.hpp"
#include "game/sys.hpp"
#include "game/time.hpp"
#include "lang/defines.hpp"
#include "math/fixed.hpp"
#include "math/macros.hpp"
#include "math/rotation.hpp"
#include "math/special.hpp"
#include "math/units.hpp"
#include "sound/fx.hpp"

namespace antares {

namespace {

// const int32_t kSendMessageVOffset = 20;
const int32_t kCursorBoundsSize = 16;  // should be same in instruments.c

}  // namespace

const static ticks kKlaxonInterval      = ticks(125);
const static ticks kDestKeyHoldDuration = ticks(45);
const static ticks kHotKeyHoldDuration  = ticks(51);  // Compatibility

int32_t HotKey_GetFromObject(Handle<SpaceObject> object);
void    Update_LabelStrings_ForHotKeyChange(void);

namespace {

enum DestKeyState {
    DEST_KEY_UP,       // up
    DEST_KEY_DOWN,     // down, and possibly usable for self-selection
    DEST_KEY_BLOCKED,  // down, but used for something else
};

enum HotKeyState {
    HOT_KEY_UP,
    HOT_KEY_SELECT,
    HOT_KEY_TARGET,
};

static ANTARES_GLOBAL DestKeyState gDestKeyState = DEST_KEY_UP;
static ANTARES_GLOBAL wall_time    gDestKeyTime;

static ANTARES_GLOBAL HotKeyState gHotKeyState[10];
static ANTARES_GLOBAL wall_time   gHotKeyTime[10];

static ANTARES_GLOBAL Zoom gPreviousZoomMode;

pn::string name_with_hot_key_suffix(Handle<SpaceObject> space_object) {
    int h = HotKey_GetFromObject(space_object);
    if (h < 0) {
        return space_object->long_name().copy();
    }

    Key keyNum = sys.prefs->key(h + kFirstHotKeyNum);
    if (keyNum == Key::NONE) {
        return space_object->long_name().copy();
    }

    return pn::format("{0} < {1} >", space_object->long_name(), keyNum.long_name());
};

}  // namespace

bool PlayerEvent::operator==(PlayerEvent other) const {
    return (type == other.type) && (data == other.data);
}

bool PlayerEvent::operator<(PlayerEvent other) const {
    return (type != other.type) ? (type < other.type) : (data < other.data);
}

void ResetPlayerShip() {
    g.control_label = Label::add(0, 0, 0, 10, SpaceObject::none(), true, Hue::YELLOW);
    g.target_label  = Label::add(0, 0, 0, -20, SpaceObject::none(), true, Hue::SKY_BLUE);
    g.send_label    = Label::add(200, 200, 0, 30, SpaceObject::none(), false, Hue::GREEN);
    globals()->starfield.reset();
    globals()->next_klaxon = game_ticks();
    g.key_mask             = 0;
    g.zoom                 = Zoom::FOE;
    gPreviousZoomMode      = Zoom::FOE;

    for (int h = 0; h < kHotKeyNum; h++) {
        globals()->hotKey[h].object   = SpaceObject::none();
        globals()->hotKey[h].objectID = -1;
    }
    for (auto& k : gHotKeyState) {
        k = HOT_KEY_UP;
    }
    gDestKeyState = DEST_KEY_UP;
}

PlayerShip::PlayerShip()
        : gTheseKeys(0),
          _gamepad_state(NO_BUMPER),
          _control_active(false),
          _control_direction(0) {}

static void zoom_to(Zoom zoom) {
    if (g.zoom != zoom) {
        g.zoom = zoom;
        sys.sound.click();
        Messages::zoom(g.zoom);
    }
}

static void zoom_shortcut(Zoom zoom) {
    if (g.key_mask & kShortcutZoomMask) {
        return;
    }
    Zoom previous     = gPreviousZoomMode;
    gPreviousZoomMode = g.zoom;
    if (g.zoom == zoom) {
        zoom_to(previous);
    } else {
        zoom_to(zoom);
    }
}

Zoom wheel_zoom(Zoom current, bool in) {
    if (current > Zoom::SIXTEENTH) {
        return in ? Zoom::SIXTEENTH : current;
    } else if (in) {
        return (current > Zoom::DOUBLE) ? static_cast<Zoom>(static_cast<int>(current) - 1) : current;
    }
    return (current < Zoom::SIXTEENTH) ? static_cast<Zoom>(static_cast<int>(current) + 1) : current;
}

// `wheel`: the step came from the mouse wheel (see wheel_zoom()), not a zoom key.
static void zoom_in(bool wheel) {
    if (g.key_mask & kZoomInKey) {
        return;
    }
    if (wheel) {
        zoom_to(wheel_zoom(g.zoom, true));
    } else if (g.zoom > Zoom::DOUBLE) {
        zoom_to(static_cast<Zoom>(static_cast<int>(g.zoom) - 1));
    }
}

static void zoom_out(bool wheel) {
    if (g.key_mask & kZoomOutKey) {
        return;
    }
    if (wheel) {
        zoom_to(wheel_zoom(g.zoom, false));
    } else if (g.zoom < Zoom::ALL) {
        zoom_to(static_cast<Zoom>(static_cast<int>(g.zoom) + 1));
    }
}

static void engage_autopilot() {
    auto player = g.ship;
    if (!(player->attributes & kOnAutoPilot)) {
        player->keysDown |= kAutoPilotKey;
    }
    player->keysDown |= kAdoptTargetKey;
}

static void select_object(Handle<SpaceObject> ship, bool target, Handle<Admiral> adm) {
    Handle<SpaceObject> flagship = adm->flagship();
    Handle<Label>       label;
    Hue                 hue;

    if (adm == g.admiral) {
        globals()->lastSelectedObject   = ship;
        globals()->lastSelectedObjectID = ship->id;
    }
    if (target) {
        adm->set_target(ship);
        label = g.target_label;
        hue   = Hue::SKY_BLUE;

        if (!(flagship->attributes & kOnAutoPilot)) {
            SetObjectDestination(flagship);
        }
    } else {
        adm->set_control(ship);
        label = g.control_label;
        hue   = Hue::YELLOW;
    }

    if (adm == g.admiral) {
        sys.sound.select();
        label->set_object(ship);
        if (ship == g.ship) {
            label->set_age(Label::kVisibleTime);
        }
        label->text() = StyledText::plain(
                name_with_hot_key_suffix(ship), sys.fonts.tactical,
                GetRGBTranslateColorShade(hue, LIGHTEST));
    }
}

static void pick_object(
        Handle<SpaceObject> origin_ship, int32_t direction, bool target, int32_t attributes,
        int32_t nonattributes, Handle<SpaceObject> select_ship, Allegiance allegiance) {
    uint64_t huge_distance;
    if (select_ship.get()) {
        uint32_t difference = ABS<int>(origin_ship->location.h - select_ship->location.h);
        uint32_t dcalc      = difference;
        difference          = ABS<int>(origin_ship->location.v - select_ship->location.v);
        uint32_t distance   = difference;

        if ((dcalc > kMaximumRelevantDistance) || (distance > kMaximumRelevantDistance)) {
            huge_distance = dcalc;  // must be positive
            MyWideMul(huge_distance, huge_distance, &huge_distance);
            select_ship->distanceFromPlayer = distance;
            MyWideMul(
                    select_ship->distanceFromPlayer, select_ship->distanceFromPlayer,
                    &select_ship->distanceFromPlayer);
            select_ship->distanceFromPlayer += huge_distance;
        } else {
            select_ship->distanceFromPlayer = distance * distance + dcalc * dcalc;
        }
        huge_distance = select_ship->distanceFromPlayer;
    } else {
        huge_distance = 0;
    }

    select_ship = GetManualSelectObject(
            origin_ship, direction, attributes, nonattributes, &huge_distance, select_ship,
            allegiance);

    if (select_ship.get()) {
        select_object(select_ship, target, g.admiral);
    }
}

static void select_friendly(Handle<SpaceObject> origin_ship, int32_t direction) {
    pick_object(
            origin_ship, direction, false, kCanBeDestination, kIsDestination, g.admiral->control(),
            FRIENDLY);
}

static void target_friendly(Handle<SpaceObject> origin_ship, int32_t direction) {
    pick_object(
            origin_ship, direction, true, kCanBeDestination, kIsDestination, g.admiral->target(),
            FRIENDLY);
}

static void target_hostile(Handle<SpaceObject> origin_ship, int32_t direction) {
    pick_object(
            origin_ship, direction, true, kCanBeDestination, kIsDestination, g.admiral->target(),
            HOSTILE);
}

static void select_base(Handle<SpaceObject> origin_ship, int32_t direction) {
    pick_object(origin_ship, direction, false, kIsDestination, 0, g.admiral->control(), FRIENDLY);
}

static void target_base(Handle<SpaceObject> origin_ship, int32_t direction) {
    pick_object(
            origin_ship, direction, true, kIsDestination, 0, g.admiral->target(),
            FRIENDLY_OR_HOSTILE);
}

static void target_self() { select_object(g.ship, true, g.admiral); }

static bool use_target_key() {
    if (gDestKeyState == DEST_KEY_DOWN) {
        gDestKeyState = DEST_KEY_BLOCKED;
    }
    return gDestKeyState == DEST_KEY_BLOCKED;
}

static void hot_key_down(int i) {
    gHotKeyTime[i] = now();
    if (gDestKeyState == DEST_KEY_UP) {
        gHotKeyState[i] = HOT_KEY_SELECT;
    } else {
        gHotKeyState[i] = HOT_KEY_TARGET;
    }
}

static PlayerEvent hot_key_up(int i) {
    bool target     = (gHotKeyState[i] == HOT_KEY_TARGET);
    gHotKeyState[i] = HOT_KEY_UP;
    if (now() >= gHotKeyTime[i] + kHotKeyHoldDuration) {
        return PlayerEvent{PlayerEventType::HOTKEY_SET, i};
    } else if (use_target_key() || target) {
        return PlayerEvent{PlayerEventType::HOTKEY_TARGET, i};
    } else {
        return PlayerEvent{PlayerEventType::HOTKEY_SELECT, i};
    }
}

// An input pressed: runs every action bound to it. Inputs are remembered while held, so a key the
// system repeats doesn't press again, and its release always reaches the actions it pressed.
void PlayerShip::input_down(const Binding& b) {
    if (std::find(_pressed.begin(), _pressed.end(), b) != _pressed.end()) {
        return;
    }
    _pressed.push_back(b);
    for (int a : sys.prefs->actions_for(b)) {
        press(a);
    }
}

void PlayerShip::input_up(const Binding& b) {
    auto it = std::find(_pressed.begin(), _pressed.end(), b);
    if (it == _pressed.end()) {
        return;
    }
    _pressed.erase(it);
    for (int a : sys.prefs->actions_for(b)) {
        release(a);
    }
}

// Each action counts how many of its inputs are held: it starts on the first press and ends on the
// last release, so holding thrust on both a key and the gamepad and letting go of one keeps thrust.
void PlayerShip::press(int action) {
    if ((action < 0) || (action >= ACTION_COUNT)) {
        return;
    }
    if (_held[action]++ == 0) {
        action_on(action);
    }
}

void PlayerShip::release(int action) {
    if ((action < 0) || (action >= ACTION_COUNT) || (_held[action] == 0)) {
        return;
    }
    if ((--_held[action] == 0) && active()) {
        action_off(action);
    }
}

void PlayerShip::action_on(int action) {
    PlayerEventType k;
    switch (action) {
        case kUpKeyNum: k = PlayerEventType::ACCEL_ON; break;
        case kDownKeyNum: k = PlayerEventType::DECEL_ON; break;
        case kLeftKeyNum: k = PlayerEventType::CCW_ON; break;
        case kRightKeyNum: k = PlayerEventType::CW_ON; break;
        case kOneKeyNum: k = PlayerEventType::FIRE_1_ON; break;
        case kTwoKeyNum: k = PlayerEventType::FIRE_2_ON; break;
        case kEnterKeyNum: k = PlayerEventType::FIRE_S_ON; break;

        case kWarpKeyNum:
            k = use_target_key() ? PlayerEventType::AUTOPILOT : PlayerEventType::WARP_ON;
            break;

        case kDestinationKeyNum:
            gDestKeyState = DEST_KEY_DOWN;
            gDestKeyTime  = now();
            return;

        case kSelectFriendKeyNum:
            k = use_target_key() ? PlayerEventType::TARGET_FRIEND : PlayerEventType::SELECT_FRIEND;
            break;
        case kSelectFoeKeyNum:
            use_target_key();
            k = PlayerEventType::TARGET_FOE;
            break;
        case kSelectBaseKeyNum:
            k = use_target_key() ? PlayerEventType::TARGET_BASE : PlayerEventType::SELECT_BASE;
            break;

        case kOrderKeyNum:
            if (g.modern_controls) {
                // Modern: on release, unless a number is pressed meanwhile (Ctrl is both the
                // Order key in some layouts and the group modifier).
                _order_held = true;
                _order_used = false;
                return;
            }
            k = PlayerEventType::ORDER;
            break;
        case kTransferKeyNum: k = PlayerEventType::TRANSFER; break;

        case kCompUpKeyNum:
        case kCompDownKeyNum:
        case kCompAcceptKeyNum:
        case kCompCancelKeyNum:
            minicomputer_interpret_key_down(static_cast<KeyNum>(action), &_player_events);
            return;

        case kZoomInKeyNum: k = PlayerEventType::ZOOM_IN; break;
        case kZoomOutKeyNum: k = PlayerEventType::ZOOM_OUT; break;
        case kScale121KeyNum: k = PlayerEventType::ZOOM_1X; break;
        case kScale122KeyNum: k = PlayerEventType::ZOOM_2X; break;
        case kScale124KeyNum: k = PlayerEventType::ZOOM_4X; break;
        case kScale1216KeyNum: k = PlayerEventType::ZOOM_16X; break;
        case kScaleHostileKeyNum: k = PlayerEventType::ZOOM_FOE; break;
        case kScaleObjectKeyNum: k = PlayerEventType::ZOOM_OBJ; break;
        case kScaleAllKeyNum: k = PlayerEventType::ZOOM_ALL; break;
        case kMessageNextKeyNum: k = PlayerEventType::NEXT_PAGE; break;

        // Handled by GamePlay before input reaches the ship.
        case kHelpKeyNum:
        case kVolumeDownKeyNum:
        case kVolumeUpKeyNum:
        case kActionMusicKeyNum:
        case kNetSettingsKeyNum:
        case kFastMotionKeyNum: return;

        case kHotKey1Num:
        case kHotKey2Num:
        case kHotKey3Num:
        case kHotKey4Num:
        case kHotKey5Num:
        case kHotKey6Num:
        case kHotKey7Num:
        case kHotKey8Num:
        case kHotKey9Num:
        case kHotKey10Num:
            if (g.modern_controls) {
                modern_group_key(action - kFirstHotKeyNum);  // control groups, not hotkeys
            } else {
                hot_key_down(action - kFirstHotKeyNum);
            }
            return;

        // Modern commands: bindable now, given behaviour with the Modern mode
        // (docs/modern-controls.md). A level that disables orders (the "order" key mask) will
        // disable them too.
        case kAttackMoveKeyNum:
        case kPatrolKeyNum: return;

        default: return;
    }
    _player_events.push_back(PlayerEvent{k});
}

void PlayerShip::action_off(int action) {
    PlayerEventType k;
    switch (action) {
        case kUpKeyNum: k = PlayerEventType::ACCEL_OFF; break;
        case kDownKeyNum: k = PlayerEventType::DECEL_OFF; break;
        case kLeftKeyNum: k = PlayerEventType::CCW_OFF; break;
        case kRightKeyNum: k = PlayerEventType::CW_OFF; break;
        case kOneKeyNum: k = PlayerEventType::FIRE_1_OFF; break;
        case kTwoKeyNum: k = PlayerEventType::FIRE_2_OFF; break;
        case kEnterKeyNum: k = PlayerEventType::FIRE_S_OFF; break;
        case kWarpKeyNum: k = PlayerEventType::WARP_OFF; break;

        case kDestinationKeyNum:
            if ((now() >= (gDestKeyTime + kDestKeyHoldDuration) &&
                 (gDestKeyState == DEST_KEY_DOWN))) {
                gDestKeyState = DEST_KEY_UP;
                k             = PlayerEventType::TARGET_SELF;
            } else {
                gDestKeyState = DEST_KEY_UP;
                return;
            }
            break;

        case kCompUpKeyNum:
        case kCompDownKeyNum:
        case kCompAcceptKeyNum:
        case kCompCancelKeyNum:
            minicomputer_interpret_key_up(static_cast<KeyNum>(action), &_player_events);
            return;

        case kHotKey1Num:
        case kHotKey2Num:
        case kHotKey3Num:
        case kHotKey4Num:
        case kHotKey5Num:
        case kHotKey6Num:
        case kHotKey7Num:
        case kHotKey8Num:
        case kHotKey9Num:
        case kHotKey10Num:
            if (!g.modern_controls) {
                _player_events.push_back(hot_key_up(action - kFirstHotKeyNum));
            }
            return;

        case kOrderKeyNum:
            if (g.modern_controls && _order_held) {
                _order_held = false;
                if (!_order_used) {
                    _player_events.push_back(PlayerEvent{PlayerEventType::ORDER});
                }
            }
            return;

        default: return;
    }
    _player_events.push_back(PlayerEvent{k});
}

void PlayerShip::key_down(const KeyDownEvent& event) {
    if ((event.key() == Key::CONTROL) || (event.key() == Key::R_CONTROL)) {
        _ctrl_held = true;
    }
    if (event.key() == Key::RETURN) {
        _message.start_editing();
        return;
    }

    if (!active()) {
        return;
    }
    input_down(Binding::key(event.key()));
}

void PlayerShip::key_up(const KeyUpEvent& event) {
    if ((event.key() == Key::CONTROL) || (event.key() == Key::R_CONTROL)) {
        _ctrl_held = false;
    }
    input_up(Binding::key(event.key()));
}

// Replays (and only replays) send actions directly.
void PlayerShip::action_down(const ActionDownEvent& event) {
    if (active()) {
        press(event.action());
    }
}

void PlayerShip::action_up(const ActionUpEvent& event) { release(event.action()); }

void PlayerShip::mouse_down(const MouseDownEvent& event) {
    _cursor.mouse_down(event);

    Point where = event.where();
    switch (event.button()) {
        case 0:
            if (event.count() == 2) {
                MiniComputerHandleDoubleClick(where, &_player_events);
            } else if (event.count() == 1) {
                MiniComputerHandleClick(where);
            }
            if (g.modern_controls && viewport().contains(where) && !(g.key_mask & kMouseMask)) {
                modern_left_down(where);
                return;
            }
            break;
        case 1:
            if (g.modern_controls) {
                // Modern: target what is under the cursor and order the selected ship there.
                modern_right_click(where);
                return;
            }
            break;
        default:
            // Left and right select and target; other buttons can be bound to actions.
            if (active()) {
                input_down(Binding::mouse(event.button()));
            }
            return;
    }
    PlayerShipHandleClick(where, event.button());
}

void PlayerShip::mouse_up(const MouseUpEvent& event) {
    _cursor.mouse_up(event);

    Point where = event.where();
    if (event.button() == 0) {
        MiniComputerHandleMouseStillDown(where);
        MiniComputerHandleMouseUp(where, &_player_events);
        if (_dragging) {
            modern_left_up(where);
        }
    } else if (event.button() > 1) {
        input_up(Binding::mouse(event.button()));
    }
}

void PlayerShip::mouse_move(const MouseMoveEvent& event) { _cursor.mouse_move(event); }

// Modern left button (docs/modern-controls.md §5.2). Down: Shift+click on one of the player's ships
// adds it to the selection; a plain click selects as Classic does (a ship replaces the selection,
// empty space changes nothing). Either way the button may start a drag box.
void PlayerShip::modern_left_down(Point where) {
    const bool shift = (gDestKeyState != DEST_KEY_UP);
    if (shift) {
        use_target_key();  // Shift is a modifier here: no "target own ship" on its release
        Rect bounds{where.h - kCursorBoundsSize, where.v - kCursorBoundsSize,
                    where.h + kCursorBoundsSize, where.v + kCursorBoundsSize};
        auto ship = GetSpritePointSelectObject(
                &bounds, g.ship, kCanBeDestination, g.admiral->control(), FRIENDLY);
        if (ship.get()) {
            const bool first = modern_selection().empty();
            if (first) {
                select_object(ship, false, g.admiral);
            }
            modern_select({ship}, true);
            modern_selection_matches_control();
        }
    } else {
        PlayerShipHandleClick(where, 0);
    }
    _dragging   = true;
    _drag_start = where;
    modern_drag(true, where);
}

// A number key in the Modern mode (docs/modern-controls.md §5.1): Ctrl+number saves the selection
// as that group; the number selects the group (its ships that can still take orders; an empty
// group warns and changes nothing); Shift+number adds the group to the selection. The same number
// twice within 0.5 s also shows a pointer to the group (view only).
void PlayerShip::modern_group_key(int group) {
    _order_used = true;  // a number while the Order key is held: Ctrl+number, not an order
    if (_ctrl_held) {
        modern_group_save(group);
        sys.sound.select();
        return;
    }
    const bool shift = (gDestKeyState != DEST_KEY_UP);
    auto       ships = modern_group_ships(group);
    if (ships.empty()) {
        sys.sound.warning();
        return;
    }
    if (shift) {
        use_target_key();  // Shift is a modifier here: no "target own ship" on its release
        if (modern_selection().empty()) {
            select_object(ships[0], false, g.admiral);
        }
        modern_select(ships, true);
    } else {
        select_object(ships[0], false, g.admiral);
        modern_select(ships, false);
    }
    modern_selection_matches_control();
    if ((group == _last_group) && (now() < _last_group_time + std::chrono::milliseconds(500))) {
        modern_group_pointer(group);
    }
    _last_group      = group;
    _last_group_time = now();
}

// Up: if the mouse moved more than a few pixels, the player's orderable ships inside the box are
// selected (added with Shift). A box with none in it changes nothing.
void PlayerShip::modern_left_up(Point where) {
    _dragging = false;
    modern_drag(false, where);
    if ((std::abs(where.h - _drag_start.h) <= kDragThreshold) &&
        (std::abs(where.v - _drag_start.v) <= kDragThreshold)) {
        return;
    }
    const bool shift = (gDestKeyState != DEST_KEY_UP);
    auto       ships = modern_ships_in_box(_drag_start, where);
    if (ships.empty()) {
        return;
    }
    if (!shift || modern_selection().empty()) {
        select_object(ships[0], false, g.admiral);  // the first becomes the control object
    }
    modern_select(ships, shift);
    modern_selection_matches_control();
}

// Each notch runs the actions bound to its direction. On a zoom action a notch is a zoom step (data
// 1 marks it as the wheel's: it doesn't step into the automatic zooms); on any other action it is a
// press and release. It counts as that action, not as a mouse click: the masks levels set for the
// action apply when it runs (tutorial 1 enables zooming while the mouse is still off).
void PlayerShip::mouse_wheel(const MouseWheelEvent& event) {
    if (!active() || (event.steps() == 0)) {
        return;
    }
    const std::vector<int> actions = sys.prefs->actions_for(Binding::wheel(event.steps()));
    for (int i = 0; i < std::abs(event.steps()); ++i) {
        for (int a : actions) {
            if (a == kZoomInKeyNum) {
                _player_events.push_back(PlayerEvent{PlayerEventType::ZOOM_IN, 1});
            } else if (a == kZoomOutKeyNum) {
                _player_events.push_back(PlayerEvent{PlayerEventType::ZOOM_OUT, 1});
            } else {
                press(a);
                release(a);
            }
        }
    }
}

void PlayerShip::gamepad_button_down(const GamepadButtonDownEvent& event) {
    switch (event.button) {
        case Gamepad::Button::LB:
            if (_gamepad_state & SELECT_BUMPER) {
                _gamepad_state = TARGET_BUMPER_OVERRIDE;
            } else if (!(_gamepad_state & TARGET_BUMPER)) {
                _gamepad_state = TARGET_BUMPER;
            }
            return;
        case Gamepad::Button::RB:
            if (_gamepad_state & TARGET_BUMPER) {
                _gamepad_state = SELECT_BUMPER_OVERRIDE;
            } else if (!(_gamepad_state & SELECT_BUMPER)) {
                _gamepad_state = SELECT_BUMPER;
            }
            return;
        default: break;
    }

    if (!active()) {
        return;
    }

    // With a bumper held, A, B, X, Y and the left stick button give the bumper commands, aimed
    // with the left stick. These are fixed.
    auto player = g.ship;
    if (_gamepad_state) {
        switch (event.button) {
            case Gamepad::Button::A:
                if (_control_active) {
                    if (_gamepad_state & SELECT_BUMPER) {
                        select_friendly(player, _control_direction);
                    } else {
                        target_friendly(player, _control_direction);
                    }
                }
                return;
            case Gamepad::Button::B:
                if (_control_active) {
                    if (_gamepad_state & TARGET_BUMPER) {
                        target_hostile(player, _control_direction);
                    }
                }
                return;
            case Gamepad::Button::X:
                if (_control_active) {
                    if (_gamepad_state & SELECT_BUMPER) {
                        select_base(player, _control_direction);
                    } else {
                        target_base(player, _control_direction);
                    }
                }
                return;
            case Gamepad::Button::Y:
                if (_gamepad_state & SELECT_BUMPER) {
                    _player_events.push_back(PlayerEvent{PlayerEventType::ORDER});
                } else {
                    _player_events.push_back(PlayerEvent{PlayerEventType::AUTOPILOT});
                }
                return;
            case Gamepad::Button::LSB:
                if (_gamepad_state & TARGET_BUMPER) {
                    target_self();
                } else {
                    transfer_control(g.admiral);
                }
                return;
            default: break;
        }
    }

    // Otherwise the button runs the actions bound to it. A gamepad button bound to warp toggles it,
    // as Ares' gamepad did: held until the warp starts, it then stays on until pressed again.
    const Binding b = Binding::gamepad(event.button);
    if ((player->presenceState == kWarpingPresence) && (_held[kWarpKeyNum] > 0)) {
        for (int a : sys.prefs->actions_for(b)) {
            if (a == kWarpKeyNum) {
                _held[kWarpKeyNum] = 1;
                release(kWarpKeyNum);
                return;
            }
        }
    }
    input_down(b);
}

void PlayerShip::gamepad_button_up(const GamepadButtonUpEvent& event) {
    switch (event.button) {
        case Gamepad::Button::LB:
            if (_gamepad_state & OVERRIDE) {
                _gamepad_state = SELECT_BUMPER;
            } else {
                _gamepad_state = NO_BUMPER;
            }
            return;
        case Gamepad::Button::RB:
            if (_gamepad_state & OVERRIDE) {
                _gamepad_state = TARGET_BUMPER;
            } else {
                _gamepad_state = NO_BUMPER;
            }
            return;
        default: break;
    }

    const Binding b = Binding::gamepad(event.button);
    if (g.ship.get() && (g.ship->presenceState == kWarpingPresence)) {
        const std::vector<int> actions = sys.prefs->actions_for(b);
        if (std::find(actions.begin(), actions.end(), kWarpKeyNum) != actions.end()) {
            // Keep warping: release the button's other actions but leave warp held.
            auto it = std::find(_pressed.begin(), _pressed.end(), b);
            if (it != _pressed.end()) {
                _pressed.erase(it);
                for (int a : actions) {
                    if (a != kWarpKeyNum) {
                        release(a);
                    }
                }
            }
            return;
        }
    }
    input_up(b);
}

void PlayerShip::gamepad_stick(const GamepadStickEvent& event) {
    bool active;
    int  direction = 0;
    if ((event.x * event.x + event.y * event.y) < 0.30) {
        active = false;
    } else {
        active    = true;
        direction = GetAngleFromVector(event.x * 32768, event.y * 32768);
        mAddAngle(direction, ROT_180);
    }
    switch (event.stick) {
        case Gamepad::Stick::LS:
            _control_active    = active;
            _control_direction = direction;
            break;
        default: break;
    }
}

bool PlayerShip::active() const {
    auto player = g.ship;
    return player.get() && player->active && (player->attributes & kIsPlayerShip);
}

static void handle_destination_key(const std::vector<PlayerEvent>& player_events) {
    for (const auto& e : player_events) {
        if (e.type == PlayerEventType::TARGET_SELF && (g.ship->attributes & kCanBeDestination)) {
            target_self();
        }
    }
}

static void handle_hotkeys(const std::vector<PlayerEvent>& player_events) {
    for (const auto& e : player_events) {
        switch (e.type) {
            case PlayerEventType::HOTKEY_SET:
                if (globals()->lastSelectedObject.get()) {
                    auto o = globals()->lastSelectedObject;
                    if (o->active && (o->id == globals()->lastSelectedObjectID)) {
                        globals()->hotKey[e.data].object   = globals()->lastSelectedObject;
                        globals()->hotKey[e.data].objectID = globals()->lastSelectedObjectID;
                        Update_LabelStrings_ForHotKeyChange();
                        sys.sound.select();
                    }
                }
                break;

            case PlayerEventType::HOTKEY_SELECT:
            case PlayerEventType::HOTKEY_TARGET:
                if (globals()->hotKey[e.data].object.get()) {
                    auto o = globals()->hotKey[e.data].object;
                    if (o->active && (o->id == globals()->hotKey[e.data].objectID)) {
                        bool target = (e.type == PlayerEventType::HOTKEY_TARGET) ||
                                      (o->owner != g.admiral);
                        select_object(o, target, g.admiral);
                    } else {
                        globals()->hotKey[e.data].object = SpaceObject::none();
                    }
                }
                break;

            default: break;
        }
    }
}

static void handle_target_keys(const std::vector<PlayerEvent>& player_events) {
    // for this we check lastKeys against theseKeys & relevent keys now being pressed
    for (const auto& e : player_events) {
        switch (e.type) {
            case PlayerEventType::SELECT_FRIEND: select_friendly(g.ship, g.ship->direction); break;
            case PlayerEventType::TARGET_FRIEND: target_friendly(g.ship, g.ship->direction); break;
            case PlayerEventType::TARGET_FOE: target_hostile(g.ship, g.ship->direction); break;
            case PlayerEventType::SELECT_BASE: select_base(g.ship, g.ship->direction); break;
            case PlayerEventType::TARGET_BASE: target_base(g.ship, g.ship->direction); break;
            default: continue;
        }
    }
}

static void handle_pilot_keys(
        Handle<SpaceObject> flagship, int32_t these_keys, bool gamepad_control,
        int32_t gamepad_control_direction) {
    if (flagship->attributes & kOnAutoPilot) {
        if (these_keys & (kUpKey | kDownKey | kLeftKey | kRightKey)) {
            flagship->keysDown = these_keys | kAutoPilotKey;
        }
    } else {
        flagship->keysDown = these_keys;
        if (gamepad_control) {
            int difference = mAngleDifference(gamepad_control_direction, flagship->direction);
            if (abs(difference) < 15) {
                // pass
            } else if (difference < 0) {
                flagship->keysDown |= kRightKey;
            } else {
                flagship->keysDown |= kLeftKey;
            }
        }
    }
}

static void handle_order_key(const std::vector<PlayerEvent>& player_events) {
    for (const auto& e : player_events) {
        if (e.type == PlayerEventType::ORDER) {
            g.ship->keysDown |= kGiveCommandKey;
        }
    }
}

static void handle_autopilot_keys(const std::vector<PlayerEvent>& player_events) {
    for (const auto& e : player_events) {
        if (e.type == PlayerEventType::AUTOPILOT) {
            engage_autopilot();
        }
    }
}

void PlayerShip::update() {
    if (!g.ship.get()) {
        return;
    }

    if (_message.editing()) {
        _player_events.clear();
        gTheseKeys = 0;
    }

    for (auto e : _player_events) {
        switch (e.type) {
            case PlayerEventType::ACCEL_ON: gTheseKeys |= (kUpKey & ~g.key_mask); break;
            case PlayerEventType::DECEL_ON: gTheseKeys |= (kDownKey & ~g.key_mask); break;
            case PlayerEventType::CCW_ON: gTheseKeys |= (kLeftKey & ~g.key_mask); break;
            case PlayerEventType::CW_ON: gTheseKeys |= (kRightKey & ~g.key_mask); break;
            case PlayerEventType::FIRE_1_ON: gTheseKeys |= (kPulseKey & ~g.key_mask); break;
            case PlayerEventType::FIRE_2_ON: gTheseKeys |= (kBeamKey & ~g.key_mask); break;
            case PlayerEventType::FIRE_S_ON: gTheseKeys |= (kSpecialKey & ~g.key_mask); break;
            case PlayerEventType::WARP_ON: gTheseKeys |= (kWarpKey & ~g.key_mask); break;

            case PlayerEventType::ACCEL_OFF: gTheseKeys &= ~(kUpKey & ~g.key_mask); break;
            case PlayerEventType::DECEL_OFF: gTheseKeys &= ~(kDownKey & ~g.key_mask); break;
            case PlayerEventType::CCW_OFF: gTheseKeys &= ~(kLeftKey & ~g.key_mask); break;
            case PlayerEventType::CW_OFF: gTheseKeys &= ~(kRightKey & ~g.key_mask); break;
            case PlayerEventType::FIRE_1_OFF: gTheseKeys &= ~(kPulseKey & ~g.key_mask); break;
            case PlayerEventType::FIRE_2_OFF: gTheseKeys &= ~(kBeamKey & ~g.key_mask); break;
            case PlayerEventType::FIRE_S_OFF: gTheseKeys &= ~(kSpecialKey & ~g.key_mask); break;
            case PlayerEventType::WARP_OFF: gTheseKeys &= ~(kWarpKey & ~g.key_mask); break;

            case PlayerEventType::ZOOM_OUT: zoom_out(e.data == 1); break;
            case PlayerEventType::ZOOM_IN: zoom_in(e.data == 1); break;
            case PlayerEventType::ZOOM_1X: zoom_shortcut(Zoom::ACTUAL); break;
            case PlayerEventType::ZOOM_2X: zoom_shortcut(Zoom::HALF); break;
            case PlayerEventType::ZOOM_4X: zoom_shortcut(Zoom::QUARTER); break;
            case PlayerEventType::ZOOM_16X: zoom_shortcut(Zoom::SIXTEENTH); break;
            case PlayerEventType::ZOOM_FOE: zoom_shortcut(Zoom::FOE); break;
            case PlayerEventType::ZOOM_OBJ: zoom_shortcut(Zoom::OBJECT); break;
            case PlayerEventType::ZOOM_ALL: zoom_shortcut(Zoom::ALL); break;
            case PlayerEventType::TRANSFER: transfer_control(g.admiral); break;

            case PlayerEventType::MINI_BUILD: build_ship(g.admiral, e.data); break;

            case PlayerEventType::MINI_HOLD: hold_position(g.admiral); break;
            case PlayerEventType::MINI_COME: come_to_me(g.admiral); break;
            case PlayerEventType::MINI_FIRE_1: fire_weapon(g.admiral, kPulseKey); break;
            case PlayerEventType::MINI_FIRE_2: fire_weapon(g.admiral, kBeamKey); break;
            case PlayerEventType::MINI_FIRE_S: fire_weapon(g.admiral, kSpecialKey); break;

            case PlayerEventType::NEXT_PAGE: next_message(g.admiral); break;
            case PlayerEventType::MINI_NEXT_PAGE: next_message(g.admiral); break;
            case PlayerEventType::MINI_PREV_PAGE: prev_message(g.admiral); break;
            case PlayerEventType::MINI_LAST_MESSAGE: last_message(g.admiral); break;

            default: break;
        }
    }

    /*
    while ((globals()->gKeyMapBufferBottom != globals()->gKeyMapBufferTop)) {
        bufMap = globals()->gKeyMapBuffer + globals()->gKeyMapBufferBottom;
        globals()->gKeyMapBufferBottom++;
        if (globals()->gKeyMapBufferBottom >= kKeyMapBufferNum) {
            globals()->gKeyMapBufferBottom = 0;
        }
        if (*enterMessage) {
            String* message = Label::get_string(g.send_label);
            if (message->empty()) {
                message->assign("<>");
            }
            if ((mReturnKey(*bufMap)) && (!AnyKeyButThisOne(*bufMap, Keys::RETURN))) {
                *enterMessage = false;
                StringSlice sliced = message->slice(1, message->size() - 2);
                int cheat = GetCheatNumFromString(sliced);
                if (cheat > 0) {
                    ExecuteCheat(cheat, g.admiral);
                } else if (!sliced.empty()) {
                    if (globals()->gActiveCheats[g.admiral] & kNameObjectBit)
                    {
                        SetAdmiralBuildAtName(g.admiral, sliced);
                        globals()->gActiveCheats[g.admiral] &= ~kNameObjectBit;
                    }
                }
                Label::set_position(
                        g.send_label,
                        viewport.left + ((viewport.width() / 2)),
                        viewport.top + ((play_screen.height() / 2)) +
                        kSendMessageVOffset);
                Label::recalc_size(g.send_label);
            } else {
                if ((mDeleteKey(*bufMap)) || (mLeftArrowKey(*bufMap))) {
                    if (message->size() > 2) {
                        if (mCommandKey(*bufMap)) {
                            // delete whole message
                            message->assign("<>");
                        } else {
                            message->resize(message->size() - 2);
                            message->append(1, '>');
                        }
                    }
                } else {
                    if (message->size() < 120) {
                        uint8_t ch = GetAsciiFromKeyMap(*bufMap, globals()->gLastMessageKeyMap);
                        if (ch) {
                            message->resize(message->size() - 1);
                            String s(macroman::decode(sfz::BytesSlice(&ch, 1)));
                            message->append(s);
                            message->append(1, '>');
                        }
                    }
                }
                width = sys.fonts.tactical->string_width(*message);
                strlen = viewport.left + ((viewport.width() / 2) - (width / 2));
                if ((strlen + width) > (viewport.right))
                {
                    strlen -= (strlen + width) - (viewport.right);
                }
                Label::recalc_size(g.send_label);
                Label::set_position(g.send_label, strlen, viewport.top +
                    ((play_screen.height() / 2) + kSendMessageVOffset));
            }
        } else {
            if ((mReturnKey(*bufMap)) && (!(g.key_mask & kReturnKeyMask))) {
                *enterMessage = true;
            }
        }
        globals()->gLastMessageKeyMap.copy(*bufMap);
    }
    */

    if (!g.ship->active) {
        return;
    }

    if (g.ship->health() < (g.ship->base->health >> 2L)) {
        if (g.time > globals()->next_klaxon) {
            if (globals()->next_klaxon == game_ticks()) {
                sys.sound.loud_klaxon();
            } else {
                sys.sound.klaxon();
            }
            Messages::shields_low();
            globals()->next_klaxon = g.time + kKlaxonInterval;
        }
    } else {
        globals()->next_klaxon = game_ticks();
    }

    if (!(g.ship->attributes & kIsPlayerShip)) {
        return;
    }

    Handle<SpaceObject> flagship = g.ship;  // Pilot same ship even after minicomputer transfer.
    for (auto e : _player_events) {
        switch (e.type) {
            case PlayerEventType::MINI_TRANSFER: transfer_control(g.admiral); break;
            default: break;
        }
    }
    handle_destination_key(_player_events);
    handle_hotkeys(_player_events);
    if (!_cursor.active()) {
        handle_target_keys(_player_events);
    }
    handle_pilot_keys(
            flagship, gTheseKeys, (_gamepad_state == NO_BUMPER) && _control_active,
            _control_direction);
    handle_order_key(_player_events);
    handle_autopilot_keys(_player_events);

    _player_events.clear();
}

bool PlayerShip::show_select() const {
    return _control_active && (_gamepad_state & SELECT_BUMPER);
}

bool PlayerShip::show_target() const {
    return _control_active && (_gamepad_state & TARGET_BUMPER);
}

int32_t PlayerShip::control_direction() const { return _control_direction; }

PlayerShip::MessageText::MessageText() : EditableText{"<", ">"} {}

void PlayerShip::MessageText::start_editing() {
    _editing = true;
    if (sys.video->start_editing(this)) {
        update("<>", {1, 1}, {-1, -1});
    }
}

void PlayerShip::MessageText::stop_editing() {
    _editing = false;
    sys.video->stop_editing(this);
    g.send_label->text() = StyledText{};
}

void PlayerShip::MessageText::update(pn::string_view text, range<int> selection, range<int> mark) {
    g.send_label->text() = StyledText::plain(
            text, {sys.fonts.tactical, viewport().width() / 2},
            GetRGBTranslateColorShade(Hue::GREEN, LIGHTEST));
    g.send_label->text().select(selection.begin, selection.end);
    g.send_label->text().mark(mark.begin, mark.end);

    g.send_label->set_position(
            viewport().left + ((viewport().width() / 2) - (g.send_label->width() / 2)),
            viewport().top + ((play_screen().height() / 2)));
}

StyledText&       PlayerShip::MessageText::styled_text() { return g.send_label->text(); }
const StyledText& PlayerShip::MessageText::styled_text() const { return g.send_label->text(); }

void PlayerShip::MessageText::accept() {
    Cheat cheat = GetCheatFromString(text());
    if (cheat != Cheat::NONE) {
        ExecuteCheat(cheat, g.admiral);
    } else if (!text().empty()) {
        if (g.admiral->cheats() & kNameObjectBit) {
            SetAdmiralBuildAtName(g.admiral, text());
            g.admiral->cheats() &= ~kNameObjectBit;
        }
    }

    stop_editing();
}

void PlayerShip::MessageText::escape() {
    stop_editing();
    g.admiral->cheats() &= ~kNameObjectBit;
}

void PlayerShipHandleClick(Point where, int button) {
    if (g.key_mask & kMouseMask) {
        return;
    }

    bool target = use_target_key() || (button == 1);
    if (g.ship.get()) {
        if ((g.ship->active) && (g.ship->attributes & kIsPlayerShip)) {
            Rect bounds = {
                    where.h - kCursorBoundsSize,
                    where.v - kCursorBoundsSize,
                    where.h + kCursorBoundsSize,
                    where.v + kCursorBoundsSize,
            };

            if (target) {
                auto target        = g.admiral->target();
                auto selectShipNum = GetSpritePointSelectObject(
                        &bounds, g.ship, kCanBeDestination | kIsDestination, target,
                        FRIENDLY_OR_HOSTILE);
                if (selectShipNum.get()) {
                    select_object(selectShipNum, true, g.admiral);
                }
            } else {
                auto control       = g.admiral->control();
                auto selectShipNum = GetSpritePointSelectObject(
                        &bounds, g.ship, kCanBeDestination | kIsDestination, control, FRIENDLY);
                if (selectShipNum.get()) {
                    select_object(selectShipNum, false, g.admiral);
                }
            }
        }
    }
}

// ChangePlayerShipNumber()
// assumes that newShipNumber is the number of a valid (legal, living) ship and that
// gPlayerShip already points to the current, legal living ship

void ChangePlayerShipNumber(Handle<Admiral> adm, Handle<SpaceObject> newShip) {
    auto flagship = adm->flagship();
    if (!flagship.get()) {
        throw std::runtime_error(
                pn::format("adm: {0}, newShip: {1}", adm.number(), newShip.number()).c_str());
    }

    if (adm == g.admiral) {
        flagship->attributes &= ~kIsPlayerShip;
        if (newShip != g.ship) {
            g.ship = newShip;
            globals()->starfield.reset();
        }

        flagship = g.ship;
        if (!flagship.get()) {
            throw std::runtime_error(pn::format(
                                             "adm: {0}, newShip: {1}, gPlayerShip: {2}",
                                             adm.number(), newShip.number(), g.ship.number())
                                             .c_str());
        }

        flagship->attributes |= kIsPlayerShip;

        if (newShip == g.admiral->control()) {
            g.control_label->set_age(Label::kVisibleTime);
        }
        if (newShip == g.admiral->target()) {
            g.target_label->set_age(Label::kVisibleTime);
        }
    } else {
        flagship->attributes &= ~kIsPlayerShip;
        flagship = newShip;
        flagship->attributes |= kIsPlayerShip;
    }
    adm->set_flagship(newShip);
}

void TogglePlayerAutoPilot(Handle<SpaceObject> flagship) {
    if (flagship->attributes & kOnAutoPilot) {
        flagship->attributes &= ~kOnAutoPilot;
        if ((flagship->owner == g.admiral) && (flagship->attributes & kIsPlayerShip)) {
            Messages::autopilot(false);
        }
    } else {
        SetObjectDestination(flagship);
        flagship->attributes |= kOnAutoPilot;
        if ((flagship->owner == g.admiral) && (flagship->attributes & kIsPlayerShip)) {
            Messages::autopilot(true);
        }
    }
}

bool IsPlayerShipOnAutoPilot() { return g.ship.get() && (g.ship->attributes & kOnAutoPilot); }

void PlayerShipGiveCommand(Handle<Admiral> whichAdmiral) {
    auto control = whichAdmiral->control();

    if (control.get()) {
        SetObjectDestination(control);
        if (whichAdmiral == g.admiral) {
            sys.sound.order();
        }
    }
}

void PlayerShipBodyExpire(Handle<SpaceObject> flagship) {
    auto selectShip = flagship->owner->control();

    if (selectShip.get()) {
        if ((selectShip->active != kObjectInUse) || (!(selectShip->attributes & kCanThink)) ||
            (selectShip->attributes & kStaticDestination) ||
            (selectShip->owner != flagship->owner) ||
            (!(selectShip->attributes & kCanAcceptDestination)))
            selectShip = SpaceObject::none();
    }
    if (!selectShip.get()) {
        selectShip = g.root;
        while (selectShip.get() && ((selectShip->active != kObjectInUse) ||
                                    (selectShip->attributes & kStaticDestination) ||
                                    (!((selectShip->attributes & kCanThink) &&
                                       (selectShip->attributes & kCanAcceptDestination))) ||
                                    (selectShip->owner != flagship->owner))) {
            selectShip = selectShip->nextObject;
        }
    }
    if (selectShip.get()) {
        ChangePlayerShipNumber(flagship->owner, selectShip);
    } else {
        if (!g.game_over) {
            g.game_over    = true;
            g.game_over_at = g.time + secs(3);
        }
        const sfz::optional<pn::string>* victory_text;
        switch (g.level->type()) {
            case Level::Type::SOLO: victory_text = &g.level->solo.no_ships; break;
            case Level::Type::NET:
                if (flagship->owner == g.admiral) {
                    victory_text = &g.level->net.own_no_ships;
                } else {
                    victory_text = &g.level->net.foe_no_ships;
                }
                break;
            default: victory_text = nullptr; break;
        }
        if (victory_text && *victory_text) {
            g.victory_text.emplace((*victory_text)->copy());
        }
        if (flagship->owner.get()) {
            flagship->owner->set_flagship(SpaceObject::none());
        }
        if (flagship == g.ship) {
            g.ship = SpaceObject::none();
        }
    }
}

int32_t HotKey_GetFromObject(Handle<SpaceObject> object) {
    if (!object.get() && !object->active) {
        return -1;
    }
    for (int32_t i = 0; i < kHotKeyNum; ++i) {
        if (globals()->hotKey[i].object == object) {
            if (globals()->hotKey[i].objectID == object->id) {
                return i;
            }
        }
    }
    return -1;
}

void Update_LabelStrings_ForHotKeyChange(void) {
    auto target = g.admiral->target();
    if (target.get()) {
        g.target_label->set_object(target);
        if (target == g.ship) {
            g.target_label->set_age(Label::kVisibleTime);
        }
        g.target_label->text() = StyledText::plain(
                name_with_hot_key_suffix(target), sys.fonts.tactical,
                GetRGBTranslateColorShade(Hue::SKY_BLUE, LIGHTEST));
    }

    auto control = g.admiral->control();
    if (control.get()) {
        g.control_label->set_object(control);
        if (control == g.ship) {
            g.control_label->set_age(Label::kVisibleTime);
        }
        sys.sound.select();
        g.control_label->text() = StyledText::plain(
                name_with_hot_key_suffix(control), sys.fonts.tactical,
                GetRGBTranslateColorShade(Hue::YELLOW, LIGHTEST));
    }
}

}  // namespace antares
