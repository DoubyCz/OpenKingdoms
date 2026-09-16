/*
 * tak_model_store.h -- where the 3D view gets a model from.
 *
 * The view asks by object name and team colour and gets back a model
 * baked into GPU buffers with its named pieces, and never learns which
 * source it came from. Today the one source is the shipped 3DO through
 * the unit renderer's bake (model_store.c). A glTF source belongs
 * beside it as a second file that produces the same UnitMesh, which
 * is the contract docs/PRD_3D_MODE.md describes.
 */
#ifndef TAK_MODEL_STORE_H
#define TAK_MODEL_STORE_H

#include "tak_gl3d.h"
#include "tak_unit.h"

#define MODEL_STORE_MAX_BATCHES 96

typedef struct GpuModel {
    char        name[TAK_UNITDEF_OBJ_MAX];
    int         color_idx;
    /* The baked pieces, owned here: node names, parents, offsets and
     * the vertex to node map the piece transforms are composed over. */
    UnitMesh   *mesh;
    GL3D_Mesh  *gl;
    GL3D_ModelBatch batches[MODEL_STORE_MAX_BATCHES];
    int         batch_count;
    /* Bounds at rest in model units, for culling, and the sphere that
     * holds them in map pixels once the model scale is applied. */
    float       aabb_min[3], aabb_max[3];
    float       radius_px;
    float       height_px;
    float       foot_radius_px;   /* the footprint's reach on the ground */
} GpuModel;

/* The model for an object name in a team colour, baked on first use
 * and cached. NULL when there is no such model or GL is not up. */
const GpuModel *ModelStore_Get(const char *object_name, int color_idx);

/* Drop every cached model. Run with the GL context alive. */
void ModelStore_Clear(void);

/* How many models the store holds. */
int  ModelStore_Count(void);

#endif /* TAK_MODEL_STORE_H */
