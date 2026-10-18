# Star Bound: Art Direction (ships, v0.1)

How new ship sprites should look. It applies to every sprite rendered with
`tools/sprite-render/`. Based on a study of the original sprites across the factions
(2026-10-18); see `docs/current-state.md` §4.3 for how sprites work technically.

## What players actually see

- **Each faction's own paint.** A side is tinted only if its level sets a `hue`, and almost none
  do: the default is grey, which draws `image.png` exactly as painted. So `image.png` is the look
  of the game. The team-colour `overlay.png` is a fallback for levels that assign colours.
- **Tiny sprites.** At 1× most ships are 16–60 px. Fighters are about 16 px; the largest carrier
  is 108 px. Players see them at 1:1, 1:2 and smaller, and as icons below 1:4. Higher UI scales
  use the `@2x`/`@4x` sheets, which is where extra detail shows.

## What stays the same

1. **Silhouette and proportions.** Each ship keeps its recognisable outline, length-to-width
   ratio and the placement of its main parts (hull, pods, wings, turrets). New art must fit the
   original frame rectangles and pivots: collision bounds and weapon mounts come from them. Aim
   for under 1 % overflow per frame (the render script's fit check).
2. **The three-quarter camera.** The originals are seen from above at a tilt, from the bottom of
   the screen, so you see a little of each ship's side. Tilt is set per ship (about 44° for the
   Human cruiser) to match its original. Frame 0 points up and frames turn clockwise.
3. **Team colour on the whole ship, as in the originals.** The overlay covers every part,
   engine glows and lights included, so a tinted side reads as one colour and its lights take the
   tint too (decided 2026-10-18, matching the originals' overlays). Surface detail must therefore
   come from shape and shading, not only from paint. Under a tint, only the overlay's brightness
   survives, so glows must stay the brightest parts of the overlay.
4. **Readability at small sizes.** At 1× and at 1:2 zoom a ship must still read as itself:
   - a clear silhouette and a light-to-dark separation between major parts;
   - no busy, high-contrast surface noise that turns to grain;
   - facing obvious from the nose and the engine glows;
   - detail added at 1× only if it survives the reduction; fine detail belongs to `@2x`/`@4x`.
5. **The original colour mood.** Each faction keeps its palette and overall brightness:

   | Faction | Mood (from the originals) |
   |---|---|
   | Human (`uns`) | Neutral grey/white metal, mid brightness, red engine points |
   | Ishiman (`ish`) | Saturated cyan |
   | Cantharan (`can`) | Mid green, darker |
   | Gaitori (`gai`) | Bright white/silver |
   | Salrilian (`sal`) | Near-black gloss, very dark |
   | Audemedon (`aud`) | Orange/bronze |
   | Obish (`obi`) | Saturated blue |
   | Bazidan (`baz`) | Dusky rose |

   Engine glows keep their original colours (Human: red).

## What we're modernising

1. **Physically based metal.** Metallic materials with varied roughness, so panels and trim
   catch light differently instead of looking like one smooth plastic.
2. **Bevelled edges and panel lines.** Small bevels on every hard edge so highlights trace the
   form; recessed panel lines and seams that follow the shape. Lines live in geometry or the
   normal, not just paint, so they also show under team tint.
3. **Subtle surface detail.** Plating variation, vents, ribbing and small greebles where the
   original suggests them, kept at a scale that adds richness at `@2x`/`@4x` without becoming
   noise at 1×.
4. **Ambient occlusion and soft self-shadowing.** Ray-traced (Cycles) so parts sit in contact
   shadows and crevices darken, which separates parts at small sizes.
5. **Lighting fixed to the camera.** A key light from the upper left (as the originals) plus a
   cooler rim light from behind to outline the silhouette against black space. Lights stay fixed
   while the ship turns, so every frame is lit the same way on screen.
6. **Emissive glows and lights.** Engine nozzles and running lights are real emitters in the
   faction's glow colour, bright enough to read at 1×. Like the rest of the ship they take the
   team tint (§3).

## Levels of modernisation (trial on the Human cruiser)

Three variants test how far to go. All three share the same silhouette.

- **A, subtle.** Close to the original: PBR metal with mild roughness variation, small bevels,
  AO, key light with a faint rim, emissive engines.
- **B, balanced.** A plus panel lines, ribbed engine pods, varied plating, stronger rim light,
  small running lights.
- **C, richest.** B plus greebles (vents, antennas, radiator fins), edge wear, contrasting trim
  and the strongest rim and glow.

The chosen level becomes the standard for the rest of the fleet.

*Superseded (2026-10-18):* none of the three was chosen; they drifted from the original design.
Ships are now remastered from their original frames instead (`docs/design.md` §6), with the
modernisation in "What we're modernising" applied to the fitted shape.
