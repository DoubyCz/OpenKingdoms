# Which builds may share a room

Lockstep sends orders, not state. Every machine in a match runs the same
simulation over the same orders and has to arrive at the same answer, so
any difference in arithmetic is a desync a few minutes later.

This note started as a measurement of how far apart the platforms were.
It now records how they were brought together. The short version: every
build shares one room again, and two gates in CI are what keep it true.

## What was measured

`tools/float_probe.c` runs four fixed workloads and hashes the bit
pattern of every result:

- `trig`, a whole turn of headings through `sinf` and `cosf`
- `dist`, the distance between two units, `sqrtf` over a sum of squares
- `aim`, the angle from one unit to another through `atan2f`
- `walk`, a position built up one tick at a time from `sinf` and `cosf`

Built with the same optimisation each platform's release uses:

| workload | Windows, MSVC x64 | Linux, gcc and glibc | browser, Emscripten |
|---|---|---|---|
| trig | `87e65ad4` | `1224a512` | `d8532442` |
| dist | `2bbe6af1` | `2bbe6af1` | `2bbe6af1` |
| aim | `16b6de04` | `a421a6c1` | `fd3f792f` |
| walk | `c3c5dc73` | `bba8a81d` | `c3c5dc73` |

Three platforms, three answers.

`sqrtf` agrees everywhere, which is expected. IEEE 754 requires a
correctly rounded square root, so there is only one right answer and
every libm gives it. `floorf` is exact for the same kind of reason.
Both of them stay in the simulation and always will. Replacing them
would be slower for nothing.

`sinf`, `cosf` and `atan2f` carry no such requirement, and all three
platforms differ. The differences are in the last place or two, which is
exactly the size that survives into a different cell after enough ticks.

`walk` is the interesting row. Windows and the browser agree while Linux
does not, and that is luck rather than safety. Adding a small rounding
difference to a coordinate in the thousands loses it, until the tick
where it does not. A workload that agrees is not evidence of a platform
that agrees.

## What was done about it

The simulation carries its own trigonometry. `include/tak_trig.h` and
`src/core/trig.c` give `tak_sinf`, `tak_cosf`, `tak_tanf`, `tak_atanf`
and `tak_atan2f`, built from pinned double constants and the four
operations IEEE 754 pins, so the answer is the same on every platform by
construction. They are correctly rounded to the last place for every
angle the game produces. `src/core/test_trig.c` checks the answers
against the platform's own libm in units of the last place and asserts a
pinned bit pattern that every platform in CI recomputes.

`src/render/units.c` and `include/tak_math.h` now call those and nothing
else. Every call moved, the drawing ones included. The alternative was a
list of which call sites matter, kept by hand forever, where one wrong
entry is a desync. `cmake/libm_guard.cmake` enforces it as a grep with
no judgement in it, registered as the `test_libm_guard` ctest and run on
every platform including the browser.

Moving the drawing calls cost nothing measurable, which was not the
expectation going in. On the owner's machine, MSVC on 32 bit x86,
`tak_sinf` and `tak_cosf` run at 14.7 ns a call against libm's 15.1, and
`tak_atan2f` at 17.5 ns against 47.6, so most of the file got faster.
Only `tak_tanf` is slower, 16.8 ns against 13.0, and it is called once
per shot from a weapon with a spray angle.

`perf_probe_shadows` draws 64 units for 60 frames and times them. Four
runs before the migration read 10.6, 10.9, 11.2 and 12.0 ms a frame
without shadows, and four after read 9.9, 9.9, 10.0 and 10.2. With
shadows on it was 18.1, 18.3, 18.8 and 20.8 before, and 16.9, 16.9, 16.9
and 17.1 after. The after runs are faster and steadier than the before
runs, but a machine shared with other work is not the place to claim a
speed up, so the reading to take away is that the cost feared here did
not appear.

The reason the fear did not materialise is that the model drawing paths
hoist their trigonometry out of the vertex loop already. `transform_unit_verts`,
`transform_shadow_verts` and `submit_static_mesh_run` each take their
sines once per instance per frame, not once per vertex. The one call
site that repeats is the selection ring, forty distinct angles per
dashed circle, and it is drawn only for units the player has selected.

## How it is kept true

Two gates, both data free, both run on Windows, macOS, Linux and in the
browser.

`test_trig` pins the arithmetic. `test_sim_probe` pins the simulation
built on it: sixteen units of two kinds walking into each other and
firing arced shots with a spray angle, 1800 ticks on a synthetic flat
world, with `TAK_SimHash` folded over the run into one number. It refuses
to pass a run in which no shot ever flew, so it cannot go quietly blind.

Measured on the owner's machine before the classes were merged, the
same probe built once against libm and once against the engine's own
trigonometry:

| build | with libm | with the engine's own trigonometry |
|---|---|---|
| Windows, MSVC, 32 bit | `5f1ce20f` | `5f1ce20f` |
| Linux, gcc 13 and glibc, x86-64 | `3eac5d66` | `5f1ce20f` |
| browser, Emscripten, node | `c9e3561d` | `5f1ce20f` |

Three answers became one. The left column is also the proof that the
gate can tell the difference, which a pinned hash needs before it is
worth anything.

Windows reaching the same number either way is worth explaining.
Compared bit for bit over four million angles across the range the mover
produces, MSVC's 32 bit CRT never disagrees with the engine's own
answer, and the other two do:

| build | sinf | cosf | atan2f | tanf |
|---|---|---|---|---|
| Windows, MSVC, 32 bit | 0 | 0 | 0 | 0 |
| Linux, gcc 13 and glibc | 51780 | 51700 | 824641 | 152618 |
| browser, Emscripten | 1216 | 1112 | 831346 | 544382 |

So MSVC was already returning the correctly rounded float and the other
two libms were not. That is a fact about one toolchain on one machine
and not a rule to rely on. Nothing in the C standard requires it, a
CRT update may change it, and the room has to hold whatever the other
three platforms are doing anyway.

macOS was not measured here. CI covers it, and the gate has to be green
there before any of this reaches a player.

## What it means for rooms

A determinism class is one float environment. There is one class again:
`TAK_CLASS_OWN_TRIG`. Windows, macOS, Linux and browser players share
rooms.

The per platform classes `TAK_CLASS_WINDOWS`, `TAK_CLASS_MACOS`,
`TAK_CLASS_LINUX` and the old `TAK_CLASS_BROWSER` are retired but their
numbers are kept, so a build from before the migration, which really
would desync, cannot land in a room with a build from after it. It is
refused with `TAK_REJECT_DETERMINISM_CLASS`, and the lobby lists such a
room greyed with the reason rather than hiding it.

## What is still float

The simulation still keeps headings, speeds and subpixel positions as
floats. That is fine for lockstep now that every operation under them is
pinned, and the sim hash reads them by bit pattern. The fixed point
conversion is a separate piece of work about precision and about the
original's behaviour, not about platforms disagreeing.

Reproduce the old measurement with:

    cl /O2 tools/float_probe.c && float_probe.exe
    gcc -O2 tools/float_probe.c -lm -o float_probe && ./float_probe
    emcc -O2 tools/float_probe.c -o probe.js && node probe.js

Reproduce the current one with `ctest -R test_sim_probe` natively and
`node <build>/src/test_sim_probe.js` in the browser.
