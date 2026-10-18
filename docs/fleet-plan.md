# Star Bound: Fleet Remaster Plan

**Approved 2026-10-18.** Decisions:
- the batch order, starting with batch 1 (Ishiman core);
- art in an `art/` folder with Git LFS;
- keep the original EVA troopers (kit F) for now.

The `ASSETS.md` licensing question stays open (`docs/design.md` §10).

This plan covers remastering every rotating ship sprite in the way the Human cruiser, fighter and
carrier were done (`docs/current-state.md` §4.3.1): the shape is fitted to the original frames,
the colours are calibrated against them, and the look is modernised. Nothing here has been
rendered yet. Written 2026-10-18.

## 1. Inventory

`tools/sprite-render/fleet_inventory.py` scans `data/` (read only) and writes the inventory and
contact sheets to `out/sprite-render/fleet/`: `inventory.md`/`.csv`, one sheet per faction and one
per batch.

- **78 distinct rotating sprites are in use**, by 106 objects once template chains are resolved.
  The earlier "82" in `docs/design.md` counted objects, not sprites.
  - 3 of the 78 are already remastered (the Human cruiser, fighter and carrier).
  - 8 aren't ships: 3 missiles, gunsmoke, a castaway crew member, a capture-the-flag flag, and two
    tiny human and Obish figures. They're left for a later weapons-and-effects pass.
  - That leaves **67 ships to do**.
- **Frames:** all have 24 frames except the Bazidan battleship (36). Frames are 8–108 px at 1×.
- **Draw scale:** capital ships (carriers, battleships, liners, the research vessel) are drawn at
  2×, and the Cantharan gateship at 4×. With the effective-draw-scale fix they use their @2x/@4x
  sheets at UI 1×.
- **How often players see each ship:**
  - **Levels:** the number of level files that place the ship or let a player build it.
  - **Placed:** how many start positions use it.
  - **First:** the earliest solo level, in play order (1 = Tutorial 1, 2 = Chapter 1, 3 = Tutorial
    2, 4 = Chapter 2, 5 = Tutorial 3, 6 = Chapter 3, … 23 = Chapter 20).
- **Exposure:**
  - The player's own **Ishiman** fleet dominates. The Ishiman cruiser is in 16 levels from the
    first tutorial on.
  - It's followed by the **Gaitori** and **Cantharan** opponents of the early chapters.
  - **Human** ships, despite being the trial, appear only in the last chapters (play order 20–21)
    and the net levels.

## 2. Shape kits

Each ship is a small hand-written shape module on top of a reusable kit.

| Kit | Covers | Status | Ships remaining |
|---|---|---|---:|
| **T** twin-pod | hull loft, two engine pods, engine block | done (`twin_pod.py`) | 9 |
| **Q** twin-pod with extra pods | four or more pods, as on the Gaitori gunship and heavy destroyer | small extension of T | 3 |
| **B** box-kit | any set of fitted rounded boxes: mirrored, repeated, chamfered | done (`box_kit.py`); needs boxes along X for struts and cross-bars | 28 |
| **S** smooth kit | blended ellipsoids, lofts along any axis, curved blades and horns | **new**; needed by the rounded and organic factions | 22 |
| **F** figures | 16 px EVA troopers | **recommend keeping the originals** for now; they're people, not hulls | 5 |

Which kits each faction needs:

| Faction | Kits | Notes |
|---|---|---|
| Ishiman (cyan) | B, S | Struts between hull and pods; rounded carrier, fighter and research vessel |
| Gaitori (white/silver) | T, Q, B | Wheel-like pods; the carrier is two long blocks |
| Cantharan (green) | B, S | Angular ships; organic carrier; the gateship is two curved blades (4×) |
| Obish (blue) | S | Ribbed, rounded hulls |
| Salrilian (near-black gloss) | S | Smooth ellipsoids; very dark, so calibration needs care |
| Audemedon (orange/bronze) | B, S | Blocks with spherical tanks |
| Bazidan (rose) | S | One 36-frame battleship with curved horns |
| Elejeetian (olive) | B | Long ribbed liner, boxy cruiser |
| Human (grey) | T, B | Three done |
| Grolk (net levels only) | T | — |
| Location | S | The border drone is a sphere |

## 3. Batches, in order of exposure

The table ranks batches by how early and how often players meet the ships. The review contact
sheet of the originals in each batch is `out/sprite-render/fleet/batch-<n>-….png`; during the
work each batch also gets a remaster review sheet (section 4).

| Batch | Ships | Kits | Before it starts | Estimate (agent time) |
|---|---:|---|---|---|
| ~~1 Ishiman core~~ | 7 | B, S | — | **Stopped 2026-10-19: Ishiman ships stay original for now** (see "Batch 1 outcome") |
| 2 Gaitori core | 6 | T, Q, B | extra pods (Q) | 1.5 h |
| 3 Cantharan core | 8 | B, S | — | 2 h |
| 4 Early support craft | 8 | S, B | — | 2 h |
| 5 Mid campaign | 10 | S, T, B, F | Salrilian calibration check | 2.5 h |
| 6 Late campaign | 16 | S, B, T | 36-frame Bazidan; 4× gateship | 4 h |
| 7 Not in the solo campaign | 12 | T, Q, S, F | — | 2 h |
| **Total** | **67** | | | **about 17–20 h**, plus your review of each batch |

The estimates come from the trial: about 7–12 minutes per ship on an existing kit, 15–25 minutes
on a new kit, and 2–4 minutes of compute per ship. Each batch adds about 30 minutes for its
in-game check and review sheet. Smooth-kit ships are the least certain, because no S ship has been
done yet. I'll re-estimate after batch 1.

### Ships per batch

#### Batch 1 Ishiman core (player fleet, play order 1-5)

| Sprite | Ship | Kit | Max frame (1×) | Drawn | Levels | First (play order) | Placed |
|---|---|---|---|---:|---:|---:|---:|
| `ish/cruiser` | Cruiser | B (pods on struts) | 32×30 | 1× | 16 | 1 | 20 |
| `ish/transport` | Transport | B | 56×55 | 1× | 12 | 1 | 4 |
| `ish/carrier` | Carrier | S | 64×46 | 2× | 9 | 1 | 10 |
| `ish/fighter` | Fighter | S | 16×15 | 1× | 13 | 3 | 11 |
| `ish/hvc` | Heavy Cruiser | B (pods on struts) | 40×37 | 1× | 10 | 3 | 15 |
| `ish/gunship` | Gunship | B | 48×46 | 1× | 10 | 3 | 4 |
| `ish/hvd` | Heavy Destroyer | B | 68×48 | 1× | 8 | 5 | 12 |

#### Batch 2 Gaitori core (play order 2-8)

| Sprite | Ship | Kit | Max frame (1×) | Drawn | Levels | First (play order) | Placed |
|---|---|---|---|---:|---:|---:|---:|
| `gai/cruiser` | Cruiser | T | 40×35 | 1× | 9 | 2 | 30 |
| `gai/fighter` | Fighter | T | 20×20 | 1× | 5 | 2 | 9 |
| `gai/transport` | Transport | B | 60×54 | 1× | 4 | 2 | 5 |
| `gai/gunship` | Gunship | Q | 68×66 | 1× | 6 | 4 | 15 |
| `gai/carrier` | Carrier | B | 84×82 | 2× | 3 | 6 | 7 |
| `gai/hvd` | Gaitori Heavy Destroyer | Q | 76×56 | 1× | 1 | 8 | 1 |

#### Batch 3 Cantharan core (play order 1-12)

| Sprite | Ship | Kit | Max frame (1×) | Drawn | Levels | First (play order) | Placed |
|---|---|---|---|---:|---:|---:|---:|
| `can/transport` | Transport | B | 52×51 | 1× | 7 | 1 | 14 |
| `can/carrier` | Carrier | S | 48×47 | 2× | 8 | 5 | 19 |
| `can/hvd` | Heavy Destroyer | B | 68×63 | 1× | 1 | 5 | 0 |
| `can/gunship` | Gunship | B | 44×53 | 1× | 6 | 10 | 3 |
| `can/hvc` | Heavy Cruiser | B | 32×38 | 1× | 6 | 10 | 7 |
| `can/schooner` | Schooner | B | 20×47 | 1× | 1 | 10 | 3 |
| `can/fighter` | Fighter | B | 16×17 | 1× | 7 | 11 | 5 |
| `can/cruiser` | Cruiser | B | 28×45 | 1× | 7 | 12 | 9 |

#### Batch 4 Early support craft (play order 4-12)

| Sprite | Ship | Kit | Max frame (1×) | Drawn | Levels | First (play order) | Placed |
|---|---|---|---|---:|---:|---:|---:|
| `obi/escort` | Escort | S | 40×37 | 1× | 3 | 4 | 5 |
| `obi/transport` | Transport | S | 52×47 | 1× | 1 | 7 | 3 |
| `loc/flak` | Border Drone | S | 40×38 | 1× | 6 | 8 | 18 |
| `ish/engineer` | Engineer Pod | S | 36×30 | 1× | 3 | 8 | 2 |
| `gai/engineer` | Engineer Pod | B | 36×34 | 1× | 2 | 8 | 1 |
| `ish/etc/astrominer` | Astrominer | B | 52×53 | 1× | 3 | 10 | 5 |
| `ish/defdrone` | Defense Drone | B | 20×21 | 1× | 1 | 12 | 0 |
| `can/defdrone` | Defense Drone | B | 24×19 | 1× | 1 | 12 | 0 |

#### Batch 5 Mid campaign (play order 14-16)

| Sprite | Ship | Kit | Max frame (1×) | Drawn | Levels | First (play order) | Placed |
|---|---|---|---|---:|---:|---:|---:|
| `sal/cruiser` | Cruiser | S | 32×28 | 1× | 3 | 14 | 5 |
| `sal/fighter` | Fighter | S | 20×13 | 1× | 2 | 14 | 1 |
| `sal/carrier` | Carrier | S | 84×63 | 2× | 2 | 16 | 3 |
| `ish/evat` | EVA Trooper | F | 12×17 | 1× | 1 | 14 | 1 |
| `ish/etc/cargo` | Cargo Ship | T | 64×47 | 1× | 1 | 15 | 5 |
| `ish/aslttran` | Assault Transport | S | 44×35 | 1× | 3 | 15 | 0 |
| `can/aslttran` | Assault Transport | B | 36×42 | 1× | 1 | 15 | 2 |
| `can/engineer` | Engineer Pod | B | 44×34 | 1× | 1 | 15 | 0 |
| `uns/etc/bttlship` | Battleship | B | 60×47 | 2× | 1 | 15 | 1 |
| `ele/etc/liner` | Luxury Liner | B | 108×94 | 2× | 1 | 16 | 2 |

#### Batch 6 Late campaign (play order 17-22)

| Sprite | Ship | Kit | Max frame (1×) | Drawn | Levels | First (play order) | Placed |
|---|---|---|---|---:|---:|---:|---:|
| `baz/bttlship` | Battleship | S | 84×71 | 2× | 2 | 17 | 3 |
| `sal/gunship` | Gunship | S | 68×51 | 1× | 1 | 18 | 1 |
| `sal/aslttran` | Assault Transport | S | 56×40 | 1× | 1 | 18 | 0 |
| `sal/transport` | Transport | S | 48×35 | 1× | 1 | 18 | 0 |
| `aud/carrier` | Carrier | S | 68×64 | 2× | 5 | 18 | 9 |
| `aud/cruiser` | Cruiser | B | 44×34 | 1× | 4 | 19 | 5 |
| `aud/fighter` | Fighter | S | 16×16 | 1× | 3 | 19 | 5 |
| `aud/gunship` | Gunship | S | 60×52 | 1× | 4 | 19 | 4 |
| `aud/aslttran` | Audemedon Assault Transport | B | 56×47 | 1× | 1 | 20 | 0 |
| `aud/hvd` | Heavy Destroyer | B | 68×57 | 1× | 2 | 20 | 0 |
| `aud/transport` | Transport | B | 68×53 | 1× | 3 | 20 | 1 |
| `ele/cruiser` | Cruiser | B | 48×40 | 1× | 2 | 19 | 4 |
| `ish/etc/research` | Research Vessel | S | 84×71 | 2× | 1 | 19 | 1 |
| `uns/gunship` | Gunship | B | 64×46 | 1× | 3 | 21 | 8 |
| `uns/transport` | Transport | T | 64×50 | 1× | 3 | 21 | 15 |
| `can/gateship` | Instead of dying, opens up jump gate & e | S | 108×97 | 4× | 2 | 22 | 2 |

#### Batch 7 Not in the solo campaign (net levels, unused)

| Sprite | Ship | Kit | Max frame (1×) | Drawn | Levels | First (play order) | Placed |
|---|---|---|---|---:|---:|---:|---:|
| `uns/aslttran` | Human Assault Transport | T | 52×40 | 1× | 0 | × | 0 |
| `uns/hvd` | Human Heavy Destroyer | T | 72×55 | 1× | 0 | × | 0 |
| `gai/aslttran` | Gaitori Assault Transport | Q | 52×41 | 1× | 0 | × | 0 |
| `sal/hvd` | Salrilian Heavy Destroyer | S | 84×62 | 1× | 0 | × | 0 |
| `ish/schooner` | Schooner | T | 48×43 | 1× | 0 | × | 0 |
| `obi/etc/liner` | Battlecruiser | S | 56×54 | 2× | 0 | × | 0 |
| `gro/cruiser` | Light Cruiser, Grolk | T | 56×44 | 1× | 1 | × | 2 |
| `gro/ctf/cruiser` | Cruiser, Grolk for CTF, P1 | T | 44×37 | 1× | 1 | × | 16 |
| `aud/evat` | Audemedon Extra-vehicular Assault Troope | F | 16×14 | 1× | 0 | × | 0 |
| `can/evat` | EVA Trooper | F | 16×15 | 1× | 0 | × | 0 |
| `gai/evat` | Gaitori Extra-vehicular Assault Trooper | F | 16×15 | 1× | 0 | × | 0 |
| `sal/evat` | Salrilian Extra-vehicular Assault Troope | F | 16×15 | 1× | 0 | × | 0 |

#### Batch Done (Human trial)

| Sprite | Ship | Kit | Max frame (1×) | Drawn | Levels | First (play order) | Placed |
|---|---|---|---|---:|---:|---:|---:|
| `uns/cruiser` | Cruiser | done (T) | 44×34 | 1× | 4 | 20 | 6 |
| `uns/fighter` | Fighter | done (T) | 24×18 | 1× | 3 | 21 | 7 |
| `uns/carrier` | Carrier | done (B) | 108×78 | 2× | 3 | 21 | 10 |


## 4. How each batch runs

1. For each ship:
   - camera sweep, carve, write the shape module (choose a kit, set starting values, record the
     look);
   - fit, render, measure and store the offset, calibrate colours;
   - make the per-ship comparison sheet (`remaster_sheet.py`).
2. **Batch review sheet** (a new tool, built in batch 1). For every ship in the batch:
   - original and remaster at the size the game draws them, at UI 1×, 2× and 4×;
   - team-tinted;
   - the fit numbers against the targets.
3. **In-game check:** install the whole batch through the override folder and view it in the
   save-free test scenario beside original ships, at UI 1×, 2× and 4× and 1:1 and 1:2 zoom.
   Remove the files afterwards.
4. **Stop for your approval.** Then commit: tool changes, each faction's ship files, and docs as
   separate commits. The art moves into the art folder (section 5) once that exists.

5. **Acceptance gate** (added 2026-10-19). A ship goes into `art/` only if, in game at UI 2×, it
   looks better than the original and keeps the original's features. Otherwise it stays
   original, and the report says so.
6. **Appearance is measured as well as the outline** (`appearance_metric.py`):
   - each colour region's mean colour and brightness spread (10th/50th/90th percentiles);
   - per frame at 1×: structural similarity (SSIM), luminance difference and contrast.
   - A flat mid-tone can't pass for the original's dark gaps and bright highlights.

Fit targets stay as set:
- IoU mean ≥ 0.88 and worst ≥ 0.85;
- outline mean ≤ 0.5 px and worst p95 ≤ 1.5 px.

For ships of 16 px or less, the outline targets are the ones that matter. Misses get reported per
frame, like the carrier's 1 px antenna, for you to accept or reject.

### Batch 1 outcome (2026-10-18/19)

Two attempts, both measured.

**Smooth kit, all seven ships.** The outlines fitted well (IoU 0.89–0.94), but the ships read as
soft pillows. They lost the originals' crisp structure, contrast and fittings.

**Box kit, the gunship only, as a trial.** It had bevelled blocks, rounded pod fronts, dark gaps,
tones per part and lights matched to the original.
- Fit: IoU 0.92 / 0.91, outline 0.59 px.
- Appearance: region colours within 8–11 levels; SSIM 0.43 against 0.42 for the pillow version;
  contrast 0.60 of the original's.
- In game at UI 2× it was cleaner but plainer than the original. It lost the lit-to-shadow
  gradient, the crown of nose fins, the lamp glints and most tier detail, so it failed the gate.

Why it fails: most Ishiman ships were rendered nearly overhead (camera tilt 12–22°). Their frames
can't measure heights, so tiers, fins and steps would be invented. A fitted shape then comes out
flatter and plainer than the hand-detailed original.

**Decision (2026-10-19): Ishiman ships stay original.** Only the reusable tools were committed. The
trial files are kept, uncommitted, in `out/sprite-render/trials/ish/`.

## 5. Where finished art lives (proposal)

**A Star Bound art folder, `art/`, at the repository root, checked before `data/`.** This follows
your suggestion.

- **Layout** mirrors `data/` but holds only the files we replace, for example
  `art/sprites/uns/cruiser/image.png`, `overlay.png`, `image@2x.png`, `overlay@2x.png`,
  `image@4x.png` and `overlay@4x.png`.
  - Frame tables (`.pn`) are never copied, so geometry keeps coming from `data/`.
  - Resources are looked up file by file, so a sprite can take its images from `art/` and its
    frame table from `data/`.
- **Loader:** one more entry in the resource search order (`src/data/resource.cpp`, which today
  checks plugin, then factory scenario folder, then `data/`). The new order: plugin → factory
  scenario folder (still used for test overrides) → `art/` → `data/`.
  - A command-line flag (for example `--art <dir>` or `--no-art`) allows A/B comparison with
    the originals.
  - `scripts/dist.py` copies `art/` next to `data/`.
  - `data/` (a submodule of the upstream data repository) stays untouched.
- **Size:** the three finished ships take 4 MB of PNGs (the carrier alone 2.8 MB), so the fleet
  would be roughly 80–120 MB. Re-renders make git history grow fast.
  - I recommend tracking `art/**/*.png` with Git LFS. GitHub's free LFS quota is 1 GB of storage
    and 1 GB of bandwidth a month.
  - The alternative is a separate private art repository as a submodule.
  - Render inputs (the shape modules and fit files) stay in `tools/sprite-render/`, so any sheet
    can be rebuilt.

## 6. When `ASSETS.md` rows change

A sprite's row changes from **original** to **replaced** when all of these are true:
1. its 1×, @2x and @4x `image`/`overlay` sheets are committed in `art/`;
2. a normal build loads them by default (the `art/` loader is in place);
3. you've approved its batch;
4. the CI tests and replays pass.

That happens in the same commit that adds the art.

Two refinements are needed for accuracy:
- **Frame tables stay original.** Only the images are replaced; the `.pn` frame table (the
  geometry used for collisions) stays the original data. The row should read "replaced (images);
  frame table original", or be split into two rows.
- **Portraits are separate.** `data/pictures/<race>/<ship>.png` (shown in briefings and the build
  menu) stays **original** until it is replaced in its own right.

**Licensing caution (not legal advice).** The remasters are fitted to the original sprites, and
their colours are measured from them. That makes them arguably **adaptations** of CC BY-NC-SA 3.0
material, which would still carry BY-NC-SA (non-commercial, share-alike) even when marked
"replaced". If Star Bound ever needs to be free of the original licence, the ships would need
designs made independently of the originals. That's worth deciding before the art folder becomes
the default.

## 7. Decisions needed

1. Approve the batch order (exposure-first, Ishiman first). The alternative is to start with
   batch 2 (Gaitori), which needs no new kit, and build the smooth kit second.
2. Keep the original EVA troopers (F) for now?
3. Art location: an `art/` folder with Git LFS, or a separate art repository?
4. The `ASSETS.md` wording, and the licensing question above.
5. The carrier looks brighter in game than its original neighbours: keep the average-tone
   calibration, or also match local contrast (darker, mottled decks)?
