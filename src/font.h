#if !defined(FONT_H)

#include "lib/common.h"
#include "image.h"

// @CLEANUP
#include "editor/renderer/opengl.h"

typedef struct {
	u32 utf32;

	u16 min_x;
	u16 min_y;
	u16 max_x;
	u16 max_y;
	f32 offset1_x;
	f32 offset1_y;
	f32 offset2_x;
	f32 offset2_y;
	f32 x_advance;
} FontGlyph;

typedef struct Font {
    f32 height;

	u32 first_codepoint;
	u32 glyph_count;

	OpenGLTexture atlas;
	FontGlyph *glyphs;   
} Font;  

void font_load(Font *font, char *filename, f32 height, OpenGLRenderer *opengl);


f32 get_text_width(Font *font, char *text,  f32 scale);
f32 get_textn_width(Font *font, char *text, u64 count, f32 scale);
Vec2 get_text_size(Font *font, char *text,  f32 scale);
Vec2 get_textn_size(Font *font, char *text, u64 count, f32 scale);

#define FONT_H 1
#endif
