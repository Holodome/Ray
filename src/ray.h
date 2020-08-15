#if !defined(RAY_H)

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "ray_tracer.h"
#include "sys.h"

//  Wrapper for 4-component image (little-endian RGBA), where each pixel is represented as u32
typedef struct
{
    u32 width;
    u32 height;
    u32 *pixels;
} ImageU32;

// Allocates pixel buffer
void image_u32_init(ImageU32 *image, u32 width, u32 height);
// Saves image with given filename (actually it is always PNG)
void image_u32_save(ImageU32 *image, char *filename);
u32 *image_u32_get_pixel_pointer(ImageU32 *image, u32 x, u32 y);

// Single work order from multithreaded rendering system
typedef struct
{
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

// Per-program work queue
typedef struct
{
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
