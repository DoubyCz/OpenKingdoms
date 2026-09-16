# The 3D view moves in a browser tab

Reported from play in the browser: nothing animates in the 3D view.
The engine side is `src/render/gl3d.c` and `src/platform/sdl2_platform.c`.

## What was measured

In a browser tab two 3D frames a second apart were byte identical, and
so were three frames over ten seconds, while the classic view in the
same tab moved every frame. The game's own frame counter reported 60
frames a second with the 3D view up.

A sky made to cycle red, green and blue every half second never
changed colour in a screenshot: the WebGL drawing buffer the scene was
drawn into is discarded when SDL presents. A one pixel SDL rectangle
moved every frame and was shown every frame, so SDL's own present
works with the 3D view up. The scene had to go through SDL as a
texture, and once it did the sky cycled.

The frames stayed identical after that, which sent the hunt through
the shader's uniform indexing, the depth buffer, the terrain build,
framebuffers of our own, the swap path, frame pacing and SDL's texture
cache, each ruled out by a measurement. No line printed from a per
frame path in this build reaches the browser console, while lines from
start up do, and that channel misled the hunt once. Counts drawn as
bars into the frame itself settled it: the view ran every frame, every
unit had a model, and one unit of fifteen was inside the camera's
frustum. The probe the browser runs were made with parks its workers
off screen and leaves the camera over a monarch standing still, while
the desktop run they were compared against used a probe that spreads
three hundred units across the map. A still monarch on still ground is
a still frame. On the crowd probe, the browser's 3D frames differ by
thousands of pixels with eleven units in view.

The moving scene was the wrong colour: brown ground, coloured units
and grey trees all came out one flat blue grey. The decoded chunk
bytes were printed from both loaders and matched, brown and in RGB
order, and the classic view of the same spot in the same tab was
brown. A terrain shader made to output its raw sample was still blue
grey, and every chunk uploaded as solid red came out (146, 64, 94):
red under a translucent blue. That is the water plane, a quad across
the whole map that the depth test is meant to hide wherever the ground
stands above sea level. SDL's framebuffer for a target texture has a
colour attachment and nothing else, so in the browser nothing in the
scene was depth tested and the water covered the play area, units and
all. The desktop draws to the window, which has a depth buffer.

## What the engine does now

In a browser tab the renderer is created able to take a render target.
The 3D view draws inside a target texture that SDL owns, the size of
the play rect, and SDL composites it where the play rect is, the same
way it composites the HUD canvas. Each frame a depth renderbuffer of
the target's size is attached to the framebuffer SDL bound, and
detached again if the framebuffer then reports incomplete. The context
is asked for a 24 bit depth buffer, and the model shader finds a
piece's row by walking the uniform array, since ES 1.00 only promises
a constant or loop index.

Measured after the change: sand reads (79, 71, 50) in the browser
against (85, 74, 52) on the desktop, ponds read water blue, and frames
five seconds apart differ by eight thousand play area pixels.

The desktop is unchanged: it draws to the window as before, and the
3D suite's byte identical classic frame gate holds.

## What the report was about

The 3D view draws terrain, sprite features, units, selection rings and
water. It draws no projectiles, explosions or spell effects on any
platform. Those are the animations missing from play. They are a pass
the view does not have yet, not a browser fault.
