# A scroll arrow takes its own click

Reported from play: a scrollbar moves by dragging the ball but not by
its arrows, and the report was for the bar wherever it appears. The
engine side is `src/ui/gui_render.c`, with one screen fix in
`src/ui/select_game.c`.

## What was measured

Twelve of the dialogs the port loads author a pair of scroll arrows:
load and save game, select game, choose map, the four options screens,
both battle menus, the player dialogue and the map view. A test clicks
the centre of every arrow where its art is drawn, in every one of
those dialogs, and asks the runtime which widget took the click. Before
the fix it was 36 arrows and 36 unreachable, and the test printed who
took each one: the `GameList` list box in load, save and select game,
`PlayerList` and `MapInfo` in the other two lists, and the
`SoundVolume` and `ScreenSizeSlider` tracks in the options screens.

The arrows' own rects were fine. The runtime's hit test walked the
widgets in file order and stopped at the first interactive one that
contained the pointer, and every scrollbar's list box or slider track
is authored before its arrows and encloses them. Whatever the arrow
handlers did, the click never reached them.

Three screens already handled an arrow click by widget index, the save
browser, the map chooser and the multiplayer list, and their tests
passed because they called the handler with the index directly. The
hit test was never in the test. Select Game cached its arrow indices
and never read them at all.

## What the engine does now

Of the interactive widgets under the pointer the smallest wins, and a
later one wins a tie, which is what drawing order implies: a widget
authored later draws over one authored earlier. After the fix the same
test is 36 arrows and 0 unreachable. Select Game answers its arrows by
name, a row a click, clamped to the list, with its own test.

The existing screen tests were left as they are. They prove the
handlers, and the new test proves the click reaches them, which are
the two halves that were never both true.

## Not done here

The original repeats an arrow while it is held. The arrows here step
once a click. It is a smaller thing than an arrow that does nothing,
and it is recorded rather than folded in without a look at the legacy
reference.
