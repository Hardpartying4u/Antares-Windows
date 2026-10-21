# Star Bound

Star Bound is a Windows fork of [Antares](https://github.com/arescentral/antares), the open-source
port of *Ares* by Nathan Lamont (Bigger Planet). It brings the classic space strategy game to modern
Windows with new controls, enhanced graphics and a new Arena mode.

**Based on the original game 'Ares,' by Nathan Lamont. Star Bound is not affiliated with or endorsed
by Nathan Lamont or Bigger Planet.**

## How to play

1. Download the release zip.
2. Unzip it anywhere.
3. Run `Antares.exe` in the `Antares` folder (it also holds `data`, `art`, `arena` and the licence
   files).

- **System:** Windows 10/11, x64.
- **First launch:** the game downloads the missing sound effects (about 12 MB), so it needs an
  internet connection once.
- **Your data:** saves, progress and settings are kept in `Documents\Antares` (`config.pn`,
  `Registry\`), not in the game folder.

## What's new in Star Bound

### Windows build
- Native MSVC build of Antares.
- Fixes a crash when the window is minimised.
- Controller detection runs in the background, so the game starts quickly.

### UI scale
- Integer UI scale (Auto, 1×–4×) in **Options › Video**.
- Windowed or fullscreen.

### Graphics
- **Enhanced:** AI-upscaled 2×/4× ship and interface art (in `art/`).
- **Original:** the original art.
- Chosen in **Options › Video**; Enhanced is the default.

### Key rebinding
- Rebind every action in **Options › Key Controls**.
- Each action has two keyboard/mouse bindings and a gamepad button.
- Clashes are flagged, and **Reset to Defaults** restores the chosen preset.

### Control presets
- **Classic:** Ares' original keys.
- **Modern: Ares:** Classic's keys plus the Modern mouse controls.
- **Modern: WASD:** flight on W A S D, with the Modern mouse controls.

### Modern mouse controls (Modern presets)
- Left-click or drag a box to select ships; Shift adds to the selection.
- Right-click gives orders (attack, follow, guard, move) to every selected ship; group moves spread
  into a formation.
- Attack-move: `A` (Modern: Ares) or `G` (Modern: WASD), then click; or `Ctrl`+right-click.

### Control groups and the group bar
- `Ctrl`+number saves a group, the number selects it, and `Shift`+number adds it to the selection.
- Double-tap the number to show where the group is.
- A bar along the bottom of the play area shows each group's ship icon, number and ship count,
  highlights the selected group, and can be clicked.

### Wheel zoom
- The mouse wheel steps through every zoom level.

### Controller support
- XInput and DirectInput gamepads, with Ares' gamepad layout, all rebindable.

### Main menu
- Removed: **Start Network Game** and **Demo** (the demo replays still play when the menu is left
  idle).
- Added: **Arena** (key `D`).

### Arena
- Pick a battle and command your fleet with your usual controls.
- Four battles, Easy to Very Hard:
  - **Skirmish** (Easy)
  - **Fleet Action** (Medium)
  - **Armada** (Hard): enemy reinforcements arrive, sooner if you clear the first group.
  - **Last Stand** (Very Hard): defend the planet Avalon, which earns money and builds ships,
    against six escalating waves.
- Losing your flagship passes control to another of your ships; you lose when no ships are left
  (in Last Stand, also if Avalon is captured).
- Battles are files in `arena/`.

### App icon
- An Ishiman Heavy Cruiser icon for `Antares.exe`, its window and the taskbar.

## Planned

- UI redesign.
- New visual effects.
- More Arena variety (more battles and races, a fleet picker, scores).

## Building from source

Requirements: Windows with Visual Studio 2022 Build Tools and Python 3. From a `vcvars64` prompt in
the repo folder, with that folder first on `PATH` (it holds `gn.exe` and `ninja.exe` after the
first step):

```cmd
python scripts/deps.py install
python configure
ninja -C out/win/opt
```

Tests: `python scripts/test.py`. Package: `python scripts/dist.py win zip`.

## Credits

- **Ares** by Nathan Lamont (Bigger Planet), released in 1998 and re-released by Ambrosia Software;
  its source code and media were released in 2008.
- **Antares** by the Antares authors (see `AUTHORS`): <https://github.com/arescentral/antares>,
  website <https://arescentral.org/antares>.
- Star Bound's third-party credits (GLFW, Real-ESRGAN and others) are in `CREDITS.md`; every
  asset's origin and licence is in `ASSETS.md`.

## Licence

- **Code:** GNU LGPL version 3 or later (`COPYING.LESSER`, `COPYING`), as Antares.
- **Game media:** `data/`, everything derived from it in `art/`, and the app icon are licensed
  Creative Commons Attribution-NonCommercial-ShareAlike 3.0 (`data/COPYING`): non-commercial use
  only, and derived media stay under the same licence.

**Based on the original game 'Ares,' by Nathan Lamont. Star Bound is not affiliated with or endorsed
by Nathan Lamont or Bigger Planet.**
