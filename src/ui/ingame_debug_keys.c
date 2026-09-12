/*
 * ingame_debug_keys.c -- the in-game development hotkeys.
 *
 * Compiled only when TAK_DEBUG is defined. A shipped build gets the
 * inline stub in tak_ingame_debug.h instead, so none of this exists to
 * be pressed. See that header for why.
 *
 * This file only decodes. Acting on what it decodes is ingame.c's job,
 * which keeps the decode testable with no window and no world.
 */

#include "tak_ingame_debug.h"

/* A translation unit with nothing in it is not valid C, and everything
 * below is behind the guard. */
typedef int ig_debug_keys_tu_not_empty;

#ifdef TAK_DEBUG

#include <SDL.h>

#define IG_DBG_PRESSED(sc) (keys[sc] && !prev[sc])

InGameDebugAction InGame_DebugHotkey(const uint8_t *keys, const uint8_t *prev) {
    if (!keys || !prev) return IG_DEBUG_NONE;
    /* Alt is the debug modifier. Every bare letter below is a unit
     * command in the original's bindings, and the speed keys are the
     * whole reason this file exists, so nothing here answers a bare
     * key and nothing here answers minus or equals at all. */
    if (!keys[SDL_SCANCODE_LALT] && !keys[SDL_SCANCODE_RALT])
        return IG_DEBUG_NONE;

    if (IG_DBG_PRESSED(SDL_SCANCODE_3)) return IG_DEBUG_SPAWN_MONARCH;
    if (IG_DBG_PRESSED(SDL_SCANCODE_H)) return IG_DEBUG_ROTATE_HEAD;
    if (IG_DBG_PRESSED(SDL_SCANCODE_L)) return IG_DEBUG_INVOKE_WALK;
    if (IG_DBG_PRESSED(SDL_SCANCODE_V)) return IG_DEBUG_BUMP_VELOCITY;
    if (IG_DBG_PRESSED(SDL_SCANCODE_K)) return IG_DEBUG_KILL_FIRST;
    if (IG_DBG_PRESSED(SDL_SCANCODE_E)) return IG_DEBUG_SPAWN_ENEMY;
    if (IG_DBG_PRESSED(SDL_SCANCODE_4)) return IG_DEBUG_SPAWN_GRID_500;
    if (IG_DBG_PRESSED(SDL_SCANCODE_5)) return IG_DEBUG_SPAWN_GRID_2000;
    if (IG_DBG_PRESSED(SDL_SCANCODE_LEFTBRACKET))  return IG_DEBUG_SCALE_DOWN;
    if (IG_DBG_PRESSED(SDL_SCANCODE_RIGHTBRACKET)) return IG_DEBUG_SCALE_UP;
    if (IG_DBG_PRESSED(SDL_SCANCODE_SEMICOLON))    return IG_DEBUG_TILT_DOWN;
    if (IG_DBG_PRESSED(SDL_SCANCODE_APOSTROPHE))   return IG_DEBUG_TILT_UP;
    return IG_DEBUG_NONE;
}

#undef IG_DBG_PRESSED

#endif /* TAK_DEBUG */
