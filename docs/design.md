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
- Gamepad support already exists (see `docs/current-state.md` §1.4). Both presets and the rebinding screen must cover gamepad as well as keyboard and mouse.
- Mouse-wheel zoom needs new input handling: the game currently ignores the scroll wheel. M2 includes adding it.
- Exact action list to be taken from `docs/current-state.md`.

## 5. User interface

- Modern HUD and menus with a clean sci-fi look.
- Resolution-independent layout: scales correctly at any size and aspect ratio, including ultrawide.
- Menus fully usable with the mouse; keyboard shortcuts kept.
- First step: fix the widescreen scaling problem, since the redesign depends on it.
- **Decision (2026-10-18): UI scaling uses a global integer scale** (option A of the scaling review). The whole game is drawn at a logical resolution of window ÷ N and magnified N× with crisp nearest-neighbour pixels, reusing the existing HiDPI mechanism. Details:
  - N is automatic: ⌊window height ÷ 540⌋, minimum 1 (1080p and 1440p → 2×, 4K → 4×). It is capped so the logical screen never drops below 640 × 480, the size the menus and HUD are laid out for.
  - Video options has an override: Auto, 1×, 2×, 3×, 4× (still capped as above). It applies immediately; Cancel reverts it.
  - Mouse coordinates are converted exactly. When the window isn't a multiple of N, the leftover pixels form a thin black border.
  - This also magnifies the battle view, so it shows less of the battlefield than before at large window sizes. That is closer to the original field of view. Wider windows still show more horizontally; capping that is a separate open question.
  - A full resolution-independent redesign (anchored layouts, new art) remains M3.

## 6. Graphics

- Ships are modelled in Blender and rendered to sprites by a script, so the game keeps its top-down view and gameplay.
- Pipeline: model, consistent lighting and camera, render a full set of rotation frames, export in the format the game already loads.
- Ships are replaced one type at a time. Anything not yet replaced uses the original sprite.
- After ships: planets, stations, weapon effects, explosions and thruster effects.
- High-resolution sprites (finding, 2026-10-18): the game uses each sprite frame's size and centre from the `.pn` table for **collision bounds** (`calc_bounds`, `src/game/motion.cpp`), not just for drawing. New art must therefore keep the original 1× frame rectangles and centres, and supply extra detail as `@2x`/`@4x` images used only as textures. Pictures and fonts already load `@2x` files automatically; sprites do not yet, so the sprite loader needs a small change before M4 (see `docs/current-state.md` §4.3).

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
| M1 | UI scales with the window | Menus and HUD scale crisply with the window (automatic integer scale plus Video options override), mouse input stays exact, tested at 1080p, 1440p, ultrawide and small windows, windowed and fullscreen |
| M2 | Controls | Classic and Modern presets plus a rebinding screen |
| M3 | UI redesign | New HUD and menus replace the old ones |
| M4 | Blender pipeline | One ship rendered from a script and working in game |
| M5 | All ships replaced | Every ship type uses new sprites |
| M6 | Effects | New explosions, weapons, thrusters, lighting |

## 10. Open questions

- Which parts of the UI need a full redesign and which only need scaling?
- How many ship types and frames per ship does the game load?
- Should new art keep the original ship designs or reinterpret them?
- Minimum supported resolution and graphics hardware? (The logical screen is at least 640 × 480; physical minimum not yet decided.)
- Should the battle view's field of view be capped (for example to a fixed world height and maximum width) so ultrawide windows don't see more of the battlefield than the scenarios were balanced for?
- Should new sprite art keep the original silhouettes exactly? Collision bounds come from the 1× frame sizes, so changing them changes gameplay.
