/* The in-game development hotkeys, from both sides of the build switch.
 *
 * This file is compiled twice. test_ingame_debug_keys is the developer
 * build, where the tools exist and every one of them has to take Alt.
 * test_ingame_keys_shipped is the build a player gets, where the answer
 * to every key is nothing at all. Headless and data free.
 */

#include "tak_ingame_debug.h"
#include "test_framework.h"

#include <SDL.h>
#include <string.h>

static uint8_t keys[SDL_NUM_SCANCODES];
static uint8_t prev[SDL_NUM_SCANCODES];

/* Every key the in-game screen has ever answered with a debug tool. */
static const SDL_Scancode watched[] = {
    SDL_SCANCODE_3, SDL_SCANCODE_4, SDL_SCANCODE_5,
    SDL_SCANCODE_H, SDL_SCANCODE_L, SDL_SCANCODE_V,
    SDL_SCANCODE_K, SDL_SCANCODE_E,
    SDL_SCANCODE_MINUS, SDL_SCANCODE_EQUALS,
    SDL_SCANCODE_LEFTBRACKET, SDL_SCANCODE_RIGHTBRACKET,
    SDL_SCANCODE_SEMICOLON, SDL_SCANCODE_APOSTROPHE
};
#define WATCHED_COUNT ((int)(sizeof(watched) / sizeof(watched[0])))

static InGameDebugAction press(SDL_Scancode sc, int alt) {
    memset(keys, 0, sizeof(keys));
    memset(prev, 0, sizeof(prev));
    keys[sc] = 1;
    if (alt) keys[SDL_SCANCODE_LALT] = 1;
    return InGame_DebugHotkey(keys, prev);
}

#ifdef TAK_DEBUG

TEST(alt_reaches_every_debug_tool) {
    ASSERT_EQ_INT(IG_DEBUG_SPAWN_MONARCH,   (int)press(SDL_SCANCODE_3, 1));
    ASSERT_EQ_INT(IG_DEBUG_SPAWN_GRID_500,  (int)press(SDL_SCANCODE_4, 1));
    ASSERT_EQ_INT(IG_DEBUG_SPAWN_GRID_2000, (int)press(SDL_SCANCODE_5, 1));
    ASSERT_EQ_INT(IG_DEBUG_ROTATE_HEAD,     (int)press(SDL_SCANCODE_H, 1));
    ASSERT_EQ_INT(IG_DEBUG_INVOKE_WALK,     (int)press(SDL_SCANCODE_L, 1));
    ASSERT_EQ_INT(IG_DEBUG_BUMP_VELOCITY,   (int)press(SDL_SCANCODE_V, 1));
    ASSERT_EQ_INT(IG_DEBUG_KILL_FIRST,      (int)press(SDL_SCANCODE_K, 1));
    ASSERT_EQ_INT(IG_DEBUG_SPAWN_ENEMY,     (int)press(SDL_SCANCODE_E, 1));
    ASSERT_EQ_INT(IG_DEBUG_SCALE_DOWN,      (int)press(SDL_SCANCODE_LEFTBRACKET, 1));
    ASSERT_EQ_INT(IG_DEBUG_SCALE_UP,        (int)press(SDL_SCANCODE_RIGHTBRACKET, 1));
    ASSERT_EQ_INT(IG_DEBUG_TILT_DOWN,       (int)press(SDL_SCANCODE_SEMICOLON, 1));
    ASSERT_EQ_INT(IG_DEBUG_TILT_UP,         (int)press(SDL_SCANCODE_APOSTROPHE, 1));
}

/* Bare letters are unit commands in the original: Heal on H, Load on L,
 * ToggleCloak on K, Stop on S. A debug tool on one of them takes a key
 * the binding layer needs. */
TEST(no_bare_key_reaches_a_debug_tool) {
    for (int i = 0; i < WATCHED_COUNT; i++) {
        ASSERT_EQ_INT(IG_DEBUG_NONE, (int)press(watched[i], 0));
    }
}

/* The two keys the whole job turns on. The original binds them to the
 * speed control, so nothing else may answer them, Alt or no Alt. */
TEST(the_speed_keys_are_not_a_debug_tool) {
    ASSERT_EQ_INT(IG_DEBUG_NONE, (int)press(SDL_SCANCODE_MINUS, 0));
    ASSERT_EQ_INT(IG_DEBUG_NONE, (int)press(SDL_SCANCODE_MINUS, 1));
    ASSERT_EQ_INT(IG_DEBUG_NONE, (int)press(SDL_SCANCODE_EQUALS, 0));
    ASSERT_EQ_INT(IG_DEBUG_NONE, (int)press(SDL_SCANCODE_EQUALS, 1));
    ASSERT_EQ_INT(IG_DEBUG_NONE, (int)press(SDL_SCANCODE_KP_MINUS, 0));
    ASSERT_EQ_INT(IG_DEBUG_NONE, (int)press(SDL_SCANCODE_KP_PLUS, 0));
}

/* A held key fires once, not once a frame. */
TEST(a_held_key_fires_once) {
    memset(keys, 0, sizeof(keys));
    memset(prev, 0, sizeof(prev));
    keys[SDL_SCANCODE_LALT] = 1;
    keys[SDL_SCANCODE_K] = 1;
    ASSERT_EQ_INT(IG_DEBUG_KILL_FIRST, (int)InGame_DebugHotkey(keys, prev));
    memcpy(prev, keys, sizeof(prev));
    ASSERT_EQ_INT(IG_DEBUG_NONE, (int)InGame_DebugHotkey(keys, prev));
}

#else  /* the build a player gets */

/* Minus and equals used to resize every model on the map, and the digit
 * keys used to drop two thousand monarchs into the battle. */
TEST(no_debug_key_does_anything_in_a_shipped_build) {
    for (int i = 0; i < WATCHED_COUNT; i++) {
        for (int alt = 0; alt < 2; alt++) {
            ASSERT_EQ_INT(IG_DEBUG_NONE, (int)press(watched[i], alt));
        }
    }
}

/* Not one scancode, held or tapped, with or without a modifier. */
TEST(no_key_at_all_does_anything_in_a_shipped_build) {
    for (int sc = 0; sc < SDL_NUM_SCANCODES; sc++) {
        memset(keys, 0, sizeof(keys));
        memset(prev, 0, sizeof(prev));
        keys[sc] = 1;
        keys[SDL_SCANCODE_LALT] = 1;
        keys[SDL_SCANCODE_LCTRL] = 1;
        keys[SDL_SCANCODE_LSHIFT] = 1;
        ASSERT_EQ_INT(IG_DEBUG_NONE, (int)InGame_DebugHotkey(keys, prev));
    }
}

#endif

int main(int argc, char *argv[]) {
    (void)argc; (void)argv;
#ifdef TAK_DEBUG
    TEST_SUITE("Debug hotkeys in a developer build");
    RUN(alt_reaches_every_debug_tool);
    RUN(no_bare_key_reaches_a_debug_tool);
    RUN(the_speed_keys_are_not_a_debug_tool);
    RUN(a_held_key_fires_once);
#else
    TEST_SUITE("Debug hotkeys in a shipped build");
    RUN(no_debug_key_does_anything_in_a_shipped_build);
    RUN(no_key_at_all_does_anything_in_a_shipped_build);
#endif
    TEST_REPORT();
}
