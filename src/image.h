#if !defined(IMAGE_H)

#include "common.h"

//  Wrapper for 4-component image (little-endian RGBA), where each pixel is represented as u32
typedef struct {
    u32 width;
    u32 height;
    u32 *pixels;
} ImageU32;

// Allocates pixel buffer (creates image for writing)
void image_u32_init(ImageU32 *image, u32 width, u32 height);
// Saves image with given filename (actually it is always PNG)
void image_u32_save(ImageU32 *image, char *filename);
u32 *image_u32_get_pixel_pointer(ImageU32 *image, u32 x, u32 y);
// Loads image from filepath
void image_u32_load(ImageU32 *image, char *filename);

#define IMAGE_H 1
#endif
