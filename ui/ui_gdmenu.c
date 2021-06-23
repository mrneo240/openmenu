/*
 * File: ui_gdmenu.c
 * Project: ui
 * File Created: Sunday, 13th June 2021 12:32:47 pm
 * Author: Hayden Kowalchuk
 * -----
 * Copyright (c) 2021 Hayden Kowalchuk, Hayden Kowalchuk
 * License: BSD 3-clause "New" or "Revised" License, http://www.opensource.org/licenses/BSD-3-Clause
 */

#include "ui_gdmenu.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../backend/gd_item.h"
#include "../backend/gd_list.h"
#include "../texture/txr_manager.h"
#include "draw_prototypes.h"
#include "font_prototypes.h"

#define UNUSED __attribute__((unused))

static image txr_bg_left, txr_bg_right;

/* Info taken from megavolt85 and RazorX */
/* GDMENU Default Colors */
static const uint32_t UNUSED color_window_title = PVR_PACK_ARGB(255, 103, 193, 245);
static const uint32_t UNUSED color_options_default = PVR_PACK_ARGB(255, 164, 158, 152);
static const uint32_t UNUSED color_options_highlight = PVR_PACK_ARGB(255, 103, 193, 245);
static const uint32_t UNUSED color_options_save = PVR_PACK_ARGB(255, 152, 158, 164);
static const uint32_t UNUSED color_sysinfo_default = PVR_PACK_ARGB(255, 152, 158, 164);
static const uint32_t UNUSED color_about_default = PVR_PACK_ARGB(255, 152, 158, 164);
static const uint32_t UNUSED color_exit_default = PVR_PACK_ARGB(255, 152, 158, 164);
static const uint32_t UNUSED color_exit_highlight = PVR_PACK_ARGB(255, 103, 193, 245);
static const uint32_t UNUSED color_cb_default = PVR_PACK_ARGB(255, 152, 158, 164);
static const uint32_t UNUSED color_cb_highlight = PVR_PACK_ARGB(255, 103, 193, 245);
static const uint32_t UNUSED color_main_default = PVR_PACK_ARGB(255, 152, 158, 164);
static const uint32_t UNUSED color_main_highlight = PVR_PACK_ARGB(255, 103, 193, 245);
static const uint32_t UNUSED color_update_default = PVR_PACK_ARGB(255, 152, 158, 164);
static const uint32_t UNUSED color_image_info_default = PVR_PACK_ARGB(255, 152, 158, 164);
static const uint32_t UNUSED color_image_name_default = PVR_PACK_ARGB(255, 152, 158, 164);
static const uint32_t UNUSED color_image_name_highlight = PVR_PACK_ARGB(255, 103, 193, 245);
static const uint32_t UNUSED color_cursor_default = PVR_PACK_ARGB(255, 33, 56, 82); /* unsure */
static const uint32_t UNUSED color_cursor_actual = PVR_PACK_ARGB(255, 29, 44, 66);

/* GDMENU Default positions */
static const int cursor_width = 404;
static const int cursor_height = 20;
static const int items_per_page = 20;

static const int pos_gameslist_x = 3;
static const int pos_gameslist_y = 14;

static const int pos_gameinfo_x = 424;
static const int pos_gameinfo_region_y = 85;
static const int pos_gameinfo_vga_y = 109;
static const int pos_gameinfo_disc_y = 133;
static const int pos_gameinfo_date_y = 157;
static const int pos_gameinfo_version_y = 181;

static const int pos_gametxr_x = 420;
static const int pos_gametxr_y = 213;

/* GDMENU Strings */
/* English */
#define INFO_STR_LEN (25) /* Amount of characters needed to fill space */
#define STR_REGION ("REGION")
#define STR_FREE ("FREE")
#define STR_NTSC_J ("NTSC-J")
#define STR_NTSC_U ("NTSC-U")
#define STR_PAL ("PAL")
#define STR_VGA ("VGA")
#define STR_DISC ("DISC")
#define STR_DATE ("DATE")
#define STR_VERSION ("VERSION")

#define STR_YES ("YES")
#define STR_NO ("NO")
#define STR_ON ("ON")
#define STR_OFF ("OFF")

/* Our actual gdemu items */
static const gd_item **list_current;
static int list_len;
enum sort_type { DEFAULT,
                 ALPHA,
                 SORT_END };
static enum sort_type sort_current = DEFAULT;

static image txr_focus;
extern image img_empty_boxart;

#define INPUT_TIMEOUT (5)

static int current_selected_item = 0;
static int current_starting_index = 0;
static int navigate_timeout = INPUT_TIMEOUT;

static void draw_bg_layers(void) {
  {
    const dimen_RECT left = {.x = 0, .y = 0, .w = 512, .h = 480};
    draw_draw_sub_image(0, 0, 512, 480, COLOR_WHITE, &txr_bg_left, &left);
  }
  {
    const dimen_RECT right = {.x = 0, .y = 0, .w = 128, .h = 480};
    draw_draw_sub_image(512, 0, 128, 480, COLOR_WHITE, &txr_bg_right, &right);
  }
}

static void draw_gamelist(void) {
  char buffer[128];
  const int X_ADJUST_TEXT = 7;
  const int Y_ADJUST_TEXT = 4;
  const int Y_ADJUST_CRSR = 3; /* 2 pixels higher than text */
  font_bmp_begin_draw();
  for (int i = 0; i < items_per_page; i++) {
    /* Break before issues */
    if ((current_starting_index + i) >= list_len) {
      break;
    }

    sprintf(buffer, "%02d %s", current_starting_index + i + 1, list_current[current_starting_index + i]->name);
    if ((current_starting_index + i) == current_selected_item) {
      draw_draw_quad(pos_gameslist_x, pos_gameslist_y + Y_ADJUST_TEXT + (i * 21) - Y_ADJUST_CRSR, cursor_width, cursor_height, color_cursor_actual);
      font_bmp_set_color(color_main_highlight);
    } else {
      font_bmp_set_color(color_main_default);
    }

    font_bmp_draw_main(pos_gameslist_x + X_ADJUST_TEXT, pos_gameslist_y + Y_ADJUST_TEXT + (i * 21), buffer);
  }
}

static const char *transform_date_readable(char out[11], const char *in) {
  memcpy(out, in, 4);
  out[4] = '/';
  memcpy(out + 5, in + 4, 2);
  out[7] = '/';
  memcpy(out + 8, in + 6, 2);
  out[10] = '\0';
  return out;
}

static void string_outer_concat(char *out, const char *left, const char *right, int len) {
  const int input_len = strlen(left) + strlen(right);
  strcpy(out, left);
  for (int i = 0; i < len - input_len; i++)
    strcat(out, " ");
  strcat(out, right);
}

static const char *region_code_to_readable(const char *in) {
  if (strcmp(in, "JUE") == 0) {
    return STR_FREE;
  }
  switch (in[0]) {
    case 'J':
      return STR_NTSC_J;
    case 'U':
      return STR_NTSC_U;
    case 'E':
      return STR_PAL;
  }
  return STR_FREE;
}

static void draw_gameinfo(void) {
  char line_buf[26];
  char date_buf[11];

  font_bmp_begin_draw();
  font_bmp_set_color(color_image_name_highlight); /* Unsure */
  // Region
  string_outer_concat(line_buf, STR_REGION, region_code_to_readable(list_current[current_selected_item]->region), INFO_STR_LEN);
  font_bmp_draw_main(pos_gameinfo_x, pos_gameinfo_region_y, line_buf);
  // VGA
  string_outer_concat(line_buf, STR_VGA, (list_current[current_selected_item]->vga[0] == '1' ? STR_YES : STR_NO), INFO_STR_LEN);
  font_bmp_draw_main(pos_gameinfo_x, pos_gameinfo_vga_y, line_buf);
  // DISC
  string_outer_concat(line_buf, STR_DISC, list_current[current_selected_item]->disc, INFO_STR_LEN);
  font_bmp_draw_main(pos_gameinfo_x, pos_gameinfo_disc_y, line_buf);
  // DATE
  string_outer_concat(line_buf, STR_DATE, transform_date_readable(date_buf, list_current[current_selected_item]->date), INFO_STR_LEN);
  font_bmp_draw_main(pos_gameinfo_x, pos_gameinfo_date_y, line_buf);
  // VERSION
  string_outer_concat(line_buf, STR_VERSION, list_current[current_selected_item]->version, INFO_STR_LEN);
  font_bmp_draw_main(pos_gameinfo_x, pos_gameinfo_version_y, line_buf);
}

static void draw_gameart(void) {
  txr_get_large(list_current[current_selected_item]->product, &txr_focus);
  if (txr_focus.texture == img_empty_boxart.texture) {
    txr_get_small(list_current[current_selected_item]->product, &txr_focus);
  }
  if (txr_focus.texture == img_empty_boxart.texture) {
    /* Only draw if image is present */
    return;
  }

  draw_draw_image(pos_gametxr_x, pos_gametxr_y, 210, 210, COLOR_WHITE, &txr_focus);
}

static void menu_decrement(void) {
  if (navigate_timeout > 0) {
    navigate_timeout--;
    return;
  }
  if (current_selected_item > 0) {
    current_selected_item--;
  }
  if (current_selected_item < current_starting_index) {
    current_starting_index--;
    if (current_starting_index < 0) {
      current_starting_index = 0;
    }
  }
  navigate_timeout = INPUT_TIMEOUT;
}

static void menu_increment(void) {
  if (navigate_timeout > 0) {
    navigate_timeout--;
    return;
  }

  if (++current_selected_item >= list_len) {
    current_selected_item = list_len - 1;
  }
  if (current_selected_item >= current_starting_index + items_per_page) {
    current_starting_index++;
  }
  navigate_timeout = INPUT_TIMEOUT;
}

static void menu_swap_sort(void) {
  if (navigate_timeout > 0) {
    navigate_timeout--;
    return;
  }
  sort_current++;
  if (sort_current == SORT_END) {
    sort_current = DEFAULT;
  }
  switch (sort_current) {
    case ALPHA:
      list_current = list_get_sort_name();
      break;
    case DEFAULT:
    default:
      list_current = list_get_sort_default();
      break;
  }
  current_selected_item = 0;
  current_starting_index = 0;
  navigate_timeout = INPUT_TIMEOUT * 2;
}

static void menu_accept(void) {
  dreamcast_rungd(list_current[current_selected_item]->slot_num);
}

FUNCTION(UI_NAME, init) {
  texman_clear();

  unsigned int temp = texman_create();
  draw_load_texture_buffer("THEME/GDMENU/BG_L.PVR", &txr_bg_left, texman_get_tex_data(temp));
  texman_reserve_memory(txr_bg_left.width, txr_bg_left.height, 2 /* 16Bit */);

  temp = texman_create();
  draw_load_texture_buffer("THEME/GDMENU/BG_R.PVR", &txr_bg_right, texman_get_tex_data(temp));
  texman_reserve_memory(txr_bg_right.width, txr_bg_right.height, 2 /* 16Bit */);

  font_bmp_init("FONT/GDMNUFNT.PVR", 8, 16);

  printf("Texture scratch free: %d/%d KB (%d/%d bytes)\n", texman_get_space_available() / 1024, (1024 * 1024) / 1024, texman_get_space_available(), (1024 * 1024));
}

FUNCTION(UI_NAME, setup) {
  list_current = list_get();
  list_len = list_length();

  current_selected_item = 0;
  sort_current = DEFAULT;
}

FUNCTION_INPUT(UI_NAME, handle_input) {
  enum control input_current = button;
  switch (input_current) {
    case UP:
      menu_decrement();
      break;
    case DOWN:
      menu_increment();
      break;
    case A:
      menu_accept();
      break;
    case Y:
      menu_swap_sort();
      break;

    /* Always nothing */
    case NONE:
    default:
      break;
  }
}

FUNCTION(UI_NAME, drawOP) {
  draw_bg_layers();
  draw_gameart();
}

FUNCTION(UI_NAME, drawTR) {
  draw_gamelist();
  draw_gameinfo();
}
