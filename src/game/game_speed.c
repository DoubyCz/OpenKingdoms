/*
 * game_speed.c -- the game speed level, skirmish and story.
 *
 * The model is the original's: an integer level 0..20, default 10, one
 * step per press, and the level read as a rate of level * 0.1
 * (legacy:131742-131749, legacy:131698, legacy:131812, legacy:131823,
 * legacy:242355). Nothing here touches the simulation. All it does is
 * hand the timer a number, and the timer decides how many ticks a frame
 * gets. A tick keeps its length and keeps its content at every speed.
 */

#include "tak_game_speed.h"

#include <stdio.h>
#include <string.h>

#define GS_STR_MAX 64

static struct {
    int    available;          /* 0 outside a single player battle */
    int    level;              /* what the player asked for (legacy:+0x14) */
    int    effective;          /* what the timer runs at (legacy:+0x18) */
    int    hysteresis;         /* the saturation counter (legacy:+0x19f48) */
    char   message[GS_STR_MAX * 2];
    double message_left;       /* seconds the line has left */
    double message_seconds;
    char   str_normal[GS_STR_MAX];
    char   str_prefix[GS_STR_MAX];
} gs = {
    0, GAME_SPEED_LEVEL_NORMAL, GAME_SPEED_LEVEL_NORMAL, 0,
    "", 0.0, 5.0, "Game Speed Normal", "Game Speed"
};

void GameSpeed_Reset(void) {
    gs.level = GAME_SPEED_LEVEL_NORMAL;
    gs.effective = GAME_SPEED_LEVEL_NORMAL;
    gs.hysteresis = 0;
    gs.message[0] = '\0';
    gs.message_left = 0.0;
}

void GameSpeed_SetAvailable(int available) {
    gs.available = available ? 1 : 0;
}

int GameSpeed_IsAvailable(void) {
    return gs.available;
}

int GameSpeed_GetLevel(void) {
    return gs.level;
}

int GameSpeed_GetEffectiveLevel(void) {
    return gs.effective;
}

double GameSpeed_Multiplier(void) {
    return (double)gs.effective * 0.1;
}

void GameSpeed_SetMessageSeconds(double seconds) {
    if (seconds < 0.0) seconds = 0.0;
    if (seconds > 20.0) seconds = 20.0;   /* TextScrollTime, legacy:131695 */
    gs.message_seconds = seconds;
}

void GameSpeed_SetStrings(const char *normal, const char *prefix) {
    if (normal && normal[0]) {
        strncpy(gs.str_normal, normal, sizeof(gs.str_normal) - 1);
        gs.str_normal[sizeof(gs.str_normal) - 1] = '\0';
    }
    if (prefix && prefix[0]) {
        strncpy(gs.str_prefix, prefix, sizeof(gs.str_prefix) - 1);
        gs.str_prefix[sizeof(gs.str_prefix) - 1] = '\0';
    }
}

/* The line the original raises on every change (legacy:131758-131789).
 * At normal it is a string of its own, otherwise it is the offset from
 * normal with an explicit plus, since the minus comes free with the
 * number (legacy:131785). It goes to the message line as a system
 * message, which is why no chime plays with it (legacy:205814). */
static void gs_announce(void) {
    int offset = gs.level - GAME_SPEED_LEVEL_NORMAL;
    if (offset == 0) {
        strncpy(gs.message, gs.str_normal, sizeof(gs.message) - 1);
        gs.message[sizeof(gs.message) - 1] = '\0';
    } else {
        char sign = (offset > 0) ? '+' : ' ';
        snprintf(gs.message, sizeof(gs.message), "%s %c%d",
                 gs.str_prefix, sign, offset);
    }
    gs.message_left = gs.message_seconds;
}

void GameSpeed_SetLevel(int level) {
    if (!gs.available) return;
    if (level > GAME_SPEED_LEVEL_MAX) level = GAME_SPEED_LEVEL_MAX;
    if (level < GAME_SPEED_LEVEL_MIN) level = GAME_SPEED_LEVEL_MIN;
    if (level == gs.level) return;
    gs.level = level;
    /* A deliberate change puts the effective level back on the
     * requested one, so the throttle below starts again from what the
     * player just chose (legacy:131791-131792). */
    gs.effective = level;
    gs.hysteresis = 0;
    gs_announce();
}

void GameSpeed_Increase(void) {
    if (gs.level < GAME_SPEED_LEVEL_MAX) GameSpeed_SetLevel(gs.level + 1);
}

void GameSpeed_Decrease(void) {
    if (gs.level > GAME_SPEED_LEVEL_MIN) GameSpeed_SetLevel(gs.level - 1);
}

/* The throttle. A frame that spent its whole tick budget threw time
 * away, and a run of those means the machine cannot hold the speed the
 * player asked for, so the effective level walks down. A run of frames
 * with headroom walks it back up, never past the requested level. The
 * counters are the original's: eleven saturated frames to drop a level,
 * a hundred and one clear ones to regain one (legacy:242401-242416,
 * legacy:131830-131848). */
void GameSpeed_NoteFrame(int ticks_run, int cap) {
    if (cap > 0 && ticks_run >= cap) {
        gs.hysteresis++;
        if (gs.hysteresis > 10) {
            gs.hysteresis = 0;
            if (gs.effective > GAME_SPEED_LEVEL_MIN) gs.effective--;
        }
    } else {
        gs.hysteresis--;
        if (gs.hysteresis < -100) {
            gs.hysteresis = 0;
            if (gs.effective < gs.level) gs.effective++;
        }
    }
}

const char *GameSpeed_Message(void) {
    return (gs.message_left > 0.0) ? gs.message : "";
}

void GameSpeed_AgeMessage(double dt_seconds) {
    if (gs.message_left <= 0.0) return;
    gs.message_left -= dt_seconds;
    if (gs.message_left <= 0.0) {
        gs.message_left = 0.0;
        gs.message[0] = '\0';
    }
}
