OpenKingdoms VERSION_HERE, an engine for Total Annihilation: Kingdoms.

Play in a browser at [openkingdoms.net](https://openkingdoms.net), or download below and play on the desktop.

## You bring the game

These archives hold the engine and the SDL runtime it needs. They hold no game content and never will. You need your own copy of Total Annihilation: Kingdoms, from GOG or from the discs.

The first run looks for it in the usual places. If it cannot find yours, it says so and how to point it at the folder. That is remembered afterwards.

## Downloads

| Platform | File |
|---|---|
| Windows 10 and 11, 64 bit | `openkingdoms-VERSION_HERE-windows-x64.zip` |
| macOS, Apple silicon | `openkingdoms-VERSION_HERE-macos-arm64.tar.gz` |
| macOS, Intel | `openkingdoms-VERSION_HERE-macos-x86_64.tar.gz` |
| Linux, x86_64 | `openkingdoms-VERSION_HERE-linux-x86_64.tar.gz` |

`SHA256SUMS` carries the checksums. `HOW-TO-RUN.txt` inside each archive covers the rest.

macOS asks about an unidentified developer, because this build is not signed by Apple. Clear the download flag once with `xattr -dr com.apple.quarantine OpenKingdoms`.

Linux binaries are built on Ubuntu 22.04 and need that glibc or newer.

## Multiplayer

Deterministic lockstep over one relay. Type the server address on Select Game and it is remembered.

A room holds players whose builds agree on arithmetic, so Windows plays Windows, macOS plays macOS, Linux plays Linux and the browser plays the browser. Each platform's maths library rounds sines its own way, which is enough to pull two machines apart over a match, so the handshake refuses a mix rather than desyncing it. A game you cannot join is still listed, greyed, with the reason. Lifting that is tracked in [docs/notes/2026-09-14-float-determinism.md](https://github.com/OpenKingdoms/OpenKingdoms/blob/main/docs/notes/2026-09-14-float-determinism.md).

## What is missing

The Bink video clips are not decoded in these builds, so the animated menu doors and the intro movie are still. Everything else runs.

Problems go to [issues](https://github.com/OpenKingdoms/OpenKingdoms/issues), with the version line from the main menu.
