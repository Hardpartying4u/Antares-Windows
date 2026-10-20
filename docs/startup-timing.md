# Startup timing and the occasional 30 s start (investigation, 2026-10-21)

Read-only. The user reports that the game sometimes takes about 30 s to start on their PC (it
seemed to wait on controller detection; SteelSeries keyboard), and once started in 0.5 s.

## Measured

Downloads build (v0.9.1-405-g807c90ba), windowed test config, `--timing`, three launches. The
times are ms since the process was created (`src/game/timing.cpp` uses the process creation time
on Windows).

| Mark            | Run 1 | Run 2 | Run 3 |
|-----------------|------:|------:|------:|
| start           |    28 |    17 |    17 |
| init start      |   518 |   496 |   493 |
| sys_init done   |   719 |   528 |   531 |
| plugin loaded   |   945 |   597 |   605 |
| init done       |   951 |   598 |   607 |
| first frame     |   954 |   600 |   610 |

"main menu shown" (about 3.2 s) only says when the script pressed Esc past the intro, so it isn't a
startup cost. None of the three runs were slow.

- Startup order (`src/win/main.cpp`):
  1. `XAudio2SoundDriver`;
  2. `GLFWVideoDriver`, whose `glfwInit()` enumerates controllers;
  3. `loop()` creates the window and GL context;
  4. `Master` ("init start" onward).

  So the 0.5 s between "start" and "init start" covers sound, controller detection and the window.
  There are no marks inside that span.
- A standalone probe (scratchpad, the same calls, no window, three runs) timed each call:

  | Call | Time |
  |---|---|
  | `DirectInput8Create` | 21–84 ms |
  | `EnumDevices(DI8DEVCLASS_GAMECTRL, DIEDFL_ALLDEVICES)` | 124–138 ms, 0 devices |
  | `XInputGetCapabilities` ×4 | 0.2 ms |
  | XAudio2 with a mastering voice | 27–33 ms |

## The likely waiting call

GLFW 3.3.2 (`ext/glfw/glfw-3.3.2`) detects joysticks inside `glfwInit()`:
- `win32_init.c:583` calls `_glfwInitJoysticksWin32`, which calls
  `IDirectInput8_EnumDevices(DI8DEVCLASS_GAMECTRL, …, DIEDFL_ALLDEVICES)`
  (`win32_joystick.c:574`).
- That runs synchronously on the main thread before the window exists.
- It runs again on every device arrival (`WM_DEVICECHANGE`, `win32_window.c:537`), also on the
  main thread, mid-game.

DirectInput enumeration is known to stall for many seconds when a HID device doesn't answer the
query. Gaming keyboards and mice expose extra vendor HID interfaces (SteelSeries GG/Engine), so
this fits. It is consistent with "usually 0.5 s, sometimes 30 s", but these runs could not
reproduce the stall, so it is not proven.

## Built (2026-10-21)

Option 2 below, with the timing marks (`docs/current-state.md` §1.4, "Controller detection on
Windows"). The new build, three launches (ms since process creation):

| Mark | Run 1 | Run 2 | Run 3 |
|---|---:|---:|---:|
| sound init done | 44 | 45 | 45 |
| glfwInit done (XInput only) | 123 | 125 | 125 |
| window created | 267 | 273 | 269 |
| first frame | 329 | 334 | 332 |
| controller scan done | 166 ms | 166 ms | 166 ms |

The scan ran in the background, starting when the main menu showed.

- First frame is now about 0.33 s after the process starts, against 0.6–0.95 s before.
- The window and GL context take about 145 ms.
- Not reproduced: the 30 s stall itself. If it comes back, the timing log will show
  `controller scan done` late and the game unaffected; a slow start elsewhere will show between
  two of the new marks.
- Not tried: a real DirectInput-only pad (none here).

## Proposed fix (as reviewed)

1. **Add timing marks** around `XAudio2SoundDriver`, `glfwInit()` and `glfwCreateWindow`. Then the
   next slow start names the call, and the fix is checked against real data. This is cheap and
   needed in any case.
2. **Keep DirectInput off the startup path** with a small patch to the vendored GLFW:
   - At `glfwInit`, detect XInput pads only. This is instant, and it covers Xbox pads and most
     modern pads.
   - Run the DirectInput enumeration on a worker thread after the main menu shows.
   - Hand the found devices to the main thread at the next `poll_gamepad()`.
   - Do the same for `WM_DEVICECHANGE`.

   If enumeration hangs, only DirectInput-only pads are late; the game is never blocked.

   A simpler alternative: XInput only by default, with a config switch to also use DirectInput
   pads. It removes the stall completely, but DirectInput-only pads (e.g. some DS4/Switch pads
   without Steam Input) need the switch.
