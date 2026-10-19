# Upscale decisions (approved 2026-10-19)

One row per rotating sprite (78). Image and overlay: **ai** = Real-ESRGAN realesr-animevideov3 inside the tolerant outline; **tolerant** = Scale2x within 24 RGB levels; **original** = no @2x/@4x sheet; **art** = the approved 3D remaster in `art/`. Auto flags from `upscale_report.py` (AI version); "manual" = seen in the visual pass. Unsure rows are marked ⚠.

| Sprite | Ship | Image | Overlay | Auto flags (AI) | Agreement orig→chosen | Reason |
|---|---|---|---|---|---|---|
| `aud/aslttran` | Audemedon Assault Transport | tolerant | tolerant | — | 1.00→1.00 | changed 2026-10-19: the AI image added dark blue-black streaks along the hull |
| `aud/carrier` | Carrier | ai | tolerant | — | 1.00→0.96 | clean hull, tanks and fins kept |
| `aud/cruiser` | Cruiser | ai | tolerant | — | 1.00→0.91 | clean, shape kept |
| `aud/evat` | Audemedon Extra-vehicular As | original | original | overlay | — | EVA trooper: stays original (decision 2026-10-18); AI turns it into a white blob |
| ⚠ `aud/fighter` | Fighter | ai | tolerant | — | 1.00→0.92 | 16 px: AI is a soft blob, no clear features either way |
| `aud/gunship` | Gunship | ai | tolerant | — | 1.00→0.96 | clean, spherical tanks kept |
| `aud/hvd` | Heavy Destroyer | ai | tolerant | — | 1.00→0.95 | clean, keeps blocks and tanks |
| `aud/transport` | Transport | ai | tolerant | — | 1.00→0.94 | clean, fins kept |
| `baz/bttlship` | Battleship | ai | tolerant | — | 0.99→0.92 | excellent: smooth organic hull, horns and blue undersides kept |
| `can/aslttran` | Assault Transport | ai | tolerant | overlay | 1.00→0.88 | clean, keeps fins |
| `can/carrier` | Carrier | ai | tolerant | — | 1.00→0.90 | clean organic hull |
| `can/cruiser` | Cruiser | tolerant | tolerant | overlay | 0.99→0.99 | AI loses the one-pixel ladder of stripes |
| ⚠ `can/defdrone` | Defense Drone | tolerant | tolerant | overlay | 0.64→0.62 | tiny; AI tinted version messy |
| `can/engineer` | Engineer Pod | ai | tolerant | overlay | 0.93→0.81 | clean, light kept |
| `can/evat` | EVA Trooper | original | original | overlay | — | EVA trooper: stays original |
| ⚠ `can/fighter` | Fighter | ai | tolerant | overlay | 0.49→0.43 | tiny; AI slightly cleaner, little to gain; partial overlay coverage |
| `can/gateship` | Instead of dying, opens up j | tolerant | tolerant | — | 0.99→0.99 | changed 2026-10-19: the AI image turned a small structure on the upper blade into glyph-like squiggles |
| ⚠ `can/gunship` | Gunship | ai | tolerant | softened | 1.00→0.90 | AI merges the plate pattern into smooth blocks |
| `can/hvc` | Heavy Cruiser | ai | tolerant | overlay | 1.00→0.90 | clean, smoother |
| `can/hvd` | Heavy Destroyer | ai | tolerant | — | 0.99→0.92 | clean, keeps fins |
| `can/schooner` | Schooner | tolerant | tolerant | overlay, softened | 0.99→0.99 | AI fades the red light dots and loses plate lines |
| `can/transport` | Transport | ai | tolerant | softened | 1.00→0.92 | clean, slight softening only |
| `dev/amissile` | Missile | original | original | overlay | — | weapon, not a ship: later effects pass |
| `dev/cm` | C Missile | original | original | overlay | — | weapon, not a ship: later effects pass |
| `dev/fusion` | Fusion Pulse | original | original | overlay | — | weapon, not a ship: later effects pass |
| `ele/cruiser` | Cruiser | ai | tolerant | overlay | 0.98→0.90 | clean, grooves kept; tolerant stays noisy |
| `ele/etc/liner` | Luxury Liner | ai | tolerant | — | 0.99→0.95 | clean ribbed hull, speckle dither gone |
| `gai/aslttran` | Gaitori Assault Transport | ai | tolerant | overlay | 0.99→0.89 | clean, drums and guns kept |
| `gai/carrier` | Carrier | ai | tolerant | softened | 1.00→0.91 | panel detail kept; 'softened' flag was dither |
| `gai/cruiser` | Cruiser | tolerant | tolerant | overlay, softened | 0.98→0.98 | AI loses the blue accent lights and thickens the barrels |
| `gai/engineer` | Engineer Pod | ai | tolerant | overlay | 0.99→0.88 | clean |
| `gai/evat` | Gaitori Extra-vehicular Assa | original | original | overlay | — | EVA trooper: stays original |
| `gai/fighter` | Fighter | tolerant | tolerant | overlay | 0.98→0.98 | AI loses the blue accent lights |
| ⚠ `gai/gunship` | Gunship | ai | tolerant | overlay | 1.00→0.87 | AI keeps barrels but smooths the ribbed drum pods |
| ⚠ `gai/hvd` | Gaitori Heavy Destroyer | ai | tolerant | — | 0.99→0.93 | AI smooths the ribbed drum pods |
| `gai/transport` | Transport | ai | tolerant | overlay | 0.99→0.90 | container segments and antenna kept |
| `gro/cruiser` | Light Cruiser, Grolk | ai | tolerant | overlay | 1.00→0.91 | clean, barrels kept |
| `gro/ctf/cruiser` | Cruiser, Grolk for CTF, P1 | ai | tolerant | overlay | 1.00→0.90 | clean |
| `gro/ctf/flag` | Flag, for CTF, P1 | ai | tolerant | overlay | 1.00→0.89 | clean |
| `ish/aslttran` | Assault Transport | ai | tolerant | — | 0.60→0.56 | clean |
| `ish/carrier` | Carrier | ai | tolerant | overlay | 0.93→0.85 | clean, detail lines kept |
| ⚠ `ish/cruiser` | Development Notes | ai | tolerant | overlay | 0.55→0.58 | hub and arms clean, dither gone; overlay covers only part of the ship, so under tint the AI image shows through |
| ⚠ `ish/defdrone` | Defense Drone | tolerant | tolerant | overlay | 0.46→0.45 | tiny; AI tinted version messy |
| `ish/engineer` | Engineer Pod | ai | tolerant | overlay | 0.65→0.63 | clean, light kept |
| `ish/etc/astrominer` | Astrominer | ai | tolerant | — | 0.87→0.83 | clean, white stripe kept |
| `ish/etc/cargo` | Cargo Ship | ai | tolerant | — | 0.84→0.78 | clean, light kept |
| `ish/etc/research` | Research Vessel | ai | tolerant | — | 0.84→0.82 | clean cylinders, lights kept |
| `ish/evat` | EVA Trooper | original | original | overlay | — | EVA trooper: stays original |
| ⚠ `ish/fighter` | Fighter | ai | tolerant | overlay, softened | 1.00→0.88 | tiny; AI slightly softer |
| `ish/gunship` | Gunship | ai | tolerant | — | 1.00→0.93 | clean, tiers and pods kept |
| `ish/hvc` | Heavy Cruiser | tolerant | tolerant | overlay | 0.20→0.17 | changed 2026-10-19: the overlay covers only part of the ship, so under tint the AI image showed through as cyan streaks |
| `ish/hvd` | Heavy Destroyer | ai | tolerant | — | 0.95→0.92 | clean, lights kept |
| ⚠ `ish/schooner` | Schooner | ai | tolerant | overlay | 0.64→0.63 | thin spine kept; overlay covers only part of the ship, so under tint the AI image shows through |
| `ish/transport` | Transport | tolerant | tolerant | overlay | 0.11→0.10 | changed 2026-10-19: the overlay covers only part of the ship, so under tint the AI image showed through as cyan streaks |
| `loc/flak` | Border Drone | ai | tolerant | overlay | 1.00→0.89 | clean panelled sphere |
| `obi/escort` | Escort | tolerant | tolerant | overlay | 0.93→0.93 | AI melts the ribbed fan disc (one-pixel ribs), the class's defining feature |
| ⚠ `obi/etc/liner` | Battlecruiser | ai | tolerant | overlay | 0.99→0.85 | AI keeps the comb of fins but smooths the ribbed hull |
| `obi/etc/obiard/turning` | Returning to Base | original | original | — | — | tiny figure, not a ship |
| `obi/transport` | Transport | tolerant | tolerant | overlay | 1.00→1.00 | AI melts the ribbed fan disc |
| `sal/aslttran` | Assault Transport | ai | tolerant | softened | 0.94→0.88 | pods and spikes kept, slightly blurred |
| `sal/carrier` | Carrier | ai | tolerant | softened | 0.92→0.90 | clearly better: clean gloss; 'softened' flag was dither speckle |
| ⚠ `sal/cruiser` | Cruiser | ai | tolerant | overlay, softened | 0.03→0.02 (never followed) | special case: AI image cleans the speckle untinted; the tolerant overlay (fidelity 0.976) keeps the tinted look like the original's dark speckle |
| `sal/evat` | Salrilian Extra-vehicular As | original | original | overlay | — | EVA trooper: stays original |
| ⚠ `sal/fighter` | Fighter | tolerant | tolerant | overlay | 0.17→0.15 | AI invents a bright white streak on a near-invisible dark sliver |
| ⚠ `sal/gunship` | Gunship | ai | tolerant | — | 0.98→0.91 | AI loses the faint plating pattern |
| ⚠ `sal/hvd` | Salrilian Heavy Destroyer | ai | tolerant | overlay | 0.49→0.49 | clean gloss, lights kept; overlay covers only part of the ship, so under tint the AI image shows through |
| ⚠ `sal/transport` | Transport | ai | tolerant | overlay, softened | 0.67→0.57 | AI smooths small details |
| `sfx/crew` | Castaway Crew Member | original | original | overlay | — | effect, not a ship: later effects pass |
| `sfx/gunsmoke` | Gunsmoke | original | original | size/alpha | — | semi-transparent smoke effect: later effects pass |
| `uns/aslttran` | Human Assault Transport | ai | tolerant | — | 0.98→0.91 | clean, red lights kept |
| `uns/carrier` | Carrier | art | art | overlay | 0.99→0.60 | approved 3D remaster in art/; AI loses the red engine lights and smears the deck |
| `uns/cruiser` | Cruiser | art | art | overlay | 0.99→0.42 | approved 3D remaster in art/; AI dulls the red lights |
| `uns/etc/bttlship` | Battleship | ai | tolerant | overlay | 0.84→0.75 | clean, two-tone kept |
| `uns/etc/human/turning` | Needs rescuing, Not Waving | original | original | overlay | — | tiny figure, not a ship |
| `uns/fighter` | Fighter | art | art | overlay | 0.98→0.69 | approved 3D remaster in art/ |
| `uns/gunship` | Gunship | ai | tolerant | overlay | 0.99→0.90 | clean |
| `uns/hvd` | Human Heavy Destroyer | ai | tolerant | — | 0.97→0.91 | clean, red lights kept (dimmer) |
| `uns/transport` | Transport | ai | tolerant | — | 0.98→0.92 | clean, red lights kept |

Totals: 50 image ai / overlay tolerant, 12 image original / overlay original, 13 image tolerant / overlay tolerant, 3 image art / overlay art. Unsure: 16.
