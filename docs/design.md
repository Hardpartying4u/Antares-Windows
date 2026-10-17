# Star Bound: Design Document (v0.1)

Working title. Based on Ares (Nathan Lamont) via the Antares open-source port.

## 1. Vision

A faithful, modernised version of Ares for Windows PC players. The gameplay, scenarios and story stay as they are. The controls, interface and graphics are brought up to modern PC standards.

## 2. Principles

- **Faithful first.** Rules, units, weapons, orders, AI and scenarios behave as in the original.
- **Always playable.** Every change is small, tested and committed. The game must work after each step.
- **Fallbacks.** Original art and the classic control scheme stay available until replacements are proven.
- **Written down.** Decisions go in this file so every session (human or Claude Code) starts from the same facts.

## 3. What stays unchanged

Core rules, unit and weapon behaviour, fleet orders, AI, scenarios, story text. (To be confirmed against `docs/current-state.md`.)

## 4. Controls

- Two built-in presets:
  - **Classic:** the original scheme, unchanged.
  - **Modern:** WASD flight, mouse aim and fire, click to select ships and give fleet orders, mouse wheel zoom.
- A rebinding screen so every action can be reassigned to keyboard keys or mouse buttons.
- Bindings are saved in the player's config and survive updates.
- Later, maybe: gamepad support.
- Exact action list to be taken from `docs/current-state.md`.

## 5. User interface

- Modern HUD and menus with a clean sci-fi look.
- Resolution-independent layout: scales correctly at any size and aspect ratio, including ultrawide.
- Menus fully usable with the mouse; keyboard shortcuts kept.
- First step: fix the widescreen scaling problem, since the redesign depends on it.

## 6. Graphics

- Ships are modelled in Blender and rendered to sprites by a script, so the game keeps its top-down view and gameplay.
- Pipeline: model, consistent lighting and camera, render a full set of rotation frames, export in the format the game already loads.
- Ships are replaced one type at a time. Anything not yet replaced uses the original sprite.
- After ships: planets, stations, weapon effects, explosions and thruster effects.

## 7. Out of scope for now

New factions or missions, multiplayer, moving to Unity or Unreal, real 3D gameplay. Revisit after the milestones below.

## 8. Licensing and naming

- Antares code is open source, so Star Bound's source must stay open.
- Original Ares art, sound and text are licensed non-commercial and share-alike. Fine for a free game. To ever sell it, every original asset must be replaced.
- Keep `ASSETS.md` listing each asset and whether it is original, replaced, or new.
- "Star Bound" is a working title only. A commercial game called Starbound already exists, so choose a distinct name before release.

## 9. Milestones

| # | Milestone | Done when |
|---|-----------|-----------|
| M0 | Stage 1: Antares runs on Windows | Done: builds, plays, packaged, in git |
| M1 | Widescreen and UI scaling fixed | UI looks right at 16:9 and ultrawide |
| M2 | Controls | Classic and Modern presets plus a rebinding screen |
| M3 | UI redesign | New HUD and menus replace the old ones |
| M4 | Blender pipeline | One ship rendered from a script and working in game |
| M5 | All ships replaced | Every ship type uses new sprites |
| M6 | Effects | New explosions, weapons, thrusters, lighting |

## 10. Open questions

- Which parts of the UI need a full redesign and which only need scaling?
- How many ship types and frames per ship does the game load?
- Should new art keep the original ship designs or reinterpret them?
- Minimum supported resolution and graphics hardware?
