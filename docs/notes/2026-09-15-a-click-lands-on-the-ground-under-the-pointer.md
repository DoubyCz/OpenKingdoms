# A click lands on the ground under the pointer

Reported from play in the browser: a unit ordered to a spot stopped
about an inch from it, and which way it missed seemed to depend on the
direction it walked. The engine side is `src/ui/ingame.c`,
`src/ui/hud.c`, `src/render/units.c` and the shared arithmetic in
`src/ui/click_map.c`.

## What was measured

The page's window to world mapping was exact. On openkingdoms.net at
device pixel ratios 1, 1.25 and 1.5, in 16:9 and letterboxed windows,
a click at page pixel (x, y) ordered a move to world (cam_x + x,
cam_y + y), one world pixel per css pixel on both axes. SDL's
Emscripten port scales a page position by canvas element over css box,
and the page keeps the element at the css box, so the ratio never
enters. Movement was exact too: a walker and a knight ordered to seven
points in different directions stop within 8 px of each.

What was off was the point itself. The terrain draws lifted by half its
height (:197689), and units draw lifted the same way. A move, patrol,
attack ground, unload or build order carried the flat reading of the
pointer, so the unit walked to that world point and was drawn half the
ground's height above the click. On the default skirmish map the sand
sits at a raw height of about 60 to 90, so the unit stood 30 to 45 px
above the pointer in every direction. Walking up the screen it went past
the click, walking down it stopped short, which is the direction
dependence that was reported.

The revive cursor, the raise click and the sweep click already took the
ground under the pointer (#149). The desktop build has the same code
and the same error.

The sideways part of the report was not reproduced. Across six orders
on the deployed page the unit stopped within 9 px of the click on the
x axis, at 1x and 1.5x ratios, which is inside the 8 px arrival radius
that leaves a unit on whichever side it came from. The miss that was
measured was 19 to 45 px, always up the screen.

This was a first sighting, not a regression: the flat pointer reading
in `ingame.c`, the tilt of 0.5, the draw lift and `Terrain_SampleHeight`
all date to the initial import (33de9b2, with the lift touched again
the same day in #8), the only later commit to `tnt.c` (#78) changed no
heightmap line, and no commit since has touched the path.

## What the original does

The original finds the cell under the pointer by walking the terrain
for the one that projects there (:212277). Every order on the ground
resolves through that cell.

## What the engine does now

Every ground order takes the ground under the pointer through
`Units_GroundUnderPoint`, the build ghost stands on it, and a marquee
takes the units drawn inside it rather than the ones whose flat
position falls inside. The window to canvas, window to world and
pointer to ground arithmetic lives in `click_map.c` with no SDL in it,
and `test_click_map` pins it in numbers on every platform, the browser
under node included. `test_movement` orders a unit through
`InGame_WorldClick` and checks it stops where the pointer was, and
that a box over the drawn unit takes it.
