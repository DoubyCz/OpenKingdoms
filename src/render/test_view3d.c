/*
 * test_view3d.c -- the 3D view leaves the classic view untouched.
 *
 * A battle drawn once in the classic view, then in the 3D view, then
 * in the classic view again has to give the same classic pixels as if
 * the 3D view had never been up, the camera has to come back over the
 * same ground, and the selection has to survive. The 3D frame itself
 * has to be a real picture, and its pointer has to land where the
 * camera looks. The read only accessors the view depends on are
 * checked here too.
 *
 * It needs game data and a window with a GL context, so it carries
 * the needs-data label and runs under the shared test lock. Where the
 * opengl renderer cannot be made, every case skips.
 */

#include "test_framework.h"

#include "tak_battle_config.h"
#include "tak_crash.h"
#include "tak_features.h"
#include "tak_gameloop.h"
#include "tak_hpi.h"
#include "tak_hud.h"
#include "tak_ingame.h"
#include "tak_loading.h"
#include "tak_platform.h"
#include "tak_terrain.h"
#include "tak_ui.h"
#include "tak_unit.h"
#include "tak_view.h"
#include "tak_view3d.h"
#include "tak_world.h"

#include <SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef TAK_GAME_DIR
#define TAK_GAME_DIR "C:/GOG Games/Total Annihilation Kingdoms"
#endif
#ifndef TAK_DATA_DIR
#define TAK_DATA_DIR "data/extracted"
#endif

#define MAP_NAME  "two castles"
#define MAP_WORLD "aramon"
#define WIN_W 800
#define WIN_H 600

/* The feature sprite frame counter steps once per classic render and
 * picks a frame every four, so two classic frames this far apart draw
 * the same picture of a frozen world. */
#define CLASSIC_PHASE 4

static int setup_platform(TAK_Platform *p) {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        SKIP_MARK("SDL init failed: %s", SDL_GetError());
        return -1;
    }
    memset(p, 0, sizeof(*p));
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    p->window = SDL_CreateWindow("tak-view3d", SDL_WINDOWPOS_CENTERED,
                                 SDL_WINDOWPOS_CENTERED, WIN_W, WIN_H,
                                 SDL_WINDOW_HIDDEN | SDL_WINDOW_OPENGL);
    if (!p->window) { SKIP_MARK("window failed"); return -1; }
    SDL_SetHint(SDL_HINT_RENDER_DRIVER, "opengl");
    SDL_SetHint(SDL_HINT_RENDER_BATCHING, "1");
    p->renderer = SDL_CreateRenderer(p->window, -1, SDL_RENDERER_ACCELERATED);
    if (!p->renderer) { SKIP_MARK("no opengl renderer: %s", SDL_GetError()); return -1; }
    SDL_RendererInfo info;
    if (SDL_GetRendererInfo(p->renderer, &info) != 0 ||
        !info.name || strcmp(info.name, "opengl") != 0) {
        SKIP_MARK("renderer is not opengl");
        return -1;
    }
    p->renderer_gen = TAK_Platform_NewRendererGen();
    p->canvas_w = 640; p->canvas_h = 480;
    p->window_w = WIN_W; p->window_h = WIN_H;
    p->scale = 1.0f;
    /* No focus: no cursor animation and no scroll, so a frame is a
     * function of the world alone. */
    p->has_focus = 0;
    p->canvas_tex = SDL_CreateTexture(p->renderer, SDL_PIXELFORMAT_RGBA32,
                                      SDL_TEXTUREACCESS_STREAMING,
                                      p->canvas_w, p->canvas_h);
    if (!p->canvas_tex) { SKIP_MARK("canvas texture failed"); return -1; }
    return 0;
}

static void teardown_platform(TAK_Platform *p) {
    if (p->canvas_tex) SDL_DestroyTexture(p->canvas_tex);
    if (p->renderer) SDL_DestroyRenderer(p->renderer);
    if (p->window) SDL_DestroyWindow(p->window);
    SDL_Quit();
}

/* 0 on success, 1 when there is no data or no GL to run against. */
static int boot(TAK_Platform *plat, GameWorld **out) {
    if (VFS_IsInitialized()) VFS_Shutdown();
    if (VFS_Init(TAK_GAME_DIR, TAK_DATA_DIR) != 0) {
        SKIP_MARK("no data dir");
        return 1;
    }
    if (setup_platform(plat) != 0) { VFS_Shutdown(); return 1; }
    if (UI_Init() != 0) return -1;
    BattleConfig cfg;
    BattleConfig_SetDefaults(&cfg);
    strncpy(cfg.map_name, MAP_NAME, sizeof(cfg.map_name) - 1);
    cfg.players[1].kind = TAK_SLOT_AI;
    cfg.line_of_sight = 0;
    if (World_BeginLoad(plat, &cfg, MAP_NAME, MAP_WORLD) != 0) return -1;
    if (Loading_Init(plat) != 0) return -1;
    int next = GAMESTATE_GAME_LOADING;
    for (int i = 0; i < 4000 && next == GAMESTATE_GAME_LOADING; i++) {
        next = Loading_Tick(plat, 1.0f / 60.0f);
    }
    if (next != GAMESTATE_IN_GAME) return -1;
    *out = World_Get();
    if (!*out) return -1;
    if (InGame_Init(plat) != 0) return -1;
    return 0;
}

static void shutdown_all(TAK_Platform *plat) {
    InGame_Shutdown();
    Loading_Shutdown();
    World_End(plat);
    UI_Shutdown();
    teardown_platform(plat);
    VFS_Shutdown();
}

/* One frame with no simulation time: the world stays where it is. */
static int frame(TAK_Platform *plat, Timer *timer) {
    timer->accumulator = 0.0;
    TAK_Platform_FrameBegin(plat);
    return InGame_Tick(plat, timer) == GAMESTATE_IN_GAME;
}

static int capture(TAK_Platform *plat, uint32_t *out) {
    return SDL_RenderReadPixels(plat->renderer, NULL, SDL_PIXELFORMAT_RGBA32,
                                out, WIN_W * 4) == 0;
}

static int differing_pixels(const uint32_t *a, const uint32_t *b) {
    int n = 0;
    for (int i = 0; i < WIN_W * WIN_H; i++) if (a[i] != b[i]) n++;
    return n;
}

/* Lit pixels and distinct colours, the same measure the screen suite
 * uses to tell a picture from a blank. */
static void signal_of(const uint32_t *px, int *out_lit, int *out_bins) {
    uint32_t bins[64];
    int nb = 0, lit = 0;
    for (int i = 0; i < WIN_W * WIN_H; i += 3) {
        uint32_t p = px[i];
        uint8_t r = (uint8_t)p, g = (uint8_t)(p >> 8), b = (uint8_t)(p >> 16);
        if (!(r || g || b)) continue;
        lit++;
        uint32_t bin = ((uint32_t)(r >> 4) << 8) | ((uint32_t)(g >> 4) << 4) | (b >> 4);
        int seen = 0;
        for (int k = 0; k < nb; k++) if (bins[k] == bin) { seen = 1; break; }
        if (!seen && nb < 64) bins[nb++] = bin;
    }
    *out_lit = lit;
    *out_bins = nb;
}

TEST(toggling_3d_on_and_off_leaves_the_classic_frame_byte_identical) {
    TAK_Platform platform;
    GameWorld *world = NULL;
    int rc = boot(&platform, &world);
    if (rc == 1) return;
    ASSERT_EQ_INT(0, rc);
    Timer timer;
    Timer_Init(&timer);
    uint32_t *a = (uint32_t *)malloc((size_t)WIN_W * WIN_H * 4);
    uint32_t *b = (uint32_t *)malloc((size_t)WIN_W * WIN_H * 4);
    uint32_t *c = (uint32_t *)malloc((size_t)WIN_W * WIN_H * 4);
    ASSERT(a && b && c);

    /* Something on screen worth comparing: the monarch, selected. */
    int n = 0;
    const Unit *units = Units_GetActive(&n);
    ASSERT(n > 0);
    Units_SelectSingle(0);
    world->cam_x = units[0].world_x - world->viewport_w / 2;
    world->cam_y = units[0].world_y - world->viewport_h / 2;
    if (world->cam_x < 0) world->cam_x = 0;
    if (world->cam_y < 0) world->cam_y = 0;
    const int32_t cam_x0 = world->cam_x, cam_y0 = world->cam_y;

    /* The control: with the world frozen, two classic frames a phase
     * apart draw the same bytes. This is what makes the comparison
     * below mean something. */
    for (int i = 0; i < CLASSIC_PHASE; i++) ASSERT(frame(&platform, &timer));
    ASSERT(capture(&platform, a));
    for (int i = 0; i < CLASSIC_PHASE; i++) ASSERT(frame(&platform, &timer));
    ASSERT(capture(&platform, b));
    ASSERT_EQ_INT(0, differing_pixels(a, b));
    int lit = 0, bins = 0;
    signal_of(a, &lit, &bins);
    ASSERT(lit > 20000);
    ASSERT(bins >= 8);

    /* Into the 3D view, draw a few frames there, and back. */
    ASSERT_EQ_INT(1, InGame_SetView3D(1));
    ASSERT_EQ_INT(1, InGame_IsView3D());
    for (int i = 0; i < 3; i++) ASSERT(frame(&platform, &timer));
    ASSERT(capture(&platform, c));
    ASSERT_EQ_INT(1, InGame_SetView3D(0));
    ASSERT_EQ_INT(0, InGame_IsView3D());
    for (int i = 0; i < CLASSIC_PHASE; i++) ASSERT(frame(&platform, &timer));
    ASSERT(capture(&platform, b));
    ASSERT_EQ_INT(0, differing_pixels(a, b));

    /* The camera came back over the same ground and the selection
     * survived the trip. */
    ASSERT_EQ_INT((int)cam_x0, (int)world->cam_x);
    ASSERT_EQ_INT((int)cam_y0, (int)world->cam_y);
    int sel_n = 0;
    const int *sel = Units_GetSelection(&sel_n);
    ASSERT_EQ_INT(1, sel_n);
    ASSERT_EQ_INT(0, sel[0]);

    /* And the 3D frame was a different, real picture. */
    ASSERT(differing_pixels(a, c) > 20000);
    signal_of(c, &lit, &bins);
    ASSERT(lit > 20000);
    ASSERT(bins >= 8);

    free(a); free(b); free(c);
    shutdown_all(&platform);
}

/* Height of the ground at a point, for the pointer check. */
TEST(the_3d_pointer_lands_where_the_camera_looks) {
    TAK_Platform platform;
    GameWorld *world = NULL;
    int rc = boot(&platform, &world);
    if (rc == 1) return;
    ASSERT_EQ_INT(0, rc);
    Timer timer;
    Timer_Init(&timer);
    ASSERT(frame(&platform, &timer));
    ASSERT_EQ_INT(1, InGame_SetView3D(1));
    ASSERT(frame(&platform, &timer));
    const Camera3D *cam = View3D_Camera();
    /* The play area's middle pixel looks at the target. */
    SDL_Rect play = { 0, 0, WIN_W, WIN_H };
    (void)HUD_GetViewportRect(&platform, &play);
    int mx = play.x + play.w / 2, my = play.y + play.h / 2;
    int32_t fx = 0, fy = 0;
    ASSERT_EQ_INT(1, View_3D()->pointer_to_world(world, &platform, mx, my, &fx, &fy));
    int h = Terrain_SampleHeight(world, (int32_t)cam->target_x, (int32_t)cam->target_z);
    int32_t want_y = (int32_t)cam->target_z - (int32_t)((float)h * Units_GetTanTilt());
    ASSERT(abs(fx - (int32_t)cam->target_x) <= 2);
    ASSERT(abs(fy - want_y) <= 3);
    /* Off the play area, over the sidebar, is off the world. */
    ASSERT_EQ_INT(0, View_3D()->pointer_to_world(world, &platform, WIN_W + 50, my, &fx, &fy));
    shutdown_all(&platform);
}

TEST(a_scroll_in_3d_moves_the_classic_camera_with_it) {
    TAK_Platform platform;
    GameWorld *world = NULL;
    int rc = boot(&platform, &world);
    if (rc == 1) return;
    ASSERT_EQ_INT(0, rc);
    Timer timer;
    Timer_Init(&timer);
    world->cam_x = 400; world->cam_y = 400;
    ASSERT(frame(&platform, &timer));
    ASSERT_EQ_INT(1, InGame_SetView3D(1));
    ASSERT(frame(&platform, &timer));
    const Camera3D *cam = View3D_Camera();
    float tx = cam->target_x, tz = cam->target_z;
    /* The classic preset looks north, so right is east and down is south. */
    View_3D()->scroll(world, 100, 50);
    float scale = cam->dist / 1000.0f;
    ASSERT(cam->target_x > tx + 50.0f * scale);
    ASSERT(cam->target_z > tz + 25.0f * scale);
    ASSERT_EQ_INT((int)cam->target_x - world->viewport_w / 2, (int)world->cam_x);
    ASSERT_EQ_INT((int)cam->target_z - world->viewport_h / 2, (int)world->cam_y);
    /* A minimap jump moves the classic camera; the 3D view follows. */
    world->cam_x = 900; world->cam_y = 700;
    ASSERT(frame(&platform, &timer));
    ASSERT_EQ_INT(900 + world->viewport_w / 2, (int)cam->target_x);
    ASSERT_EQ_INT(700 + world->viewport_h / 2, (int)cam->target_z);
    shutdown_all(&platform);
}

TEST(the_accessors_hand_out_the_baked_model_and_its_pose) {
    TAK_Platform platform;
    GameWorld *world = NULL;
    int rc = boot(&platform, &world);
    if (rc == 1) return;
    ASSERT_EQ_INT(0, rc);

    UnitMesh *m = Units_BakeObjectMesh("araking", 0);
    ASSERT_NOT_NULL(m);
    ASSERT(m->node_count > 1);
    ASSERT(m->vert_count > 0);
    ASSERT_EQ_INT(-1, (int)m->nodes[0].parent);
    /* Every vertex names a node the mesh has. */
    for (int v = 0; v < m->vert_count; v++) ASSERT(m->vert_node_idx[v] < m->node_count);
    /* At rest each node sits at its parent plus its own offset. */
    UnitNodeXform xf[UNIT_MESH_MAX_NODES];
    Units_ComposeNodeXforms(m, NULL, xf, 1);
    for (int i = 1; i < m->node_count; i++) {
        int p = m->nodes[i].parent;
        ASSERT(p >= 0 && p < i);
        for (int k = 0; k < 3; k++) {
            float want = xf[p].trans[k] + m->nodes[i].offset[k];
            ASSERT(xf[i].trans[k] > want - 0.01f && xf[i].trans[k] < want + 0.01f);
        }
    }
    /* A missing model is a NULL, not a crash. */
    ASSERT_NULL(Units_BakeObjectMesh("no_such_model_here", 0));
    Units_FreeBakedMesh(m);

    /* A feature with a sprite on this map decodes to a frame. */
    int found = 0;
    for (int i = 0; i < world->feature_count && !found; i++) {
        const FeatureDef *fd = Features_GetByIndex(world->features[i].global_idx);
        if (!fd || fd->object[0] || !fd->filename[0]) continue;
        const uint32_t *px = NULL;
        int w = 0, h = 0, ox = 0, oy = 0;
        int frames = Units_FeatureSpriteFrame(fd, world->features_rgba, 0,
                                              &px, &w, &h, &ox, &oy);
        if (frames <= 0) continue;
        ASSERT_NOT_NULL(px);
        ASSERT(w > 0 && h > 0);
        found = 1;
    }
    ASSERT_EQ_INT(1, found);
    shutdown_all(&platform);
}

/* Which nodes of a mesh moved between two poses. */
static int nodes_that_moved(const UnitMesh *m, const UnitNodeXform *a,
                            const UnitNodeXform *b) {
    int moved = 0;
    for (int i = 0; i < m->node_count; i++) {
        for (int k = 0; k < 9; k++) {
            if (a[i].rot[k] != b[i].rot[k]) { moved++; break; }
        }
    }
    return moved;
}

/* The walk cycle reaches the 3D pose: a unit ordered to march has piece
 * rotations that change from one moment to the next, composed over the
 * store's own bake of its model, which is what the 3D view draws. */
TEST(a_walking_units_pieces_move_in_the_3d_pose) {
    TAK_Platform platform;
    GameWorld *world = NULL;
    int rc = boot(&platform, &world);
    if (rc == 1) return;
    ASSERT_EQ_INT(0, rc);
    int n = 0;
    const Unit *units = Units_GetActive(&n);
    ASSERT(n > 0);
    const UnitDef *def = Units_GetDef(units[0].def_idx);
    ASSERT_NOT_NULL(def);
    UnitMesh *m = Units_BakeObjectMesh(def->objectname, units[0].team_color_idx);
    ASSERT_NOT_NULL(m);
    ASSERT_NOT_NULL(units[0].cob);

    /* At rest nothing moves between two moments. */
    static UnitNodeXform a[UNIT_MESH_MAX_NODES], b[UNIT_MESH_MAX_NODES];
    InGame_DebugRunSimTicks(30);
    Units_ComposeNodeXforms(m, units[0].cob->pieces, a, 1);
    InGame_DebugRunSimTicks(30);
    Units_ComposeNodeXforms(m, units[0].cob->pieces, b, 1);
    ASSERT_EQ_INT(0, nodes_that_moved(m, a, b));

    /* Marching, the legs and arms swing. */
    ASSERT_EQ_INT(0, Units_OrderMove(0, units[0].world_x + 600, units[0].world_y));
    InGame_DebugRunSimTicks(90);
    Units_ComposeNodeXforms(m, units[0].cob->pieces, a, 1);
    InGame_DebugRunSimTicks(7);
    Units_ComposeNodeXforms(m, units[0].cob->pieces, b, 1);
    ASSERT(nodes_that_moved(m, a, b) >= 2);
    Units_FreeBakedMesh(m);
    shutdown_all(&platform);
}

int main(int argc, char **argv) {
    (void)argc; (void)argv;
    TAK_Crash_Install();
    printf("test_view3d\n");
    TEST_ALLOW_SKIPS("no game data or no opengl renderer on this machine");
    RUN(toggling_3d_on_and_off_leaves_the_classic_frame_byte_identical);
    RUN(the_3d_pointer_lands_where_the_camera_looks);
    RUN(a_scroll_in_3d_moves_the_classic_camera_with_it);
    RUN(the_accessors_hand_out_the_baked_model_and_its_pose);
    RUN(a_walking_units_pieces_move_in_the_3d_pose);
    TEST_REPORT();
}
