#if !defined(OPENGL_H)

#include "lib/common.h"
#include "lib/ray_math.h"
#include "editor/renderer/opengl_api.h"

struct Font;

typedef struct {
	u32 index;
	u16 width;
	u16 height;
} OpenGLTexture;

#define empty_texture ((OpenGLTexture) { .index = U32_MAX, .width = 0, .height = 0 })

typedef struct {
	Vec3 pos;
	Vec2 uv;
	u32  packed_color;
	u16  texture_index;
} OpenGLVertex;

typedef struct {
    i32 version_major;
    i32 version_minor;
    char *version_string;
    char *renderer_string;
    char *vendor_string;
    char *glsl_version_string;
	
    GLint max_multisample_count;
    GLint max_texture_array_layers;
    GLint max_texture_dimension_size;
    GLint max_indices;
    GLint max_vertices;
    GLint max_viewports;
} OpenGLInfo;

typedef struct {
	GLuint handle;
	
	GLint mvp_location;
	GLint texture_sampler_location;
} OpenGLQuadShader;

typedef u32 RendererCommandType;
enum {
	RendererCommand_RendererCommandQuads,
};


typedef struct {
	RendererCommandType type;
} RendererCommandHeader;

// @NOTE(hl): Reason quads is a separate command is that we want the most performant way of drawing them for UI and other small stuff.

typedef struct {
	u64 vertex_array_offset;
	u64 index_array_offset;
	
	u32 quad_count;
} RendererCommandQuads;

typedef struct OpenGLRenderer {
	// @NOTE(hl): Per-frame state data
	u64 request_buffer_size;
	u8 *request_buffer_base;
	u8 *request_buffer_at;
	
	u64 max_vertex_buffer_size;
	u64 vertex_buffer_size;
	OpenGLVertex *vertex_buffer;
	
	u64 max_index_buffer_size;
	u64 index_buffer_size;
	u16 *index_buffer;
	
	RendererCommandHeader *last_command_header;
	
	Vec3 clear_color;
	Vec2 display_size;
	
	// @NOTE(hl): Initialized in platform
    OpenGLApi gl;
	// @NOTE(hl): Initialized in init
    OpenGLInfo info;
	OpenGLTexture white_texture;
	
    GLenum default_texture_internal_format;
    GLenum default_texture_format;
	
	OpenGLQuadShader quad_shader;
	GLuint quad_vertex_array;
	GLuint quad_vertex_buffer;
	GLuint quad_index_buffer;
	
	u32 max_texture_count;
	u32 texture_count;
	u32 texture_array;
} OpenGLRenderer;

const Vec2 opengl_texture_size = { .x = 512, .y = 512 };

void push_quad(OpenGLRenderer *renderer,
			   Vec3 v00, Vec3 v01, Vec3 v10, Vec3 v11,
			   Vec4 c00, Vec4 c01, Vec4 c10, Vec4 c11,
			   Vec2 uv00, Vec2 uv01, Vec2 uv10, Vec2 uv11,
			   OpenGLTexture texture);

void push_quadr(OpenGLRenderer *renderer,
				Rect2 rect, Vec4 color,
				OpenGLTexture texture);

void push_text(OpenGLRenderer *renderer, Vec2 position, Vec4 color,
			   char *text, struct Font *font, f32 scale);

void push_clip_rect(OpenGLRenderer *renderer, Rect2 rect);
void pop_clip_rect(OpenGLRenderer *renderer);

// These function are made for visualization, uv or testing.
// They are not very performat because they always draw rects under the hood,
// wasting space on repeating vertices
void quad_outline(OpenGLRenderer *renderer,
				  Vec2 v00, Vec2 v01, Vec2 v10, Vec2 v11,
				  Vec4 color,
				  f32  width);

void quad_outliner(OpenGLRenderer *renderer,
				   Rect2 rect, Vec4 color,
				   f32 width);

void triangle(OpenGLRenderer *renderer,
			  Vec3 v0, Vec3 v1, Vec3 v2,
			  Vec4 c0, Vec4 c1, Vec4 c2);


void opengl_init(OpenGLRenderer *ogl);
void opengl_begin_frame(OpenGLRenderer *ogl, Vec2 display_size, Vec3 clear_color);
void opengl_end_frame(OpenGLRenderer *ogl);

void opengl_init_texture(OpenGLRenderer *opengl, OpenGLTexture *dest, void *pixels, u32 width, u32 height);

#define OPENGL_H 1
#endif
