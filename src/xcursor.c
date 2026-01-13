#include "xcursor.h"

#include <stdint.h>
#include <xcb/xcb_cursor.h>
#include <xcb/xproto.h>

#include "base.h"
#include "utils.h"
#include "wm.h"

static const char *const xcursor_font[] = {
  [XC_X_cursor] = "X_cursor",
  [XC_arrow] = "arrow",
  [XC_based_arrow_down] = "based_arrow_down",
  [XC_based_arrow_up] = "based_arrow_up",
  [XC_boat] = "boat",
  [XC_bogosity] = "bogosity",
  [XC_bottom_left_corner] = "bottom_left_corner",
  [XC_bottom_right_corner] = "bottom_right_corner",
  [XC_bottom_side] = "bottom_side",
  [XC_bottom_tee] = "bottom_tee",
  [XC_box_spiral] = "box_spiral",
  [XC_center_ptr] = "center_ptr",
  [XC_circle] = "circle",
  [XC_clock] = "clock",
  [XC_coffee_mug] = "coffee_mug",
  [XC_cross] = "cross",
  [XC_cross_reverse] = "cross_reverse",
  [XC_crosshair] = "crosshair",
  [XC_diamond_cross] = "diamond_cross",
  [XC_dot] = "dot",
  [XC_dotbox] = "dotbox",
  [XC_double_arrow] = "double_arrow",
  [XC_draft_large] = "draft_large",
  [XC_draft_small] = "draft_small",
  [XC_draped_box] = "draped_box",
  [XC_exchange] = "exchange",
  [XC_fleur] = "fleur",
  [XC_gobbler] = "gobbler",
  [XC_gumby] = "gumby",
  [XC_hand1] = "hand1",
  [XC_hand2] = "hand2",
  [XC_heart] = "heart",
  [XC_icon] = "icon",
  [XC_iron_cross] = "iron_cross",
  [XC_left_ptr] = "left_ptr",
  [XC_left_side] = "left_side",
  [XC_left_tee] = "left_tee",
  [XC_leftbutton] = "leftbutton",
  [XC_ll_angle] = "ll_angle",
  [XC_lr_angle] = "lr_angle",
  [XC_man] = "man",
  [XC_middlebutton] = "middlebutton",
  [XC_mouse] = "mouse",
  [XC_pencil] = "pencil",
  [XC_pirate] = "pirate",
  [XC_plus] = "plus",
  [XC_question_arrow] = "question_arrow",
  [XC_right_ptr] = "right_ptr",
  [XC_right_side] = "right_side",
  [XC_right_tee] = "right_tee",
  [XC_rightbutton] = "rightbutton",
  [XC_rtl_logo] = "rtl_logo",
  [XC_sailboat] = "sailboat",
  [XC_sb_down_arrow] = "sb_down_arrow",
  [XC_sb_h_double_arrow] = "sb_h_double_arrow",
  [XC_sb_left_arrow] = "sb_left_arrow",
  [XC_sb_right_arrow] = "sb_right_arrow",
  [XC_sb_up_arrow] = "sb_up_arrow",
  [XC_sb_v_double_arrow] = "sb_v_double_arrow",
  [XC_shuttle] = "shuttle",
  [XC_sizing] = "sizing",
  [XC_spider] = "spider",
  [XC_spraycan] = "spraycan",
  [XC_star] = "star",
  [XC_target] = "target",
  [XC_tcross] = "tcross",
  [XC_top_left_arrow] = "top_left_arrow",
  [XC_top_left_corner] = "top_left_corner",
  [XC_top_right_corner] = "top_right_corner",
  [XC_top_side] = "top_side",
  [XC_top_tee] = "top_tee",
  [XC_trek] = "trek",
  [XC_ul_angle] = "ul_angle",
  [XC_umbrella] = "umbrella",
  [XC_ur_angle] = "ur_angle",
  [XC_watch] = "watch",
  [XC_xterm] = "xterm",
};

static xcb_cursor_context_t *get_xcb_cursor_context(void) {
  static xcb_cursor_context_t *ctx = nullptr;
  if (ctx) return ctx;

  if (xcb_cursor_context_new(wm.xcb_conn, wm.screen, &ctx) < 0) {
    return nullptr;
  }

  return ctx;
}

static const char *xcursor_font_tostr(uint16_t c) {
  if (c < (uint16_t)countof(xcursor_font)) {
    return xcursor_font[c];
  }
  return nullptr;
}

static xcb_cursor_t cursor_list[countof(xcursor_font)];
static xcb_cursor_context_t *cursor_ctx = nullptr;

xcb_cursor_t xcursor_get_xcb_cursor(cursor_t cursor) {
  if (!cursor_ctx) cursor_ctx = get_xcb_cursor_context();
  if (!cursor_ctx) fatal("cannot get cursor");

  if (!cursor_list[cursor]) {
    const char *name = xcursor_font_tostr(cursor);
    cursor_list[cursor] = xcb_cursor_load_cursor(cursor_ctx, name);
  }

  return cursor_list[cursor];
}

void xcursor_clean(void) {
  for (int i = 0; i < countof(xcursor_font); i++) {
    xcb_cursor_t cursor = cursor_list[i];
    if (cursor) xcb_free_cursor(wm.xcb_conn, cursor);
  }
  if (cursor_ctx) xcb_cursor_context_free(cursor_ctx);
}

point_t xcursor_query_pointer_position(void) {
  xcb_query_pointer_cookie_t cookie =
    xcb_query_pointer(wm.xcb_conn, wm.screen->root);
  xcb_query_pointer_reply_t *reply =
    xcb_query_pointer_reply(wm.xcb_conn, cookie, nullptr);
  point_t pos = {.x = reply->root_x, .y = reply->root_y};
  p_delete(&reply);
  return pos;
}

void xcursor_set_pointer_position(point_t point) {
  xcb_window_t window = XCB_WINDOW_NONE;
  xcb_window_t root = wm.screen->root;
  xcb_warp_pointer(wm.xcb_conn, window, root, 0, 0, 1, 1, point.x, point.y);
}
