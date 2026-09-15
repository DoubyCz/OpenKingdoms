/*
 * sdl2_platform.c -- SDL2 platform layer.
 *
 * Owns the window, the SDL_Renderer, and the canvas streaming texture.
 * The rest of the game composites into a fixed-size SDL_Surface (the UI
 * canvas, owned by render/ui.c) which we upload every frame and scale
 * into the window with aspect-preserving letterbox.
 */

#include "tak_platform.h"
#include "tak_click_map.h"
#include <SDL.h>
#include <stdio.h>
#include <string.h>

TAK_DisplayConfig TAK_DisplayConfig_Default(void) {
    TAK_DisplayConfig c;
    c.window_w        = 1280;
    c.window_h        = 720;
    c.canvas_w        = 640;
    c.canvas_h        = 480;
    c.fullscreen      = 0;
    c.vsync           = 1;
    /* Off by default: users expect content to grow continuously as they
     * drag the window edge. Pixel-perfect mode locks to integer steps,
     * which leaves large black bars at "in between" window sizes (e.g.
     * 1280×720 shows the canvas at 1× because 2× would overflow the
     * 720px height). Pass --pixel-perfect to opt in. */
    c.pixel_perfect   = 0;
    c.use_sw_renderer = 0;
    return c;
}

/* Recompute scale + letterbox offsets from current window/canvas sizes.
 * Pixel-perfect mode picks the largest integer scale that fits; if no
 * integer scale fits (window smaller than canvas on either axis) or the
 * user disabled it, fall back to a fractional fit. */
static void recompute_layout(TAK_Platform *plat) {
    if (plat->canvas_w <= 0 || plat->canvas_h <= 0) return;

    float sx = (float)plat->window_w / (float)plat->canvas_w;
    float sy = (float)plat->window_h / (float)plat->canvas_h;
    float fit = sx < sy ? sx : sy;
    if (fit <= 0.f) fit = 1.f;

    float scale = fit;
    if (plat->pixel_perfect) {
        int isc = (int)fit;
        if (isc >= 1) scale = (float)isc;
    }

    int scaled_w = (int)(plat->canvas_w * scale + 0.5f);
    int scaled_h = (int)(plat->canvas_h * scale + 0.5f);

    plat->scale    = scale;
    plat->offset_x = (plat->window_w - scaled_w) / 2;
    plat->offset_y = (plat->window_h - scaled_h) / 2;
}

/* Counted, never zero, and never reused inside a run, so a texture
 * cache can tell one renderer from the next even when the allocator
 * hands out the same address twice. */
uint32_t TAK_Platform_NewRendererGen(void) {
    static uint32_t next = 1;
    return next++;
}

int TAK_Platform_Init(TAK_Platform *plat, const TAK_DisplayConfig *cfg) {
    if (!plat || !cfg) {
        fprintf(stderr, "TAK_Platform_Init: null args\n");
        return -1;
    }
    memset(plat, 0, sizeof(*plat));

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) {
        SDL_Log("SDL_Init: %s", SDL_GetError());
        return -1;
    }

    Uint32 win_flags = SDL_WINDOW_RESIZABLE;
    if (cfg->fullscreen) win_flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;

    plat->window = SDL_CreateWindow("Total Annihilation: Kingdoms",
                                     SDL_WINDOWPOS_CENTERED,
                                     SDL_WINDOWPOS_CENTERED,
                                     cfg->window_w, cfg->window_h,
                                     win_flags);
    if (!plat->window) {
        fprintf(stderr, "SDL_CreateWindow failed: %s\n", SDL_GetError());
        SDL_Quit();
        return -1;
    }

    Uint32 rend_flags = cfg->use_sw_renderer
        ? SDL_RENDERER_SOFTWARE
        : SDL_RENDERER_ACCELERATED;
    if (cfg->vsync) rend_flags |= SDL_RENDERER_PRESENTVSYNC;

    plat->renderer = SDL_CreateRenderer(plat->window, -1, rend_flags);
    if (!plat->renderer) {
        /* Retry without vsync, then without hardware, before giving up. */
        rend_flags &= ~SDL_RENDERER_PRESENTVSYNC;
        plat->renderer = SDL_CreateRenderer(plat->window, -1, rend_flags);
    }
    if (!plat->renderer) {
        plat->renderer = SDL_CreateRenderer(plat->window, -1,
                                             SDL_RENDERER_SOFTWARE);
    }
    if (!plat->renderer) {
        fprintf(stderr, "SDL_CreateRenderer failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(plat->window);
        SDL_Quit();
        return -1;
    }
    plat->renderer_gen = TAK_Platform_NewRendererGen();

    /* Log the backend we ended up with so it's visible in runs. */
    SDL_RendererInfo info;
    if (SDL_GetRendererInfo(plat->renderer, &info) == 0) {
        fprintf(stderr, "Renderer: %s (flags=0x%08x)\n",
                info.name ? info.name : "?", info.flags);
    }

    plat->canvas_tex = SDL_CreateTexture(plat->renderer,
                                          SDL_PIXELFORMAT_RGBA32,
                                          SDL_TEXTUREACCESS_STREAMING,
                                          cfg->canvas_w, cfg->canvas_h);
    if (!plat->canvas_tex) {
        fprintf(stderr, "SDL_CreateTexture(canvas) failed: %s\n", SDL_GetError());
        SDL_DestroyRenderer(plat->renderer);
        SDL_DestroyWindow(plat->window);
        SDL_Quit();
        return -1;
    }
    /* Linear filtering by default so the canvas looks clean at
     * fractional scales; switch to nearest when pixel_perfect is on and
     * the current scale is an exact integer (see recompute_layout). */
    SDL_SetTextureScaleMode(plat->canvas_tex,
        cfg->pixel_perfect ? SDL_ScaleModeNearest : SDL_ScaleModeLinear);
    /* Alpha-compose the canvas on top of whatever the game drew to the
     * window this frame (e.g. terrain). UI screens that want an opaque
     * backdrop fill their canvas with alpha=255; screens that want
     * underlying draws to show through (in-game) fill with alpha=0. */
    SDL_SetTextureBlendMode(plat->canvas_tex, SDL_BLENDMODE_BLEND);

    plat->window_w        = cfg->window_w;
    plat->window_h        = cfg->window_h;
    plat->canvas_w        = cfg->canvas_w;
    plat->canvas_h        = cfg->canvas_h;
    plat->fullscreen      = cfg->fullscreen;
    plat->has_focus       = 1;
    plat->use_sw_renderer = cfg->use_sw_renderer;
    plat->pixel_perfect   = cfg->pixel_perfect;
    plat->vsync           = cfg->vsync;

    /* Pull the real window size — fullscreen-desktop or HiDPI may have
     * given us something other than what we requested. */
    SDL_GetWindowSize(plat->window, &plat->window_w, &plat->window_h);
    recompute_layout(plat);

    /* SDL turns text input on with the window on some platforms. Start
     * it off, so a screen that wants typed characters asks for them and
     * nothing else collects any. */
    SDL_StopTextInput();
    plat->text_in[0] = '\0';
    plat->text_in_len = 0;

    return 0;
}

void TAK_Platform_FrameBegin(TAK_Platform *plat) {
    if (!plat || !plat->renderer) return;

    SDL_SetRenderDrawColor(plat->renderer, 0, 0, 0, 255);
    SDL_RenderClear(plat->renderer);
}

void TAK_Platform_Shutdown(TAK_Platform *plat) {
    if (!plat) return;
    if (plat->canvas_tex) SDL_DestroyTexture(plat->canvas_tex);
    if (plat->renderer)   SDL_DestroyRenderer(plat->renderer);
    if (plat->window)     SDL_DestroyWindow(plat->window);
    memset(plat, 0, sizeof(*plat));
    SDL_Quit();
}

int TAK_Platform_PumpEvents(TAK_Platform *plat) {
    if (!plat) return 0;
    /* A frame's typing starts empty. Whatever no screen reads is gone
     * by the next pump, which is what keeps a key held through a mode
     * change from arriving somewhere it does not belong. */
    plat->text_in[0] = '\0';
    plat->text_in_len = 0;
    plat->pressed_enter = 0;
    plat->pressed_escape = 0;
    plat->pressed_backspace = 0;
    plat->pressed_mouse_left = 0;
    SDL_Event ev;
    while (SDL_PollEvent(&ev)) {
        switch (ev.type) {
        case SDL_QUIT:
            return 0;

        case SDL_TEXTINPUT:
            /* SDL only sends these between SDL_StartTextInput and
             * SDL_StopTextInput, so nothing is collected while the chat
             * console is shut. */
            for (const char *p = ev.text.text; *p; p++) {
                unsigned char c = (unsigned char)*p;
                if (c < 0x20 || c > 0x7e) continue;
                if (plat->text_in_len >= (int)sizeof(plat->text_in) - 1) break;
                plat->text_in[plat->text_in_len++] = (char)c;
            }
            plat->text_in[plat->text_in_len] = '\0';
            break;

        case SDL_KEYDOWN:
            /* Alt+Enter toggles fullscreen. Escape is handled per-screen. */
            if (ev.key.keysym.sym == SDLK_RETURN &&
                (ev.key.keysym.mod & KMOD_ALT)) {
                TAK_Platform_ToggleFullscreen(plat);
                break;
            }
            if (ev.key.repeat) break;
            switch (ev.key.keysym.scancode) {
            case SDL_SCANCODE_RETURN:
            case SDL_SCANCODE_KP_ENTER:  plat->pressed_enter = 1; break;
            case SDL_SCANCODE_ESCAPE:    plat->pressed_escape = 1; break;
            case SDL_SCANCODE_BACKSPACE: plat->pressed_backspace = 1; break;
            default: break;
            }
            break;

        case SDL_MOUSEBUTTONDOWN:
            if (ev.button.button == SDL_BUTTON_LEFT) {
                plat->pressed_mouse_left = 1;
                plat->press_x = ev.button.x;
                plat->press_y = ev.button.y;
            }
            break;

        case SDL_WINDOWEVENT:
            switch (ev.window.event) {
            case SDL_WINDOWEVENT_FOCUS_GAINED: plat->has_focus = 1; break;
            case SDL_WINDOWEVENT_FOCUS_LOST:   plat->has_focus = 0; break;
            case SDL_WINDOWEVENT_SIZE_CHANGED:
            case SDL_WINDOWEVENT_RESIZED:
                plat->window_w = ev.window.data1;
                plat->window_h = ev.window.data2;
                recompute_layout(plat);
                break;
            }
            break;
        }
    }
    return 1;
}

void TAK_Platform_ToggleFullscreen(TAK_Platform *plat) {
    if (!plat || !plat->window) return;
    plat->fullscreen = !plat->fullscreen;
    SDL_SetWindowFullscreen(plat->window,
        plat->fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
    SDL_GetWindowSize(plat->window, &plat->window_w, &plat->window_h);
    recompute_layout(plat);
}

void TAK_Platform_UpdateCanvas(TAK_Platform *plat, SDL_Surface *canvas) {
    if (!plat || !plat->canvas_tex || !canvas) return;
    if (canvas->w != plat->canvas_w || canvas->h != plat->canvas_h) {
        /* Defensive — something drifted. The UI canvas is expected to
         * stay a fixed size; if it doesn't, recreate the texture rather
         * than crash in SDL_UpdateTexture. */
        SDL_DestroyTexture(plat->canvas_tex);
        plat->canvas_tex = SDL_CreateTexture(plat->renderer,
                                              SDL_PIXELFORMAT_RGBA32,
                                              SDL_TEXTUREACCESS_STREAMING,
                                              canvas->w, canvas->h);
        if (!plat->canvas_tex) return;
        SDL_SetTextureScaleMode(plat->canvas_tex, SDL_ScaleModeNearest);
        SDL_SetTextureBlendMode(plat->canvas_tex, SDL_BLENDMODE_BLEND);
        plat->canvas_w = canvas->w;
        plat->canvas_h = canvas->h;
        recompute_layout(plat);
    }
    SDL_LockSurface(canvas);
    SDL_UpdateTexture(plat->canvas_tex, NULL, canvas->pixels, canvas->pitch);
    SDL_UnlockSurface(canvas);
    /* If the user asked for pixel-perfect but we ended up on a
     * fractional scale (window doesn't fit an integer multiple), fall
     * back to linear filtering for the non-integer portion. At exact
     * integer scales nearest is strictly better. */
    if (plat->pixel_perfect) {
        float s = plat->scale;
        int is_integer = (s >= 1.0f && s == (float)(int)s);
        SDL_SetTextureScaleMode(plat->canvas_tex,
            is_integer ? SDL_ScaleModeNearest : SDL_ScaleModeLinear);
    }
}

void TAK_Platform_Present(TAK_Platform *plat) {
    if (!plat || !plat->renderer) return;
    if (plat->canvas_tex) {
        /* The UI canvas (640×480 native, hosting menu + HUD) is
         * stretched to fill the entire window so the in-game HUD
         * scales with the window. Letterboxing was causing the HUD
         * to render at native size with black bars at high-DPI
         * resolutions — not what we want for a modern remake.
         * Linear filtering keeps the upscale readable. */
        SDL_RenderCopy(plat->renderer, plat->canvas_tex, NULL, NULL);
    }
    SDL_RenderPresent(plat->renderer);
}

int TAK_Platform_MouseThisFrame(const TAK_Platform *plat, int *out_cx, int *out_cy) {
    int wx = 0, wy = 0;
    uint32_t held = SDL_GetMouseState(&wx, &wy) & SDL_BUTTON(SDL_BUTTON_LEFT);
    int down = held ? 1 : 0;
    if (plat && plat->pressed_mouse_left && !held) {
        /* Down and up inside one frame: the press is the event, and
         * where it landed is the position that matters. */
        down = 1;
        wx = plat->press_x;
        wy = plat->press_y;
    }
    int cx = -1, cy = -1;
    if (!TAK_Platform_MapMouseToCanvas(plat, wx, wy, &cx, &cy)) { cx = -1; cy = -1; }
    if (out_cx) *out_cx = cx;
    if (out_cy) *out_cy = cy;
    return down;
}

int TAK_Platform_MapMouseToCanvas(const TAK_Platform *plat,
                                   int wx, int wy,
                                   int *out_cx, int *out_cy) {
    if (!plat) return 0;
    /* The canvas is stretched over the whole window, no letterbox. The
     * arithmetic lives in click_map.c so the browser build checks it. */
    return ClickMap_WindowToCanvas(plat->window_w, plat->window_h,
                                   plat->canvas_w, plat->canvas_h,
                                   wx, wy, out_cx, out_cy);
}

SDL_Rect TAK_Platform_CanvasRectToWindow(const TAK_Platform *plat,
                                          SDL_Rect r) {
    SDL_Rect o = r;
    if (!plat) return o;
    /* Same transform TAK_Platform_Present uses for the canvas texture,
     * inverted from TAK_Platform_MapMouseToCanvas. */
    int out[4];
    ClickMap_CanvasRectToWindow(plat->window_w, plat->window_h,
                                plat->canvas_w, plat->canvas_h,
                                r.x, r.y, r.w, r.h, out);
    o.x = out[0]; o.y = out[1]; o.w = out[2]; o.h = out[3];
    return o;
}
