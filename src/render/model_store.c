/*
 * model_store.c -- the 3DO source of the model store, see
 * tak_model_store.h.
 *
 * A baked UnitMesh keeps its vertices in node local space with a node
 * index per vertex, which is what a per piece transform on the GPU
 * needs. This turns one into a MODEL layout buffer with a flat normal
 * per triangle and splits each texture batch into runs whose node
 * range fits one draw's uniform budget.
 */

#include "tak_model_store.h"
#include "tak_gpu.h"
#include "tak_memory.h"
#include "tak_util.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

#define MODEL_STORE_CAP 512

static GpuModel *g_models[MODEL_STORE_CAP];
static int       g_model_count;

int ModelStore_Count(void) { return g_model_count; }

static void free_model(GpuModel *m) {
    if (!m) return;
    if (m->gl) GL3D_FreeMesh(m->gl);
    if (m->mesh) Units_FreeBakedMesh(m->mesh);
    tak_free(m);
}

void ModelStore_Clear(void) {
    for (int i = 0; i < g_model_count; i++) free_model(g_models[i]);
    g_model_count = 0;
}

/* Split one atlas batch into node ranges of at most `budget` nodes.
 * Vertices of a batch come in node order, so a range is contiguous. */
static int add_batches(GpuModel *m, const UnitMesh *src, int b, int budget) {
    const UnitMeshBatch *sb = &src->batches[b];
    SDL_Texture *sdl_tex = GPU_TextureSDL(sb->atlas_tex);
    int first = sb->first_index;
    int end = sb->first_index + sb->index_count;
    while (first < end) {
        int lo = src->vert_node_idx[src->indices[first]];
        int hi = lo;
        int i = first;
        for (; i + 2 < end; i += 3) {
            int n = src->vert_node_idx[src->indices[i]];
            if (n < lo) lo = n;
            if (n - lo >= budget) break;
            if (n > hi) hi = n;
        }
        if (m->batch_count >= MODEL_STORE_MAX_BATCHES) return -1;
        GL3D_ModelBatch *out = &m->batches[m->batch_count++];
        out->sdl_tex = sdl_tex;
        out->tex = NULL;
        out->first_index = first;
        out->index_count = i - first;
        out->node_lo = lo;
        out->node_hi = hi;
        first = i;
    }
    return 0;
}

static GpuModel *build(const char *name, int color_idx) {
    UnitMesh *src = Units_BakeObjectMesh(name, color_idx);
    if (!src || src->vert_count <= 0 || src->tri_count <= 0) {
        if (src) Units_FreeBakedMesh(src);
        return NULL;
    }
    GpuModel *m = (GpuModel *)tak_malloc(sizeof(GpuModel));
    if (!m) { Units_FreeBakedMesh(src); return NULL; }
    memset(m, 0, sizeof(*m));
    snprintf(m->name, sizeof(m->name), "%s", name);
    m->color_idx = color_idx;
    m->mesh = src;

    const int V = src->vert_count;
    const int floats = GL3D_LayoutFloats(GL3D_LAYOUT_MODEL);
    float *verts = (float *)tak_malloc(sizeof(float) * (size_t)floats * (size_t)V);
    float *normals = (float *)tak_malloc(sizeof(float) * 3 * (size_t)V);
    if (!verts || !normals) {
        if (verts) tak_free(verts);
        if (normals) tak_free(normals);
        free_model(m);
        return NULL;
    }
    memset(normals, 0, sizeof(float) * 3 * (size_t)V);

    /* Flat normals: every triangle's face normal summed onto its
     * vertices, which a fan of one primitive shares. */
    for (int t = 0; t < src->tri_count; t++) {
        int i0 = src->indices[3 * t], i1 = src->indices[3 * t + 1], i2 = src->indices[3 * t + 2];
        const float *p0 = &src->positions[3 * i0];
        const float *p1 = &src->positions[3 * i1];
        const float *p2 = &src->positions[3 * i2];
        float ex = p1[0] - p0[0], ey = p1[1] - p0[1], ez = p1[2] - p0[2];
        float fx = p2[0] - p0[0], fy = p2[1] - p0[1], fz = p2[2] - p0[2];
        float nx = ey * fz - ez * fy, ny = ez * fx - ex * fz, nz = ex * fy - ey * fx;
        float len = sqrtf(nx * nx + ny * ny + nz * nz);
        if (len <= 0.0f) continue;
        nx /= len; ny /= len; nz /= len;
        int idx[3] = { i0, i1, i2 };
        for (int k = 0; k < 3; k++) {
            normals[3 * idx[k] + 0] += nx;
            normals[3 * idx[k] + 1] += ny;
            normals[3 * idx[k] + 2] += nz;
        }
    }

    /* Which vertices belong to a textured batch: those keep the texture
     * and get a white tint, the rest carry their palette colour. */
    for (int b = 0; b < src->batch_count; b++) {
        const UnitMeshBatch *sb = &src->batches[b];
        int textured = sb->atlas_tex != NULL;
        for (int i = sb->first_index; i < sb->first_index + sb->index_count; i++) {
            int v = src->indices[i];
            float *o = verts + (size_t)v * floats;
            o[0] = src->positions[3 * v + 0];
            o[1] = src->positions[3 * v + 1];
            o[2] = src->positions[3 * v + 2];
            float nx = normals[3 * v], ny = normals[3 * v + 1], nz = normals[3 * v + 2];
            float len = sqrtf(nx * nx + ny * ny + nz * nz);
            if (len > 0.0f) { nx /= len; ny /= len; nz /= len; } else { ny = 1.0f; }
            o[3] = nx; o[4] = ny; o[5] = nz;
            o[6] = src->uvs[2 * v + 0];
            o[7] = src->uvs[2 * v + 1];
            uint32_t c = src->colors[v];
            if (textured) {
                o[8] = o[9] = o[10] = 1.0f;
                o[11] = (float)((c >> 24) & 0xFF) / 255.0f;
            } else {
                o[8]  = (float)(c & 0xFF) / 255.0f;
                o[9]  = (float)((c >> 8) & 0xFF) / 255.0f;
                o[10] = (float)((c >> 16) & 0xFF) / 255.0f;
                o[11] = (float)((c >> 24) & 0xFF) / 255.0f;
            }
            o[12] = (float)src->vert_node_idx[v];
        }
    }
    tak_free(normals);

    m->gl = GL3D_UploadMesh(GL3D_LAYOUT_MODEL, verts, V, src->indices, src->tri_count * 3);
    tak_free(verts);
    if (!m->gl) { free_model(m); return NULL; }

    int budget = GL3D_MaxNodesPerDraw();
    for (int b = 0; b < src->batch_count; b++) {
        if (add_batches(m, src, b, budget) != 0) {
            fprintf(stderr, "ModelStore: %s has too many draw runs\n", name);
            break;
        }
    }

    memcpy(m->aabb_min, src->aabb_min, sizeof(m->aabb_min));
    memcpy(m->aabb_max, src->aabb_max, sizeof(m->aabb_max));
    float ta = Units_GetTAScale();
    float r = 0.0f;
    for (int k = 0; k < 3; k++) {
        float a = fabsf(src->aabb_min[k]), bb = fabsf(src->aabb_max[k]);
        if (a > r) r = a;
        if (bb > r) r = bb;
    }
    m->radius_px = r * ta * 1.75f;
    m->height_px = src->aabb_max[1] * ta;
    float fr = 0.0f;
    const int axes[2] = { 0, 2 };
    for (int k = 0; k < 2; k++) {
        float a = fabsf(src->aabb_min[axes[k]]), bb = fabsf(src->aabb_max[axes[k]]);
        if (a > fr) fr = a;
        if (bb > fr) fr = bb;
    }
    m->foot_radius_px = fr * ta;
    return m;
}

const GpuModel *ModelStore_Get(const char *object_name, int color_idx) {
    if (!object_name || !object_name[0] || !GL3D_Available()) return NULL;
    if (color_idx < 0 || color_idx > 11) color_idx = 0;
    for (int i = 0; i < g_model_count; i++) {
        if (g_models[i]->color_idx == color_idx &&
            tak_stricmp(g_models[i]->name, object_name) == 0)
            return g_models[i];
    }
    if (g_model_count >= MODEL_STORE_CAP) return NULL;
    GpuModel *m = build(object_name, color_idx);
    if (!m) return NULL;
    g_models[g_model_count++] = m;
    return m;
}
