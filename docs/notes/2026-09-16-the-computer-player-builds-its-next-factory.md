# The computer player builds its next factory

Reported from play, as two things: Taros fire demons do not attack,
and the Taros computer player used its monarch to build everything and
never advanced. They were one thing. The engine side is
`src/game/ai.c`.

## What was measured

Hand placed fire demons fire. A probe spawns every unit whose weapon
is Guided beside a passive enemy inside its range and ticks: all four
such units in the shipped data put a shot in the air, the fire demon
among them. So the weapon was never the fault.

A Taros computer player on Adamantine Gate for ten minutes built
eighteen ranged units and never a fire demon, and every structure it
raised was a castle or a lodestone. Its units did acquire targets and
marched at them, which is what looked like standing still: a wave at
two thousand pixels from a three hundred pixel weapon takes a while.

The fire demon comes from the dungeon. Eleven Taros unit types come
only from the dungeon or the hell: the fire demon, archer, beak and
witch from one, the demon, knight, lich, mage, mind, priest and spout
from the other. The profile weights all three factories at ten and
limits each to two. The computer player built two castles and stopped.

## Why

The planner priced one factory a tick, the first in the builder's list
that qualified. Taros lists its castle first, so the dungeon and the
hell were never candidates, and once the castle reached its limit the
plan had nothing left to price. A seat at its limit was asked for a
third castle every tick and refused every tick, which reads in play
as the computer player having stopped.

## What the engine does now

The factory the plan prices is the most desirable one the profile
limit still allows, by the same weight and first of type bias the
original picks with (legacy:21300, legacy:19859), and the same for a
walking producer. A seat at its castle limit prices the dungeon next.

A test holds two castles at a limit of two, with the dungeon in the
monarch's list, and ticks. On the parent the monarch starts nothing
for twenty ticks. On this it starts the dungeon on the first.

In the twenty minute game the dungeon is ordered near the ten minute
mark and the monarch stands at it and raises it, eighty odd points of
its twenty three thousand a frame, so it is still going up when the
run ends. That is why the game test counts a frame under construction
as well as a finished one: the report was that it was never ordered.

## Also here

Two seams for tests, a limit and a weight for one definition, marking
the profile loaded so the first tick cannot reset them. The test that
runs a Taros computer player for twenty minutes and counts its shots
stays, as the record of the report, and the probe over the Guided
class stays as the record that the weapon was never it.

## Not done here

A wave walks at its target at full distance rather than gathering
first. That is what made the march read as idleness and it is worth
its own look at what the original does before it is touched.
