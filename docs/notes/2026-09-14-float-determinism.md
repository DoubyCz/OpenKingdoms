# Which builds may share a room

Lockstep sends orders, not state. Every machine in a match runs the same
simulation over the same orders and has to arrive at the same answer, so
any difference in arithmetic is a desync a few minutes later.

The movement code calls `sinf`, `cosf`, `atan2f` and `sqrtf` about sixty
times in `src/render/units.c`. Those come from each platform's own libm,
and nothing in the C standard says two libms round a sine the same way.
This note records what was measured rather than what was assumed.

## The measurement

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

`sqrtf` agrees everywhere, which is expected: IEEE 754 requires a
correctly rounded square root, so there is only one right answer and
every libm gives it.

`sinf`, `cosf` and `atan2f` carry no such requirement, and all three
platforms differ. The differences are in the last place or two, which is
exactly the size that survives into a different cell after enough ticks.

`walk` is the interesting row. Windows and the browser agree while Linux
does not, and that is luck rather than safety: adding a small rounding
difference to a coordinate in the thousands loses it, until the tick
where it does not. A workload that agrees is not evidence of a platform
that agrees.

## What it means for rooms

A determinism class is one float environment. Before this was measured
the browser was class 1 and every native build was class 2, so the
handshake let a Windows player and a Linux player into one room. They
would have desynced, and the turn clock would have halted the match
somewhere in the middle with no explanation a player could act on.

Each platform now names itself: `TAK_CLASS_WINDOWS`, `TAK_CLASS_MACOS`,
`TAK_CLASS_LINUX`, `TAK_CLASS_BROWSER`. `TAK_Room_Compatible` already
refuses a mismatch with `TAK_REJECT_DETERMINISM_CLASS`, and the lobby
lists such a room greyed with the reason rather than hiding it, so a
player sees the game and is told why it cannot be joined.

So today: browser plays browser, Windows plays Windows, Linux plays
Linux, macOS plays macOS. Everything else in the game is unaffected.

## What lifts it

The simulation has to carry its own arithmetic rather than borrow the
platform's. That means the angle and distance work in `units.c` moving
to fixed point, or to the engine's own polynomial, so the answer is the
same everywhere by construction. `CLAUDE.md` already asks for this under
its determinism rules, and `TAK_CLASS_NATIVE` is reserved for the day
every native build can honestly claim it.

The probe is the check for that work. When all three columns above match,
the classes can merge, and a browser player and a desktop player can sit
in the same room.

Reproduce it with:

    cl /O2 tools/float_probe.c && float_probe.exe
    gcc -O2 tools/float_probe.c -lm -o float_probe && ./float_probe
    emcc -O2 tools/float_probe.c -o probe.js && node probe.js
