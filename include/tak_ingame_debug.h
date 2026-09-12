#ifndef TAK_INGAME_DEBUG_H
#define TAK_INGAME_DEBUG_H

#include <stdint.h>

/*
 * The in-game development hotkeys.
 *
 * These spawn units, kill units, drive scripts and retune model
 * constants. None of them belongs in a player's hands, and two of them
 * sat on the keys the original binds to the speed control, so pressing
 * + in a shipped build resized every model on the map instead of
 * speeding the battle up.
 *
 * They are compiled only when TAK_DEBUG is defined, which CMake sets for
 * the Debug configuration alone. A shipped build gets the inline stub
 * below, which decodes nothing, so there is no key a player can press to
 * reach any of this. Same shape as tak_debug_panel.h.
 *
 * Every one of them also takes Alt now. The original treats a bare
 * letter as a unit command (Heal on H, Load on L, ToggleCloak on K,
 * Stop on S), so a debug tool on a bare letter would collide with real
 * input the moment the binding layer lands.
 *
 * Decoding is separate from acting so a test can press a key and read
 * back what the build would have done, with no window and no world.
 */

typedef enum InGameDebugAction {
    IG_DEBUG_NONE = 0,
    IG_DEBUG_SPAWN_MONARCH,     /* Alt+3 */
    IG_DEBUG_ROTATE_HEAD,       /* Alt+H */
    IG_DEBUG_INVOKE_WALK,       /* Alt+L */
    IG_DEBUG_BUMP_VELOCITY,     /* Alt+V */
    IG_DEBUG_KILL_FIRST,        /* Alt+K */
    IG_DEBUG_SPAWN_ENEMY,       /* Alt+E */
    IG_DEBUG_SPAWN_GRID_500,    /* Alt+4 */
    IG_DEBUG_SPAWN_GRID_2000,   /* Alt+5 */
    IG_DEBUG_SCALE_DOWN,        /* Alt+[ */
    IG_DEBUG_SCALE_UP,          /* Alt+] */
    IG_DEBUG_TILT_DOWN,         /* Alt+; */
    IG_DEBUG_TILT_UP            /* Alt+' */
} InGameDebugAction;

/* One action per call, in the order listed above, from an SDL keyboard
 * state array and last frame's copy of it. IG_DEBUG_NONE when no debug
 * key went down this frame. */
#ifdef TAK_DEBUG
InGameDebugAction InGame_DebugHotkey(const uint8_t *keys, const uint8_t *prev);
#else
static inline InGameDebugAction InGame_DebugHotkey(const uint8_t *keys,
                                                   const uint8_t *prev) {
    (void)keys; (void)prev;
    return IG_DEBUG_NONE;
}
#endif

#endif /* TAK_INGAME_DEBUG_H */
