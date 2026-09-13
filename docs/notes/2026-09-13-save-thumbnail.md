# The picture in a saved game

Line numbers of the form `legacy:NNNN` are anchors into the legacy
reference, as in every note here.

The load dialog authors a panel called `RadarView`, 127 by 127, and the
save dialog authors one at 125. The original paints the battle into it
from a radar image kept in the save's Summary (legacy:164966-164975).
Ours was an empty frame. Two gaps, not one: no code drew the panel, and
no save carried a picture to draw.

## The section

`THMB` is optional, like `CAMR` and for the same reason. It is local
view state, not simulation state, so it is outside the hash and a
reader that does not know it steps over it. A save written before this
existed has none, and its panel stays as the art authored it. That is
correct and is not the bug coming back.

The payload is six bytes of header then the pixels: a `u16` width, a
`u16` height, a byte of bytes per pixel and a byte of padding, then
width by height RGB triples, row major. The size travels in the payload
rather than living only in a constant, so changing it later needs no
version bump and an older file still reads.

128 by 128 at three bytes is 48 KB before the container deflates it,
against a save of a few hundred kilobytes to low single digit
megabytes. Most of a thumbnail is flat fog and flat terrain, which
deflate flattens hard.

## Who draws it

Not the container. `src/game/savegame.c` knows about bytes and nothing
about maps, and the thing that can draw a map lives in the UI lane, so
the screen that presses Save draws the picture and hands it over with
`Save_SetThumbnail`. The writer copies it, uses it, and clears it,
whether or not that save carried it.

That split is not tidiness. `test_savegame` links the container without
the renderer, and a container that reached for `Minimap_RenderThumbnail`
failed to link there. Making the caller responsible is what keeps the
container testable with no window, which is the property the whole save
format was built around.

`Minimap_RenderThumbnail` is the same composite the sidebar radar
draws, on the CPU with no renderer and no window: the TNT overview
image through the map's palette, then fog in the three cases the radar
uses, then one dot per unit the viewer can see in its owner's colour.
Fog is gated on `line_of_sight` exactly as the radar gates it, so a
battle played with it off makes a thumbnail with no fog shading, which
is what that player was looking at.

## What it costs when it cannot be drawn

Nothing. A world with no overview image, or a save written where there
is no pixel format because no window was ever opened, produces no
picture and the save is written without one. A battle the player cannot
save because a panel could not be painted would be a far worse answer,
and `a_battle_with_no_picture_still_saves` pins it.

## What the dialog does

`sync_details` decodes the selected save's picture once per selection
change, not once per frame: it means opening the file, and the list can
be walked with the arrow keys. `draw_radar` paints it over the authored
panel at the panel's own size through `Blit_RGBA_Scaled`, which is
nearest neighbour, so the test can assert the panel is the save's
picture pixel for pixel rather than asserting it merely changed. That
matters: the authored frame already has more than one colour in it, so
a weaker assertion passes on a panel nothing ever drew into.
