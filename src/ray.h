#if !defined(RAY_H)

#include "general.h"
#include "ray_math.h"
#include "ray_random.h"
#include "ray_misc.h"
#include "image.h"
#include "trace.h"
#include "perlin.h"
#include "obj.h"

// Record of work order in queue.
// Comtains bounds of rectangle part of image that need to be done
// And some per-thread data
typedef struct {
    u32 x_min;
    u32 x_max;
    u32 y_min;
    u32 y_max;
    // Sometimes we need to make memory allocations during ray casting.
    // To avoid locking, we provide some memory for each working thread
    MemoryArena arena;
    RandomSeries entropy;
} RenderWorkOrder;

typedef struct {
    Image *output;
    World *world;
    // Some settings, they also could be global variables, but its cleaner to put them here
    u32 samples_per_pixel;
    u32 max_bounce_count;
    
    RenderWorkOrder *orders;
    u32 order_count;
    // Incremented as new work order is being picked by working thread
    volatile u64 next_order_index;
    // Incremented as order is done. If is equal to order count, queue has finished all jobs
    volatile u64 orders_done;
    
    volatile RayCastStatistics stats;
} RenderWorkQueue;

// Command-line settable settings
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

bool render_tile(RenderWorkQueue *queue);
void init_render_queue(RenderWorkQueue *queue, Image *image, World *world,
                       u32 tile_w, u32 tile_h, u32 samples_per_pixel, u32 max_bounce_count);

#define RAY_H 1
#endif
