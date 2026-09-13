# The room, end to end

Line numbers of the form `legacy:NNNN` are anchors into the legacy
reference, as in every note here.

Two people on the public site could join one room and do nothing in it:
no name, no chat, no rules, no colour, no team, no map, no Go, and so
no game. Then a ghost sat in a seat. This note records what each of
those was, because several were not the thing they looked like.

## The ghost

The relay pinged every client every two seconds and never looked at
the replies. A client was forgotten only when its socket closed, and
through a proxy a dead tab's socket can stay open for a long time. So
the ghost held a seat and kept its room alive, and when the same
person came back they were a new client whose seat no longer matched
their row, so every press on Go was refused.

`TAK_RELAY_SILENT_MS` is four heartbeats. A welcomed client heard from
not at all in that time is dropped, whatever its socket says, and the
room it was alone in goes with it. Every frame that reaches the
dispatcher counts as being heard, so an idle client that answers pings
is never touched. The lobby is told when a room appears, starts or
goes, so a phantom game leaves every screen on its own rather than at
the next press of Update.

## The connection that closed on its own

Typing a name in the lobby reconnects, because the name travels in the
greeting. The page closed the old socket and opened a new one in the
same call, and the old socket's close event arrived a moment later and
marked the new link closed. Every socket callback now checks the
socket it was made for is still the one the game is using.

## Keys that were never seen

The screens read SDL's held key state once a frame and look for an
edge. A press that goes down and up between two frames has no edge. A
person's press outlasts a frame at sixty a second and not at ten, and
a script's lasts no time at all. The platform now latches Enter,
Escape and Backspace when the key down event arrives, and the screens
read the latch as well as the held state.

## What the room draws, and from where

Everything from the server's snapshot and nothing from local guesses,
which is the rule the original follows (legacy:136219-136222). The Go
box is frame 4 when the seat is ready and 3 when it is not. The rule
boxes draw the room's options word. The unit limit draws the cap. The
map name draws the room's map. The colour cell and the team cell
author no art of their own, so the badge is painted from the side's
team logo sheet and the team as a number, as the skirmish screen does.

A refusal from the server lands on the help strip. It landed nowhere,
so Play with a seat not ready did nothing anyone could see, which is
what made Go look broken before the ghost was found.

## What the host may change

The rules, the unit limit and the map, each as one edit to the server.
A guest pressing the same box sends nothing and is told whose it is
(legacy:136832). The map chooser is choosemap.gui for the host and
viewmap.gui for a guest (legacy:136832-136851), and OK on the host's
sends the name and the fingerprint this install computed for it, so
the room's map is a specific file and not a name two installs might
disagree about.

## Chat

Text input is on for the whole of the room, so a player just types.
Enter sends the line to the room, and lines from the server go under
the ones before, newest at the bottom, the way the original keeps the
list (legacy:136856).

## The run

`scripts/room-browser-smoke.js` drives two pages through all of it
with clicks that last sixty milliseconds, which is a person's click:
names, chat both ways, a colour, teams, a rule, the unit limit, the
map, a refused Play, both ready, and Play into the battle on the map
that was chosen. It fails on any page error and on any click the room
reports as unhandled.
