#include "image.h"

#include "stb_image_write.h"
#include "stb_image.h"


void 
image_u32_init(ImageU32 *image, u32 width, u32 height)
{
    image->width = width;
    image->height = height;
    image->pixels = malloc(sizeof(u32) * width * height);
}

u32 *
image_u32_get_pixel_pointer(ImageU32 *image, u32 x, u32 y)
{
    assert((x < image->width) && (y < image->height));

    u32 stride = image->width;
    u32 *result = image->pixels + stride * y + x;
    return result;
}

void 
image_u32_save(ImageU32 *image, char *filename)
{
    stbi_flip_vertically_on_write(true);
    bool saved = stbi_write_png(filename, image->width, image->height, 4, image->pixels, sizeof(u32) * image->width);
    if (!saved)
    {
        fprintf(stderr, "[ERROR] Failed to save image '%s'\n", filename);
    }
}

void 
image_u32_load(ImageU32 *image, char *filename)
{
    image->pixels = (u32 *)stbi_load(filename, (int *)&image->width, (int *)&image->height, 0, 4);
    if (!image->pixels)
    {
        fprintf(stderr, "[ERROR] Failed to read image '%s'\n", filename);
    }
}