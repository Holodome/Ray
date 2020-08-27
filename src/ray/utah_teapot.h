#if !defined(UTAH_TEAPOT_H)

#include "lib/common.h"
#include "lib/ray_math.h"

#define UTAH_TEAPOT_NUM_PATCHES  32
#define UTAH_TEAPOT_NUM_VERTICES 306

extern u32 utah_teapot_patches [UTAH_TEAPOT_NUM_PATCHES][16];
extern f32 utah_teapot_vertices[UTAH_TEAPOT_NUM_VERTICES][3];

Vec3 eval_bezier_curve(Vec3 p[static 4], f32 t);
Vec3 eval_bezier_patch(Vec3 control_p[static 16], f32 u, f32 v);
Vec3 deriv_bezier(Vec3 p[static 4], f32 t);
Vec3 deirv_u_bezier(Vec3 control_p[static 16], f32 u, f32 v);
Vec3 deriv_v_bezier(Vec3 control_p[static 16], f32 u, f32 v);

#define UTAH_TEAPOT_H 1
#endif
