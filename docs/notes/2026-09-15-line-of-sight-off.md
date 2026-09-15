# What Line of Sight off actually turns off

Battle setup offers Map Revealed and Line of Sight as two separate
checkboxes. They are two separate bits in the original as well, packed
next to each other in the session options byte and unpacked into two
bytes of the input state (legacy:135486-135487). Our engine had folded
them into one behaviour, where Line of Sight off meant the whole map was
drawn. That is not what the original does.

## The two maps

Each player has two per cell maps.

The sight map is a count of how many of that player's units cover the
cell right now. It is the one a unit's reveal stamps into and out of as
the unit moves.

The explored map is a bitmask over the whole map, one bit per player,
recording ground that player has ever had in sight. Bits are only ever
set, never cleared.

## What each option does

The second bit decides how the explored map starts and whether it grows.
One way round, every bit is set at load and the stamp does not bother
setting any more (legacy:167192-167202). The other way, the map starts
empty and every unit's reveal ORs the player's bit into the cells it
covers (legacy:167404-167409). Map Revealed is the only setup option
that would do that, so that is what this bit is taken to be. Nothing
below turns on the label being right, only on the bit being a different
bit from Line of Sight.

Line of Sight decides only how the sight map starts. With it on, the
sight map starts empty and fills from unit reveals. With it off, every
player's sight map is filled solid at load (legacy:167211-167219), so
every cell counts as currently seen for as long as the match runs.

The reveal stamp itself never asks what Line of Sight says
(legacy:167323-167436), and the call that re-stamps a unit that has
moved makes the add unconditionally (legacy:167466). So the explored map
keeps growing with Line of Sight off. That is the whole point of the
distinction: the option grants full sight, not a revealed map.

## What the screen does with them

One visibility test serves drawing, picking, the minimap blips,
projectiles and positional sound (legacy:206797). With Line of Sight on
it reads the viewer's sight map (legacy:206887-206892). With it off it
reads the viewer's explored map instead (legacy:206877-206884). An enemy
standing on ground the player walked past an hour ago therefore draws,
and an enemy on ground the player has never been near does not.

The fog overlay sorts every cell into three levels (legacy:130295-130310):

- explored bit clear gives opaque black, in both modes
- the option off, or the cell currently seen, gives no shading
- otherwise the grey level

So with Line of Sight off the grey level is unreachable and unexplored
ground is still black.

The minimap terrain pass is the same three cases in palette terms
(legacy:208407-208418). It blacks a cell whose explored bit is clear
whatever the option says, and since the sight map is full with the
option off, everything explored reads as the plain map byte.

## What it does not touch

The simulation reads the sight map, never the explored map. With Line of
Sight off that map is full, so `Fog_IsVisibleForPlayer` answers visible
everywhere and the AI, the influence maps and the idle target search see
no change. `Fog_ShowsAt` is the drawing test and has no simulation
caller.

One wrinkle worth recording. The original's AI map evaluation calls the
same visibility test the screen does (legacy:20511-20545), and that test
in its Line of Sight off branch reads the explored map of the local
player rather than of the player being asked about. On one machine with
one human that is a quirk rather than a bug, but it does not generalise
to lockstep, so we do not copy it.

## Determinism

The explored map is part of the simulation hash and travels in a save,
and the per unit reveal anchor does too. The stamping runs in the fixed
tick for every player on a schedule derived from the tick number, off
replicated configuration only, so every machine builds the same map. A
Line of Sight off match played against a build from before this change
will not hash the same, because that build stamped nothing. Two builds
from after it will.
