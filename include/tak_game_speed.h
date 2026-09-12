#ifndef TAK_GAME_SPEED_H
#define TAK_GAME_SPEED_H

/*
 * Game speed: skirmish and story only.
 *
 * The original keeps an integer level, not a multiplier and not a frame
 * delay. The level runs 0..20 with a default of 10, one step per press,
 * and the level becomes a rate as level * 0.1 (legacy:131742-131749,
 * legacy:131698, legacy:131812, legacy:131823, legacy:242355). So the
 * range is a full stop at 0, normal at 10 and double speed at 20.
 *
 * Speed changes how OFTEN a simulation tick runs. It never changes what
 * a tick does and never changes the length of a tick, which is why the
 * same battle hashes the same at any speed for the same tick count
 * (legacy:242391-242441).
 *
 * Two levels are kept on purpose (legacy:131832, legacy:131843). The
 * requested level is what the keys set and what the on screen message
 * prints. The effective level is what the timer runs at. A machine that
 * cannot keep up has its effective level walked down by GameSpeed_NoteFrame
 * and walked back up when it recovers, while the number the player chose
 * stays where they put it.
 *
 * A networked battle does not get this control at all. That is an owner
 * decision, not a parity one: the original did broadcast a speed change
 * (legacy:131795-131800), we do not route one through the turn clock.
 */

#define GAME_SPEED_LEVEL_MIN     0
#define GAME_SPEED_LEVEL_MAX     20
#define GAME_SPEED_LEVEL_NORMAL  10

/* Back to normal with both levels equal and the message cleared. Call
 * at the start of every battle (legacy:131713-131715). */
void GameSpeed_Reset(void);

/* Whether the player may change the speed at all. Off for a networked
 * battle. Off until something turns it on, so a screen that forgets is
 * silent rather than wrong. */
void GameSpeed_SetAvailable(int available);
int  GameSpeed_IsAvailable(void);

/* The level the player asked for, 0..20. */
int  GameSpeed_GetLevel(void);

/* The level the timer actually runs at, 0..requested. */
int  GameSpeed_GetEffectiveLevel(void);

/* Set the requested level. Out of range values clamp
 * (legacy:131742-131749). Setting it also sets the effective level
 * (legacy:131791-131792) and raises the on screen message. Does nothing
 * when the control is unavailable. */
void GameSpeed_SetLevel(int level);

/* One step up or down (legacy:131808-131825). */
void GameSpeed_Increase(void);
void GameSpeed_Decrease(void);

/* The effective level as a rate, effective * 0.1 (legacy:242355). */
double GameSpeed_Multiplier(void);

/* Told how a frame went: how many ticks ran and what the frame's cap
 * was. Sustained saturation walks the effective level down, sustained
 * headroom walks it back up (legacy:242401-242416). */
void GameSpeed_NoteFrame(int ticks_run, int cap);

/* The transient line shown on a change, "" when nothing is showing.
 * "Game Speed Normal" at 10, else "Game Speed +5" or "Game Speed -3"
 * (legacy:131758-131789). */
const char *GameSpeed_Message(void);

/* Age the message by a frame of wall time. */
void GameSpeed_AgeMessage(double dt_seconds);

/* How long a message stays up, in seconds. The original's TextScrollTime
 * option, range 0..20 (legacy:131695). */
void GameSpeed_SetMessageSeconds(double seconds);

/* The two strings the message is built from. The original looks both up
 * in the translate table (legacy:131766, legacy:131786) and a miss hands
 * back the key (legacy:267931), which is what the defaults are. */
void GameSpeed_SetStrings(const char *normal, const char *prefix);

#endif /* TAK_GAME_SPEED_H */
