# Menu door clips and the cut scenes

How the original runs its Bink clips: the four doors on the main menu,
the full screen reels (logo, intro, credits) and the arch on the loading
screen. Line anchors are `:NNNNN` in the legacy reference. The engine
side is `src/render/bink_player.c`, `src/render/main_menu.c`,
`src/ui/credits.c`, `src/ui/loading.c` and `src/main.c`.

## Pacing

A clip is stepped by the decoder's own clock. The draw routine waits
on the decoder until the next frame is due, decodes that one frame,
draws it and moves the frame counter on by one (:35289-35341). Nothing
in the game code skips a frame to catch up. The engine keeps the same
rule: one frame at most per tick, and whatever a stalled frame left
over beyond one frame duration is forgotten.

The container header carries the rate. The door clips are 30 fps, the
loading arch 20 fps and the reels 15 fps. The demuxer's guessed rate is
wrong on the one frame stills, so the header's rate is what the player
reads.

## The doors

Each door is a button with eight states (:147778). State 2 is rest,
drawn from the sprite sheet. States 4 to 7 play clips 4 to 7,
`Movies/Gui/<name>N.bik`: 4 is the still after a click, 5 the enter
clip, 6 the hover clip, 7 the leave clip.

Entering sets the hover flag and, from rest, starts clip 5 (:148064-148070).
Each tick the button hands over only once the clip's frame counter has
reached its frame count (:148038-148054): 5 goes to 6, 7 goes to 2,
and 4 goes to 6 while the flag is up or to 2 once it is down. Leaving
is honoured only in state 6, where it clears the flag and starts clip
7 (:148027-148035). A cursor that leaves during the enter clip waits
for it. State 6 has no hand-over of its own, and the decoder wraps a
clip nobody stops (:35341), so the hover clip loops while the cursor
stays. machine6.bik is forty frames of the machine idling and girl6.bik
thirty, which only makes sense as a loop. knight6.bik and snort6.bik
are single frames.

A click drops the door to state 4 (:148002-148011).

## The full screen reels

One player runs the logo at startup (:241882, skipped by the
`-skiplogo` switch, :252020), the intro on the first Story click of a
session (:140763-140767, the flag is never cleared) and the credits from
the Credits door (:140736-140738). The music is paused around the reel.

The reel ends on its last frame, or early on a key. The player's message
loop ends on a character message or a system key message, that is any
key that types a character, Escape and Enter included, and Alt or F10
(:34816-34849). A mouse click is dispatched and ignored, and the mouse
events queued during the reel are drained afterwards (:34793-34797).
Skipping the victory reel also skips the credits that would follow it
(:154103-154106).

## The loading arch

The arch's clip, `Movies/Gui/Loadscreen.bik`, is opened at the
AnimatedControl widget's corner at the clip's own size (:158297-158299).
It is not played. Each tick the frame is set to the load percentage's
share of the frame count, never below the first frame and never past
the last (:158702-158710). The exact float expression is not readable in the
legacy reference, but the inputs are the percentage and the count and the
result is clamped to 1..count.

## In the browser

The decoder is the same FFmpeg 7.1.1 the desktop release builds, the
Bink decoder and demuxer and nothing else, compiled with Emscripten by
scripts/build-ffmpeg-wasm.sh and linked in by scripts/build-wasm.sh
and the site workflow. bink_player.c is unchanged: it opens a clip by
path with fopen and streams it frame by frame. --enable-small takes
200 KB off the archives. The engine's wasm grows from 1.62 MB to
2.44 MB, and from 553 KB to 915 KB gzipped, which is what the wire
carries. The site build caches the installed prefix on the FFmpeg and
Emscripten versions, so a push does not build FFmpeg again.

The page never copies a clip into the engine's memory. The archives
are copied, 289 MB of them, and that is already most of what the tab
holds. The Movies folder is another 539 MB, of which the engine plays
39 MB: the logo, the intro, the two credits reels and the seventeen
pieces under Gui. Those are the only ones the page takes, and each one
becomes a file node in the in-memory filesystem whose reads slice the
File the player picked, or its copy in browser storage, a megabyte at a
time. A slice comes across through a synchronous request on a blob
URL. That is the one way the main thread can read a File without
waiting on a promise, and the engine runs on the main thread with no
way to wait, since the build has ASYNCIFY off. Four slices a clip stay
cached, enough for a door clip to rewind without a second read and for
a reel to stream front to back. The other two designs were WORKERFS,
which mounts File objects for free but only inside a worker, and a
custom AVIOContext fed from JavaScript, which would have needed the
same synchronous read plus a second open path in the player.

The names are lower cased when mounted. An install spells them
LOGO.BIK and CREDITS.BIK, the engine asks for logo.bik and
Credits.bik, and the lookup's third try is the lower case name.

Measured by step 7 of scripts/web-smoke.js against the GOG install on
this machine, in headless Edge on the software renderer, booting from
the copy in browser storage: the logo opened in 17 to 24 ms and its
frames a second apart differed in half their pixels, the sixteen door
clips opened in 2 to 9 ms each, a hovered machine door changed 14 to
21 percent of its pixels every 250 ms and the door at rest changed
none, the credits reel opened in 16 ms from a click on its door and
turned its first page 5 s in, and Escape ended it. The smoke holds
the mouse down across a tick, since the menu polls the button between
ticks and a click that lands inside one is never seen.
