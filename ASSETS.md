# Star Bound asset register

This file lists every game asset and whether it is **original** (inherited from Ares via the
Antares data, not yet replaced by Star Bound), **replaced** (an original we have swapped for our
own work), **adapted** (an original we have reworked, such as upscaled @2x/@4x sheets, so it is
still derived from the original and keeps its licence), or **new** (added by Star Bound with no
original counterpart). It exists because the
original Ares media is licensed CC BY-NC-SA 3.0 (`data/COPYING`): free, non-commercial use only.
Selling the game requires every entry to be *replaced* or *new*. When you replace or add an asset,
update its row in the same commit. See `docs/design.md` §6 and §8.

Notes:
- Per `data/README.md`, some in-repo sounds are new sfxr-generated sounds made by the Antares
  project for sounds missing from the CC release. Which ones is not recorded; they are listed as
  original here until confirmed.
- Paths are relative to the repo root unless noted. Generated from the data tree on 2026-10-18.

## Summary

| Category | Entries |
|---|---|
| Sprites | 191 |
| Sounds — in repo | 81 |
| Sounds — downloaded factory scenario | 29 |
| Music | 9 |
| Text — long text | 4 |
| Text — UI strings | 15 |
| Text — scenario info | 1 |
| Text — levels | 31 |
| Text — races | 16 |
| Text — objects | 336 |
| Interface art — pictures | 106 |
| Interface art — fonts | 5 |
| Interface art — layouts | 11 |
| Other original data | 4 |
| **Total** | **839** |

163 entries are **original**, 30 are **adapted** and 3 are **replaced** (of 196).
- **Replaced (3):** the Human fighter, cruiser and carrier images, 3D remasters rendered into `art/`
  (2026-10-18); their frame tables stay original.
- **Adapted (30):** ship sprites whose @2x/@4x sheets were upscaled from the original pixels into
  `art/` (2026-10-19, `docs/upscale-decisions.md`): Real-ESRGAN for the image where it looked
  better, tolerant Scale2x otherwise and for every overlay. Their 1x sheets and frame tables are the
  originals, read from `data/`.

Both kinds are derived from the originals: the remasters are fitted to and coloured from them, and
the upscales are made from their pixels. So they are most likely *adaptations* of the CC BY-NC-SA
3.0 originals and keep that licence; they don't free the game from it. Whether the remasters
count for selling the game is an open question (`docs/design.md` §10).

## Sprites, sounds, music, text and interface art

### Sprites (191)

Each entry is a frame table (`.pn`) plus its `image.png` and `overlay.png` (the overlay is tinted with the owner's colour).

| Asset | Images | Status |
|---|---|---|
| `data/sprites/ast/nasty.pn` | image.png, overlay.png | original |
| `data/sprites/ast/regular/big.pn` | image.png, overlay.png | original |
| `data/sprites/ast/regular/medium.pn` | image.png, overlay.png | original |
| `data/sprites/ast/regular/small.pn` | image.png, overlay.png | original |
| `data/sprites/aud/aslttran.pn` | image.png, overlay.png | adapted: @2x/@4x upscaled from the original (image AI (Real-ESRGAN realesr-animevideov3), overlay tolerant Scale2x), CC BY-NC-SA, `art/sprites/aud/aslttran/`; 1x and frame table original |
| `data/sprites/aud/carrier.pn` | image.png, overlay.png | adapted: @2x/@4x upscaled from the original (image AI (Real-ESRGAN realesr-animevideov3), overlay tolerant Scale2x), CC BY-NC-SA, `art/sprites/aud/carrier/`; 1x and frame table original |
| `data/sprites/aud/cruiser.pn` | image.png, overlay.png | adapted: @2x/@4x upscaled from the original (image AI (Real-ESRGAN realesr-animevideov3), overlay tolerant Scale2x), CC BY-NC-SA, `art/sprites/aud/cruiser/`; 1x and frame table original |
| `data/sprites/aud/etc/device.pn` | image.png, overlay.png | original |
| `data/sprites/aud/evat.pn` | image.png, overlay.png | original |
| `data/sprites/aud/fighter.pn` | image.png, overlay.png | adapted: @2x/@4x upscaled from the original (image AI (Real-ESRGAN realesr-animevideov3), overlay tolerant Scale2x), CC BY-NC-SA, `art/sprites/aud/fighter/`; 1x and frame table original |
| `data/sprites/aud/gunship.pn` | image.png, overlay.png | adapted: @2x/@4x upscaled from the original (image AI (Real-ESRGAN realesr-animevideov3), overlay tolerant Scale2x), CC BY-NC-SA, `art/sprites/aud/gunship/`; 1x and frame table original |
| `data/sprites/aud/hvd.pn` | image.png, overlay.png | adapted: @2x/@4x upscaled from the original (image AI (Real-ESRGAN realesr-animevideov3), overlay tolerant Scale2x), CC BY-NC-SA, `art/sprites/aud/hvd/`; 1x and frame table original |
| `data/sprites/aud/transport.pn` | image.png, overlay.png | adapted: @2x/@4x upscaled from the original (image AI (Real-ESRGAN realesr-animevideov3), overlay tolerant Scale2x), CC BY-NC-SA, `art/sprites/aud/transport/`; 1x and frame table original |
| `data/sprites/baz/bttlship.pn` | image.png, overlay.png | adapted: @2x/@4x upscaled from the original (image AI (Real-ESRGAN realesr-animevideov3), overlay tolerant Scale2x), CC BY-NC-SA, `art/sprites/baz/bttlship/`; 1x and frame table original |
| `data/sprites/can/aslttran.pn` | image.png, overlay.png | adapted: @2x/@4x upscaled from the original (image AI (Real-ESRGAN realesr-animevideov3), overlay tolerant Scale2x), CC BY-NC-SA, `art/sprites/can/aslttran/`; 1x and frame table original |
| `data/sprites/can/carrier.pn` | image.png, overlay.png | adapted: @2x/@4x upscaled from the original (image AI (Real-ESRGAN realesr-animevideov3), overlay tolerant Scale2x), CC BY-NC-SA, `art/sprites/can/carrier/`; 1x and frame table original |
| `data/sprites/can/cruiser.pn` | image.png, overlay.png | adapted: @2x/@4x upscaled from the original (image tolerant Scale2x, overlay tolerant Scale2x), CC BY-NC-SA, `art/sprites/can/cruiser/`; 1x and frame table original |
| `data/sprites/can/defdrone.pn` | image.png, overlay.png | adapted: @2x/@4x upscaled from the original (image tolerant Scale2x, overlay tolerant Scale2x), CC BY-NC-SA, `art/sprites/can/defdrone/`; 1x and frame table original |
| `data/sprites/can/engineer.pn` | image.png, overlay.png | adapted: @2x/@4x upscaled from the original (image AI (Real-ESRGAN realesr-animevideov3), overlay tolerant Scale2x), CC BY-NC-SA, `art/sprites/can/engineer/`; 1x and frame table original |
| `data/sprites/can/etc/cantharan.pn` | image.png, overlay.png | original |
| `data/sprites/can/evat.pn` | image.png, overlay.png | original |
| `data/sprites/can/fighter.pn` | image.png, overlay.png | adapted: @2x/@4x upscaled from the original (image AI (Real-ESRGAN realesr-animevideov3), overlay tolerant Scale2x), CC BY-NC-SA, `art/sprites/can/fighter/`; 1x and frame table original |
| `data/sprites/can/gateship.pn` | image.png, overlay.png | adapted: @2x/@4x upscaled from the original (image AI (Real-ESRGAN realesr-animevideov3), overlay tolerant Scale2x), CC BY-NC-SA, `art/sprites/can/gateship/`; 1x and frame table original |
| `data/sprites/can/gunship.pn` | image.png, overlay.png | adapted: @2x/@4x upscaled from the original (image AI (Real-ESRGAN realesr-animevideov3), overlay tolerant Scale2x), CC BY-NC-SA, `art/sprites/can/gunship/`; 1x and frame table original |
| `data/sprites/can/hvc.pn` | image.png, overlay.png | adapted: @2x/@4x upscaled from the original (image AI (Real-ESRGAN realesr-animevideov3), overlay tolerant Scale2x), CC BY-NC-SA, `art/sprites/can/hvc/`; 1x and frame table original |
| `data/sprites/can/hvd.pn` | image.png, overlay.png | adapted: @2x/@4x upscaled from the original (image AI (Real-ESRGAN realesr-animevideov3), overlay tolerant Scale2x), CC BY-NC-SA, `art/sprites/can/hvd/`; 1x and frame table original |
| `data/sprites/can/schooner.pn` | image.png, overlay.png | adapted: @2x/@4x upscaled from the original (image tolerant Scale2x, overlay tolerant Scale2x), CC BY-NC-SA, `art/sprites/can/schooner/`; 1x and frame table original |
| `data/sprites/can/transport.pn` | image.png, overlay.png | adapted: @2x/@4x upscaled from the original (image AI (Real-ESRGAN realesr-animevideov3), overlay tolerant Scale2x), CC BY-NC-SA, `art/sprites/can/transport/`; 1x and frame table original |
| `data/sprites/dev/amissile.pn` | image.png, overlay.png | original |
| `data/sprites/dev/anti.pn` | image.png, overlay.png | original |
| `data/sprites/dev/atomic.pn` | image.png, overlay.png | original |
| `data/sprites/dev/cform.pn` | image.png, overlay.png | original |
| `data/sprites/dev/chronon.pn` | image.png, overlay.png | original |
| `data/sprites/dev/cluster.pn` | image.png, overlay.png | original |
| `data/sprites/dev/cm.pn` | image.png, overlay.png | original |
| `data/sprites/dev/fireball.pn` | image.png, overlay.png | original |
| `data/sprites/dev/fullerene.pn` | image.png, overlay.png | original |
| `data/sprites/dev/fusion.pn` | image.png, overlay.png | original |
| `data/sprites/dev/inasa.pn` | image.png, overlay.png | original |
| `data/sprites/dev/magneto.pn` | image.png, overlay.png | original |
| `data/sprites/dev/magno.pn` | image.png, overlay.png | original |
| `data/sprites/dev/neutron.pn` | image.png, overlay.png | original |
| `data/sprites/dev/onas.pn` | image.png, overlay.png | original |
| `data/sprites/dev/photo.pn` | image.png, overlay.png | original |
| `data/sprites/dev/pp.pn` | image.png, overlay.png | original |
| `data/sprites/dev/quasi.pn` | image.png, overlay.png | original |
| `data/sprites/dev/rplasma.pn` | image.png, overlay.png | original |
| `data/sprites/ele/cruiser.pn` | image.png, overlay.png | adapted: @2x/@4x upscaled from the original (image AI (Real-ESRGAN realesr-animevideov3), overlay tolerant Scale2x), CC BY-NC-SA, `art/sprites/ele/cruiser/`; 1x and frame table original |
| `data/sprites/ele/etc/escape-pod.pn` | image.png, overlay.png | original |
| `data/sprites/ele/etc/liner.pn` | image.png, overlay.png | adapted: @2x/@4x upscaled from the original (image AI (Real-ESRGAN realesr-animevideov3), overlay tolerant Scale2x), CC BY-NC-SA, `art/sprites/ele/etc/liner/`; 1x and frame table original |
| `data/sprites/gai/aslttran.pn` | image.png, overlay.png | adapted: @2x/@4x upscaled from the original (image AI (Real-ESRGAN realesr-animevideov3), overlay tolerant Scale2x), CC BY-NC-SA, `art/sprites/gai/aslttran/`; 1x and frame table original |
| `data/sprites/gai/carrier.pn` | image.png, overlay.png | adapted: @2x/@4x upscaled from the original (image AI (Real-ESRGAN realesr-animevideov3), overlay tolerant Scale2x), CC BY-NC-SA, `art/sprites/gai/carrier/`; 1x and frame table original |
| `data/sprites/gai/cruiser.pn` | image.png, overlay.png | adapted: @2x/@4x upscaled from the original (image tolerant Scale2x, overlay tolerant Scale2x), CC BY-NC-SA, `art/sprites/gai/cruiser/`; 1x and frame table original |
| `data/sprites/gai/engineer.pn` | image.png, overlay.png | adapted: @2x/@4x upscaled from the original (image AI (Real-ESRGAN realesr-animevideov3), overlay tolerant Scale2x), CC BY-NC-SA, `art/sprites/gai/engineer/`; 1x and frame table original |
| `data/sprites/gai/etc/lab.pn` | image.png, overlay.png | original |
| `data/sprites/gai/evat.pn` | image.png, overlay.png | original |
| `data/sprites/gai/fighter.pn` | image.png, overlay.png | adapted: @2x/@4x upscaled from the original (image tolerant Scale2x, overlay tolerant Scale2x), CC BY-NC-SA, `art/sprites/gai/fighter/`; 1x and frame table original |
| `data/sprites/gai/gunship.pn` | image.png, overlay.png | adapted: @2x/@4x upscaled from the original (image AI (Real-ESRGAN realesr-animevideov3), overlay tolerant Scale2x), CC BY-NC-SA, `art/sprites/gai/gunship/`; 1x and frame table original |
| `data/sprites/gai/hvd.pn` | image.png, overlay.png | adapted: @2x/@4x upscaled from the original (image AI (Real-ESRGAN realesr-animevideov3), overlay tolerant Scale2x), CC BY-NC-SA, `art/sprites/gai/hvd/`; 1x and frame table original |
| `data/sprites/gai/transport.pn` | image.png, overlay.png | adapted: @2x/@4x upscaled from the original (image AI (Real-ESRGAN realesr-animevideov3), overlay tolerant Scale2x), CC BY-NC-SA, `art/sprites/gai/transport/`; 1x and frame table original |
| `data/sprites/gro/cruiser.pn` | image.png, overlay.png | original |
| `data/sprites/gro/ctf/cruiser.pn` | image.png, overlay.png | original |
| `data/sprites/gro/ctf/flag.pn` | image.png, overlay.png | original |
| `data/sprites/gro/ctf/moor.pn` | image.png, overlay.png | original |
| `data/sprites/gui/cursor.pn` | image.png, overlay.png | original |
| `data/sprites/ish/aslttran.pn` | image.png, overlay.png | original |
| `data/sprites/ish/carrier.pn` | image.png, overlay.png | original |
| `data/sprites/ish/cruiser.pn` | image.png, overlay.png | original |
| `data/sprites/ish/defdrone.pn` | image.png, overlay.png | original |
| `data/sprites/ish/engineer.pn` | image.png, overlay.png | original |
| `data/sprites/ish/etc/astrominer.pn` | image.png, overlay.png | original |
| `data/sprites/ish/etc/cargo.pn` | image.png, overlay.png | original |
| `data/sprites/ish/etc/research.pn` | image.png, overlay.png | original |
| `data/sprites/ish/etc/trgtdrne.pn` | image.png, overlay.png | original |
| `data/sprites/ish/evat.pn` | image.png, overlay.png | original |
| `data/sprites/ish/fighter.pn` | image.png, overlay.png | original |
| `data/sprites/ish/gateship.pn` | image.png, overlay.png | original |
| `data/sprites/ish/gunship.pn` | image.png, overlay.png | original |
| `data/sprites/ish/hvc.pn` | image.png, overlay.png | original |
| `data/sprites/ish/hvd.pn` | image.png, overlay.png | original |
| `data/sprites/ish/schooner.pn` | image.png, overlay.png | original |
| `data/sprites/ish/transport.pn` | image.png, overlay.png | original |
| `data/sprites/loc/bunker.pn` | image.png, overlay.png | original |
| `data/sprites/loc/flak.pn` | image.png, overlay.png | original |
| `data/sprites/loc/habitat.pn` | image.png, overlay.png | original |
| `data/sprites/loc/jump/node.pn` | image.png, overlay.png | original |
| `data/sprites/loc/moor.pn` | image.png, overlay.png | original |
| `data/sprites/loc/outpost.pn` | image.png, overlay.png | original |
| `data/sprites/loc/planets/aqua-minerale.pn` | image.png, overlay.png | original |
| `data/sprites/loc/planets/bahamas.pn` | image.png, overlay.png | original |
| `data/sprites/loc/planets/bluegas.pn` | image.png, overlay.png | original |
| `data/sprites/loc/planets/blueyellow-marble.pn` | image.png, overlay.png | original |
| `data/sprites/loc/planets/brightgas.pn` | image.png, overlay.png | original |
| `data/sprites/loc/planets/bruised.pn` | image.png, overlay.png | original |
| `data/sprites/loc/planets/calmpurple.pn` | image.png, overlay.png | original |
| `data/sprites/loc/planets/clay.pn` | image.png, overlay.png | original |
| `data/sprites/loc/planets/cloudtones.pn` | image.png, overlay.png | original |
| `data/sprites/loc/planets/dirtcloud.pn` | image.png, overlay.png | original |
| `data/sprites/loc/planets/earth.pn` | image.png, overlay.png | original |
| `data/sprites/loc/planets/fall.pn` | image.png, overlay.png | original |
| `data/sprites/loc/planets/goldtone.pn` | image.png, overlay.png | original |
| `data/sprites/loc/planets/graysplotch.pn` | image.png, overlay.png | original |
| `data/sprites/loc/planets/jungle.pn` | image.png, overlay.png | original |
| `data/sprites/loc/planets/lava.pn` | image.png, overlay.png | original |
| `data/sprites/loc/planets/marble-streakedpurple.pn` | image.png, overlay.png | original |
| `data/sprites/loc/planets/marshmellow.pn` | image.png, overlay.png | original |
| `data/sprites/loc/planets/marslike.pn` | image.png, overlay.png | original |
| `data/sprites/loc/planets/moldymud.pn` | image.png, overlay.png | original |
| `data/sprites/loc/planets/nastyswirl.pn` | image.png, overlay.png | original |
| `data/sprites/loc/planets/purple-stripes-ugly.pn` | image.png, overlay.png | original |
| `data/sprites/loc/planets/purpleblue.pn` | image.png, overlay.png | original |
| `data/sprites/loc/planets/purplegas.pn` | image.png, overlay.png | original |
| `data/sprites/loc/planets/purpleyellow-trouble.pn` | image.png, overlay.png | original |
| `data/sprites/loc/planets/purtygas.pn` | image.png, overlay.png | original |
| `data/sprites/loc/planets/redwhite-excitement.pn` | image.png, overlay.png | original |
| `data/sprites/loc/planets/sandyred.pn` | image.png, overlay.png | original |
| `data/sprites/loc/planets/saturny.pn` | image.png, overlay.png | original |
| `data/sprites/loc/planets/splatterpaint-pink.pn` | image.png, overlay.png | original |
| `data/sprites/loc/planets/sta-puft.pn` | image.png, overlay.png | original |
| `data/sprites/loc/planets/tiedye-lava.pn` | image.png, overlay.png | original |
| `data/sprites/loc/planets/uglypaint.pn` | image.png, overlay.png | original |
| `data/sprites/loc/planets/yellow-cookie.pn` | image.png, overlay.png | original |
| `data/sprites/loc/planets/yellow.pn` | image.png, overlay.png | original |
| `data/sprites/loc/planets/yerrear.pn` | image.png, overlay.png | original |
| `data/sprites/loc/power.pn` | image.png, overlay.png | original |
| `data/sprites/loc/relay.pn` | image.png, overlay.png | original |
| `data/sprites/loc/shipyard.pn` | image.png, overlay.png | original |
| `data/sprites/loc/sun.pn` | image.png, overlay.png | original |
| `data/sprites/obi/escort.pn` | image.png, overlay.png | original |
| `data/sprites/obi/etc/liner.pn` | image.png, overlay.png | original |
| `data/sprites/obi/etc/obiard/turning.pn` | image.png, overlay.png | original |
| `data/sprites/obi/etc/obiard/waving.pn` | image.png, overlay.png | original |
| `data/sprites/obi/transport.pn` | image.png, overlay.png | original |
| `data/sprites/sal/aslttran.pn` | image.png, overlay.png | original |
| `data/sprites/sal/carrier.pn` | image.png, overlay.png | original |
| `data/sprites/sal/cruiser.pn` | image.png, overlay.png | original |
| `data/sprites/sal/etc/battlestation.pn` | image.png, overlay.png | original |
| `data/sprites/sal/etc/lab.pn` | image.png, overlay.png | original |
| `data/sprites/sal/evat.pn` | image.png, overlay.png | original |
| `data/sprites/sal/fighter.pn` | image.png, overlay.png | original |
| `data/sprites/sal/gunship.pn` | image.png, overlay.png | original |
| `data/sprites/sal/hvd.pn` | image.png, overlay.png | original |
| `data/sprites/sal/transport.pn` | image.png, overlay.png | original |
| `data/sprites/sfx/crew.pn` | image.png, overlay.png | original |
| `data/sprites/sfx/debris.pn` | image.png, overlay.png | original |
| `data/sprites/sfx/energy.pn` | image.png, overlay.png | original |
| `data/sprites/sfx/explosion/energy.pn` | image.png, overlay.png | original |
| `data/sprites/sfx/explosion/flak.pn` | image.png, overlay.png | original |
| `data/sprites/sfx/explosion/gas.pn` | image.png, overlay.png | original |
| `data/sprites/sfx/explosion/large-hit.pn` | image.png, overlay.png | original |
| `data/sprites/sfx/explosion/quasi.pn` | image.png, overlay.png | original |
| `data/sprites/sfx/gunsmoke.pn` | image.png, overlay.png | original |
| `data/sprites/sfx/invisible.pn` | image.png, overlay.png | original |
| `data/sprites/sfx/jump.pn` | image.png, overlay.png | original |
| `data/sprites/sfx/sparkle.pn` | image.png, overlay.png | original |
| `data/sprites/sfx/warp.pn` | image.png, overlay.png | original |
| `data/sprites/tmp/foe/carrier.pn` | image.png, overlay.png | original |
| `data/sprites/tmp/foe/cruiser.pn` | image.png, overlay.png | original |
| `data/sprites/tmp/foe/fighter.pn` | image.png, overlay.png | original |
| `data/sprites/tmp/foe/transport.pn` | image.png, overlay.png | original |
| `data/sprites/tmp/friend/carrier.pn` | image.png, overlay.png | original |
| `data/sprites/tmp/friend/cruiser.pn` | image.png, overlay.png | original |
| `data/sprites/tmp/friend/fighter.pn` | image.png, overlay.png | original |
| `data/sprites/tmp/friend/transport.pn` | image.png, overlay.png | original |
| `data/sprites/uns/ares.pn` | image.png, overlay.png | original |
| `data/sprites/uns/aslttran.pn` | image.png, overlay.png | original |
| `data/sprites/uns/carrier.pn` | image.png, overlay.png | replaced (images, remastered: `art/sprites/uns/carrier/`, adaptation); frame table original |
| `data/sprites/uns/cruiser.pn` | image.png, overlay.png | replaced (images, remastered: `art/sprites/uns/cruiser/`, adaptation); frame table original |
| `data/sprites/uns/etc/bttlship.pn` | image.png, overlay.png | original |
| `data/sprites/uns/etc/human/turning.pn` | image.png, overlay.png | original |
| `data/sprites/uns/etc/human/waving.pn` | image.png, overlay.png | original |
| `data/sprites/uns/fighter.pn` | image.png, overlay.png | replaced (images, remastered: `art/sprites/uns/fighter/`, adaptation); frame table original |
| `data/sprites/uns/gunship.pn` | image.png, overlay.png | original |
| `data/sprites/uns/hvd.pn` | image.png, overlay.png | original |
| `data/sprites/uns/transport.pn` | image.png, overlay.png | original |
| `data/sprites/zrb/base.pn` | image.png, overlay.png | original |
| `data/sprites/zzz/ast/meteor.pn` | image.png, overlay.png | original |
| `data/sprites/zzz/aud/carrier.pn` | image.png, overlay.png | original |
| `data/sprites/zzz/baz/bttlship.pn` | image.png, overlay.png | original |
| `data/sprites/zzz/can/fighter.pn` | image.png, overlay.png | original |
| `data/sprites/zzz/dev/green-rplasma.pn` | image.png, overlay.png | original |
| `data/sprites/zzz/dev/ugly-sphere.pn` | image.png, overlay.png | original |
| `data/sprites/zzz/earth.pn` | image.png, overlay.png | original |
| `data/sprites/zzz/fat/goldship.pn` | image.png, overlay.png | original |
| `data/sprites/zzz/ish/escort.pn` | image.png, overlay.png | original |
| `data/sprites/zzz/ish/etc/mothership.pn` | image.png, overlay.png | original |
| `data/sprites/zzz/loc/buoy.pn` | image.png, overlay.png | original |
| `data/sprites/zzz/loc/moor.pn` | image.png, overlay.png | original |
| `data/sprites/zzz/question-mark.pn` | image.png, overlay.png | original |
| `data/sprites/zzz/sfx/ftl.pn` | image.png, overlay.png | original |
| `data/sprites/zzz/sfx/supernova.pn` | image.png, overlay.png | original |

### Sounds — in repo (81)

AIFF files in `data/sounds/`.

| Asset | Status |
|---|---|
| `data/sounds/dev/anti.aiff` | original |
| `data/sounds/dev/asb.aiff` | original |
| `data/sounds/dev/assault/aud.aiff` | original |
| `data/sounds/dev/assault/can.aiff` | original |
| `data/sounds/dev/assault/gai.aiff` | original |
| `data/sounds/dev/assault/sal.aiff` | original |
| `data/sounds/dev/assault/uns.aiff` | original |
| `data/sounds/dev/atomic.aiff` | original |
| `data/sounds/dev/bay.aiff` | original |
| `data/sounds/dev/cform.aiff` | original |
| `data/sounds/dev/chronon.aiff` | original |
| `data/sounds/dev/cluster.aiff` | original |
| `data/sounds/dev/cm.aiff` | original |
| `data/sounds/dev/conc.aiff` | original |
| `data/sounds/dev/conc/ricochet/1.aiff` | original |
| `data/sounds/dev/conc/ricochet/2.aiff` | original |
| `data/sounds/dev/fireball.aiff` | original |
| `data/sounds/dev/fusion.aiff` | original |
| `data/sounds/dev/inasa.aiff` | original |
| `data/sounds/dev/kl.aiff` | original |
| `data/sounds/dev/laser/gun.aiff` | original |
| `data/sounds/dev/laser/turret.aiff` | original |
| `data/sounds/dev/lepton.aiff` | original |
| `data/sounds/dev/magneto.aiff` | original |
| `data/sounds/dev/magno.aiff` | original |
| `data/sounds/dev/mine.aiff` | original |
| `data/sounds/dev/neutron.aiff` | original |
| `data/sounds/dev/newo.aiff` | original |
| `data/sounds/dev/onas.aiff` | original |
| `data/sounds/dev/photo.aiff` | original |
| `data/sounds/dev/pk/can.aiff` | original |
| `data/sounds/dev/pk/ish.aiff` | original |
| `data/sounds/dev/pk/sal.aiff` | original |
| `data/sounds/dev/pp.aiff` | original |
| `data/sounds/dev/quasi/collide.aiff` | original |
| `data/sounds/dev/quasi/create.aiff` | original |
| `data/sounds/dev/reactivator.aiff` | original |
| `data/sounds/dev/repulser.aiff` | original |
| `data/sounds/dev/rplasma.aiff` | original |
| `data/sounds/dev/stealth/off.aiff` | original |
| `data/sounds/dev/stealth/on.aiff` | original |
| `data/sounds/dev/trazer.aiff` | original |
| `data/sounds/dev/tspace.aiff` | original |
| `data/sounds/gui/beep/build.aiff` | original |
| `data/sounds/gui/beep/button.aiff` | original |
| `data/sounds/gui/beep/message.aiff` | original |
| `data/sounds/gui/beep/morse.aiff` | original |
| `data/sounds/gui/beep/naughty.aiff` | original |
| `data/sounds/gui/beep/order.aiff` | original |
| `data/sounds/gui/beep/select.aiff` | original |
| `data/sounds/gui/beep/zoom.aiff` | original |
| `data/sounds/gui/klaxon.aiff` | original |
| `data/sounds/ish/etc/tug/attach.aiff` | original |
| `data/sounds/sfx/energy.aiff` | original |
| `data/sounds/sfx/explosion/energy.aiff` | original |
| `data/sounds/sfx/explosion/flak.aiff` | original |
| `data/sounds/sfx/explosion/gas.aiff` | original |
| `data/sounds/sfx/explosion/large.aiff` | original |
| `data/sounds/sfx/explosion/small.aiff` | original |
| `data/sounds/sfx/jump.aiff` | original |
| `data/sounds/sfx/pop.aiff` | original |
| `data/sounds/sfx/warp/charge/1.aiff` | original |
| `data/sounds/sfx/warp/charge/2.aiff` | original |
| `data/sounds/sfx/warp/charge/3.aiff` | original |
| `data/sounds/sfx/warp/charge/4.aiff` | original |
| `data/sounds/sfx/warp/in.aiff` | original |
| `data/sounds/sfx/warp/out.aiff` | original |
| `data/sounds/tpl/engineer/arrive.aiff` | original |
| `data/sounds/tpl/evat/board.aiff` | original |
| `data/sounds/tpl/evat/destroy.aiff` | original |
| `data/sounds/tpl/transport/land.aiff` | original |
| `data/sounds/zrb/collide.aiff` | original |
| `data/sounds/zrb/destroy.aiff` | original |
| `data/sounds/zzz/dev/ele-missile.aiff` | original |
| `data/sounds/zzz/dev/inert.aiff` | original |
| `data/sounds/zzz/dev/retro.aiff` | original |
| `data/sounds/zzz/dev/whip-laser.aiff` | original |
| `data/sounds/zzz/lectrocute.aiff` | original |
| `data/sounds/zzz/sfx/explosion/huge.aiff` | original |
| `data/sounds/zzz/transport/land/2.aiff` | original |
| `data/sounds/zzz/transport/land/3.aiff` | original |

### Sounds — downloaded factory scenario (29)

Not in the repo. Fetched from the original Ares 1.2.0 release by `antares-download-sounds` (or on first launch) into `Documents\Antares\Scenarios\4cab7415715aeeacf1486a352267ae82c0efb220\`.

| Asset | Status |
|---|---|
| `sounds/dev/anti.aiff` | original |
| `sounds/dev/conc.aiff` | original |
| `sounds/dev/conc/ricochet/1.aiff` | original |
| `sounds/dev/conc/ricochet/2.aiff` | original |
| `sounds/dev/fusion.aiff` | original |
| `sounds/dev/onas.aiff` | original |
| `sounds/dev/pk/can.aiff` | original |
| `sounds/dev/pk/sal.aiff` | original |
| `sounds/dev/repulser.aiff` | original |
| `sounds/dev/rplasma.aiff` | original |
| `sounds/gui/beep/build.aiff` | original |
| `sounds/gui/beep/button.aiff` | original |
| `sounds/gui/beep/message.aiff` | original |
| `sounds/gui/beep/order.aiff` | original |
| `sounds/gui/beep/select.aiff` | original |
| `sounds/ish/etc/tug/attach.aiff` | original |
| `sounds/sfx/energy.aiff` | original |
| `sounds/sfx/explosion/flak.aiff` | original |
| `sounds/sfx/explosion/gas.aiff` | original |
| `sounds/sfx/explosion/large.aiff` | original |
| `sounds/sfx/pop.aiff` | original |
| `sounds/tpl/evat/board.aiff` | original |
| `sounds/tpl/evat/destroy.aiff` | original |
| `sounds/tpl/transport/land.aiff` | original |
| `sounds/zrb/destroy.aiff` | original |
| `sounds/zzz/lectrocute.aiff` | original |
| `sounds/zzz/sfx/explosion/huge.aiff` | original |
| `sounds/zzz/transport/land/2.aiff` | original |
| `sounds/zzz/transport/land/3.aiff` | original |

### Music (9)

Tracker modules converted from the original 'MADH' resources.

| Asset | Status |
|---|---|
| `data/music/autoregret.s3m` | original |
| `data/music/doomtroopers-unite.s3m` | original |
| `data/music/eyes-of-fire.s3m` | original |
| `data/music/freds-theme.s3m` | original |
| `data/music/getalong.s3m` | original |
| `data/music/moonrise-patrol.s3m` | original |
| `data/music/targetron.s3m` | original |
| `data/music/technobee.s3m` | original |
| `data/music/yesterday.s3m` | original |

### Text — long text (4)

Help, object and weapon descriptions, par text.

| Asset | Status |
|---|---|
| `data/text/help.txt` | original |
| `data/text/object.txt` | original |
| `data/text/par.txt` | original |
| `data/text/weapon.txt` | original |

### Text — UI strings (15)

Menu, message, control-name and cheat strings.

| Asset | Status |
|---|---|
| `data/strings/1.pn` | original |
| `data/strings/2.pn` | original |
| `data/strings/cheats/disable.pn` | original |
| `data/strings/cheats/enable.pn` | original |
| `data/strings/cheats/secret.pn` | original |
| `data/strings/controls/long.pn` | original |
| `data/strings/controls/short.pn` | original |
| `data/strings/gamepad/long.pn` | original |
| `data/strings/gamepad/short.pn` | original |
| `data/strings/keyboard/long.pn` | original |
| `data/strings/keyboard/short.pn` | original |
| `data/strings/messages.pn` | original |
| `data/strings/minicomputer.pn` | original |
| `data/strings/object.pn` | original |
| `data/strings/tabs.pn` | original |

### Text — scenario info (1)

Scenario title, intro/about text references, credits.

| Asset | Status |
|---|---|
| `data/info.pn` | original |

### Text — levels (31)

Mission definitions, including briefings, prologues and epilogues.

| Asset | Status |
|---|---|
| `data/levels/ch01.pn` | original |
| `data/levels/ch02.pn` | original |
| `data/levels/ch03.pn` | original |
| `data/levels/ch04.pn` | original |
| `data/levels/ch05.pn` | original |
| `data/levels/ch06.pn` | original |
| `data/levels/ch07.pn` | original |
| `data/levels/ch08.pn` | original |
| `data/levels/ch09.pn` | original |
| `data/levels/ch10.pn` | original |
| `data/levels/ch11.pn` | original |
| `data/levels/ch12.pn` | original |
| `data/levels/ch13.pn` | original |
| `data/levels/ch14.pn` | original |
| `data/levels/ch15.pn` | original |
| `data/levels/ch16.pn` | original |
| `data/levels/ch17.pn` | original |
| `data/levels/ch18.pn` | original |
| `data/levels/ch19.pn` | original |
| `data/levels/ch20.pn` | original |
| `data/levels/dev1.pn` | original |
| `data/levels/net1.pn` | original |
| `data/levels/net2.pn` | original |
| `data/levels/net3.pn` | original |
| `data/levels/net4.pn` | original |
| `data/levels/net5.pn` | original |
| `data/levels/srtm.pn` | original |
| `data/levels/tut1.pn` | original |
| `data/levels/tut2.pn` | original |
| `data/levels/tut3.pn` | original |
| `data/levels/tutz.pn` | original |

### Text — races (16)

Race definitions and names.

| Asset | Status |
|---|---|
| `data/races/aud.pn` | original |
| `data/races/baz.pn` | original |
| `data/races/can.pn` | original |
| `data/races/ele.pn` | original |
| `data/races/gai.pn` | original |
| `data/races/gro.pn` | original |
| `data/races/ish.pn` | original |
| `data/races/obi.pn` | original |
| `data/races/sal.pn` | original |
| `data/races/uns.pn` | original |
| `data/races/zz1.pn` | original |
| `data/races/zz2.pn` | original |
| `data/races/zz3.pn` | original |
| `data/races/zz4.pn` | original |
| `data/races/zz5.pn` | original |
| `data/races/zz6.pn` | original |

### Text — objects (336)

Ship, base, weapon and effect definitions, including names and descriptions. (Game data as well as text; part of the original media.)

| Asset | Status |
|---|---|
| `data/objects/ast/nasty.pn` | original |
| `data/objects/ast/otr/big.pn` | original |
| `data/objects/ast/otr/medium.pn` | original |
| `data/objects/ast/otr/small.pn` | original |
| `data/objects/ast/rand-dir.pn` | original |
| `data/objects/ast/regular/big.pn` | original |
| `data/objects/ast/regular/medium.pn` | original |
| `data/objects/ast/regular/small.pn` | original |
| `data/objects/ast/shoot/lite/down.pn` | original |
| `data/objects/ast/shoot/lite/up.pn` | original |
| `data/objects/ast/shoot/narrow/down.pn` | original |
| `data/objects/ast/shoot/narrow/up.pn` | original |
| `data/objects/ast/shoot/nasty.pn` | original |
| `data/objects/ast/shoot/wide/down.pn` | original |
| `data/objects/ast/shoot/wide/up.pn` | original |
| `data/objects/aud/aslttran.pn` | original |
| `data/objects/aud/carrier.pn` | original |
| `data/objects/aud/cruiser.pn` | original |
| `data/objects/aud/etc/device.pn` | original |
| `data/objects/aud/evat.pn` | original |
| `data/objects/aud/fighter.pn` | original |
| `data/objects/aud/gunship.pn` | original |
| `data/objects/aud/hvd.pn` | original |
| `data/objects/aud/transport.pn` | original |
| `data/objects/baz/bttlship.pn` | original |
| `data/objects/baz/etc/bttlship/moored.pn` | original |
| `data/objects/can/aslttran.pn` | original |
| `data/objects/can/carrier.pn` | original |
| `data/objects/can/cruiser.pn` | original |
| `data/objects/can/defdrone.pn` | original |
| `data/objects/can/engineer.pn` | original |
| `data/objects/can/etc/border-drone.pn` | original |
| `data/objects/can/etc/spawner/item.pn` | original |
| `data/objects/can/etc/spawner/obj.pn` | original |
| `data/objects/can/etc/transport/tut1.pn` | original |
| `data/objects/can/evat.pn` | original |
| `data/objects/can/fighter.pn` | original |
| `data/objects/can/gateship/base.pn` | original |
| `data/objects/can/gateship/damaged.pn` | original |
| `data/objects/can/gateship/dying.pn` | original |
| `data/objects/can/gateship/jump.pn` | original |
| `data/objects/can/gunship.pn` | original |
| `data/objects/can/hvc.pn` | original |
| `data/objects/can/hvd.pn` | original |
| `data/objects/can/schooner.pn` | original |
| `data/objects/can/transport.pn` | original |
| `data/objects/dev/amissile/launcher.pn` | original |
| `data/objects/dev/amissile/missile.pn` | original |
| `data/objects/dev/anti1/gun.pn` | original |
| `data/objects/dev/anti1/pulse.pn` | original |
| `data/objects/dev/anti1/rapid.pn` | original |
| `data/objects/dev/anti2/gun.pn` | original |
| `data/objects/dev/anti2/pulse.pn` | original |
| `data/objects/dev/anti2/pulse2.pn` | original |
| `data/objects/dev/asb/bolt.pn` | original |
| `data/objects/dev/asb/gun.pn` | original |
| `data/objects/dev/assault/aud.pn` | original |
| `data/objects/dev/assault/can.pn` | original |
| `data/objects/dev/assault/gai.pn` | original |
| `data/objects/dev/assault/ish.pn` | original |
| `data/objects/dev/assault/sal.pn` | original |
| `data/objects/dev/atomic/pulse.pn` | original |
| `data/objects/dev/atomic/turret.pn` | original |
| `data/objects/dev/bay/aud.pn` | original |
| `data/objects/dev/bay/can.pn` | original |
| `data/objects/dev/bay/gai.pn` | original |
| `data/objects/dev/bay/ish.pn` | original |
| `data/objects/dev/bay/sal.pn` | original |
| `data/objects/dev/bay/uns.pn` | original |
| `data/objects/dev/cform/gun.pn` | original |
| `data/objects/dev/cform/pulse.pn` | original |
| `data/objects/dev/chronon/gun.pn` | original |
| `data/objects/dev/chronon/pulse.pn` | original |
| `data/objects/dev/cluster/pulse.pn` | original |
| `data/objects/dev/cluster/turret.pn` | original |
| `data/objects/dev/cm/five.pn` | original |
| `data/objects/dev/cm/launcher.pn` | original |
| `data/objects/dev/cm/missile.pn` | original |
| `data/objects/dev/conc/gun.pn` | original |
| `data/objects/dev/conc/pellet.pn` | original |
| `data/objects/dev/fireball/gun.pn` | original |
| `data/objects/dev/fireball/pulse.pn` | original |
| `data/objects/dev/fireball/trail.pn` | original |
| `data/objects/dev/flak/flak.pn` | original |
| `data/objects/dev/flak/pulse.pn` | original |
| `data/objects/dev/flak/turret.pn` | original |
| `data/objects/dev/fullerene/gun.pn` | original |
| `data/objects/dev/fullerene/pulse.pn` | original |
| `data/objects/dev/fusion/gun.pn` | original |
| `data/objects/dev/fusion/pulse.pn` | original |
| `data/objects/dev/fusion/rapid.pn` | original |
| `data/objects/dev/holo/decoy.pn` | original |
| `data/objects/dev/holo/item.pn` | original |
| `data/objects/dev/inasa/gun.pn` | original |
| `data/objects/dev/inasa/pulse.pn` | original |
| `data/objects/dev/inasa/rapid.pn` | original |
| `data/objects/dev/jammer/turret.pn` | original |
| `data/objects/dev/kl/bolt.pn` | original |
| `data/objects/dev/kl/gun.pn` | original |
| `data/objects/dev/laser/gun-bolt.pn` | original |
| `data/objects/dev/laser/gun.pn` | original |
| `data/objects/dev/laser/turret-bolt.pn` | original |
| `data/objects/dev/laser/turret.pn` | original |
| `data/objects/dev/lepton/bolt.pn` | original |
| `data/objects/dev/lepton/gun.pn` | original |
| `data/objects/dev/magneto/gun.pn` | original |
| `data/objects/dev/magneto/pulse.pn` | original |
| `data/objects/dev/magneto/rapid.pn` | original |
| `data/objects/dev/magno/pulse.pn` | original |
| `data/objects/dev/magno/turret.pn` | original |
| `data/objects/dev/mine/gun.pn` | original |
| `data/objects/dev/mine/pulse.pn` | original |
| `data/objects/dev/neutron/gun.pn` | original |
| `data/objects/dev/neutron/pulse.pn` | original |
| `data/objects/dev/newo/bolt.pn` | original |
| `data/objects/dev/newo/gun.pn` | original |
| `data/objects/dev/onas/gun.pn` | original |
| `data/objects/dev/onas/pulse.pn` | original |
| `data/objects/dev/photo/gun.pn` | original |
| `data/objects/dev/photo/light.pn` | original |
| `data/objects/dev/photo/pulse.pn` | original |
| `data/objects/dev/pk/can/bolt.pn` | original |
| `data/objects/dev/pk/can/gun.pn` | original |
| `data/objects/dev/pk/ish/bolt.pn` | original |
| `data/objects/dev/pk/ish/gun.pn` | original |
| `data/objects/dev/pk/ish/hand.pn` | original |
| `data/objects/dev/pk/ish/rapid.pn` | original |
| `data/objects/dev/pk/sal/bolt.pn` | original |
| `data/objects/dev/pk/sal/gun.pn` | original |
| `data/objects/dev/pp/pulse.pn` | original |
| `data/objects/dev/pp/turret.pn` | original |
| `data/objects/dev/pp/twin.pn` | original |
| `data/objects/dev/pp2/gun.pn` | original |
| `data/objects/dev/pp2/pulse.pn` | original |
| `data/objects/dev/quasi/gun.pn` | original |
| `data/objects/dev/quasi/pulse.pn` | original |
| `data/objects/dev/reactivator/p1.pn` | original |
| `data/objects/dev/reactivator/p2.pn` | original |
| `data/objects/dev/repulser/bolt.pn` | original |
| `data/objects/dev/repulser/turret.pn` | original |
| `data/objects/dev/rplasma/gun.pn` | original |
| `data/objects/dev/rplasma/pulse.pn` | original |
| `data/objects/dev/shrikeolator/pulse.pn` | original |
| `data/objects/dev/shrikeolator/turret.pn` | original |
| `data/objects/dev/stealth/item.pn` | original |
| `data/objects/dev/trazer/gun.pn` | original |
| `data/objects/dev/trazer/ray.pn` | original |
| `data/objects/dev/tspace/gun.pn` | original |
| `data/objects/dev/tspace/ray.pn` | original |
| `data/objects/dev/zbeam/bolt.pn` | original |
| `data/objects/dev/zbeam/turret.pn` | original |
| `data/objects/ele/cruiser.pn` | original |
| `data/objects/ele/etc/escape-pod.pn` | original |
| `data/objects/ele/etc/liner.pn` | original |
| `data/objects/ele/etc/liner/disabled.pn` | original |
| `data/objects/gai/aslttran.pn` | original |
| `data/objects/gai/carrier.pn` | original |
| `data/objects/gai/cruiser.pn` | original |
| `data/objects/gai/engineer.pn` | original |
| `data/objects/gai/etc/lab/base.pn` | original |
| `data/objects/gai/etc/lab/burning.pn` | original |
| `data/objects/gai/etc/spawner/item.pn` | original |
| `data/objects/gai/etc/spawner/obj.pn` | original |
| `data/objects/gai/etc/transport/ch1.pn` | original |
| `data/objects/gai/evat.pn` | original |
| `data/objects/gai/fighter.pn` | original |
| `data/objects/gai/gunship.pn` | original |
| `data/objects/gai/hvd.pn` | original |
| `data/objects/gai/transport.pn` | original |
| `data/objects/gro/cruiser.pn` | original |
| `data/objects/gro/ctf/cruiser/p1-active.pn` | original |
| `data/objects/gro/ctf/cruiser/p1-inactive.pn` | original |
| `data/objects/gro/ctf/cruiser/p2-active.pn` | original |
| `data/objects/gro/ctf/cruiser/p2-inactive.pn` | original |
| `data/objects/gro/ctf/flag/p1.pn` | original |
| `data/objects/gro/ctf/flag/p2.pn` | original |
| `data/objects/gro/ctf/moor/p1-active.pn` | original |
| `data/objects/gro/ctf/moor/p1-inactive.pn` | original |
| `data/objects/gro/ctf/moor/p2-active.pn` | original |
| `data/objects/gro/ctf/moor/p2-inactive.pn` | original |
| `data/objects/ish/aslttran.pn` | original |
| `data/objects/ish/carrier.pn` | original |
| `data/objects/ish/cruiser.pn` | original |
| `data/objects/ish/defdrone.pn` | original |
| `data/objects/ish/engineer.pn` | original |
| `data/objects/ish/etc/astrominer.pn` | original |
| `data/objects/ish/etc/cargo.pn` | original |
| `data/objects/ish/etc/cruiser/tut3.pn` | original |
| `data/objects/ish/etc/hvc/mod.pn` | original |
| `data/objects/ish/etc/hvd/rescue.pn` | original |
| `data/objects/ish/etc/research.pn` | original |
| `data/objects/ish/etc/tug.pn` | original |
| `data/objects/ish/evat.pn` | original |
| `data/objects/ish/fighter.pn` | original |
| `data/objects/ish/gunship.pn` | original |
| `data/objects/ish/hvc.pn` | original |
| `data/objects/ish/hvd.pn` | original |
| `data/objects/ish/schooner.pn` | original |
| `data/objects/ish/transport.pn` | original |
| `data/objects/loc/bunker.pn` | original |
| `data/objects/loc/flak.pn` | original |
| `data/objects/loc/jump/gate.pn` | original |
| `data/objects/loc/jump/node.pn` | original |
| `data/objects/loc/moon.pn` | original |
| `data/objects/loc/moor.pn` | original |
| `data/objects/loc/outpost.pn` | original |
| `data/objects/loc/planet.pn` | original |
| `data/objects/loc/power.pn` | original |
| `data/objects/loc/relay.pn` | original |
| `data/objects/loc/shipyard.pn` | original |
| `data/objects/loc/sun.pn` | original |
| `data/objects/obi/escort.pn` | original |
| `data/objects/obi/etc/liner.pn` | original |
| `data/objects/obi/etc/obiard/returning.pn` | original |
| `data/objects/obi/etc/obiard/turning.pn` | original |
| `data/objects/obi/etc/obiard/waving.pn` | original |
| `data/objects/obi/etc/transport/rescue.pn` | original |
| `data/objects/obi/transport.pn` | original |
| `data/objects/sal/aslttran.pn` | original |
| `data/objects/sal/carrier.pn` | original |
| `data/objects/sal/cruiser.pn` | original |
| `data/objects/sal/etc/lab/base.pn` | original |
| `data/objects/sal/etc/lab/burning.pn` | original |
| `data/objects/sal/evat.pn` | original |
| `data/objects/sal/fighter.pn` | original |
| `data/objects/sal/gunship.pn` | original |
| `data/objects/sal/hvd.pn` | original |
| `data/objects/sal/transport.pn` | original |
| `data/objects/sfx/crew.pn` | original |
| `data/objects/sfx/debris/a.pn` | original |
| `data/objects/sfx/debris/b.pn` | original |
| `data/objects/sfx/debris/c.pn` | original |
| `data/objects/sfx/energy.pn` | original |
| `data/objects/sfx/explosion/asteroid.pn` | original |
| `data/objects/sfx/explosion/energy.pn` | original |
| `data/objects/sfx/explosion/flak.pn` | original |
| `data/objects/sfx/explosion/gas.pn` | original |
| `data/objects/sfx/explosion/giant.pn` | original |
| `data/objects/sfx/explosion/lab.pn` | original |
| `data/objects/sfx/explosion/large-hit.pn` | original |
| `data/objects/sfx/explosion/large.pn` | original |
| `data/objects/sfx/explosion/missile-hit.pn` | original |
| `data/objects/sfx/explosion/quasi.pn` | original |
| `data/objects/sfx/explosion/shrikeolator.pn` | original |
| `data/objects/sfx/explosion/small.pn` | original |
| `data/objects/sfx/explosion/supernova.pn` | original |
| `data/objects/sfx/gunsmoke.pn` | original |
| `data/objects/sfx/jump/large.pn` | original |
| `data/objects/sfx/jump/small.pn` | original |
| `data/objects/sfx/sparkle.pn` | original |
| `data/objects/sfx/warp/in.pn` | original |
| `data/objects/sfx/warp/out.pn` | original |
| `data/objects/tpl/animation.pn` | original |
| `data/objects/tpl/anipulse.pn` | original |
| `data/objects/tpl/any.pn` | original |
| `data/objects/tpl/aslttran.pn` | original |
| `data/objects/tpl/bolt.pn` | original |
| `data/objects/tpl/capship.pn` | original |
| `data/objects/tpl/carrier.pn` | original |
| `data/objects/tpl/cruiser.pn` | original |
| `data/objects/tpl/defdrone.pn` | original |
| `data/objects/tpl/device.pn` | original |
| `data/objects/tpl/drone.pn` | original |
| `data/objects/tpl/effect.pn` | original |
| `data/objects/tpl/engineer.pn` | original |
| `data/objects/tpl/evat.pn` | original |
| `data/objects/tpl/fighter.pn` | original |
| `data/objects/tpl/gun.pn` | original |
| `data/objects/tpl/gunship.pn` | original |
| `data/objects/tpl/hazard.pn` | original |
| `data/objects/tpl/hvc.pn` | original |
| `data/objects/tpl/hvd.pn` | original |
| `data/objects/tpl/missile.pn` | original |
| `data/objects/tpl/planet.pn` | original |
| `data/objects/tpl/ray.pn` | original |
| `data/objects/tpl/rotation.pn` | original |
| `data/objects/tpl/rotpulse.pn` | original |
| `data/objects/tpl/scenery.pn` | original |
| `data/objects/tpl/schooner.pn` | original |
| `data/objects/tpl/ship.pn` | original |
| `data/objects/tpl/special.pn` | original |
| `data/objects/tpl/station.pn` | original |
| `data/objects/tpl/structure.pn` | original |
| `data/objects/tpl/transport.pn` | original |
| `data/objects/tpl/turret.pn` | original |
| `data/objects/tpl/utilship.pn` | original |
| `data/objects/tpl/warship.pn` | original |
| `data/objects/tpl/waypoint.pn` | original |
| `data/objects/uns/ares.pn` | original |
| `data/objects/uns/aslttran.pn` | original |
| `data/objects/uns/carrier.pn` | original |
| `data/objects/uns/cruiser.pn` | original |
| `data/objects/uns/etc/bttlship.pn` | original |
| `data/objects/uns/etc/human/returning.pn` | original |
| `data/objects/uns/etc/human/turning.pn` | original |
| `data/objects/uns/etc/human/waving.pn` | original |
| `data/objects/uns/etc/transport/ch18.pn` | original |
| `data/objects/uns/fighter.pn` | original |
| `data/objects/uns/gunship.pn` | original |
| `data/objects/uns/hvd.pn` | original |
| `data/objects/uns/transport.pn` | original |
| `data/objects/zrb/base.pn` | original |
| `data/objects/zrb/spawn.pn` | original |
| `data/objects/zzz/baz/alter.pn` | original |
| `data/objects/zzz/dev/cm/dumb.pn` | original |
| `data/objects/zzz/dev/inert/bolt.pn` | original |
| `data/objects/zzz/dev/inert/turret.pn` | original |
| `data/objects/zzz/dev/pk/ish/bolt.pn` | original |
| `data/objects/zzz/dev/retro/item.pn` | original |
| `data/objects/zzz/fx/explosion/combustion.pn` | original |
| `data/objects/zzz/fx/ftl.pn` | original |
| `data/objects/zzz/gai/etc/kamikaze.pn` | original |
| `data/objects/zzz/gai/etc/kamipulse.pn` | original |
| `data/objects/zzz/ish/cruiser/active.pn` | original |
| `data/objects/zzz/ish/cruiser/inactive.pn` | original |
| `data/objects/zzz/ish/etc/prctcdrn.pn` | original |
| `data/objects/zzz/ish/etc/trgtdrne.pn` | original |
| `data/objects/zzz/ish/etc/trgtdrne/dead.pn` | original |
| `data/objects/zzz/ish/transport/sl.pn` | original |
| `data/objects/zzz/loc/station.pn` | original |
| `data/objects/zzz/monkey-friend.pn` | original |
| `data/objects/zzz/obi/bttlcrsr.pn` | original |
| `data/objects/zzz/uns/carrier/mod.pn` | original |
| `data/objects/zzz/uns/transport/fleeing.pn` | original |
| `data/objects/zzz/untitled/0.pn` | original |
| `data/objects/zzz/untitled/1.pn` | original |
| `data/objects/zzz/untitled/10.pn` | original |
| `data/objects/zzz/untitled/11.pn` | original |
| `data/objects/zzz/untitled/2.pn` | original |
| `data/objects/zzz/untitled/3.pn` | original |
| `data/objects/zzz/untitled/4.pn` | original |
| `data/objects/zzz/untitled/5.pn` | original |
| `data/objects/zzz/untitled/6.pn` | original |
| `data/objects/zzz/untitled/7.pn` | original |
| `data/objects/zzz/untitled/8.pn` | original |
| `data/objects/zzz/untitled/9.pn` | original |

### Interface art — pictures (106)

Splash, menu frames, briefing and scroll-text art, star map, credits.

| Asset | Status |
|---|---|
| `data/pictures/ast/regular/big.png` | original |
| `data/pictures/aud/aslttran.png` | original |
| `data/pictures/aud/carrier.png` | original |
| `data/pictures/aud/cruiser.png` | original |
| `data/pictures/aud/fighter.png` | original |
| `data/pictures/aud/gunship.png` | original |
| `data/pictures/aud/race.png` | original |
| `data/pictures/baz/bttlship.png` | original |
| `data/pictures/baz/race.png` | original |
| `data/pictures/brf/asteroid.png` | original |
| `data/pictures/brf/aud-device.png` | original |
| `data/pictures/brf/bazidanese.png` | original |
| `data/pictures/brf/habitat.png` | original |
| `data/pictures/brf/jommo.png` | original |
| `data/pictures/brf/jump-gate.png` | original |
| `data/pictures/brf/los-ran.png` | original |
| `data/pictures/brf/placeholder.png` | original |
| `data/pictures/brf/relay.png` | original |
| `data/pictures/brf/shipyard.png` | original |
| `data/pictures/brf/thasero.png` | original |
| `data/pictures/brf/thrntz.png` | original |
| `data/pictures/brf/uns-ares.png` | original |
| `data/pictures/can/aslttran.png` | original |
| `data/pictures/can/carrier.png` | original |
| `data/pictures/can/cruiser.png` | original |
| `data/pictures/can/defdrone.png` | original |
| `data/pictures/can/engineer.png` | original |
| `data/pictures/can/etc/border-drone.png` | original |
| `data/pictures/can/fighter.png` | original |
| `data/pictures/can/gunship.png` | original |
| `data/pictures/can/hvc.png` | original |
| `data/pictures/can/race.png` | original |
| `data/pictures/can/schooner.png` | original |
| `data/pictures/can/transport.png` | original |
| `data/pictures/credit.png` | original |
| `data/pictures/ele/cruiser.png` | original |
| `data/pictures/ele/etc/escape-pod.png` | original |
| `data/pictures/ele/etc/liner.png` | original |
| `data/pictures/ele/race.png` | original |
| `data/pictures/gai/cruiser.png` | original |
| `data/pictures/gai/engineer.png` | original |
| `data/pictures/gai/fighter.png` | original |
| `data/pictures/gai/gunship.png` | original |
| `data/pictures/gai/hvd.png` | original |
| `data/pictures/gai/race.png` | original |
| `data/pictures/gai/transport.png` | original |
| `data/pictures/gui/instruments/left.png` | original |
| `data/pictures/gui/instruments/right.png` | original |
| `data/pictures/gui/main/bottom.png` | original |
| `data/pictures/gui/main/left.png` | original |
| `data/pictures/gui/main/right.png` | original |
| `data/pictures/gui/main/top.png` | original |
| `data/pictures/gui/tablet/bottom.png` | original |
| `data/pictures/gui/tablet/left.png` | original |
| `data/pictures/gui/tablet/right.png` | original |
| `data/pictures/gui/tablet/top.png` | original |
| `data/pictures/ish/aslttran.png` | original |
| `data/pictures/ish/carrier.png` | original |
| `data/pictures/ish/cruiser.png` | original |
| `data/pictures/ish/defdrone.png` | original |
| `data/pictures/ish/engineer.png` | original |
| `data/pictures/ish/etc/astrominer.png` | original |
| `data/pictures/ish/etc/cargo.png` | original |
| `data/pictures/ish/etc/research.png` | original |
| `data/pictures/ish/etc/tug.png` | original |
| `data/pictures/ish/fighter.png` | original |
| `data/pictures/ish/gunship.png` | original |
| `data/pictures/ish/hvc.png` | original |
| `data/pictures/ish/hvd.png` | original |
| `data/pictures/ish/race.png` | original |
| `data/pictures/ish/transport.png` | original |
| `data/pictures/loc/bunker.png` | original |
| `data/pictures/loc/flak.png` | original |
| `data/pictures/loc/moor.png` | original |
| `data/pictures/loc/outpost.png` | original |
| `data/pictures/loc/power.png` | original |
| `data/pictures/log/about-header.png` | original |
| `data/pictures/log/ares-christened.png` | original |
| `data/pictures/log/ares-departs.png` | original |
| `data/pictures/log/ares-returns.png` | original |
| `data/pictures/log/ish-comms.png` | original |
| `data/pictures/log/ish-encounter.png` | original |
| `data/pictures/log/placeholder.png` | original |
| `data/pictures/log/starfield/a.png` | original |
| `data/pictures/log/starfield/b.png` | original |
| `data/pictures/log/starfield/c.png` | original |
| `data/pictures/obi/escort.png` | original |
| `data/pictures/obi/race.png` | original |
| `data/pictures/obi/transport.png` | original |
| `data/pictures/sal/aslttran.png` | original |
| `data/pictures/sal/carrier.png` | original |
| `data/pictures/sal/cruiser.png` | original |
| `data/pictures/sal/fighter.png` | original |
| `data/pictures/sal/gunship.png` | original |
| `data/pictures/sal/race.png` | original |
| `data/pictures/sal/transport.png` | original |
| `data/pictures/splash.png` | original |
| `data/pictures/starmap.png` | original |
| `data/pictures/uns/carrier.png` | original |
| `data/pictures/uns/cruiser.png` | original |
| `data/pictures/uns/fighter.png` | original |
| `data/pictures/uns/gunship.png` | original |
| `data/pictures/uns/race.png` | original |
| `data/pictures/zzz/brf/prctdrne.png` | original |
| `data/pictures/zzz/brf/trgtdrne.png` | original |
| `data/pictures/zzz/obi/bttlcrsr.png` | original |

### Interface art — fonts (5)

Bitmap fonts: glyph table plus 1× and 2× images.

| Asset | Images | Status |
|---|---|---|
| `data/fonts/button-small.pn` | button-small.png, button-small@2x.png | original |
| `data/fonts/button.pn` | button.png, button@2x.png | original |
| `data/fonts/computer.pn` | computer.png, computer@2x.png | original |
| `data/fonts/tactical.pn` | tactical.png, tactical@2x.png | original |
| `data/fonts/title.pn` | title.png, title@2x.png | original |

### Interface art — layouts (11)

Screen layouts (positions, labels, keys).

| Asset | Status |
|---|---|
| `data/interfaces/briefing.pn` | original |
| `data/interfaces/help.pn` | original |
| `data/interfaces/loading.pn` | original |
| `data/interfaces/main.pn` | original |
| `data/interfaces/options/keys.pn` | original |
| `data/interfaces/options/sound.pn` | original |
| `data/interfaces/options/video.pn` | original |
| `data/interfaces/play-again/lose.pn` | original |
| `data/interfaces/play-again/normal.pn` | original |
| `data/interfaces/play-again/tutorial.pn` | original |
| `data/interfaces/select-level.pn` | original |

### Other original data (4)

Demo replays shown from the main menu, and the original rotation (sine/cosine) table.

| Asset | Status |
|---|---|
| `data/replays/600.NLRP` | original |
| `data/replays/605.NLRP` | original |
| `data/replays/623.NLRP` | original |
| `data/rotation-table` | original |
