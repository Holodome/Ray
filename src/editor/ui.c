#include "editor/ui.h"

UI ui;

// Functions wrapping logic of going by words (or chunks) in text edit
static u32
make_chunk_jump_left(char *text, u32 index, u32 length)
{
    u32 result = index;

    if (index != 0)
    {
        bool is_current_char_identifier = char_is_identifier(text[index - 1]);
        // Currently the logic is: if we start from identifier (word, some number) we go to the start of this identifier
        // else we go to the first found identifier
        if (is_current_char_identifier)
        {
            char *previous_not_identifier_location = text + index - 1;
            while (previous_not_identifier_location - text > 0 && char_is_identifier(*(previous_not_identifier_location - 1)))
            {
                --previous_not_identifier_location;
            }

            result = previous_not_identifier_location - text;
        }
        else
        {
            char *previous_identifier_location = text + index - 1;
            while (previous_identifier_location - text > 0 && !char_is_identifier(*(previous_identifier_location - 1)))
            {
                --previous_identifier_location;
            }

            result = previous_identifier_location - text;
        }
    }

    return result;
}

static u32
make_chunk_jump_right(char *text, u32 index, u32 length)
{
    u32 result = index;

    bool is_current_char_identifier = char_is_identifier(text[index + 1]);
    if (is_current_char_identifier)
    {
        char *first_not_identifier_location = text + index + 1;
        char *end_buffer_location = text + length;
        while (char_is_identifier(*first_not_identifier_location))
        {
            if (++first_not_identifier_location == end_buffer_location)
            {
                break;
            }
        }

        result = first_not_identifier_location - text;
    }
    else
    {
        char *first_identifier_location = text + index + 1;
        while (!char_is_identifier(*first_identifier_location))
        {
            ++first_identifier_location;
        }

        result = first_identifier_location - text;
        if (result > length)
        {
            result = length;
        }
    }

    return result;
}

static u32
make_text_jump_left(char *text, u32 index, u32 length, bool jump_by_word)
{
    u32 result = index;

    if (jump_by_word)
    {
        result = make_chunk_jump_left(text, index, length);
    }
    else
    {
        if (index)
        {
            --result;
        }
    }

    return result;
}

static u32
make_text_jump_right(char *text, u32 index, u32 length, bool jump_by_word)
{
    u32 result = index;

    if (jump_by_word)
    {
        result = make_chunk_jump_right(text, index, length);
    }
    else
    {
        ++result;
        if (result > length)
        {
            result = length;
        }
    }

    return result;
}


// @NOTE(hl): This is taken from imgui.c
static u32
crc32_lookup_table[256] =
{
    0x00000000, 0x77073096, 0xEE0E612C, 0x990951BA, 0x076DC419, 0x706AF48F, 0xE963A535, 0x9E6495A3, 0x0EDB8832, 0x79DCB8A4, 0xE0D5E91E, 0x97D2D988, 0x09B64C2B, 0x7EB17CBD, 0xE7B82D07, 0x90BF1D91,
    0x1DB71064, 0x6AB020F2, 0xF3B97148, 0x84BE41DE, 0x1ADAD47D, 0x6DDDE4EB, 0xF4D4B551, 0x83D385C7, 0x136C9856, 0x646BA8C0, 0xFD62F97A, 0x8A65C9EC, 0x14015C4F, 0x63066CD9, 0xFA0F3D63, 0x8D080DF5,
    0x3B6E20C8, 0x4C69105E, 0xD56041E4, 0xA2677172, 0x3C03E4D1, 0x4B04D447, 0xD20D85FD, 0xA50AB56B, 0x35B5A8FA, 0x42B2986C, 0xDBBBC9D6, 0xACBCF940, 0x32D86CE3, 0x45DF5C75, 0xDCD60DCF, 0xABD13D59,
    0x26D930AC, 0x51DE003A, 0xC8D75180, 0xBFD06116, 0x21B4F4B5, 0x56B3C423, 0xCFBA9599, 0xB8BDA50F, 0x2802B89E, 0x5F058808, 0xC60CD9B2, 0xB10BE924, 0x2F6F7C87, 0x58684C11, 0xC1611DAB, 0xB6662D3D,
    0x76DC4190, 0x01DB7106, 0x98D220BC, 0xEFD5102A, 0x71B18589, 0x06B6B51F, 0x9FBFE4A5, 0xE8B8D433, 0x7807C9A2, 0x0F00F934, 0x9609A88E, 0xE10E9818, 0x7F6A0DBB, 0x086D3D2D, 0x91646C97, 0xE6635C01,
    0x6B6B51F4, 0x1C6C6162, 0x856530D8, 0xF262004E, 0x6C0695ED, 0x1B01A57B, 0x8208F4C1, 0xF50FC457, 0x65B0D9C6, 0x12B7E950, 0x8BBEB8EA, 0xFCB9887C, 0x62DD1DDF, 0x15DA2D49, 0x8CD37CF3, 0xFBD44C65,
    0x4DB26158, 0x3AB551CE, 0xA3BC0074, 0xD4BB30E2, 0x4ADFA541, 0x3DD895D7, 0xA4D1C46D, 0xD3D6F4FB, 0x4369E96A, 0x346ED9FC, 0xAD678846, 0xDA60B8D0, 0x44042D73, 0x33031DE5, 0xAA0A4C5F, 0xDD0D7CC9,
    0x5005713C, 0x270241AA, 0xBE0B1010, 0xC90C2086, 0x5768B525, 0x206F85B3, 0xB966D409, 0xCE61E49F, 0x5EDEF90E, 0x29D9C998, 0xB0D09822, 0xC7D7A8B4, 0x59B33D17, 0x2EB40D81, 0xB7BD5C3B, 0xC0BA6CAD,
    0xEDB88320, 0x9ABFB3B6, 0x03B6E20C, 0x74B1D29A, 0xEAD54739, 0x9DD277AF, 0x04DB2615, 0x73DC1683, 0xE3630B12, 0x94643B84, 0x0D6D6A3E, 0x7A6A5AA8, 0xE40ECF0B, 0x9309FF9D, 0x0A00AE27, 0x7D079EB1,
    0xF00F9344, 0x8708A3D2, 0x1E01F268, 0x6906C2FE, 0xF762575D, 0x806567CB, 0x196C3671, 0x6E6B06E7, 0xFED41B76, 0x89D32BE0, 0x10DA7A5A, 0x67DD4ACC, 0xF9B9DF6F, 0x8EBEEFF9, 0x17B7BE43, 0x60B08ED5,
    0xD6D6A3E8, 0xA1D1937E, 0x38D8C2C4, 0x4FDFF252, 0xD1BB67F1, 0xA6BC5767, 0x3FB506DD, 0x48B2364B, 0xD80D2BDA, 0xAF0A1B4C, 0x36034AF6, 0x41047A60, 0xDF60EFC3, 0xA867DF55, 0x316E8EEF, 0x4669BE79,
    0xCB61B38C, 0xBC66831A, 0x256FD2A0, 0x5268E236, 0xCC0C7795, 0xBB0B4703, 0x220216B9, 0x5505262F, 0xC5BA3BBE, 0xB2BD0B28, 0x2BB45A92, 0x5CB36A04, 0xC2D7FFA7, 0xB5D0CF31, 0x2CD99E8B, 0x5BDEAE1D,
    0x9B64C2B0, 0xEC63F226, 0x756AA39C, 0x026D930A, 0x9C0906A9, 0xEB0E363F, 0x72076785, 0x05005713, 0x95BF4A82, 0xE2B87A14, 0x7BB12BAE, 0x0CB61B38, 0x92D28E9B, 0xE5D5BE0D, 0x7CDCEFB7, 0x0BDBDF21,
    0x86D3D2D4, 0xF1D4E242, 0x68DDB3F8, 0x1FDA836E, 0x81BE16CD, 0xF6B9265B, 0x6FB077E1, 0x18B74777, 0x88085AE6, 0xFF0F6A70, 0x66063BCA, 0x11010B5C, 0x8F659EFF, 0xF862AE69, 0x616BFFD3, 0x166CCF45,
    0xA00AE278, 0xD70DD2EE, 0x4E048354, 0x3903B3C2, 0xA7672661, 0xD06016F7, 0x4969474D, 0x3E6E77DB, 0xAED16A4A, 0xD9D65ADC, 0x40DF0B66, 0x37D83BF0, 0xA9BCAE53, 0xDEBB9EC5, 0x47B2CF7F, 0x30B5FFE9,
    0xBDBDF21C, 0xCABAC28A, 0x53B39330, 0x24B4A3A6, 0xBAD03605, 0xCDD70693, 0x54DE5729, 0x23D967BF, 0xB3667A2E, 0xC4614AB8, 0x5D681B02, 0x2A6F2B94, 0xB40BBE37, 0xC30C8EA1, 0x5A05DF1B, 0x2D02EF8D,
};


static u32
hash_data(void *data_init, umm data_size, u32 seed)
{
    u32 crc = ~seed;
    u8  *data = (u8 *)data_init;
    u32 *crc32_lut = crc32_lookup_table;
    while (data_size--)
    {
        crc = (crc >> 8) ^ crc32_lut[(crc & 0xFF) ^ *data++];
    }
    return ~crc;
}

static u32
hash_string(char *data_init, umm data_size, u32 seed)
{
    seed = ~seed;
    u32 crc = seed;
    u8 *data = (u8 *)data_init;
    u32 *crc32_lut = crc32_lookup_table;
    if (data_size)
    {
        while (data_size--)
        {
            u8 c = *data++;
            if (c == '#' && data_size >= 2 && data[0] == '#' && data[1] == '#')
            {
                crc = seed;
            }
            crc = (crc >> 8) ^ crc32_lut[(crc & 0xFF) ^ c];
        }
    }
    else
    {
        u8 c;
        while ((c = *data++))
        {
            if (c == '#' && data[0] == '#' && data[1] == '#')
            {
                crc = seed;
            }
            crc = (crc >> 8) ^ crc32_lut[(crc & 0xFF) ^ c];
        }
    }
    return crc;
}

// Sometimes we don't want text to be printfed and so have size,
// make a wrapper functions that checks if text should be printfed
// This is when we make child widget and don't want it to have label
static Vec2
ui_get_text_size(Font *font, char *text, f32 scale)
{
    Vec2 result = {0};
    if (*text != '$')
    {
        result = get_text_size(font, text, scale);
    }
    else
    {
        result.y = font->height * scale;
    }
    return result;
}

// Used in text input
#define ui_is_key_pressed(key, ...) ui_is_key_pressed_(key, (true, ##__VA_ARGS__))
static bool
ui_is_key_pressed_(Key key, bool repeat)
{
    bool result = false;

    f32 time_pressed = ui.keys_down_time[key];
    if (time_pressed == 0)
    {
        result = true;
    }
    else
    {
        if (repeat && time_pressed > ui.style.key_repeat_delay)
        {
            f32 half_repate_rate = ui.style.key_repeat_rate * 0.5f;

            f32 press_mod = mod32(time_pressed - ui.style.key_repeat_delay, ui.style.key_repeat_rate);
            f32 abs_mod   = mod32(time_pressed - ui.style.key_repeat_delay - ui.input->dt, ui.style.key_repeat_rate);
            if ((press_mod > half_repate_rate) != (abs_mod > half_repate_rate))
            {
                result = true;
            }
        }
    }
    return result;
}

const UIID ui_empty_id = {0};

bool
ui_id_is_valid(UIID id)
{
    bool result = id.primary_id && id.secondary_id;
    return result;
}

UIID
ui_make_id(UIWindow *window, char *text, umm count)
{
    UIID result ={0};
    if (!count)
    {
        count = strlen(text);
    }
    if (ui_id_is_valid(ui.current_pushed_id))
    {
        result.primary_id = ui.current_pushed_id.secondary_id;
        result.secondary_id = hash_string(text, count, 0);
    }
    else
    {

        if (!window)
        {
            u32 hash_value = hash_string(text, count, 0);
            result.primary_id = hash_value;
        }
        else
        {
            u32 hash_value = hash_string(text, count, 0);
            result.primary_id = window->id.primary_id;
            result.secondary_id = hash_value;
        }
    }
    return result;
}

static void
ui_push_quad(
    Vec3 v00, Vec3 v01, Vec3 v10, Vec3 v11,
    Vec4 c00, Vec4 c01, Vec4 c10, Vec4 c11,
    Vec2 uv00, Vec2 uv01, Vec2 uv10, Vec2 uv11,
    OpenGLTexture texture)
{
    assert(ui.draw_list_size + 1 <= ui.max_draw_list_size);

    RendererQueueEntry *entry = ui.draw_list + ui.draw_list_size++;
    entry->vertices[0] = v00;
    entry->vertices[1] = v01;
    entry->vertices[2] = v10;
    entry->vertices[3] = v11;
    entry->colors[0] = c00;
    entry->colors[1] = c01;
    entry->colors[2] = c10;
    entry->colors[3] = c11;
    entry->uvs[0] = uv00;
    entry->uvs[1] = uv01;
    entry->uvs[2] = uv10;
    entry->uvs[3] = uv11;
    entry->renderer_texture = texture;
}

static void
ui_push_quadr(
    Rect2 rect, Vec4 color,
    OpenGLTexture texture)
{
    Vec3 vertices[4] ={0};
    rect2_store_points(rect, &vertices[0].xy, &vertices[1].xy, &vertices[2].xy, &vertices[3].xy);

    ui_push_quad(vertices[0], vertices[1], vertices[2], vertices[3],
        color, color, color, color,
        vec2(0, 0), vec2(0, 1), vec2(1, 0), vec2(1, 1),
        texture);

}

static void
ui_push_text(Vec2 position, Vec4 color, char *text, f32 scale)
{
    // @CLEANUP this is copy of code from push_text
    Font *font = &ui.font;

    f32 line_height = font->height * scale;

    f32 rwidth  = reciprocal32(font->atlas.width);
    f32 rheight = reciprocal32(font->atlas.height);

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

            ui_push_quad(
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
ui_render_text(Vec2 position, Vec4 color,
    char *text, f32 scale)
{
    if (*text != '$')
    {
        ui_push_text(position, color, text, scale);
    }
}

void
ui_set_tooltip(char *tooltip)
{
    cstring_copy(sizeof(ui.tooltip_text), ui.tooltip_text, tooltip);
}

UIButtonState
ui_update_button(Rect2 rect, UIID id, bool repeat)
{
    UIWindow *window = ui.current_window;
    assert(window);

    bool is_hot = (ui.hot_window == window) && !ui_id_is_valid(ui.hot_id) && rect2_collide_point(rect, ui.input->mouse_pos);

    bool is_pressed = false;
    if (is_hot)
    {
        ui.hot_id = id;
        if (ui.left_mouse_button_pressed)
        {
            ui.active_id = id;
        }
        else if (repeat && ui.left_mouse_button_held && ui.active_id.packed == id.packed)
        {
            is_pressed = true;
        }
    }

    bool is_held = false;
    if (ui.active_id.packed == id.packed)
    {
        if (ui.left_mouse_button_held)
        {
            is_held = true;
        }
        else
        {
            if (is_hot)
            {
                is_pressed = true;
            }
            ui.active_id = ui_empty_id;
        }
    }

    UIButtonState state;
    state.is_held = is_held;
    state.is_hot = is_hot;
    state.is_pressed = is_pressed;
    return state;
}

static bool
ui_draw_collapse_triangle(UIWindow *window, Vec2 top_right, bool is_open)
{
    bool result = false;

    Vec2 collapse_size = vec2(16, 15);
    Vec2 collapse_padding = vec2(3, 3);
    Rect2 collapse_rect = rect2_point_sizev(vec2_sub(top_right, vec2_add(vec2(collapse_size.x, 0), collapse_padding)),
        vec2_sub(collapse_size, vec2_mul(collapse_padding, vec2s(2))));
    UIID collapse_id = ui_make_id(window, "$COLLAPSE", 0);
    UIButtonState button = ui_update_button(collapse_rect, collapse_id, false);
    Vec4 collapse_color = (button.is_held ? ui.style.color_button_active :
        button.is_hot  ? ui.style.color_button_hot    :
        ui.style.color_button);

    if (button.is_pressed)
    {
        result = true;
    }

    if (is_open)
    {
        Vec3 third_point = vec3(rect2_center_x(collapse_rect),
            collapse_rect.min.y + collapse_size.y - collapse_padding.y * 2,
            0);

        ui_push_quad(vec3_from_vec2(rect2_top_left(collapse_rect), 0), vec3_from_vec2(rect2_top_right(collapse_rect), 0), third_point, third_point, 
        collapse_color, collapse_color, collapse_color, collapse_color,
        vec2(0, 0), vec2(0, 1), vec2(1, 0), vec2(1, 1), empty_texture);
    }
    else
    {
        Vec3 third_point = vec3(collapse_rect.max.x, rect2_center_y(collapse_rect), 0);

        ui_push_quad(vec3_from_vec2(rect2_top_left(collapse_rect), 0), vec3_from_vec2(rect2_bottom_left(collapse_rect), 0), third_point, third_point,
         collapse_color, collapse_color, collapse_color, collapse_color,
         vec2(0, 0), vec2(0, 1), vec2(1, 0), vec2(1, 1), empty_texture);

    }

    return result;
}

void
ui_element_size(Vec2 size, Vec2 *adjust_start_offset)
{
    UIWindow *window = ui.current_window;
    assert(window);
    if (!window->is_collapsed)
    {
        f32 line_height = max(window->current_line_height, size.y);
        if (adjust_start_offset)
        {
            adjust_start_offset->y = adjust_start_offset->y + (line_height - size.y) * 0.5f;
        }

        window->cursor_pos_last_line = vec2(window->cursor_pos.x + size.x, window->cursor_pos.y);
        window->cursor_pos = vec2(window->rect.min.x + ui.style.window_padding.x + window->column_offset_x,
            window->cursor_pos.y + line_height + ui.style.item_spacing.y);

        Vec2 temp = vec2_add(vec2_sub(vec2(window->cursor_pos_last_line.x, window->cursor_pos.y)
            , window->rect.min), vec2(0, window->scroll_y));
        if (temp.x > window->size_content_fit.x && temp.y > window->size_content_fit.y)
        {
            window->size_content_fit = temp;
        }

        window->line_height_last_line = line_height;
        window->current_line_height = 0;
    }
}

void
ui_same_line(i32 column_x, i32 spacing_w)
{
    UIWindow *window = ui.current_window;
    assert(window);
    if (!window->is_collapsed)
    {
        f32 x;
        f32 y;
        if (column_x != 0)
        {
            if (spacing_w < 0)
            {
                spacing_w = 0;
            }
            x = window->rect.min.x + column_x + spacing_w;
            y = window->cursor_pos_last_line.y;
        }
        else
        {
            if (spacing_w < 0)
            {
                spacing_w = ui.style.item_spacing.x;
            }
            x = window->cursor_pos_last_line.x + spacing_w;
            y = window->cursor_pos_last_line.y;
        }

        window->cursor_pos = vec2(x, y);
        window->current_line_height = window->line_height_last_line;
    }
}

void
ui_text(char *text)
{
    UIWindow *window = ui.current_window;
    assert(window);
    if (!window->is_collapsed)
    {
        Vec2 text_size = ui_get_text_size(&ui.font, text, window->text_scale);
        Rect2 rect = rect2_point_sizev(window->cursor_pos, vec2_add(text_size, vec2_mul(ui.style.frame_padding, vec2s(2))));

        ui_element_size(text_size, &rect.min);
        ui_render_text(rect.min, ui.style.color_text, text, window->text_scale);
    }
}

void
ui_text_v(char *text, va_list args)
{
    UIWindow *window = ui.current_window;
    assert(window);
    if (!window->is_collapsed)
    {
        char buffer[1024];
        format_string_list(buffer, sizeof(buffer), text, args);
        ui_text(buffer);
    }
}

void
ui_text_f(char *text, ...)
{
    UIWindow *window = ui.current_window;
    assert(window);
    if (!window->is_collapsed)
    {
        va_list args;
        va_start(args, text);
        ui_text_v(text, args);
        va_end(args);
    }
}

bool
ui_button(char *label, bool repeat_when_held)
{
    bool is_pressed = false;

    UIWindow *window = ui.current_window;
    assert(window);
    if (!window->is_collapsed)
    {
        Vec2 text_size = ui_get_text_size(&ui.font, label, window->text_scale);
        Vec2 size = text_size;

        UIID id = ui_make_id(window, label, 0);

        Rect2 button_rect = rect2_point_sizev(window->cursor_pos, vec2_add(size, vec2_mul(ui.style.frame_padding, vec2s(2.0f))));
        ui_element_size(rect2_size(button_rect), 0);

        UIButtonState button = ui_update_button(button_rect, id, repeat_when_held);
        is_pressed = button.is_pressed;

        Vec4 color = (button.is_held ? ui.style.color_button_active :
            button.is_hot  ? ui.style.color_button_hot    :
            ui.style.color_button);
        ui_push_quadr(button_rect, color, empty_texture);
        ui_render_text(vec2_add(button_rect.min, ui.style.frame_padding), ui.style.color_text, label, window->text_scale);
    }

    return is_pressed;
}

bool
ui_slider_float(char *label, f32 *value,
    f32 min_value, f32 max_value, char *format)
{
    bool result = false;

    UIWindow *window = ui.current_window;
    assert(window);
    if (!window->is_collapsed)
    {
        UIID id = ui_make_id(window, label, 0);

        Vec2 label_size = ui_get_text_size(&ui.font, label, window->text_scale);
        Rect2 slider_frame_rect = rect2_point_sizev(window->cursor_pos,
            vec2_add(vec2(window->current_item_width, label_size.y),
            vec2_mul(ui.style.frame_padding, vec2s(2.0f))));
        Rect2 slider_rect = rect2v(vec2_add(slider_frame_rect.min, ui.style.frame_padding),
            vec2_sub(slider_frame_rect.max, ui.style.frame_padding));
        Rect2 widget_rect = rect2v(slider_frame_rect.min, vec2_add(slider_frame_rect.max,
            vec2(ui.style.item_spacing.x + label_size.x, 0)));

        f32 grab_button_size = 10.0f;
        f32 slider_workzone_width = rect2_width(slider_rect) - grab_button_size;
        f32 slider_workzone_min_x = slider_rect.min.x + grab_button_size * 0.5f;
        f32 slider_workzone_max_x = slider_rect.max.x - grab_button_size * 0.5f;

        f32 slider_zero_pos = (min_value < 0 ? 1.0f : 0.0f);

        UIButtonState button = ui_update_button(slider_rect, id, true);

        ui_element_size(rect2_size(widget_rect), 0);
        ui_push_quadr(slider_frame_rect, ui.style.color_widget_background, empty_texture);

        if (button.is_held)
        {
            f32 slider_pos = clamp01((ui.input->mouse_pos.x - slider_workzone_min_x) / slider_workzone_width);
            f32 new_value;
            if (slider_pos < slider_zero_pos)
            {
                f32 a = 1.0f - (slider_pos / slider_zero_pos);
                new_value = lerp(min(max_value, 0.0f), max_value, a);
            }
            else
            {
                f32 a = slider_pos;
                new_value = lerp(max(min_value, 0.0f), max_value, a);
            }

            if (*value != new_value)
            {
                *value = new_value;
                result = true;
            }
        }

        f32 value_clamped = clamp(*value, min_value, max_value);
        f32 grab_pos_normalized;
        if (value_clamped < 0.0f)
        {
            f32 f = 1.0f - (value_clamped - min_value) / (min(0.0f, max_value) - min_value);
            grab_pos_normalized = (1.0f - f) * slider_zero_pos;
        }
        else
        {
            f32 f = (value_clamped - max(0.0f, min_value)) / (max_value - max(0.0f, min_value));
            grab_pos_normalized = slider_zero_pos + f * (1.0f - slider_zero_pos);
        }

        f32 grab_x = lerp(slider_workzone_min_x, slider_workzone_max_x, grab_pos_normalized);
        Rect2 grab_rect = rect2v(vec2(grab_x - grab_button_size * 0.5f, slider_frame_rect.min.y + 2.0f),
            vec2(grab_x + grab_button_size * 0.5f, slider_frame_rect.max.y - 1.0f));
        Vec4 color = (button.is_held ? ui.style.color_button_active :
            button.is_hot  ? ui.style.color_button_hot    :
            ui.style.color_button);
        ui_push_quadr(grab_rect, color, empty_texture);

        char value_buffer[64];
        format_string(value_buffer, sizeof(value_buffer), format, *value);
        Vec2 pos = vec2(rect2_center_x(slider_rect) - get_text_width(&ui.font, value_buffer, window->text_scale) * 0.5f,
            slider_frame_rect.min.y + ui.style.frame_padding.y);
        ui_push_text(pos, ui.style.color_text, value_buffer, window->text_scale);

        ui_render_text(vec2(slider_frame_rect.max.x + ui.style.item_spacing.x, slider_rect.min.y),
            ui.style.color_text, label, window->text_scale);
    }

    return result;
}

bool
ui_input_text(char *label, umm buffer_size, char *buffer)
{
    bool result = false;

    UIWindow *window = ui.current_window;
    assert(window);
    if (!window->is_collapsed)
    {
        UIID id = ui_make_id(window, label, 0);
        f32 width = window->current_item_width;

        Vec2 label_size = ui_get_text_size(&ui.font, label, window->text_scale);
        Rect2 frame_rect = rect2_point_sizev(window->cursor_pos, vec2_add(vec2(width, label_size.y),
            vec2_mul(ui.style.frame_padding, vec2s(2.0f))));
        Rect2 widget_rect = rect2v(frame_rect.min, vec2_add(frame_rect.max,
            vec2(ui.style.item_spacing.x + label_size.x, 0.0f)));

        ui_element_size(rect2_size(widget_rect), 0);

        bool is_ctrl_down  = ui.input->modifiers[KeyboardModifier_Control];
        bool is_shift_down = ui.input->modifiers[KeyboardModifier_Shift];

        bool is_hot = (ui.hot_window == window &&
            !ui_id_is_valid(ui.hot_id) &&
            rect2_collide_point(frame_rect, ui.input->mouse_pos));

        if (is_hot)
        {
            ui.hot_id = id;
        }

        if (is_hot && ui.left_mouse_button_pressed)
        {
            if (ui.active_id.packed != id.packed)
            {
                // Initialize text edit state
                memset(&ui.text_edit_state, 0, sizeof(ui.text_edit_state));
                cstring_copy(sizeof(ui.text_edit_state.text), ui.text_edit_state.text, buffer);
                cstring_copy(sizeof(ui.text_edit_state.initial_text), ui.text_edit_state.initial_text, buffer);
            }
            ui.active_id = id;
        }
        else if (ui.left_mouse_button_pressed)
        {
            if (ui.active_id.packed == id.packed)
            {
                ui.active_id = ui_empty_id;
            }
        }

        bool is_value_changed = false;
        bool cancel_edit = false;
        bool is_enter_pressed = false;
        if (ui.active_id.packed == id.packed)
        {
            ui.text_edit_state.max_length = array_size(ui.text_edit_state.text);
            if (buffer_size < ui.text_edit_state.max_length)
            {
                ui.text_edit_state.max_length = buffer_size;
            }

            //bool is_shift_down = ui.input->modifiers[KeyboardModifier_Shift];
            //bool is_ctrl_down = ui.input->modifiers[KeyboardModifier_Control];
            u32 length = strlen(ui.text_edit_state.text);

            if (ui_is_key_pressed(Key_Enter))
            {
                ui.active_id = ui_empty_id;
                is_enter_pressed = true;
            }
            else if (ui_is_key_pressed(Key_Escape, false))
            {
                ui.active_id = ui_empty_id;
                cancel_edit = true;
            }
            else if (ui_is_key_pressed(Key_Backspace))
            {
                if (ui.text_edit_state.cursor_index)
                {
                    char *dest = ui.text_edit_state.text + ui.text_edit_state.cursor_index - 1;
                    char *source = ui.text_edit_state.text + ui.text_edit_state.cursor_index;
                    u32 length = strlen(ui.text_edit_state.text);
                    memmove(dest, source, length - ui.text_edit_state.cursor_index + 1);
                    --ui.text_edit_state.cursor_index;
                }
            }
            else if (ui_is_key_pressed(Key_Delete))
            {
                u32 length = strlen(ui.text_edit_state.text);
                if (ui.text_edit_state.cursor_index < length)
                {
                    char *dest = ui.text_edit_state.text + ui.text_edit_state.cursor_index;
                    char *source = ui.text_edit_state.text + ui.text_edit_state.cursor_index + 1;
                    memmove(dest, source, length - ui.text_edit_state.cursor_index);
                }
            }
            else if (ui_is_key_pressed(Key_Home))
            {
                ui.text_edit_state.cursor_index = 0;
            }
            else if (ui_is_key_pressed(Key_End))
            {
                ui.text_edit_state.cursor_index = strlen(ui.text_edit_state.text);
            }
            if (ui_is_key_pressed(Key_Left))
            {
                if (is_shift_down)
                {
                }
                else
                {
                    ui.text_edit_state.cursor_index = make_text_jump_left(ui.text_edit_state.text, ui.text_edit_state.cursor_index, length, is_ctrl_down);
                }
            }
            else if (ui_is_key_pressed(Key_Right))
            {
                if (is_shift_down)
                {
                }
                else
                {
                    ui.text_edit_state.cursor_index = make_text_jump_right(ui.text_edit_state.text, ui.text_edit_state.cursor_index, length, is_ctrl_down);
                }
            }
            else if (ui.input->utf32_input[0])
            {
                for (u32 index = 0;
                    index < array_size(ui.input->utf32_input);
                    ++index)
                {
                    u32 symbol = ui.input->utf32_input[index];
                    if (symbol)
                    {
                        char *buffer_end = ui.text_edit_state.text + ui.text_edit_state.max_length;
                        u32 text_size = strlen(ui.text_edit_state.text);

                        if (buffer_end - (ui.text_edit_state.text + text_size + 1) >= 1)
                        {
                            memmove(ui.text_edit_state.text + ui.text_edit_state.cursor_index + 1, 
                                ui.text_edit_state.text + ui.text_edit_state.cursor_index,
                                text_size - ui.text_edit_state.cursor_index);
                            memcpy(ui.text_edit_state.text + ui.text_edit_state.cursor_index, &symbol, 1);
                            ui.text_edit_state.text[text_size + 1] = 0;
                            ++ui.text_edit_state.cursor_index;
                        }
                    }
                }
            }

            if (cancel_edit)
            {
                format_string(buffer, buffer_size, "%s", ui.text_edit_state.initial_text);
                is_value_changed = true;
            }
            else
            {
                if (strcmp(buffer, ui.text_edit_state.text))
                {
                    format_string(buffer, buffer_size, "%s", ui.text_edit_state.text);
                    is_value_changed = true;
                }
            }
        }

        result = is_value_changed;

        ui_push_quadr(frame_rect, ui.style.color_widget_background, empty_texture);
        if (ui.active_id.packed == id.packed)
        {
            Vec2 cursor_position = vec2_add(frame_rect.min, vec2_add(ui.style.frame_padding,
                vec2(get_textn_width(&ui.font, ui.text_edit_state.text, ui.text_edit_state.cursor_index, window->text_scale), 0)));
            Rect2 cursor_rect = rect2_point_sizev(cursor_position, vec2(2, ui.font.height * window->text_scale));
            ui_push_quadr(cursor_rect, vec4(1, 1, 1, 1), empty_texture);
        }

        ui_push_text(vec2_add(frame_rect.min, ui.style.frame_padding),
            ui.style.color_text, buffer, window->text_scale);
        ui_render_text(vec2(frame_rect.max.x + ui.style.item_spacing.x, frame_rect.min.y), ui.style.color_text, label, window->text_scale);
    }

    return result;
}

void
ui_checkbox(char *label, bool *value)
{
    UIWindow *window = ui.current_window;
    assert(window);
    if (!window->is_collapsed)
    {
        UIID id = ui_make_id(window, label, 0);

        Vec2 text_size = ui_get_text_size(&ui.font, label, window->text_scale);

        Rect2 checkbox_rect = rect2_point_sizev(window->cursor_pos,
            vec2(text_size.y + ui.style.frame_padding.y * 2,
                text_size.y + ui.style.frame_padding.y * 2));
        ui_element_size(rect2_size(checkbox_rect), 0);
        ui_same_line(0, ui.style.item_spacing.x);

        Rect2 text_rect = rect2_point_sizev(vec2_add(window->cursor_pos, vec2(0, ui.style.frame_padding.y)), text_size);
        ui_element_size(rect2_size(text_rect), 0);

        ui_push_quadr(checkbox_rect, ui.style.color_widget_background, empty_texture);

        UIButtonState button = ui_update_button(checkbox_rect, id, false);

        if (button.is_hot)
        {
            ui.hot_id = id;
        }
        if (button.is_pressed)
        {
            *value = !(*value);
        }

        if (*value)
        {
            Vec2 checmark_offset = vec2(4, 4);
            Rect2 checkmark_rect = rect2v(vec2_add(checkbox_rect.min, checmark_offset), vec2_sub(checkbox_rect.max, checmark_offset));
            ui_push_quadr(checkmark_rect, ui.style.color_checkbox_active, empty_texture);
        }
        ui_render_text(text_rect.min, ui.style.color_text,
            label, window->text_scale);
    }
}

bool
ui_window(char *title, Rect2 initial_rect, bool *is_closed_ptr)
{
    bool is_open = false;

    assert(title);
    // Look for window slot
    UIWindow *window = 0;
    bool used_new_slot;

    for (u32 WindowIndex = 0;
        WindowIndex < array_size(ui.windows);
        ++WindowIndex)
    {
        UIWindow *test_window = ui.windows + WindowIndex;
        // If window is is_active (not deleted) (this is possible only if it existed last frame)
        // and its title is the same - we found already created window
        if (test_window->is_active && !strcmp(test_window->title, title))
        {
            window = test_window;
            used_new_slot = false;
            break;
        }
        // If name not matches but the window is not is_active (slot is free)
        // put window to this slot
        // Practically it means to reset window rect and title, becuse it is unlikey that
        // window that needs to always float will be deleted
        else if (!test_window->is_active)
        {
            window = test_window;
            used_new_slot = true;
            break;
        }
    }

    // If we found window slot
    if (window)
    {
        ui.current_window = window;

        window->is_active = true;

        // Set connections of double-linked list
        if (!ui.first_window)
        {
            ui.first_window = window;
            ui.last_window = window;
            window->next_window = 0;
        }
        else
        {
            ui.last_window->next_window = window;
            ui.last_window = window;
            window->next_window = 0;
        }

        // If we used new slot, we must set the new title
        if (used_new_slot)
        {
            cstring_copy(sizeof(window->title), window->title, title);
            window->rect = initial_rect;
            window->id = ui_make_id(0, title, sizeof(window->title));
            window->text_scale = 0.5f;
        }
        
        window->whole_window_rect = window->rect;
        if (is_closed_ptr)
        {
            *is_closed_ptr = window->is_active;
        }

        // Move window
        UIID MoveID = ui_make_id(window, "#MOVE", 0);
        if (ui.active_id.packed == MoveID.packed)
        {
            if (ui.left_mouse_button_held)
            {
                window->rect = rect2_move(window->rect, ui.input->mouse_delta);
            }
            else
            {
                ui.active_id = ui_empty_id;
            }
        }

        Rect2 window_rect = window->rect;
        Rect2 title_bar_rect = rect2_point_size(window_rect.min.x, window_rect.min.y,
            rect2_width(window_rect), ui.style.window_title_bar_height);
        window_rect.min.y += ui.style.window_title_bar_height;
        ui_push_quadr(title_bar_rect, ui.style.color_window_titlebar, empty_texture);
        ui_push_text(vec2_add(title_bar_rect.min, vec2(2, 2 - ui.style.window_title_bar_height)),
            ui.style.color_text, window->title, 1.0f);
        window->title_bar_rect = title_bar_rect;

        if (!window->is_collapsed)
        {
            Vec4 color = ui.style.color_window_background;
            ui_push_quadr(window_rect, color, empty_texture);
        }

        if (!window->is_collapsed)
        {
            Vec2 resize_size = vec2(16, 16);
            Rect2 resize_rect = rect2_point_sizev(vec2_sub(rect2_bottom_right(window->rect), resize_size), resize_size);
            UIID resize_id = ui_make_id(window, "#RESIZE", 0);
            UIButtonState button = ui_update_button(resize_rect, resize_id, true);
            Vec4 resize_color = (button.is_held ? ui.style.color_button_active :
                button.is_hot  ? ui.style.color_button_hot    :
                ui.style.color_button);
            Rect2 new_rect = window->rect;
            if (button.is_held)
            {
                new_rect.max = vec2_add(new_rect.max, ui.input->mouse_delta);
                if (rect2_width(new_rect) < ui.style.min_window_size.x)
                {
                    new_rect.max.x = new_rect.min.x + ui.style.min_window_size.x;
                }
                if (rect2_height(new_rect) < ui.style.min_window_size.y)
                {
                    new_rect.max.y = new_rect.min.y + ui.style.min_window_size.y;
                }
            }
            window->rect = new_rect;
            ui_push_quad(
                vec3_from_vec2(rect2_top_right(resize_rect), 0), vec3_from_vec2(rect2_bottom_left(resize_rect), 0),
                vec3_from_vec2(rect2_top_right(resize_rect), 0), vec3_from_vec2(rect2_bottom_right(resize_rect), 0),
                resize_color, resize_color, resize_color, resize_color,
                vec2(0, 0), vec2(0, 1), vec2(1, 0), vec2(1, 1),
                empty_texture);
        }

        if (ui_draw_collapse_triangle(window, rect2_top_right(window->title_bar_rect), window->is_collapsed))
        {
            window->is_collapsed = !window->is_collapsed;
        }

        window->scroll_y = window->next_scroll_y;
        window->scroll_y = max(window->scroll_y, 0.0f);
        if (!window->is_collapsed)
        {
            window->scroll_y = min(window->scroll_y, max(0.0f, window->size_content_fit.y - rect2_height(window->rect)));
        }
        window->next_scroll_y = window->scroll_y;

        Rect2 scrollbar_rect = rect2(window->rect.max.x - ui.style.scroll_bar_width,
            window->title_bar_rect.max.y,
            window->rect.max.x,
            window->rect.max.y);

        f32 grab_height_normalized = clamp01(rect2_height(window->rect) / max(window->size_content_fit.y, rect2_height(window->rect)));
        f32 grab_height = rect2_height(scrollbar_rect) * grab_height_normalized;

        UIButtonState button ={0};
        if (grab_height_normalized < 1.0f)
        {
            UIID scrollbar_id = ui_make_id(window, "$SCROLLBARY", 0);
            button = ui_update_button(scrollbar_rect, scrollbar_id, true);
            if (button.is_held)
            {
                ui.hot_id = scrollbar_id;
                f32 grab_y_normalized = clamp01((ui.input->mouse_pos.y - (scrollbar_rect.min.y + grab_height * 0.5f))
                    / (rect2_height(scrollbar_rect) - grab_height))
                    * (1.0f - grab_height_normalized);
                window->scroll_y = grab_y_normalized * window->size_content_fit.y;
                window->next_scroll_y = window->scroll_y;
            }


            f32 grab_y_normalized = clamp01(window->scroll_y / max(0.0f, window->size_content_fit.y));
            Vec4 color = (button.is_held ? ui.style.color_button_active :
                button.is_hot  ? ui.style.color_button_hot    :
                ui.style.color_button);
            Rect2 grab_rect = rect2v(vec2(scrollbar_rect.min.x, lerp(scrollbar_rect.min.y, scrollbar_rect.max.y, grab_y_normalized)),
                vec2(scrollbar_rect.max.x, lerp(scrollbar_rect.min.y, scrollbar_rect.max.y, grab_y_normalized + grab_height_normalized)));
            ui_push_quadr(scrollbar_rect, ui.style.color_widget_background, empty_texture);
            ui_push_quadr(grab_rect, color, empty_texture);
        }

        window->cursor_pos = window->rect.min;
        window->cursor_pos.y += ui.style.window_title_bar_height;
        window->cursor_pos = vec2_add(window->cursor_pos, ui.style.window_padding);
        window->cursor_pos.y -= window->scroll_y;
        window->cursor_pos_last_line = window->cursor_pos;
        window->item_width_default = rect2_width(window->rect) * 0.65f;
        window->current_item_width = window->item_width_default;
        window->line_height_last_line = window->current_line_height = 0;

        Rect2 clip_rect = window->rect;
        push_clip_rect(ui.renderer, clip_rect);
    }
    else
    {
        printf("Not enough window slots for window %s!", title);
        assert(false);
    }

    return is_open;
}

void
ui_end_window()
{
    if (!ui_id_is_valid(ui.active_id) &&
        !ui_id_is_valid(ui.hot_id) &&
        rect2_collide_point(ui.current_window->title_bar_rect, ui.input->mouse_pos) &&
        ui.left_mouse_button_pressed)
    {
        ui.active_id = ui_make_id(ui.current_window, "#MOVE", 0);
    }
    pop_clip_rect(ui.renderer);
    ui.current_window = 0;
}

void
ui_init(struct OpenGLRenderer *renderer, Input *input, Font font)
{
    ui.style.font_height = 32;
    ui.style.key_repeat_delay = 0.25;
    ui.style.key_repeat_rate =  0.20;
    ui.style.min_text_scale = 0.5;
    ui.style.max_text_scale = 2.0;
    ui.style.mouse_double_click_time = 0.30;
    ui.style.mouse_wheel_font_scale_multiplier = 0.1;
    ui.style.window_title_bar_height = 20.0;
    ui.style.scroll_bar_width =        20.0;
    ui.style.tree_node_spacing =       22.0;
    ui.style.min_window_size =         vec2(100.0, 40.0);
    ui.style.item_spacing =            vec2(10.0 ,  5.0);
    ui.style.frame_padding =           vec2(5.0  ,  4.0);
    ui.style.window_padding =          vec2(8.0  ,  8.0);
    ui.style.color_text =              vec4(0.860, 0.930, 0.890, 0.780);
    ui.style.color_window_background = vec4(0.130, 0.140, 0.170, 1.000);
    ui.style.color_window_titlebar =   vec4(0.230, 0.200, 0.270, 1.000);
    ui.style.color_widget_background = vec4(0.200, 0.220, 0.270, 1.000);
    ui.style.color_checkbox_active =   vec4(0.710, 0.220, 0.270, 1.000);
    ui.style.color_button =            vec4(0.470, 0.770, 0.830, 0.140);
    ui.style.color_button_hot =        vec4(0.460, 0.200, 0.300, 0.860);
    ui.style.color_button_active =     vec4(0.460, 0.200, 0.300, 1.000);
    ui.style.color_header =            vec4(0.460, 0.200, 0.300, 0.760);
    ui.style.color_header_hot =        vec4(0.500, 0.080, 0.260, 1.000);
    ui.style.color_header_active =     vec4(0.460, 0.200, 0.300, 0.860);

    ui.renderer = renderer;
    ui.input = input;
    ui.font = font;

    u64 draw_buffer_size = sizeof(RendererQueueEntry) * 4096;
    ui.draw_pool = memory_pool(malloc(draw_buffer_size), draw_buffer_size);
}

void
ui_begin_frame()
{
    memory_pool_clear(&ui.draw_pool);
    ui.max_draw_list_size = ui.draw_pool.size / sizeof(RendererQueueEntry);
    ui.draw_list = memory_pool_alloc_array(&ui.draw_pool, RendererQueueEntry, ui.max_draw_list_size);
    ui.draw_list_size = 0;

    // Set key down time for continious text input
    for (u32 key_index = 0;
        key_index < Key_Count;
        ++key_index)
    {
        if (is_key_pressed(ui.input->keys[key_index]))
        {
            if (ui.keys_down_time[key_index] < 0)
            {
                ui.keys_down_time[key_index] = 0;
            }
            else
            {
                ui.keys_down_time[key_index] += ui.input->dt;
            }
        }
        else
        {
            ui.keys_down_time[key_index] = -1;
        }
    }


    ui.left_mouse_button_double_clicked = false;
    if (is_key_pressed(ui.input->keys[Key_MouseLeft], false))
    {
        if (ui.input->time - ui.left_mouse_button_clicked_time < ui.style.mouse_double_click_time)
        {
            ui.left_mouse_button_double_clicked = true;
            ui.left_mouse_button_clicked_time   = F32_MIN;
        }
        else
        {
            ui.left_mouse_button_clicked_time = ui.input->time;
        }
    }

    ui.left_mouse_button_pressed = is_key_pressed(ui.input->keys[Key_MouseLeft], false);
    ui.left_mouse_button_held    = is_key_pressed(ui.input->keys[Key_MouseLeft], true);

    ui.first_window = 0;
    ui.current_window = 0;
    // @NOTE(hl): We can zero only first character, so the length of string is zero
    ui.tooltip_text[0] = 0;
    // Find hovered window
    ui.hot_id = ui_empty_id;
    ui.current_pushed_id = ui_empty_id;
    ui.hot_window = 0;
    for (u32 window_index = 0;
        window_index < array_size(ui.windows);
        ++window_index)
    {
        UIWindow *window = ui.windows + window_index;
        if (window->is_active && !window->is_collapsed && rect2_collide_point(window->whole_window_rect, ui.input->mouse_pos))
        {
            ui.hot_window = window;
            // break;
        }
    }

    if (ui.hot_window && ui.input->mouse_wheel)
    {
        UIWindow *window = ui.hot_window;
        if (ui.input->modifiers[KeyboardModifier_Control])
        {
            f32 new_text_scale = clamp(window->text_scale +
                (f32)ui.input->mouse_wheel * ui.style.mouse_wheel_font_scale_multiplier,
                ui.style.min_text_scale, ui.style.max_text_scale);
            f32 scale = new_text_scale / window->text_scale;
            window->text_scale = new_text_scale;
            Vec2 offset = vec2_div(vec2_mul(vec2_mul(rect2_size(window->rect), vec2s(1.0f - scale)), vec2_sub(ui.input->mouse_pos, window->rect.min)), rect2_size(window->rect));
            window->rect.min = vec2_add(window->rect.min, offset);
            window->rect.max.x = window->rect.min.x + rect2_width(window->rect) * scale;
            window->rect.max.y = window->rect.min.y + rect2_height(window->rect) * scale;
        }
    }
}

void
ui_end_frame()
{
    push_projection(ui.renderer, mat4x4_orthographic2d(0, ui.renderer->display_size.x, ui.renderer->display_size.y, 0));
    push_view(ui.renderer, mat4x4_identity());
    
    for(u32 i = 0; 
        i < ui.draw_list_size;
        ++i)
    {
        RendererQueueEntry *entry = ui.draw_list + i;

        push_quad(ui.renderer, entry->vertices[0], entry->vertices[1], entry->vertices[2], entry->vertices[3], entry->colors[0], entry->colors[1], entry->colors[2], entry->colors[3], entry->uvs[0], entry->uvs[1], entry->uvs[2], entry->uvs[3], entry->renderer_texture);
    }
    
    pop_projection(ui.renderer);
    pop_view(ui.renderer);
}
