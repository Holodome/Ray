#include "editor/renderer/opengl.h"

#include "font.h"

static RendererCommandHeader *
push_buffer(OpenGLRenderer *renderer, umm data_size)
{
    RendererCommandHeader *result = 0;
	
    u8 *push_buffer_end = renderer->request_buffer_base + renderer->request_buffer_size;
    if ((renderer->request_buffer_at + data_size) <= push_buffer_end)
    {
        result = (RendererCommandHeader *)renderer->request_buffer_at;
        renderer->request_buffer_at += data_size;
    }
    else
    {
        //logprint("RENDERER", "Error: Command buffer size exceeded!\n");
    }
	
    return result;
}

#define push_command(renderer, type) (type *)push_command_(renderer, sizeof(type), RendererCommand_##type)
static u8 *
push_command_(OpenGLRenderer *renderer, umm size, RendererCommandType type)
{
    u8 *result = 0;
	
    size += sizeof(RendererCommandHeader);
    RendererCommandHeader *header = push_buffer(renderer, size);
    if (header)
    {
        header->type = type;
        renderer->last_command_header = header;
		
        result = (u8 *)header + sizeof(RendererCommandHeader);
    }
	
    return result;
}

// Get last quads command or create new
static RendererCommandQuads *
get_current_quads(OpenGLRenderer *renderer)
{
    RendererCommandQuads *result = 0;
	
    if (renderer->last_command_header
        && renderer->last_command_header->type == RendererCommand_RendererCommandQuads)
    {
        result = (RendererCommandQuads *)(renderer->last_command_header + 1);
        ++result->quad_count;
    }
    else
    {
        result = push_command(renderer, RendererCommandQuads);
		
        if (result)
        {
            result->index_array_offset  = renderer->index_buffer_size;
            result->vertex_array_offset = renderer->vertex_buffer_size;
            result->quad_count = 1;
        }
    }
	
    return result;
}


static void
push_quad(OpenGLRenderer *renderer,
		  Vec3 v00, Vec3 v01, Vec3 v10, Vec3 v11,
		  Vec4 c00, Vec4 c01, Vec4 c10, Vec4 c11,
		  Vec2 uv00, Vec2 uv01, Vec2 uv10, Vec2 uv11,
		  OpenGLTexture texture)
{
    RendererCommandQuads *quads = get_current_quads(renderer);
	
    if (quads)
    {
        // Texture buffer
        if (texture.index == empty_texture.index)
        {
            texture = renderer->white_texture;
        }
        u32 texture_index = texture.index;
        // @NOTE(hl): Transpose uvs from normalized space, to texture in texture array space
        // Vec2 array_size = opengl_texture_size;
        Vec2 array_texture_size   = opengl_texture_size;
        Vec2 current_texture_size = vec2(texture.width, texture.height);
        Vec2 uv_scale = vec2_div(current_texture_size, array_texture_size);
		uv00 = vec2_mul(uv00, uv_scale);
		uv01 = vec2_mul(uv01, uv_scale);
		uv10 = vec2_mul(uv10, uv_scale);
		uv11 = vec2_mul(uv11, uv_scale);
		
        u32 packed_color00 = rgba_pack_4x8(vec4_muls(c00, 255));
        u32 packed_color01 = rgba_pack_4x8(vec4_muls(c01, 255));
		u32 packed_color10 = rgba_pack_4x8(vec4_muls(c10, 255));
        u32 packed_color11 = rgba_pack_4x8(vec4_muls(c11, 255));
		
        // Vertex buffer
        OpenGLVertex *vertex_buffer = renderer->vertex_buffer + renderer->vertex_buffer_size;
		vertex_buffer[0].pos = v00;
		vertex_buffer[0].uv  = uv00;
		vertex_buffer[0].packed_color = packed_color00;
		vertex_buffer[0].texture_index = texture_index;
		
		vertex_buffer[1].pos = v01;
		vertex_buffer[1].uv  = uv01;
		vertex_buffer[1].packed_color = packed_color01;
		vertex_buffer[1].texture_index = texture_index;
		
		vertex_buffer[2].pos = v10;
		vertex_buffer[2].uv  = uv10;
		vertex_buffer[2].packed_color = packed_color10;
		vertex_buffer[2].texture_index = texture_index;
		
		vertex_buffer[3].pos = v11;
		vertex_buffer[3].uv  = uv11;
		vertex_buffer[3].packed_color = packed_color11;
		vertex_buffer[3].texture_index = texture_index;
		
        // Index buffer
        u16 *index_buffer = renderer->index_buffer + renderer->index_buffer_size;
        u16  base_index   = renderer->vertex_buffer_size - quads->vertex_array_offset;
        index_buffer[0] = base_index + 0;
        index_buffer[1] = base_index + 2;
        index_buffer[2] = base_index + 3;
        index_buffer[3] = base_index + 0;
        index_buffer[4] = base_index + 1;
        index_buffer[5] = base_index + 3;
		
        // Update buffer sizes after we are finished.
        renderer->vertex_buffer_size += 4;
        renderer->index_buffer_size  += 6;
    }
    else
    {
        assert(false);
    }
}

static void
push_quadr(OpenGLRenderer *renderer,
		   Rect2 rect, Vec4 color,
		   OpenGLTexture texture)
{
    Vec3 vertices[4] = {0};
    rect2_store_points(rect, &vertices[0].xy, &vertices[1].xy, &vertices[2].xy, &vertices[3].xy);
	
	push_quad(renderer,
			  vertices[0], vertices[1], vertices[2], vertices[3],
			  color, color, color, color,
			  vec2(0, 0), vec2(0, 1), vec2(1, 0), vec2(1, 1),
			  texture);
}


static void
push_quad_outline(OpenGLRenderer *renderer,
				  Vec2 v00, Vec2 v01, Vec2 v10, Vec2 v11,
				  Vec4 color,
				  f32  width)
{
	Vec2 width_v = vec2_muls(vec2(width, width), 0.5f);
	Rect2 top    = rect2v(vec2_sub(v00, width_v), vec2_sub(v10, width_v));
	Rect2 bottom = rect2v(vec2_sub(v01, width_v), vec2_sub(v11, width_v));
	Rect2 left   = rect2v(vec2_sub(v00, width_v), vec2_sub(v01, width_v));
	Rect2 right  = rect2v(vec2_sub(v10, width_v), vec2_sub(v11, width_v));
	
	push_quadr(renderer, top   , color, empty_texture);
	push_quadr(renderer, bottom, color, empty_texture);
	push_quadr(renderer, left  , color, empty_texture);
	push_quadr(renderer, right , color, empty_texture);
}

static void
push_quad_outliner(OpenGLRenderer *renderer,
				   Rect2 rect, Vec4 color,
				   f32 width)
{
	Vec2 rect_points[4];
	rect2_store_points(rect, &rect_points[0], &rect_points[1], &rect_points[2], &rect_points[3]);
	push_quad_outline(renderer,
					  rect_points[0], rect_points[1], rect_points[2], rect_points[3],
					  color,
					  width);
}


static void
push_triangle(OpenGLRenderer *renderer,
			  Vec3 v0, Vec3 v1, Vec3 v2,
			  Vec4 c0, Vec4 c1, Vec4 c2)
{
	push_quad(renderer,
			  v0, v1, v2, v2,
			  c0, c1, c2, c2, 
			  vec2(0, 0), vec2(0, 1), vec2(1, 0), vec2(1, 1),
			  empty_texture);
}

static void
push_text(OpenGLRenderer *renderer, Vec2 position, Vec4 color,
		  char *text, struct Font *font, f32 scale)
{
	f32 line_height = font->height * scale;
	
	f32 rwidth  = reciprocal32(font->atlas.width);
	f32 rheight = reciprocal32(font->atlas.width);
	
	Vec3 offset = { 
        .x = position.x,
        .y = position.y,
        .z = 0
    };
	offset.y += line_height;
	
	for (char *scan = text;
		 *scan;
		 ++scan)
	{
		char symbol = *scan;
		
		if ((symbol >= font->first_codepoint) && (symbol < font->first_codepoint + font->glyph_count))
		{
			FontGlyph *glyph = font->glyphs + (symbol - font->first_codepoint);
			
			f32 glyph_width  = (glyph->offset2_x - glyph->offset1_x) * scale;
			f32 glyph_height = (glyph->offset2_y - glyph->offset1_y) * scale;
			
			
			f32 y1 = offset.y + glyph->offset1_y * scale;
			f32 y2 = y1 + glyph_height;
			f32 x1 = offset.x + glyph->offset1_x * scale;
			f32 x2 = x1 + glyph_width;
			
			f32 s1 = glyph->min_x * rwidth;
			f32 t1 = glyph->min_y * rheight;
			f32 s2 = glyph->max_x * rwidth;
			f32 t2 = glyph->max_y * rheight;
			
			push_quad(renderer,
					  vec3(x1, y1, 0), vec3(x1, y2, 0), vec3(x2, y1, 0), vec3(x2, y2, 0),
					  color, color, color, color,
					  vec2(s1, t1), vec2(s1, t2), vec2(s2, t1), vec2(s2, t2),
					  font->atlas);
			
			f32 char_advance = glyph->x_advance * scale;
			offset.x += char_advance;
		}
	}
}

static void
push_clip_rect(OpenGLRenderer *renderer, Rect2 rect)
{
}

static void
pop_clip_rect(OpenGLRenderer *renderer)
{
}


static void APIENTRY
opengl_error_callback(GLenum Source, GLenum type, GLenum ID, GLenum Severity, GLsizei Length,
                      const GLchar* Message, const void *_)
{
    if (Severity == GL_DEBUG_SEVERITY_NOTIFICATION) return;
	
    char *SourceStr;
    switch(Source)
    {
        case GL_DEBUG_SOURCE_API_ARB:
		SourceStr = "Calls to OpenGL API"; break;
        case GL_DEBUG_SOURCE_WINDOW_SYSTEM_ARB:
		SourceStr = "Calls to window-system API"; break;
        case GL_DEBUG_SOURCE_SHADER_COMPILER_ARB:
		SourceStr = "A compiler for shading language"; break;
        case GL_DEBUG_SOURCE_THIRD_PARTY_ARB:
		SourceStr = "Application associated with OpenGL"; break;
        case GL_DEBUG_SOURCE_APPLICATION_ARB:
		SourceStr = "Generated by user"; break;
        case GL_DEBUG_SOURCE_OTHER_ARB:
		SourceStr = "Other"; break;
        default:
		SourceStr = "Unknown"; break;
    }
	
    char *TypeStr;
    switch (type)
    {
        case GL_DEBUG_TYPE_ERROR_ARB:
		TypeStr = "ERROR"; break;
        case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR_ARB:
		TypeStr = "DEPRECATED_BEHAVIOR"; break;
        case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR_ARB:
		TypeStr = "UNDEFINED_BEHAVIOR"; break;
        case GL_DEBUG_TYPE_PORTABILITY_ARB:
		TypeStr = "PORTABILITY"; break;
        case GL_DEBUG_TYPE_PERFORMANCE_ARB:
		TypeStr = "PERFORMANCE"; break;
        case GL_DEBUG_TYPE_OTHER_ARB:
		TypeStr = "OTHER"; break;
        default:
		TypeStr = "Unknown"; break;
    }
	
    char *SeverityStr;
    switch(Severity)
    {
        case GL_DEBUG_SEVERITY_NOTIFICATION:
		SeverityStr = "Notification"; break;
        case GL_DEBUG_SEVERITY_LOW_ARB:
		SeverityStr = "LOW"; break;
        case GL_DEBUG_SEVERITY_MEDIUM_ARB:
		SeverityStr = "MEDIUM"; break;
        case GL_DEBUG_SEVERITY_HIGH_ARB:
		SeverityStr = "HIGH"; break;
        default:
		SeverityStr = "UNKNOWN"; break;
    }
	
    printf("OpenGL Error Callback\n<Source: %s, type: %s, Severity: %s, ID: %u>:::\n%s\n",
		   SourceStr, TypeStr, SeverityStr, ID, Message);
}

static void
opengl_set_attribute(OpenGLApi *gl, GLuint index, GLsizei stride, GLint size, umm offset,
                     GLenum type, bool normalized)
{
    GLboolean OpenGLNormalized = (normalized ? GL_TRUE : GL_FALSE);
	
    gl->glEnableVertexAttribArray(index);
    gl->glVertexAttribPointer(index, size, type, OpenGLNormalized, stride, (void *)offset);
}

static void
opengl_set_attribute_int(OpenGLApi *gl, GLuint index, GLsizei stride, GLint size, umm offset,
                         GLenum type)
{
    gl->glEnableVertexAttribArray(index);
    gl->glVertexAttribIPointer(index, size, type, stride, (void *)offset);
}

static char *
COMMON_SHADER_DEFINES = ""
"#define Vec2   vec2    \n"
"#define Vec3   vec3    \n"
"#define Vec4   vec4    \n"
"#define Vec2i  ivec2   \n"
"#define Vec3i  ivec3   \n"
"#define Vec4i  ivec4   \n"
"#define Vec2u  uvec2   \n"
"#define Vec3u  uvec3   \n"
"#define Vec4u  uvec4   \n"
"#define Mat4x4 mat4    \n"
"#define Mat3x3 mat3    \n"
"#define bool bool      \n"
"#define i32  int       \n"
"#define u32  uint      \n"
"#define f32  float     \n"
"#define f64  double    \n";

static char *SHADER_VERSION      = "#version 330\n";
static char *DEF_VERTEX_SHADER	 = "#define VERTEX_SHADER  \n";
static char *DEF_FRAGMENT_SHADER = "#define FRAGMENT_SHADER\n";


static GLuint
opengl_compile_shader_base(OpenGLApi *gl, char *source)
{
    GLuint result = {0};
	
    GLuint vertex_shader   = gl->glCreateShader(GL_VERTEX_SHADER);
    GLuint fragment_shader = gl->glCreateShader(GL_FRAGMENT_SHADER);
	
    const char * const vertex_source[]   = { SHADER_VERSION, COMMON_SHADER_DEFINES, DEF_VERTEX_SHADER, source };
    const char * const fragment_source[] = { SHADER_VERSION, COMMON_SHADER_DEFINES, DEF_FRAGMENT_SHADER, source };
	
    gl->glShaderSource(vertex_shader, array_size(vertex_source), vertex_source, 0);
    gl->glShaderSource(fragment_shader, array_size(fragment_source), fragment_source, 0);
	
    gl->glCompileShader(vertex_shader);
    gl->glCompileShader(fragment_shader);
	
    GLint vertex_compiled, fragment_compiled;
    gl->glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &vertex_compiled);
    gl->glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &fragment_compiled);
	
    if (!vertex_compiled || !fragment_compiled)
    {
        char shader_log[1024];
        if (!vertex_compiled)
        {
            gl->glGetShaderInfoLog(vertex_shader, sizeof(shader_log), 0, shader_log);
            printf("Vertex shader compilation failed: %s", shader_log);
        }
        else if (!fragment_compiled)
        {
            gl->glGetShaderInfoLog(fragment_shader, sizeof(shader_log), 0, shader_log);
            printf("Fragment shader compilation failed: %s", shader_log);
        }
    }
	
    GLuint program = gl->glCreateProgram();
    gl->glAttachShader(program, vertex_shader);
    gl->glAttachShader(program, fragment_shader);
	
    gl->glLinkProgram(program);
    // gl->glDeleteShader(vertex_shader);
    // gl->glDeleteShader(fragment_shader);
    // gl->glDetachShader(program, vertex_shader);
    // gl->glDetachShader(program, fragment_shader);
	
    GLint link_success;
    gl->glGetProgramiv(program, GL_LINK_STATUS, &link_success);
	
    if (!link_success)
    {
        char program_log[1024];
        gl->glGetProgramInfoLog(program, sizeof(program_log), 0, program_log);
        printf("Program link failed: %s", program_log);
    }
	
    result = program;
	
    return result;
}


static OpenGLQuadShader
opengl_compile_quad_shader(OpenGLApi *gl)
{
    OpenGLQuadShader result = {0};
    char *source = ""
		"#ifdef VERTEX_SHADER       \n"
		"layout(location = 0) in Vec4 position;     \n"
		"layout(location = 1) in Vec4 color;        \n"
		"layout(location = 2) in Vec2 uv;       \n"
		"layout(location = 3) in i32 texture_index;     \n"
		"       \n"
		"out Vec4 rect_color;       \n"
		"out Vec2 frag_uv;      \n"
		"       \n"
		"flat out i32 frag_texture_index;       \n"
		"uniform Mat4x4 mvp_matrix = Mat4x4(1);     \n"
		"void main()        \n"
		"{      \n"
		"    Vec4 world_space = position;       \n"
		"    Vec4 clip_space = mvp_matrix * world_space;        \n"
		"    gl_Position = clip_space;      \n"
		"       \n"
		"    rect_color = color;        \n"
		"    frag_uv = uv;      \n"
		"    frag_texture_index = texture_index;        \n"
		"}      \n"
		"#endif     \n"
		"#ifdef FRAGMENT_SHADER     \n"
		"in Vec4 rect_color;        \n"
		"in Vec2 frag_uv;       \n"
		"flat in i32 frag_texture_index;        \n"
		"uniform sampler2DArray texture_sampler;        \n"
		"out Vec4 out_color;        \n"
		"void main()        \n"
		"{      \n"
		"    Vec3 array_uv = Vec3(frag_uv.x, frag_uv.y, frag_texture_index);        \n"
		"    vec4 texture_sample = texture(texture_sampler, array_uv);      \n"
		"    out_color = texture_sample * rect_color;       \n"
		"}      \n"
		"#endif     \n";
    result.handle = opengl_compile_shader_base(gl, source);
    result.mvp_location = gl->glGetUniformLocation(result.handle, "mvp_matrix");
    result.texture_sampler_location = gl->glGetUniformLocation(result.handle, "texture_sampler");
	
    return result;
}

// static bool
// opengl_texture_fits_to_array(OpenGL *opengl, Vec2u size)
// {
//     bool result = opengl->texture_size >= size;
//     return result;
// }


static OpenGLTexture
opengl_allocate_texture(OpenGLRenderer *opengl, u32 width, u32 height, void *data)
{
    // TIMED_FUNCTION();
	
    OpenGLTexture result = {0};
    // For textures we want to use arrays even if there is only one texture in it.
    // This is done so in shader we can set sampler2DArray and use any single texture with it,
    // not just regular texture array.
    // if (opengl_texture_fits_to_array(opengl, vec2u(width, height)))
    {
        result.index = opengl->texture_count++;
        opengl->gl.glBindTexture(GL_TEXTURE_2D_ARRAY, opengl->texture_array);
        opengl->gl.glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0,
                                   result.index, width, height, 1,
                                   opengl->default_texture_format, GL_UNSIGNED_BYTE, data);
    }
    // else
    // {
    //     u32 index = opengl->arbitrary_size_texture_count++;
    //     result.handle = opengl->arbitrary_size_textures[index];
	// 	opengl->gl.glBindTexture(GL_TEXTURE_2D_ARRAY, result.handle);
    //     opengl->gl.glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, opengl->default_texture_internal_format,
    //                             width, height, 1, 0, opengl->default_texture_format,
	// 							GL_UNSIGNED_BYTE, data);
    //     platform_printf("Strage shit!\n");
    // }
    result.width = (u16)(width);
    result.height = (u16)(height);
    return result;
}


void
opengl_init_texture(OpenGLRenderer *opengl, OpenGLTexture *dest, void *memory, u32 width, u32 height)
{
	*dest = opengl_allocate_texture(opengl, width, height, memory);
}

static OpenGLInfo
opengl_get_info(OpenGLApi *gl)
{
    OpenGLInfo info = {0};
    gl->glGetIntegerv(GL_MAJOR_VERSION, &info.version_major);
    gl->glGetIntegerv(GL_MINOR_VERSION, &info.version_minor);
    info.renderer_string     = (char *)gl->glGetString(GL_RENDERER);
    info.version_string      = (char *)gl->glGetString(GL_VERSION);
	info.vendor_string       = (char *)gl->glGetString(GL_VENDOR);
    info.glsl_version_string = (char *)gl->glGetString(GL_SHADING_LANGUAGE_VERSION);
	
    gl->glGetIntegerv(GL_MAX_ARRAY_TEXTURE_LAYERS, &info.max_texture_array_layers);
    gl->glGetIntegerv(GL_MAX_TEXTURE_SIZE, &info.max_texture_dimension_size);
    gl->glGetIntegerv(GL_MAX_ELEMENTS_INDICES, &info.max_indices);
    gl->glGetIntegerv(GL_MAX_ELEMENTS_VERTICES, &info.max_vertices);
    gl->glGetIntegerv(GL_MAX_VIEWPORTS, &info.max_viewports);
    return info;
}


void
opengl_init(OpenGLRenderer *opengl)
{
    assert(opengl);
    //logprint("OGL", "Initializing opengl (%p)\n", opengl);
    opengl->max_texture_count = 256;
	
	// TODO(hl): Move allocations to renderer creation
    opengl->request_buffer_size = (1 << 20) * 16;
    opengl->request_buffer_base = (u8 *)malloc(opengl->request_buffer_size);
	
    opengl->max_vertex_buffer_size = (1 << 15);
    opengl->vertex_buffer = (OpenGLVertex *)malloc(opengl->max_vertex_buffer_size * sizeof(OpenGLVertex));
	
    opengl->max_index_buffer_size = (1 << 15);
    opengl->index_buffer = (u16 *)malloc(sizeof(u16) * opengl->max_index_buffer_size);
	
    OpenGLApi *gl = &opengl->gl;
    opengl->info = opengl_get_info(gl);
    //char opengl_info_format[2048];
    //dump_structure_static(opengl_info_format, opengl->info);
    //logprint("OGL", "opengl info: %s\n", opengl_info_format);
	
    if (gl->glDebugMessageCallback)
    {
        gl->glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS_ARB);
        gl->glDebugMessageCallback(opengl_error_callback, 0);
    }
    else
    {
        //logprint("OGL", "Opengl debugging (glDebugMessageCallback) is not supported.\n");
    }
	
    opengl->default_texture_format          = GL_RGBA;
    opengl->default_texture_internal_format = GL_RGBA8;
	
    gl->glGenVertexArrays(1, &opengl->quad_vertex_array);
    gl->glBindVertexArray(opengl->quad_vertex_array);
	
    gl->glGenBuffers(1, &opengl->quad_vertex_buffer);
    gl->glBindBuffer(GL_ARRAY_BUFFER, opengl->quad_vertex_buffer);
    gl->glBufferData(GL_ARRAY_BUFFER, opengl->max_vertex_buffer_size, 0, GL_STREAM_DRAW);
	
    gl->glGenBuffers(1, &opengl->quad_index_buffer);
    gl->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, opengl->quad_index_buffer);
    gl->glBufferData(GL_ELEMENT_ARRAY_BUFFER, opengl->max_index_buffer_size, 0, GL_STREAM_DRAW);
	
    opengl_set_attribute(gl, 0, sizeof(OpenGLVertex), 3, offsetof(OpenGLVertex, pos), GL_FLOAT, false);
    opengl_set_attribute(gl, 1, sizeof(OpenGLVertex), 4, offsetof(OpenGLVertex, packed_color), GL_UNSIGNED_BYTE, true);
    opengl_set_attribute(gl, 2, sizeof(OpenGLVertex), 2, offsetof(OpenGLVertex, uv), GL_FLOAT, false);
    opengl_set_attribute_int(gl, 3, sizeof(OpenGLVertex), 1, offsetof(OpenGLVertex, texture_index), GL_UNSIGNED_SHORT);
	
    gl->glBindVertexArray(0);
	
    opengl->quad_shader = opengl_compile_quad_shader(gl);
	
    // Generate texture array
	
    gl->glGenTextures(1, &opengl->texture_array);
    gl->glBindTexture(GL_TEXTURE_2D_ARRAY, opengl->texture_array);
    assert(opengl->max_texture_count <= opengl->info.max_texture_array_layers);
    gl->glTexStorage3D(GL_TEXTURE_2D_ARRAY, 1,
                       opengl->default_texture_internal_format,
                       opengl_texture_size.x,
                       opengl_texture_size.y,
                       opengl->max_texture_count);
    gl->glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    gl->glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    gl->glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    gl->glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	
    // Create white texture
    u8 *white_texture_data = (u8 *)malloc(512 * 512 * 4);
    memset(white_texture_data, 255, 512 * 512 * 4);
    opengl->white_texture = opengl_allocate_texture(opengl, 512, 512, white_texture_data);
    free(white_texture_data);
}

void
opengl_begin_frame(OpenGLRenderer *opengl, Vec2 display_size, Vec3 clear_color)
{
	opengl->last_command_header = 0;
	
    opengl->display_size = display_size;
    opengl->clear_color  = clear_color;
	
    assert(opengl->request_buffer_size);
    opengl->request_buffer_at = opengl->request_buffer_base;
	
    assert(opengl->max_vertex_buffer_size);
    opengl->vertex_buffer_size = 0;
	
    assert(opengl->max_index_buffer_size);
    opengl->index_buffer_size = 0;
}

void
opengl_end_frame(OpenGLRenderer *opengl)
{
    OpenGLApi *gl = &opengl->gl;
    // Set up drawing and rasterization settings
    gl->glEnable(GL_DEPTH_TEST);
    gl->glEnable(GL_SCISSOR_TEST);
    gl->glDepthMask(GL_TRUE);
    gl->glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    gl->glDepthFunc(GL_LEQUAL);
    gl->glCullFace(GL_BACK);
    gl->glFrontFace(GL_CCW);
    gl->glProvokingVertex(GL_FIRST_VERTEX_CONVENTION);
    gl->glEnable(GL_BLEND);
    gl->glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    gl->glViewport(0, 0, opengl->display_size.x, opengl->display_size.y);
    gl->glScissor(0, 0, opengl->display_size.x, opengl->display_size.y);
    // Upload data from vertex array to OpenGL buffer
    gl->glBindVertexArray(opengl->quad_vertex_array);
    gl->glBindBuffer(GL_ARRAY_BUFFER, opengl->quad_vertex_buffer);
    gl->glBufferSubData(GL_ARRAY_BUFFER, 0, opengl->vertex_buffer_size * sizeof(OpenGLVertex), opengl->vertex_buffer);
    gl->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, opengl->quad_index_buffer);
    gl->glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, opengl->index_buffer_size * sizeof(u16), opengl->index_buffer);
	
    gl->glClearColor(opengl->clear_color.r, opengl->clear_color.g,
                     opengl->clear_color.b, 1.0f);
    gl->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
    Mat4x4 view_projection = mat4x4_orthographic2d(0, opengl->display_size.x, opengl->display_size.y, 0);
	
    // Iterate through requests
    for (u8 *header_at = opengl->request_buffer_base;
         header_at < opengl->request_buffer_at;
         )
    {
        RendererCommandHeader *header = (RendererCommandHeader *)header_at;
        header_at += sizeof(RendererCommandHeader);
        void *data = (u8 *)header + sizeof(RendererCommandHeader);
		
        switch (header->type)
        {
            default: assert(false); break;
            case RendererCommand_RendererCommandQuads:
            {
                header_at += sizeof(RendererCommandQuads);
                RendererCommandQuads *entry = (RendererCommandQuads *)data;
				
                OpenGLQuadShader *shader = &opengl->quad_shader;
                gl->glUseProgram(shader->handle);
				
                gl->glUniform1i(shader->texture_sampler_location, 0);
                gl->glUniformMatrix4fv(shader->mvp_location, 1, GL_FALSE, view_projection.e[0]);
                gl->glActiveTexture(GL_TEXTURE0);
                gl->glBindTexture(GL_TEXTURE_2D_ARRAY, opengl->texture_array);
                gl->glDrawElementsBaseVertex(GL_TRIANGLES, 6 * entry->quad_count, GL_UNSIGNED_SHORT,
                                             (GLvoid *)(sizeof(u16) * entry->index_array_offset),
                                             entry->vertex_array_offset);
            } break;
        }
    }
}