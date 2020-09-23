#include "font.h"

#include "stb_truetype.h"

#include "editor/renderer/opengl.h"

void 
font_load(Font *font, char *filename, f32 height, OpenGLRenderer *opengl)
{
    void *file_contents;
	u64   file_size;
    
    FILE *file = fopen(filename, "rb");
    if (!file)
    {
        fprintf(stderr, "[ERROR] Failed to open font file '%s'\n", filename);
        return;
    }
    fseek(file, 0, SEEK_END);
    file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    file_contents = malloc(file_size);
    
    fread(file_contents, 1, file_size, file);
    fclose(file);
    
	const u32 atlas_width     = 512;
	const u32 atlas_height    = 512;
	const u32 first_codepoint = 32;
	const u32 codepoint_count = 95;

	stbtt_packedchar *glyphs = (stbtt_packedchar *)malloc(codepoint_count * sizeof(stbtt_packedchar));

	u8 *loaded_atlas_data = (u8 *)malloc(atlas_width * atlas_height);
	u32 *atlas_data       = (u32 *)malloc(atlas_width * atlas_height * sizeof(u32));

	stbtt_pack_context context = {};
	stbtt_PackBegin(&context, loaded_atlas_data, atlas_width, atlas_height, 0, 1, 0);
	stbtt_PackSetOversampling(&context, 2, 2);
	stbtt_PackFontRange(&context, (u8 *)file_contents, 0, height, first_codepoint, codepoint_count, glyphs);
	stbtt_PackEnd(&context);

	for (u32 i = 0; i < atlas_width * atlas_height; ++i)
	{
		u8 *dest = (u8 *)(atlas_data + i);
		dest[0] = 255;
		dest[1] = 255;
		dest[2] = 255;
		dest[3] = loaded_atlas_data[i];
	}
	
	free(loaded_atlas_data);

	opengl_init_texture(opengl, &font->atlas, atlas_data, atlas_width, atlas_height);

	font->first_codepoint = first_codepoint;
	font->glyph_count = codepoint_count;
	font->height = height;
	font->glyphs = (FontGlyph *)malloc(sizeof(FontGlyph) * codepoint_count);

	for (u32 i = 0; 
		 i < codepoint_count;
		 ++i)
	{
		font->glyphs[i].utf32 = first_codepoint + i;
		font->glyphs[i].min_x = glyphs[i].x0;
		font->glyphs[i].min_y = glyphs[i].y0;
		font->glyphs[i].max_x = glyphs[i].x1;
		font->glyphs[i].max_y = glyphs[i].y1;
		font->glyphs[i].offset1_x = glyphs[i].xoff;
		font->glyphs[i].offset1_y = glyphs[i].yoff;
		font->glyphs[i].offset2_x = glyphs[i].xoff2;
		font->glyphs[i].offset2_y = glyphs[i].yoff2;
		font->glyphs[i].x_advance = glyphs[i].xadvance;
	}
	
	free(glyphs);
}


f32
get_text_width(Font *font, char *text, f32 scale)
{
	f32 result = 0;
    for(char symbol = *text;
        symbol;
        symbol = *++text)
    {
		if ((symbol >= font->first_codepoint) && (symbol < font->first_codepoint + font->glyph_count))
        {
            FontGlyph *glyph = font->glyphs + symbol - font->first_codepoint;
            result += glyph->x_advance * scale;
        }
    }
    return result;
}

f32
get_textn_width(Font *font, char *text, u64 count, f32 scale)
{
	f32 result = 0;

    for(u32 index = 0;
		index < count;
		++index)
    {
		char symbol = text[index];
		if ((symbol >= font->first_codepoint) && (symbol < font->first_codepoint + font->glyph_count))
        {
            FontGlyph *glyph = font->glyphs + symbol - font->first_codepoint;
            result += glyph->x_advance * scale;
        }
    }
    return result;
}

Vec2
get_text_size(Font *font, char *text, f32 scale)
{
	Vec2 result;
	result.x = get_text_width(font, text, scale);
	result.y = font->height * scale;
	return result;
}

Vec2
get_textn_size(Font *font, char *text, u64 count, f32 scale)
{
	Vec2 result;
	result.x = get_textn_width(font, text, count, scale);
	result.y = font->height * scale;
	return result;
}