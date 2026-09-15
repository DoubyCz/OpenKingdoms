# The Book of Deeds

The story screen is `bod.gui`, reached from the main menu. This note
records what the original does on it and where the engine follows.

## Which books an install offers

The campaign list is a scan of `camps\*.tdf` (legacy:141576,
legacy:143362). The misnamed `FileSystem_WriteFile` at legacy:258585 is
the FindFirst and FindNext wrapper behind it. Each entry is named
through the translate table, keyed by its own lower case file name
(legacy:143453). `-pretendnoexpansion` cuts the scan down to `book of
darien.tdf` alone before anything else looks at it (legacy:141580-141710,
legacy:143377-143405).

The blank entry is the only one the scan drops
(legacy:143366-143377). Every other file it finds becomes a book, and
nothing filters on whether a name for it exists.

The shipped data carries three such files. `data.hpi` has `book of
darien.tdf` with 48 missions. `IPData.hpi` has `the iron plague.tdf` with
25 and `ipalt.tdf`, which is byte for byte the same file except that its
last mission is `takx26_dh` rather than `takx25_dh`. Only the first two
are named in a translate table, both in `IPEnglish.hpi`'s
`translate/guiexpansion.tdf`:

    [book of darien.tdf]  English = Book of Darien
    [the iron plague.tdf] English = The Iron Plague

A base install has no `guiexpansion.tdf` at all, so it names neither, and
it does not need to: one book is the whole list.

`ipalt.tdf` is named nowhere, and a lookup that misses hands back the key
it was given (legacy:267931), so that book reads as its own lower case
file name. The string `ipalt` does not appear in the executable at all.
It is not a special case in the original and it is not one here: the
chapter art's catch-all frame at 0x31 exists precisely for a campaign
file the art does not know by name, which is what makes it plain that
such files are expected to reach this screen.

An Iron Plague install therefore offers three books. The list is sorted
on the shown name with a case-insensitive compare
(legacy:261900-261990, legacy:143490), which puts them in this order:

| shown | file | chapter art |
| --- | --- | --- |
| Book of Darien | `book of darien.tdf` | chapter index plus 1 |
| ipalt.tdf | `ipalt.tdf` | 0x31, every chapter |
| The Iron Plague | `the iron plague.tdf` | chapter index plus 0x32 |

With more than one book the Change User button opens
`PlayerCampaignDialogue.gui`, a combined player and campaign chooser, and
with one it opens the plain `PlayerDialogue.gui` (legacy:142640,
legacy:143946-143970).

## The page

`BookOf` is authored text, "Book of", and no code ever writes to it.
`BookName` carries the player's own name (legacy:144267), so the book is
the player's, not the campaign's.

`ChapterNumber` is the chapter index plus one (legacy:144452-144458).

`ChapterText` is the localised chapter title, keyed by the campaign
file's `missionname` for that chapter (legacy:144497-144540). Base game
titles are in `english/translate/missions.tdf` and the expansion's are in
`ipmissions.tdf`. When the Crusades balance is on, the original appends
the localised "Crusades Balance" in brackets (legacy:144563-144627).

`ChapterImage` picks a frame of `story1.gaf`'s one entry, `Story1`
(legacy:144460-144495):

| book | frame |
| --- | --- |
| `book of darien.tdf` | chapter index plus 1 |
| `the iron plague.tdf` | chapter index plus 0x32 |
| a blank name | chapter index plus 0x32 |
| anything else | 0x31 |

A frame at or past the entry's frame count falls back to 0. The numbers
fit the art exactly. `data.hpi`'s `Story1` has 49 frames for 48
chapters, which leaves 0x31 out of range and so blank on a base install.
`IPData.hpi` replaces the file with one of 75 frames, which covers 0x31
and the 25 expansion chapters at 0x32 through 0x4A. A widget in a `.gui`
carries at most sixteen frames, so the engine draws this one frame
itself rather than through the widget.

## The buttons

- Play starts the open chapter. Holding shift while the open chapter is
  `takx25_dh` starts `takx26_dh` instead (legacy:143903-143923).
  `takx26_dh` ships complete in `IPMissions.hpi`. There is no `Takx26`
  movie in the install, which the clip player already handles.
- Next Chapter and Previous Chapter walk the campaign's mission list,
  which is not itself clipped (legacy:141334-141368). The stop comes
  from the buttons: Next Chapter is live only while there is a next
  mission and the open chapter is below the high water mark
  (legacy:144107-144131). That is what the cheat below moves.
- Load Game opens the load dialog (legacy:143865-143868), the same one
  the F1 menu and the skirmish lobby open.
- Change User opens one of the two dialogs above.
- Previous goes back to the main menu.

Typing `wasabi` on the screen sets the high water mark to the mission
count, which leaves Next Chapter live all the way to the end
(legacy:144228-144234).

The high water mark also decides where the book opens. Picking a user
reads their progress file, takes the furthest mission it records
(legacy:141660-141680), and walks the book forward to it
(legacy:144385-144400, legacy:144405-144425).

## Saves that need the expansion

Loading a save reads its summary. A save naming the Iron Plague campaign,
and a skirmish save carrying a `[CreonUnits]` section, are both refused
with `NEED_EXPANSION_TO_PLAY` when the expansion is absent
(legacy:159443-159470, legacy:159596-159612). In this engine the
container carries a BattleConfig rather than a text summary, so the same
gate reads as Creon on a seat or the Crusades balance turned on, which is
what those two sections record. A save whose map is not installed is
still refused by the map's name first, which is what an Iron Plague
campaign save meets on a base install.

## What is not here yet

The player dialogs keep one name rather than a list of profiles, and
there is no saved high water mark on disk, so progress lasts as long as
the process. `Story_MissionFinished` is the one entry point that moves
it, and `Story_UnlockAllChapters` is what the cheat calls, so the work
that stores progress has one place to hook into.
