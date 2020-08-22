#if !defined(RAY_H)

#include <time.h>

#include "ray/ray_tracer.h"
// #include "ray/scene_file.h"
#include "lib/sys.h"

// Single work order from multithreaded rendering system
typedef struct {
    // Lockless data
    Scene *scene;
    ImageU32 *image;
    // Region bounds
    u32 xmin;
    u32 ymin;
    u32 one_past_xmax;
    u32 one_past_ymax;
    // Seed should be unique per order
    RandomSeries random_series;
} RenderWorkOrder;

typedef struct {
    char *output_filename;
    u32   output_width;
    u32   output_height;
    
    char *input_scene;
    
    u32 rays_per_pixel;
    u32 max_bounce_count;
    
    bool open_image_after_done;
} RaySettings;

// Per-program work queue
typedef struct {
    // Settings
    u32 rays_per_pixel;
    u32 max_bounce_count;
    // Total work orders
    u32 work_order_count;
    RenderWorkOrder *work_orders;
    // Incremented each time order is 'consumed'
    volatile u64 next_work_order_index;
    // Tracker for bounces
    volatile u64 bounces_computed;
    // How many orders are finished
    volatile u64 tile_retired_count;
} RenderWorkQueue;


#define RAY_H 1
#endif
