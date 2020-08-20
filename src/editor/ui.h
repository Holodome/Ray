#if !defined(UI_H)

#include "lib/common.h"
#include "lib/memory_pool.h"
#include "lib/ray_math.h"
#include "lib/sys.h"


#include "font.h"

typedef struct {
  Vec3 vertices[4];               // 48
  Vec4 colors[4];                 // 64
  Vec2 uvs[4];                    // 32
  OpenGLTexture renderer_texture; // 1
} RendererQueueEntry;
// Total size 145 bytes

// ID struct using which we can identify different widgets.
// Secondary id is hashed value of string passed to widget creation
// this means that in one window all widgets must have different names
// Primary id is window id.
typedef union {
  struct {
    u32 primary_id;   // Parent id
    u32 secondary_id; // Child id
  };
  u64 packed;
} UIID;

typedef struct UIWindow {
  char title[32];
  UIID id;

  u32 flags;
  // Current text scale. Can be changed if ctrl+mousewheel is used
  f32 text_scale;

  // Rect, where widgets are placed. If window has no title bar - whole window
  Rect2 rect;
  Rect2 title_bar_rect;
  // Rect and title bar rect
  Rect2 whole_window_rect;

  // What is the next widget pos
  Vec2 cursor_pos;
  // Used in same_line to go back to previous line
  Vec2 cursor_pos_last_line;
  f32 item_width_default;
  // When going to previous line, we must restore its height
  f32 line_height_last_line;
  f32 current_line_height;
  // Item width is universal value that is used in sliders, text inputs ...
  f32 current_item_width;
  f32 column_offset_x;
  Vec2 size_content_fit;

  f32 scroll_y;
  f32 next_scroll_y;
  // Mark that window is alive, and its place in window list cannot be reused
  bool is_active;
  // If collapse button has not been pressed
  bool is_collapsed;
  struct UIWindow *next_window;
} UIWindow;

typedef struct {
  f32 font_height;

  f32 key_repeat_delay;
  f32 key_repeat_rate;
  f32 min_text_scale;
  f32 max_text_scale;
  f32 mouse_double_click_time;
  f32 mouse_wheel_font_scale_multiplier;

  Vec2 min_window_size;
  Vec2 window_resize_button_size;
  // Size of padding of buttons, text inputs etc. between its contents and
  // border
  Vec2 frame_padding;
  // Distance between widgets
  Vec2 item_spacing;
  // Distance from window border, when its contents should start
  Vec2 window_padding;
  // _tab_ width in tree views
  f32 tree_node_spacing;
  f32 window_title_bar_height;
  f32 scroll_bar_width;

  Vec4 color_text;
  Vec4 color_window_background;
  Vec4 color_window_titlebar;
  Vec4 color_widget_background;
  Vec4 color_checkbox_active;
  Vec4 color_button;
  Vec4 color_button_hot;
  Vec4 color_button_active;
  Vec4 color_header;
  Vec4 color_header_hot;
  Vec4 color_header_active;
} UISettings;

typedef struct {
  char text[1024];
  char initial_text[1024];

  // min(arra_size(text), buffer_size)
  u32 max_length;
  // Index of selected char
  u32 cursor_index;

  bool has_selection;
  u32 selection_start;
  u32 selection_end;
} UITextEditState;

typedef struct {
  UISettings style;

  struct OpenGLRenderer *renderer;
  Input *input;

  // Immediate mode GUIs allow us to draw gui elements directly in
  // widget function calls. However, we shouldn't do that, so we can make calls
  // to gui across all the frame, and widgets won't overlap game graphics
  // because will be drawn after game renderering is ended. We pack whole draw
  // call data and store it in an array, then draw it at the end of gui frame
  MemoryPool draw_pool;
  RendererQueueEntry *draw_list;
  u32 draw_list_size;
  u32 max_draw_list_size;

  // How many seconds is key being held? - used in text input to make continious
  // input look realistic
  f32 keys_down_time[Key_Count];
  // Left mouse button state data
  f32 left_mouse_button_clicked_time;
  bool left_mouse_button_double_clicked;
  bool left_mouse_button_held;
  bool left_mouse_button_pressed;

  // Font info stored here as pointers, so we don't ask asset system too much
  // about it
  Font font;
  // Tooltip is some  text rendered next to mouse cursor
  // @TODO(hl): We probably want to expand it, so we can do multiline or smth
  char tooltip_text[128];

  UIWindow windows[16];
  UIWindow *current_window;
  UIWindow *hot_window;
  UIWindow *first_window;
  UIWindow *last_window;

  // Logically there can be only one widget that we interact with at one time
  // - it is stored at active_id
  UIID hot_id;
  UIID active_id;
  // ID that is pushed by some parent widget, so its children don't generate
  // their own Currenlty this is handled very easy, so some widget that has
  // childern pushes id in its function and removes it, but we could make RAII
  // wrapper for it Also, this works as: Some parent widget has id, which is
  // (parent, widget) id is pushed here, and children will get (widget, child)
  // so we go deeper to the id hirearchy. This works if we don't repeate childer
  // ids
  UIID current_pushed_id;
  // Logically we can interact with only one text edit
  UITextEditState text_edit_state;
} UI;

//
// Data that is returned on update button call

typedef struct {
  bool is_pressed;
  bool is_hot;
  bool is_held;
} UIButtonState;

extern UI ui;

extern const UIID ui_empty_id;

bool ui_id_is_valid(UIID id);
UIID ui_make_id(UIWindow *window, char *text, u64 count);
void ui_set_tooltip(char *tooltip);

UIButtonState ui_update_button(Rect2 rect, UIID id, bool repeat);
void ui_element_size(Vec2 size, Vec2 *adjust_start_offset);
void ui_same_line(i32 column_x, i32 spacing_w);
void ui_text(char *text);
void ui_text_v(char *text, va_list args);
void ui_text_f(char *text, ...);
bool ui_button(char *label, bool repeat_when_held);
bool ui_slider_float(char *label, f32 *value, f32 min_value, f32 max_value,
                     char *format);
bool ui_input_text(char *label, umm buffer_size, char *buffer);
void ui_checkbox(char *label, bool *value);
bool ui_window(char *title, Rect2 initial_rect, bool *is_closed_ptr);
void ui_end_window();

void ui_init(struct OpenGLRenderer *renderer, Input *input, Font font);
void ui_begin_frame();
void ui_end_frame();

inline bool char_is_identifier(u32 c) {
  return ((c == '_') || isalpha(c) || isdigit(c));
}
inline bool char_is_hex(u32 c) {
  return (isdigit(c) || (('A' <= c) && (c <= 'F')) ||
          (('a' <= c) && (c <= 'f')));
}

#define UI_H 1
#endif