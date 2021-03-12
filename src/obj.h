#if !defined(OBJ_H)

#include "general.h"
#include "ray_math.h"
#include "trace.h"
#include "ray_string.h"

typedef struct Vertex {
    Vec3 p;
    u32 uv_index;
    u32 normal_index;
    u32 index;
    struct Vertex *duplicate_vertex;
} Vertex;

TriangleMeshData  
load_obj(char *filename) {
    TriangleMeshData result = {0};
    
    FILE *file = fopen(filename, "rb");
    if (!file) {
        fprintf(stderr, "[ERROR] Failed to open file %s\n", filename);        
        goto end;
    }
    
    fseek(file, 0, SEEK_END);
    u32 file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    char *file_contents = malloc(file_size + 1);
    fread(file_contents, 1, file_size, file);
    file_contents[file_size] = 0;
    
    // First, count all elements to allocate all arrays at once
    char *cursor = file_contents;
    u32 file_vertex_count = 0;
    u32 file_uv_count = 0;
    u32 file_normal_count = 0;
    u32 file_face_count = 0;
    u32 file_index_count = 0;
    while (*cursor) {
        if (*cursor != 'v' && *cursor != 'f') {
            cursor = skip_to_next_line(cursor);
            continue;
        }
        
        if (*cursor == 'v' && *(cursor + 1) == 'n') {
            ++file_normal_count;
            cursor = skip_to_next_line(cursor);
        } else if (*cursor == 'v' && *(cursor + 1) == 't') {
            ++file_uv_count;
            cursor = skip_to_next_line(cursor);
        } else if (*cursor == 'v') {
            ++file_vertex_count;
            cursor = skip_to_next_line(cursor);
        } else if (*cursor == 'f') {
            ++file_face_count;
            
            ++cursor;
            while (*cursor && *cursor != '\n') {
                if (*cursor == ' ') {
                    ++file_index_count;
                }
                ++cursor;
            }
            ++cursor;
        }
    }

    u64 vertices_size = file_vertex_count * sizeof(Vertex) * 100;
    u64 normals_size = file_normal_count * sizeof(Vec3);
    u64 uvs_size = file_uv_count * sizeof(Vec2);
    u64 indices_size = file_index_count * sizeof(u32) * 3;
    u8 *loaded_mesh_data_buffer = malloc(vertices_size + normals_size + uvs_size + indices_size);
    Vertex *file_vertices = (Vertex *)(loaded_mesh_data_buffer);
    Vec3 *file_normals = (Vec3 *)(loaded_mesh_data_buffer + vertices_size);
    Vec2 *file_uvs = (Vec2 *)(loaded_mesh_data_buffer + vertices_size + normals_size);
    u32 *file_indices = (u32 *)(loaded_mesh_data_buffer + vertices_size + normals_size + uvs_size);
    
    u32 vertex_count = 0;
    u32 uv_count = 0;
    u32 normal_count = 0;
    u32 index_count = 0;
    
    cursor = file_contents;
    while (*cursor) {
        if (*cursor != 'v' && *cursor != 'f') {
            cursor = skip_to_next_line(cursor);
            continue;
        }

        char *end_of_line = cursor;
        while (*end_of_line && *end_of_line != '\n') {
            ++end_of_line;
        }        
        
        String line = string(cursor, end_of_line - cursor);
        line = string_rstrip(line);
        
        char number_buffer[32];
        if (string_startswith(line, STR("v "))) {
            String temp_str = string_advance(line, 2);
            file_vertices[vertex_count] = (Vertex){0};
            file_vertices[vertex_count].index = vertex_count;
            file_vertices[vertex_count].uv_index     = U32_MAX;
            file_vertices[vertex_count].normal_index = U32_MAX;
            
            f32 *dest = file_vertices[vertex_count].p.e;
			++vertex_count;
            for(;;) {
                bool need_to_break = false;
                String value = substr_till_symb(temp_str, ' ', &need_to_break);
                string_to_buffer(value, number_buffer, sizeof(number_buffer));
                *dest++ = atof(number_buffer);
                temp_str = string_advance(temp_str, value.l + 1);
                if (need_to_break) {
                    break;
                }
            }
        } else if (string_startswith(line, STR("vn "))) {
            String temp_str = string_advance(line, 3);
            f32 *dest = file_normals[normal_count++].e;
            for(;;) {
                bool need_to_break = false;
                String value = substr_till_symb(temp_str, ' ', &need_to_break);
                number_buffer[string_to_buffer(value, number_buffer, sizeof(number_buffer))] = 0;
                *dest++ = atof(number_buffer);
                temp_str = string_advance(temp_str, value.l + 1);
                if (need_to_break) {
                    break;
                }
            }
        } else if (string_startswith(line, STR("vt "))) {
            String temp_str = string_advance(line, 2);
            f32 *dest = file_uvs[uv_count++].e;
            for(;;) {
                bool need_to_break = false;
                String value = substr_till_symb(temp_str, ' ', &need_to_break);
                number_buffer[string_to_buffer(value, number_buffer, sizeof(number_buffer))] = 0;
                *dest++ = atof(number_buffer);
                temp_str = string_advance(temp_str, value.l + 1);
                if (need_to_break) {
                    break;
                }
            }
        } else if (string_startswith(line, STR("f "))) {
            String temp_str = string_advance(line, 2);
            
            for(;;) {
                bool need_to_break = false;
                String values = substr_till_symb(temp_str, ' ', &need_to_break);
                u32 values_length = values.l;
                
                u32 vertex_indices[3] = {0};
                u32 vertex_indices_count = 0;
                
                for(;;) {
                    bool need_to_break_slash = false;
                    String value = substr_till_symb(values, '/', &need_to_break_slash);
                    number_buffer[string_to_buffer(value, number_buffer, sizeof(number_buffer))] = 0;
                    vertex_indices[vertex_indices_count++] = atoi(number_buffer);                    
                    assert(vertex_indices_count <= 3);
                                     
                    if (need_to_break_slash) {
                        break;
                    }
					values = string_advance(values, value.l + 1);
                }
                
                assert(vertex_indices[0] && vertex_indices[1] && vertex_indices[2]);
                --vertex_indices[0];
                --vertex_indices[1];
                --vertex_indices[2];
                
                Vertex *current_vertex = file_vertices + vertex_indices[0];                
                if (current_vertex->uv_index == U32_MAX && current_vertex->normal_index == U32_MAX) {
                    current_vertex->uv_index = vertex_indices[1];
                    current_vertex->normal_index = vertex_indices[2];
                    file_indices[index_count++] = vertex_indices[0];
                    goto vertex_parsed;
                }

            deal_with_already_processed_vertex:
                if (current_vertex->uv_index == vertex_indices[1] && current_vertex->normal_index == vertex_indices[2]) {
                    file_indices[index_count++] = current_vertex->index;
                    goto vertex_parsed;
                }
                
                Vertex *another_vertex = current_vertex->duplicate_vertex;
                if (another_vertex) {
                    current_vertex = another_vertex;
                    goto deal_with_already_processed_vertex;
                }
                
                Vertex *duplicate_vertex = file_vertices + vertex_count;
                duplicate_vertex->index        = vertex_count;
                duplicate_vertex->p            = current_vertex->p;
                duplicate_vertex->uv_index     = vertex_indices[1];
                duplicate_vertex->normal_index = vertex_indices[2];
                current_vertex->duplicate_vertex = duplicate_vertex;
                
                file_indices[index_count++] = duplicate_vertex->index;
                ++vertex_count;

            vertex_parsed:
                if (need_to_break) {
                    break;
                }
                temp_str = string_advance(temp_str, values_length + 1);
            }
        }
        
        cursor = end_of_line;
    } 

    assert(index_count % 3 == 0);

    vertices_size = sizeof(Vec3) * vertex_count;
    uvs_size      = sizeof(Vec3) * vertex_count;
    normals_size  = sizeof(Vec3) * vertex_count;
    indices_size  = sizeof(u32) * index_count;
    u64 vertices_per_face_size = sizeof(u32) * (index_count / 3);
	u64 result_buffer_size = vertices_size + uvs_size + normals_size + indices_size + vertices_per_face_size;
    u8 *result_buffer = malloc(result_buffer_size);
    result.p = (Vec3 *)result_buffer;
    result.uv = (Vec2 *)(result_buffer + vertices_size); 
    result.n = (Vec3 *)(result_buffer + vertices_size + uvs_size); 
    result.tri_indices = (u32 *)(result_buffer + vertices_size + uvs_size + normals_size); 
    result.nvert = vertex_count;
    result.ntrig = index_count / 3;
    
    for (u32 vertex_index = 0;
         vertex_index < vertex_count;
         ++vertex_index) {
        Vertex *current_vertex = file_vertices + vertex_index;
        result.p[vertex_index] = current_vertex->p;        
        result.uv[vertex_index] = file_uvs[current_vertex->uv_index == U32_MAX ? 0 : current_vertex->uv_index];        
        result.n[vertex_index] = normalize(file_normals[current_vertex->normal_index == U32_MAX ? 0 : current_vertex->normal_index]);        
    }
    memcpy(result.tri_indices, file_indices, indices_size);
    
    free(loaded_mesh_data_buffer);
    free(file_contents);
end:
    return result;
}


#define OBJ_H 1
#endif
