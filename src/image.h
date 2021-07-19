#if !defined(IMAGE_H)

#include "general.h"

#pragma pack(push, 1)

typedef struct {
    u16 file_type;
    u32 file_size;
    u32 reserved;
    u32 bitmap_offset;
    u32 size;
    i32 width;
    i32 height;
    u16 planes;
    u16 bits_per_pixel;
    u32 compression;
    u32 size_of_bitmap;
    i32 horz_resolution;
    i32 vert_resolution;
    u32 colors_used;
    u32 colors_important;
} BMPHeader;

#pragma pack(pop)

typedef struct {
    u32 *p;
    u32 w;
    u32 h;
} Image;

static Image 
make_image_for_writing(u32 width, u32 height) {
    Image result;
    
    result.w = width;
    result.h = height;
    result.p = (u32 *)malloc(width * height * 4); 
    
    return result;
}

void 
image_save(Image *image, char *filename) {
    u32 output_pixel_size = image->w * image->h * 4;
    
    BMPHeader header = {0};
    header.file_type = 0x4D42;
    header.file_size = sizeof(header) + output_pixel_size;
    header.bitmap_offset = sizeof(header);
    header.size = sizeof(header) - 14;
    header.width = image->w;
    header.height = image->h;
    header.planes = 1;
    header.bits_per_pixel = 32;
    header.compression = 0;
    header.size_of_bitmap = 0;
    header.horz_resolution = 0;
    header.vert_resolution = 0;
    header.colors_used = 0;
    header.colors_important = 0;
    
    FILE *file = fopen(filename, "wb");
    if (file) {
        fwrite(&header, sizeof(header), 1, file);
        fwrite(image->p, output_pixel_size, 1, file);
        fclose(file);
    } else {
        fprintf(stderr, "[ERROR] Failed to open file '%s' for writing\n", filename);
    }
}

u32 *
image_get_pixel_pointer(Image *image, u32 x, u32 y) {
    u32 *result = image->p + y * image->w + x;
    return result;
}

// This is kinda stupid, but it works (whatever...)
static Image 
load_bmp(char *filename) {
    Image image = {0};
    
    FILE *file = fopen(filename, "rb");
    if (file) {
        BMPHeader header;
        fread(&header, sizeof(header), 1, file);
        
        assert(header.bits_per_pixel == 32 || header.bits_per_pixel == 24);
        assert(header.height >= 0);
        assert(header.compression == 0);
        
        image.w = header.width;
        image.h = header.height;
        image.p = malloc(image.w * image.h * sizeof(u32));
        
        u8 *pixels = malloc(image.w * image.h * header.bits_per_pixel);
        fseek(file, header.bitmap_offset, SEEK_SET);
        fread(pixels, 1, image.w * image.h * header.bits_per_pixel, file);
        for (u32 y = 0;
             y < image.h;
             ++y) {
            for (u32 x = 0;
                 x < image.w;
                 ++x) {
                u32 *dest = image.p + y * image.w + x;
                u8 *source = pixels + (header.bits_per_pixel >> 3) * (y * image.w + x);
                
                u32 r = source[2];
                u32 g = source[1];
                u32 b = source[0];
                u32 a = 0xFF;
                if (header.bits_per_pixel == 32) {
                    a = source[3];
                }
                
                *dest = rgba_pack_4x8(r, g, b, a);
            }        
        }
        
        free(pixels);
    } else {
        fprintf(stderr, "[ERROR] Failed to open file '%s' for reading\n", filename);
    }
    
    return image;
}


#define IMAGE_H 1
#endif
