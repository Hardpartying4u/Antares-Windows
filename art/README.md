# Ares (Windows edition): Enhanced art folder

Replacement art for Ares (Windows edition), laid out like `data/` and read before it (search order: plugin,
factory scenario folder, `art/`, `data/`; see `src/data/resource.cpp`). Only replaced files live
here, so `data/` is never edited. Sprites keep their frame tables (`.pn`) in `data/`; this folder
holds their `image`/`overlay` sheets at 1x, `@2x` and `@4x`.

- The game uses this folder when it sits next to the data folder; `--art DIR` points elsewhere and
  `--no-art` shows the original art for comparison. Tests and tools always use the originals.
- PNGs are stored with Git LFS (`.gitattributes`).
- Every file here is rendered by `tools/sprite-render/` from a shape fitted to the original sprite
  (see `docs/fleet-plan.md`), so it stays reproducible from the scripts in that folder.
- Licensing: the remasters are fitted to, and coloured from, the CC BY-NC-SA 3.0 originals
  (`data/COPYING`), so treat them as adaptations under the same licence.
