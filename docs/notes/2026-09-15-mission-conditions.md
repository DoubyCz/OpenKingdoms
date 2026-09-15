# How a campaign mission is won and lost

Line numbers are anchors into the legacy reference, as in every note here.

## Two lists, not one

A mission file writes its conditions as plain keys inside
`[GlobalHeader]`, next to `numplayers` and `size`. There is no
conditions section and no nesting. The original reads those keys at
:239243-239850 and builds two separate lists.

The victory list, eleven keys, in the order they are read:

- `KillEnemyCommander` :239266
- `DestroyAllUnits` :239280
- `KillAllMobileUnits` :239294
- `BuildUnitType` :239310
- `CaptureUnitType` :239352
- `KillAllOfType` :239391
- `KillUnitType` :239432
- `MoveUnitToRadius` :239474
- `UnitTypePassesX` :239532
- `UnitTypePassesZ` :239590
- `VictoryTimerRunsOut` :239648

The defeat list, seven keys:

- `CommanderKilled` :239663
- `AllUnitsKilled` :239677
- `AllUnitsKilledOfType` :239693
- `UnitTypeKilled` :239734
- `DeathTimerRunsOut` :239776
- `AnyUnitPassesX` :239791
- `AnyUnitPassesZ` :239808

Victory needs every condition in the first list at once, :239922-239950.
With no victory condition at all it never fires. Defeat needs any one
condition in the second list, :239952-239990. The per-player tick reads
victory first and only asks about defeat when victory is not met,
:206640-206672, so a tick that satisfies both is a win. Either verdict
puts a banner over the running battle for 90 ticks at the original's
30 Hz, :206542-206572, and then the statistics screen opens.

## The conditions a mission does not name

Two are filled in at :239825-239851. A mission with no victory
condition gets `DestroyAllUnits`, but only when the game mode is not
campaign, so a campaign mission is left as its file wrote it. A mission
with no defeat condition gets `AllUnitsKilled`. The second is where the
ordinary rule that you lose when you lose everything comes from.

## ANYTYPE

`MoveUnitToRadius`, `UnitTypePassesX` and `UnitTypePassesZ` accept the
literal unit type `ANYTYPE`, which is stored as an empty name and means
any type at all, :239493, :239553, :239611. No shipped mission uses it.

## What the shipped corpus says

74 of the 129 mission files carry placed units. 68 of those name at
least one condition and 6 name none. The most common key by a wide
margin is `AllUnitsKilled`, in 29 files, and it is a defeat condition
every time. `DestroyAllUnits` is next at 22 files and is a victory
condition every time. The two appear together in 14 files, which is
what the split is for: destroy the enemy to win, lose your own army to
lose.

Four keys are read by the original and used by no shipped mission:
`BuildUnitType`, `CaptureUnitType`, `AnyUnitPassesX` and
`AnyUnitPassesZ`. The first two ask whether the player owns a unit of
the named type, which covers building one and capturing one alike. The
last two take a bare coordinate and ask whether any enemy unit has
passed it.

## What is still open

Sixteen of the 48 base missions and seven of the 26 Iron Plague ones
name only defeat conditions. Read literally, the rules above give those
missions no way to win, because campaign mode adds no implicit victory
condition. The engine sets its victory flag in one place only,
:206549, reached only from the victory list, so nothing else in the
game can end those missions. Either the original shipped them that way
or there is a path this note has not found. The engine follows the
rules above until someone settles it.

## Which side a condition reads

A condition that takes no unit type has to know whose units it counts,
because otherwise the victory key and the defeat key of a pair would
ask the same question. `DestroyAllUnits` and `KillEnemyCommander` read
the enemy. `AllUnitsKilled` and `CommanderKilled` read the player.
`AnyUnitPassesX` and `AnyUnitPassesZ` read the enemy.

A condition that names a unit type does not need the side as well. The
mission author picks the key whose list the named type belongs in, so
`AllUnitsKilledOfType` and `KillAllOfType` ask the same question of the
map and differ only in which list holds the answer. The engine reads
them that way because it cannot yet prove the original narrows them,
and because narrowing them would break any mission whose escorted unit
belongs to a seat other than the player's.
