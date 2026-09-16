# The 3D view draws the fighting

Reported from play: in 3D mode the animations do not show up. The
view drew terrain, sprite features, units, selection rings and water,
and nothing a weapon does. The engine side is `src/render/view3d.c`
with the art handed over by `src/render/units.c`.

## What the classic view draws that the 3D view did not

The classic renderer has four passes after the units: model
projectiles, sprite projectiles, the impact sprites an explosion class
plays where a shot lands, and the beams and bright dots of weapons
with no art of their own. A magic bolt, a catapult stone, a fireball
and the burst where it hits were all in the classic frame and all
absent from the 3D one.

## What the engine does now

Each frame the 3D view walks the same projectile and effect lists the
classic passes walk, through three accessors `units.c` now offers a
second view: the decoded sprite strip of a weapon's art without a
renderer, the visibility gate a projectile has to pass for the local
player, and the model name of a model projectile.

A model projectile is drawn through the model store like a unit with
no pieces moving, at its heading, pitch and roll. A sprite projectile
and an impact sprite stand up as billboards the way sprite features
do, the frame chosen by the same arithmetic as the classic pass, each
frame cut out of one strip texture per art. Beams are a ribbon from
muzzle to target in three widths, the outer ones fainter, jittered
afresh each frame the way the classic rays flicker. A shot with no art
at all is the bright dot the classic view draws.

The strips are freed with the rest of the view's GPU state when the
world changes or the view shuts down.

## How it is checked

Two cases in `test_view3d`. The monarch fires at the ground and the
frame after the shot leaves the muzzle has to count a projectile or a
beam drawn. A shooter whose weapon has an explosion class, chosen from
the data rather than by name since the monarch's beam has none, fires
at the ground and the frame after the shot lands has to count an
impact sprite drawn. Both failed against a view that counted nothing
before the pass went in.

One thing the second case had to learn: the first effects alive in a
game are the computer player's own build sparkles at its base, under
fog, and the classic pass hides those too. The case waits for an
effect the local player can see.
