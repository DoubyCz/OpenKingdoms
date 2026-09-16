/*
 * tak_camera3d.h -- the free camera of the 3D view, as arithmetic.
 *
 * No SDL and no GL in here, so the browser build runs the same check
 * the desktop does (src/render/test_camera3d.c). World space is the
 * map's: x east in map pixels, z south in map pixels, y up in the same
 * pixels the terrain heights and the unit heights use. Matrices are
 * column major, sixteen floats, the layout GL takes as is.
 */
#ifndef TAK_CAMERA3D_H
#define TAK_CAMERA3D_H

#include <stdint.h>

typedef struct Camera3D {
    float target_x, target_y, target_z;  /* the ground point orbited   */
    float yaw;      /* radians, 0 looks north from the south           */
    float pitch;    /* radians above the ground plane, positive is down */
    float dist;     /* eye to target, map pixels                       */
    float fov_y;    /* vertical field of view, radians                 */
    float aspect;   /* viewport width over height                      */
    float near_z, far_z;
} Camera3D;

/* Height of the ground at a world point, for the clamps and the ray. */
typedef float (*Camera3D_HeightFn)(void *ctx, float x, float z);

/* The classic angle over a ground point: north up, tilted so a unit's
 * height draws at half its size the way the classic view draws it
 * (legacy:197689), and far enough back that the ground across the
 * target spans the play area's width. */
void Camera3D_ClassicPreset(Camera3D *cam, float target_x, float target_z,
                            int viewport_w, int viewport_h);

/* Where the eye is for the camera's target, yaw, pitch and distance. */
void Camera3D_Eye(const Camera3D *cam, float out[3]);

/* Column major view, projection and their product. */
void Camera3D_ViewMatrix(const Camera3D *cam, float out[16]);
void Camera3D_ProjMatrix(const Camera3D *cam, float out[16]);
void Camera3D_ViewProj(const Camera3D *cam, float out[16]);

/* The ray under a viewport pixel: origin at the eye, unit direction.
 * px and py are pixels inside a viewport of the given size, y down. */
void Camera3D_PointerRay(const Camera3D *cam, int vp_w, int vp_h,
                         float px, float py, float origin[3], float dir[3]);

/* Where a ray meets the ground, marching from the origin and settling
 * the crossing to under a tenth of a pixel. Returns 1 with the hit in
 * out_x and out_z, 0 when the ray never comes down inside max_dist. */
int  Camera3D_RayHitGround(const float origin[3], const float dir[3],
                           float max_dist, Camera3D_HeightFn height,
                           void *ctx, float *out_x, float *out_z);

/* Keep the target on the map, the tilt and distance in range, and the
 * eye above the ground under it. */
void Camera3D_Clamp(Camera3D *cam, float map_w, float map_h,
                    Camera3D_HeightFn height, void *ctx);

/* Move the target along the camera's own right and forward directions
 * projected on the ground, so a pan feels the same at any yaw. */
void Camera3D_Pan(Camera3D *cam, float right_px, float forward_px);

/* Rotate and tilt by radians, zoom by a factor on the distance. */
void Camera3D_Orbit(Camera3D *cam, float dyaw, float dpitch);
void Camera3D_Zoom(Camera3D *cam, float factor);

/* The six planes of a view projection, each a, b, c, d with the unit
 * normal pointing inward, and whether a sphere touches the inside. */
void Camera3D_FrustumPlanes(const float viewproj[16], float planes[6][4]);
int  Camera3D_SphereInFrustum(const float planes[6][4], const float centre[3],
                              float radius);

/* Matrix helpers shared with the renderer. */
void Mat4_Identity(float m[16]);
void Mat4_Multiply(const float a[16], const float b[16], float out[16]);
void Mat4_TransformPoint(const float m[16], const float p[3], float out[4]);

#endif /* TAK_CAMERA3D_H */
