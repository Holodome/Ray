#if !defined(RAY_H)

#include "general.h"
#include "ray_math.h"
#include "ray_random.h"
#include "ray_misc.h"
#include "image.h"
#include "trace.h"
#include "perlin.h"
#include "obj.h"

// 
// RenderWorkQueue
//

typedef struct {
    u32 x_min;
    u32 x_max;
    u32 y_min;
    u32 y_max;
    
    RandomSeries entropy;
} RenderWorkOrder;

typedef struct {
    Image *output;
    
    World *world;
    
    RenderWorkOrder *orders;
    u32 order_count;
    
    volatile u64 next_order_index;
    volatile u64 orders_done;
    
    volatile RayCastStatistics stats;
} RenderWorkQueue;

typedef struct {
    u32 image_w;
    u32 image_h;
    char *image_filename;
    bool open_image_after_done;
    u32 thread_count;
    u32 samples_per_pixel;
    u32 max_bounce_count;
    u32 tile_w;
    u32 tile_h;
} RaySettings;

#define RAY_H 1
#endif
