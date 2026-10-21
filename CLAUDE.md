# Star Bound (Antares on Windows)

## Every session
1. **Before any work, read** `docs/design.md` (project goals, principles, milestones,
   open questions) and `docs/current-state.md` (how the game works today, with code
   locations). Paths here are relative to this repo's root.
2. **Keep them current.** When a decision is made, record it in `docs/design.md` (and resolve
   the matching open question). When a change alters controls, UI, ship/order behaviour or
   graphics, update `docs/current-state.md`. Commit doc updates with the change they describe.
3. Follow the design principles: small, tested, committed steps; the game must work after each.

# Build & run notes

Antares (https://github.com/arescentral/antares) is cloned at `<repo>`
(this repo; this file lives at its root) with all submodules. Work is on branch `windows-working-build`, pushed to the user's **private**
repo https://github.com/Hardpartying4u/Antares-Windows (remote `starbound`; `origin` is still
upstream Antares — never push there). Push only when the user asks; Git Credential Manager
handles sign-in. Commit
`651bb17e` fixes a divide-by-zero crash on minimize (`src/glfw/video-driver.cpp`
ignores 0x0 window sizes). Upstream already supports a native MSVC build — no porting needed.

## Toolchain on this machine
- VS 2022 Build Tools (MSVC 14.44), Windows SDK 10.0.26100, Python 3.11 (`python`).
- **PATH traps:** `link` resolves to MSYS `/usr/bin/link`, `ninja` to Strawberry Perl's,
  `clang-cl` to AMD ROCm's. Always build from a vcvars64 environment with the repo dir
  first on PATH (it holds the downloaded `gn.exe`/`ninja.exe`, untracked on purpose).
- No git identity is configured; commit with
  `git -c user.name="David" -c user.email="246752892+Hardpartying4u@users.noreply.github.com" commit ...`.

## Build (same steps as upstream CI), in cmd
```bat
call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat"
cd /d <repo>
set "PATH=%CD%;%PATH%"
python scripts/deps.py install          :: once: downloads gn.exe + ninja.exe
python configure --antares-version=<output of: git describe --tags HEAD>
ninja -C out/win/opt
```
From PowerShell/Bash, wrap each step as `cmd /c "<bat that does the above> <command>"`.
~3.7k MSVC warnings (C4005 APIENTRY, C4715) are normal.
**After changing any header, do a clean rebuild** (`ninja -C out/win/opt -t clean` first): the
Windows toolchain doesn't track header dependencies, and stale objects crash at runtime
(seen as replay tests failing with 0xC0000409).

## Tests (38 total; all pass except `main-screen`, which needs OpenGL offscreen, absent on Windows)
Diff tests need GNU `diff`; append Git's tools to the **end** of PATH so MSVC `link` still wins:
```bat
set "PATH=%PATH%;C:\Program Files\Git\usr\bin"
python scripts/test.py --type=unit
python scripts/test.py --type=data
python scripts/test.py --type=replay
```
- Star Bound tests keep their expected output in `test-starbound/` (never upstream's `test/`).
- An offscreen script must end with a key that quits (`Key::Q` at the main menu); otherwise the
  tool waits forever. A screen that waits for input with no events left exits with an error.
- The replay tool plays Arena battles: `--arena=NAME [--arena-dir=DIR]`, with a tiny `.NLRP` for
  seed and length (see `arena_replay_test` in `scripts/test.py`).

### Read-only requests
- When a prompt says read-only, investigation or stop for review, do not change code. Report and
  wait.

### In-game UI-scale checks
- Only when a change involves mouse input or on-screen drawing, and then at UI 2× only.
- Add 4× only for new drawn elements or new sizes/positions.
- Key-only changes rely on unit and replay tests; no in-game scale check.

## Package & run
- `python scripts/dist.py win zip` → `dist\antares-win-git.zip`; expand it into `dist\` to get
  `dist\Antares\{Antares.exe, data\, art\, COPYING, COPYING.LESSER, ASSETS.md, CREDITS.md}` — a
  standalone, double-clickable folder (~89 MB; zip ~82 MB). Graphics defaults to Enhanced (`art\`).
  CRT is static; only Windows system DLLs are used (XAudio2, OpenGL loaded at runtime).
- Running the raw build output needs the data path: `out\win\opt\antares.exe -a <repo>\data`.
- Use `-c <path>\config.pn` when testing so the user's real config isn't touched.
- Saves and progress (`Registry\`) live only in Documents\Antares, never in the game folder. Never
  touch the user's play copy (`Downloadsntares-win-git\Antares`) or save unless asked.
- Clean release for sharing: `dist.py win zip`, expand into `Downloads\Antares-Release\Antares`,
  zip that folder as `Antares-Release.zip` beside it.
- User data lives in `%USERPROFILE%\Documents\Antares`:
  `config.pn` (currently `fullscreen: true`), `Scenarios\` (sound effects). Sounds come from
  `antares-download-sounds.exe`, or are downloaded automatically (~12 MB, silently) on first launch.

## Debugging crashes
- Crash records: Event Viewer → Application, event ID 1000 (gives exception code + fault offset).
- PDBs are in `out\win\opt`. Symbolize an offset:
  `"C:\Program Files\AMD\ROCm\6.2\bin\llvm-symbolizer.exe" --obj=out\win\opt\antares.exe --relative-address 0x<offset>`

## Git and GitHub
- Commit with the GitHub noreply address (see "Toolchain"); never the private email.
- `pre-rewrite` (branch) and `pre-rewrite-v1.0` (tag) hold the pre-rewrite history with the
  private email: local only, never push them.
- GitHub CLI: `"C:\Program Files\GitHub CLI\gh.exe"`, signed in as Hardpartying4u.
- An old upstream tag (`v0.3.0`) has a path ending in a space, so whole-history tools fail on
  Windows; limit them to the fork's commits (`d8ce5826..`).

## Arena level rules
- `arena/*.pn` are the source (no generator is kept). A `win` action needs `text`.
- A "ships = 0" win must start disabled and be enabled a moment after the last group is revealed:
  counts lag one think pass and include thinking objects such as missiles.

## Local model
- houtini-lm (an MCP server) runs a local model.
- Use it only for: summarising long logs, build errors and test output; first drafts of docs;
  boilerplate test scaffolding. Pass file paths, not pasted text, where the tool allows
  (`code_task_files` takes `paths`).
- Never use it for design, investigation, engine rules, replay or determinism work.
- The local model never edits files, runs git or pushes. Check everything it returns before
  using it (its sums can be wrong).
- If the local model isn't running, just do the work yourself.
- Every report includes one line saying whether houtini-lm was used and for what, or why not.

## Open items
- Not yet tested on a clean PC (Windows Sandbox isn't enabled; its software OpenGL is too old
  for Antares anyway). Copy `dist\Antares` to another PC with internet and double-click.
