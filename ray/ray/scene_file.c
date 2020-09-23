#include "ray/scene_file.h"


void 
scene_write_to_scene_file(Scene *scene, char *filename)
{
    FILE *file = fopen(filename, "wb");
    assert(file);
    
    u64 material_count = scene->material_count;
    u64 sphere_count = scene->sphere_count;
    u64 plane_count = scene->plane_count;
    u64 rect_count = scene->rect_count;
    u64 triangle_count = scene->triangle_count;
    
    SceneFileHeader header = {};
    header.magic_number = SCENE_FILE_MAGIC_NUMBER;
    header.version = SCENE_FILE_VERSION;
    header.material_count = material_count;
    header.sphere_count = sphere_count;
    header.plane_count = plane_count;
    header.aarect_count = rect_count;
    header.triangle_count = triangle_count;
    
    SceneFileCamera camera;
    camera.pos = scene->camera.camera_pos;
    header.camera = camera;
    
    u64 materials_size = sizeof(SceneFileMaterial) * material_count;
    u64 spheres_size = sizeof(SceneFileSphere) * sphere_count;
    u64 planes_size = sizeof(SceneFilePlane) * plane_count;
    u64 aarects_size = sizeof(SceneFileAARect) * rect_count;
    u64 triangles_size = sizeof(SceneFileTriangle) * triangle_count;
    
    u64 materials_loc = sizeof(header);
    u64 spheres_loc = materials_loc + materials_size;
    u64 planes_loc = spheres_loc + spheres_size;
    u64 aarects_loc = planes_loc + aarects_size;
    u64 triangles_loc = aarects_loc + aarects_size;
        
    header.materials_loc = materials_loc;
    header.spheres_loc = spheres_loc;
    header.planes_loc = planes_loc;
    header.aarects_loc = aarects_loc;
    header.triangles_loc = triangles_loc;    
    
    fwrite(&header, sizeof(header), 1, file);
    
    u64 temp_buffer_size = max(materials_size, max(spheres_size, max(planes_size, max(aarects_size, triangles_size))));
    void *temp_buffer = malloc(temp_buffer_size);
    
    SceneFileMaterial *file_materials = (SceneFileMaterial *)temp_buffer;
    for (u32 material_index = 0;
         material_index < material_count;
         ++material_index)
    {
        SceneFileMaterial *dest = file_materials + material_index;
        Material *source = scene->materials + material_index;
        
        dest->scatter = source->scatter;
        dest->refraction_probability = source->refraction_probability;
        dest->emit_color = source->emit_color;
        switch(source->texture.type)
        {
            case Texture_Solid:
            {
                dest->type = SceneFileTexture_Solid;
                dest->solid = source->texture.solid_color;
            } break;
            case Texture_Checkered:
            {
                dest->type = SceneFileTexture_Checkered;
                dest->checkered1 = source->texture.checkered1;
                dest->checkered2 = source->texture.checkered2;
            } break;
            case Texture_Image:
            {
                dest->type = SceneFileTexture_Image;
                format_string(dest->image_file, sizeof(dest->image_file), "%s", source->texture.filename);
            } break;     
        }
    }
    fwrite(file_materials, materials_size, 1, file);

    SceneFileSphere *file_spheres = temp_buffer;
     for (u32 sphere_index = 0;
         sphere_index < sphere_count;
         ++sphere_index)
    {
        SceneFileSphere *dest = file_spheres + sphere_index;
        Sphere *source = scene->spheres + sphere_index;
        
        dest->pos = source->pos;
        dest->r = source->radius;
        dest->mat_index = source->mat_index;
    }
    fwrite(file_spheres, spheres_size, 1, file);
    
    SceneFilePlane *file_planes = temp_buffer;
     for (u32 plane_index = 0;
         plane_index < plane_count;
         ++plane_index)
    {
        SceneFilePlane *dest = file_planes + plane_index;
        Plane *source = scene->planes + plane_index;
        
        dest->normal = source->normal;
        dest->d = source->dist;
        dest->mat_index = source->mat_index;
    }
    fwrite(file_planes, planes_size, 1, file);
    
    SceneFileAARect *file_rects = temp_buffer;
     for (u32 rect_index = 0;
         rect_index < rect_count;
         ++rect_index)
    {
        SceneFileAARect *dest = file_rects + rect_index;
        Rect *source = scene->rects + rect_index;
        
        SceneFileAARectType type;
        switch(source->type)
        {
            case RectType_XY:
            {
                type = SceneFileAARect_XY;
            } break;
            case RectType_YZ:
            {
                type = SceneFileAARect_YZ;
            } break;
            case RectType_XZ:
            {
                type = SceneFileAARect_XZ;
            } break;
        }
        dest->type = type;
        dest->a0_0 = source->xy.x0;
        dest->a0_1 = source->xy.x1;
        dest->a1_0 = source->xy.y0;
        dest->a1_1 = source->xy.y1;
        dest->a2 = source->xy.k;
        dest->mat_index = source->xy.mat_index;
    }
    fwrite(file_rects, aarects_size, 1, file);
    
    SceneFileTriangle *file_triangles = temp_buffer;
     for (u32 triangle_index = 0;
         triangle_index < triangle_count;
         ++triangle_index)
    {
        SceneFileTriangle *dest = file_triangles + triangle_index;
        Triangle *source = scene->triangles + triangle_index;
        
        dest->vertex0 = source->vertex0;
        dest->vertex1 = source->vertex1;
        dest->vertex2 = source->vertex2;
        dest->mat_index = source->mat_index;
    }
    fwrite(file_triangles, triangles_size, 1, file);
    
    free(temp_buffer);
    fclose(file);
}


void 
scene_init_from_file(Scene *scene, ImageU32 *image, char *filename)
{
    FILE *file = fopen(filename, "rb");
    assert(file);
    SceneFileHeader header;
    fread(&header, sizeof(header), 1, file);
    
    assert(header.magic_number == SCENE_FILE_MAGIC_NUMBER);
    assert(header.version == SCENE_FILE_VERSION);
       
    u64 material_count = header.material_count;
    u64 sphere_count = header.sphere_count;
    u64 plane_count = header.plane_count;
    u64 rect_count = header.aarect_count;
    u64 triangle_count = header.triangle_count;
    
    scene->material_count = material_count;
    scene->sphere_count = sphere_count;
    scene->plane_count = plane_count;
    scene->rect_count = rect_count;
    scene->triangle_count = triangle_count;
       
    u64 materials_size = sizeof(SceneFileMaterial) * material_count;
    u64 spheres_size = sizeof(SceneFileSphere) * sphere_count;
    u64 planes_size = sizeof(SceneFilePlane) * plane_count;
    u64 aarects_size = sizeof(SceneFileAARect) * rect_count;
    u64 triangles_size = sizeof(SceneFileTriangle) * triangle_count;
    u64 temp_buffer_size = max(materials_size, max(spheres_size, max(planes_size, max(aarects_size, triangles_size))));
    void *temp_buffer = malloc(temp_buffer_size);
    
    fseek(file, header.materials_loc, SEEK_SET);
    
    SceneFileMaterial *materials = temp_buffer;
    fread(materials, materials_size, 1, file);    
    scene->materials = calloc(material_count, sizeof(Material));
    for (u32 material_index = 0;
         material_index < material_count;
         ++material_index)
    {
        SceneFileMaterial *source = materials + material_index;
        Material *dest = scene->materials + material_index;
        
        dest->scatter = source->scatter;
        dest->refraction_probability = source->refraction_probability;
        dest->emit_color = source->emit_color;
        switch(source->type)
        {
            case SceneFileTexture_Solid:
            {
                dest->texture = texture_solid_color(source->solid);
            } break;
            case SceneFileTexture_Checkered:
            {
                dest->texture = texture_checkered(source->checkered1, source->checkered2);
            } break;
            case SceneFileTexture_Image:
            {
                dest->texture = texture_image(source->image_file);
            } break;
        }
    }

    SceneFileSphere *file_spheres = temp_buffer;
    fread(file_spheres, spheres_size, 1, file);    
    scene->spheres = calloc(sphere_count, sizeof(Sphere));
    for (u32 sphere_index = 0;
         sphere_index < sphere_count;
         ++sphere_index)
    {
        SceneFileSphere *source = file_spheres + sphere_index;
        Sphere *dest = scene->spheres + sphere_index;
        
        dest->pos = source->pos;
        dest->radius = source->r;
        dest->mat_index = source->mat_index;
    }
    
    SceneFilePlane *file_planes = temp_buffer;
    fread(materials, materials_size, 1, file);    
    scene->planes = calloc(plane_count, sizeof(Plane));
     for (u32 plane_index = 0;
         plane_index < plane_count;
         ++plane_index)
    {
        SceneFilePlane *source = file_planes + plane_index;
        Plane *dest = scene->planes + plane_index;
        
        dest->normal = source->normal;
        dest->dist = source->d;
        dest->mat_index = source->mat_index;
    }
    
    SceneFileAARect *file_rects = temp_buffer;
    fread(materials, materials_size, 1, file);    
    scene->rects = calloc(rect_count, sizeof(Rect));
    for (u32 rect_index = 0;
         rect_index < rect_count;
         ++rect_index)
    {
        SceneFileAARect *source = file_rects + rect_index;
        Rect *dest = scene->rects + rect_index;
        
        RectType type;
        switch(source->type)
        {
            case SceneFileAARect_XY:
            {
                type = RectType_XY;
            } break;
            case SceneFileAARect_YZ:
            {
                type = RectType_YZ;
            } break;
            case SceneFileAARect_XZ:
            {
                type = RectType_XZ;
            } break;
        }
        dest->type = type;
        dest->xy.x0 = source->a0_0;
        dest->xy.x1 = source->a0_1;
        dest->xy.y0 = source->a1_0;
        dest->xy.y1 = source->a1_1;
        dest->xy.k = source->a2;
        dest->xy.mat_index = source->mat_index;
    }
    
    SceneFileTriangle *file_triangles = temp_buffer;
    fread(materials, materials_size, 1, file);    
    scene->triangles = calloc(triangle_count, sizeof(Triangle));
    for (u32 triangle_index = 0;
         triangle_index < triangle_count;
         ++triangle_index)
    {
        SceneFileTriangle *source = file_triangles + triangle_index;
        Triangle *dest = scene->triangles + triangle_index;
        
        dest->vertex0 = source->vertex0;
        dest->vertex1 = source->vertex1;
        dest->vertex2 = source->vertex2;
    }
    
    free(temp_buffer);
    
    scene->camera = camera(header.camera.pos, image);
    
    fclose(file);
}

