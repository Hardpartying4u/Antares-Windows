# Status (handoff, 2026-10-22)

## What this is
- **Ares (Windows edition)**: an unofficial Windows fork of Antares (the open-source port of Ares).
- Repo: `<repo>` (`C:\Projects\Starbound\antares`), branch `windows-working-build`.
- Remotes: `starbound` = github.com/Hardpartying4u/Antares-Windows (push here only). `origin` =
  upstream arescentral/antares (never push).
- Play copy: `%USERPROFILE%\Downloads\antares-win-git\Antares` (older builds beside it as
  `antares-old-<commit>`). Clean release: `%USERPROFILE%\Downloads\Antares-Release\Antares` and
  `Antares-Release.zip` (141.9 MB, 1,732 files, no saves or prefs). Neither is in git.

## Done this session (commit IDs after the history rewrite)
- Arena (`9dec3539` … `34974db2`): Network and Demo buttons gone (idle demos kept), Arena (D) with
  Skirmish, Fleet Action, Armada and Last Stand; flagship transfer without escape pod (Arena only);
  cleared groups arrive early, and the win needs the last group in. Details:
  `docs/arena-investigation.md` §§1–11.
- Group bar (`dafe9ddb`, restyle `3cd13ad5`); Modern: Ares = Classic keys + attack-move on A.
- App icon (`90463901`): Ishiman Heavy Cruiser, `GLFW_ICON` resource, `scripts/make_icon.py`.
- README (`55eb013a`); player-facing rename to "Ares (Windows edition)" (`16b9f0f9`); your own
  README edits on GitHub (`e2e0472e`, `4086238a`).
- GitHub Release **"Ares v1.0"**, tag `v1.0` → `16b9f0f9`, zip attached:
  https://github.com/Hardpartying4u/Antares-Windows/releases/tag/v1.0
  (the zip's exe says `g3f00a3d1`, its pre-rewrite ID; same code).

## Sharing status
- The repo is still **private**.
- Pre-public check: no secrets, saves or personal files in the history. Your Gmail was on all 105
  fork commits, and `CLAUDE.md` had local paths and the LM Studio setup.
- Fixed by a history rewrite to `246752892+Hardpartying4u@users.noreply.github.com`, force-pushed
  (branch `4086238a`, tag `v1.0`); upstream history unchanged.
- Old history is kept locally only: the `pre-rewrite` branch, the `pre-rewrite-v1.0` tag and
  `C:\Projects\Starbound\backup-pre-rewrite.bundle`. Never push them; delete them when happy.
- Still to do before going public:
  - GitHub Settings → Emails: "Keep my email addresses private" and "Block command line pushes
    that expose my email". Optional: ask GitHub Support to purge cached old commits.
  - About 20 old commit IDs inside the docs (e.g. `807c90ba`) are now stale.
- Licences for sharing:
  - Code is LGPL-3+: make the source available (public repo or source alongside the zip).
  - Ares media and everything derived from it (`art/`, the icon) is CC BY-NC-SA 3.0:
    non-commercial only, same licence, no endorsement implied.
  - The attribution line ("Based on the original game 'Ares,' by Nathan Lamont. … not affiliated…")
    must appear wherever the title does: it's in the README and `CREDITS.md`, **not yet in the game**
    (no title or credits screen shows it).
  - Naming the fork "Ares" is close to the original's title; "unofficial" and the line help.

## In progress
- Nothing is half-done; the tree is clean (untracked `gn.exe`, `ninja.exe` only). Tests: 38, all
  pass except `main-screen`.

## Next steps, in order
1. **M3 UI redesign:** new HUD and menus. This includes the main menu's empty row where Demo was
   (menu buttons can't be moved without new code) and showing the attribution line in game.
2. **M6 effects:** explosions, weapons, thrusters, lighting.
3. **Arena variety:** more battles and races, a fleet picker, scores.
4. **Attack-move "keep chasing past 15 s":** an optional setting, not built.
5. **Watch item, wheel zoom:** it now steps through every zoom level (it used to stop at 1:16).
   Watch in play whether stepping into the auto zooms is wanted.

Gotchas: the permanent rules learned this session (tests, release, git, Arena levels) are in CLAUDE.md.
