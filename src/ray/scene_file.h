#if !defined(SCENE_FILE_H)

#include "lib/common.h"
#include "lib/ray_math.h"

#define ASCII_PACK(a, b, c, d) ((a << 0) | (b << 8) | (c << 16) | (d << 24))
#define SCENE_FILE_MAGIC_NUMBER ASCII_PACK('h', 'r', 's', 'f')
#define SCENE_FILE_VERSION 0 

#pragma pack(push, 1)

typedef u8 SceneFileTextureType;
enum {
    SceneFileTexture_Solid,
    SceneFileTexture_Checkered,
    SceneFileTexture_Image
};

typedef struct {
    f32 scatter;
    f32 refraction_probability;
    Vec3 emit_color;
    SceneFileTextureType type;
    union {
        Vec3 solid;
        struct {
            Vec3 checkered1;
            Vec3 checkered2;  
        };
        // @TODO(hl): Remove this stupidity by either enumerating all images or baking images in scene file
        char image_file[128];
    };
} SceneFileMaterial;

typedef struct {
    Vec3 pos;
    f32  r;
    u64  mat_index;
} SceneFileSphere;

typedef struct {
    Vec3 normal;
    f32  d;
    u64  mat_index;
} SceneFilePlane;

typedef u8 SceneFileAARectType;
enum {
    SceneFileAARect_XY,
    SceneFileAARect_YZ,
    SceneFileAARect_XZ,
};

typedef struct {
    SceneFileAARectType type;
    f32 a0_0;
    f32 a0_1;
    f32 a1_0;
    f32 a1_1;
    f32 a2;
    u64 mat_index;
} SceneFileAARect;

typedef struct {
    Vec3 vertex0;
    Vec3 vertex1;
    Vec3 vertex2;
    u64 mat_index;    
} SceneFileTriangle;

typedef struct {
    u32 magic_number;
    u32 version;
    
    u32 _reserved[10];
    
    u64 material_count;
    u64 sphere_count;
    u64 plane_count;
    u64 aarect_count;
    u64 triangle_count;
    
    u64 _reserved1[10];
        
    u64 materials_loc;
    u64 spheres_loc;
    u64 planes_loc;
    u64 aarects_loc;
    u64 triangles_loc;
    
    u64 _reserved2[10];
} SceneFileHeader;

#pragma pack(pop)

#if 0 

void read_example(char *filename)
{
    FILE *file = fopen(filename, "rb");
    assert(file);
    SceneFileHeader header;
    fread(&header, sizeof(header), 1, file);
    
    assert(header.magic_number == SCENE_FILE_MAGIC_NUMBER);
    assert(header.version == SCENE_FILE_VERSION);
    
    SceneFileMaterial *materials = (SceneFileMaterial *)calloc(header.material_count, sizeof(SceneFileMaterial));
    fseek(file, header.materials_loc, SEEK_SET);
    fread(materials, sizeof(SceneFileMaterial), header.material_count, file);
    
    // ...
    // ...
    // ...
}

#endif 

#define SCENE_FILE_H 1
#endif
