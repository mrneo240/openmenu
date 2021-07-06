/*
 * File: ui_line_desc.c
 * Project: ui
 * File Created: Wednesday, 19th May 2021 9:08:07 pm
 * Author: Hayden Kowalchuk
 * -----
 * Copyright (c) 2021 Hayden Kowalchuk, Hayden Kowalchuk
 * License: BSD 3-clause "New" or "Revised" License, http://www.opensource.org/licenses/BSD-3-Clause
 */

#include "ui_line_desc.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../backend/db_list.h"
#include "../backend/gd_item.h"
#include "../backend/gd_list.h"
#include "../texture/txr_manager.h"
#include "draw_prototypes.h"
#include "font_prototypes.h"

extern void ui_cycle_next(void);

/* List managment */
#define INPUT_TIMEOUT (10)
#define FOCUSED_HIRES_FRAMES (60 * 1) /* 1 second load in */

static int current_selected_item;
static int navigate_timeout;
static int frames_focused;

db_item *current_meta;

/* For drawing */
static image txr_icon_list[16]; /* Lower list of 9 icons */
static image txr_focus;         /* current selected item, either lowres or hires */
static image txr_highlight;     /* Highlight square*/
static image txr_bg_left, txr_bg_right;
static image txr_icons_white, txr_icons_black;
static image *txr_icons_current;

extern image img_empty_boxart;

/* Our actual gdemu items */
static const gd_item **list_current;
static int list_len;
enum sort_type { DEFAULT,
                 ALPHA,
                 DATE,
                 PRODUCT,
                 SORT_END };
static enum sort_type sort_current = DEFAULT;

typedef struct theme_region {
  const char *bg_left;
  const char *bg_right;
  image *icon_set;
  uint32_t text_color;
  uint32_t highlight_color;
} theme_region;

static theme_region themes[] = {
#if defined(ASPECT_WIDE) && ASPECT_WIDE
    (theme_region){
        .bg_left = "THEME/NTSC_U/BG_U_L.PVR",
        .bg_right = "THEME/NTSC_U/BG_U_R.PVR",
        .icon_set = &txr_icons_white,
        .text_color = COLOR_WHITE,
        .highlight_color = COLOR_ORANGE_U},
    (theme_region){
        .bg_left = "THEME/NTSC_J/BG_J_L_WIDE.PVR",
        .bg_right = "THEME/NTSC_J/BG_J_R_WIDE.PVR",
        .icon_set = &txr_icons_black,
        .text_color = COLOR_BLACK,
        .highlight_color = COLOR_ORANGE_J},
    (theme_region){
        .bg_left = "THEME/PAL/BG_E_L_WIDE.PVR",
        .bg_right = "THEME/PAL/BG_E_R_WIDE.PVR",
        .icon_set = &txr_icons_black,
        .text_color = COLOR_BLACK,
        .highlight_color = COLOR_BLUE},
#else
    (theme_region){
        .bg_left = "THEME/NTSC_U/BG_U_L.PVR",
        .bg_right = "THEME/NTSC_U/BG_U_R.PVR",
        .icon_set = &txr_icons_white,
        .text_color = COLOR_WHITE,
        .highlight_color = COLOR_ORANGE_U},
    (theme_region){
        .bg_left = "THEME/NTSC_J/BG_J_L.PVR",
        .bg_right = "THEME/NTSC_J/BG_J_R.PVR",
        .icon_set = &txr_icons_black,
        .text_color = COLOR_BLACK,
        .highlight_color = COLOR_ORANGE_J},
    (theme_region){
        .bg_left = "THEME/PAL/BG_E_L.PVR",
        .bg_right = "THEME/PAL/BG_E_R.PVR",
        .icon_set = &txr_icons_black,
        .text_color = COLOR_BLACK,
        .highlight_color = COLOR_BLUE},
#endif
};
enum theme { NTSC_U = 0,
             NTSC_J,
             PAL,
             THEME_END };
static enum theme theme_current = NTSC_U;

static void
draw_bg_layers(void) {
  {
    const dimen_RECT left = {.x = 0, .y = 0, .w = 512, .h = 480};
    draw_draw_sub_image(0, 0, 512, 480, COLOR_WHITE, &txr_bg_left, &left);
  }
  {
    const dimen_RECT right = {.x = 0, .y = 0, .w = 128, .h = 480};
    draw_draw_sub_image(512, 0, 128, 480, COLOR_WHITE, &txr_bg_right, &right);
  }
}

static void draw_big_box(void) {
  draw_draw_image(92 - 24, 92 - 20, 208 + 24, 208 + 24, COLOR_WHITE, &txr_focus);
}

static void draw_small_boxes(void) {
#define LIST_ADJUST (2)
  int i;
  int num_icons = 10; /* really 9..., the math is easier for 10 */
  int starting_icon_idx = current_selected_item - 4;
  float x_start = -24.0f;
  float y_pos = 350.0f;
  float icon_size = 68.0f;
  float icon_spacing = 8.0f;

  /* possible change how many we draw based on if we are not quite at the 5th item in the list */
  if (current_selected_item < 5) {
    num_icons = 5 + current_selected_item;
    x_start += (4 - current_selected_item) * (icon_size + icon_spacing);
    starting_icon_idx = 0;
    num_icons++;
  }

  for (i = 0; (i < num_icons - 1) && (i + starting_icon_idx < list_len); i++) {
    txr_get_small(list_current[starting_icon_idx + i]->product, &txr_icon_list[i]);
    draw_draw_square(x_start + (icon_size + icon_spacing) * i, y_pos, icon_size, COLOR_WHITE, &txr_icon_list[i]);
  }

#undef LIST_ADJUST
}

static void draw_small_box_highlight(void) {
#define LIST_ADJUST (2)
  int num_icons = 10; /* really 9..., the math is easier for 10 */
  float x_start = -24.0f;
  float y_pos = 350.0f;
  float icon_size = 68.0f;
  float icon_spacing = 8.0f;
  int highlighted_icon = (num_icons / 2 - 1);

  /* possible change how many we draw based on if we are not quite at the 5th item in the list */
  if (current_selected_item < 5) {
    num_icons = 5 + current_selected_item;
    x_start += (4 - current_selected_item) * (icon_size + icon_spacing);
    highlighted_icon = current_selected_item;
    num_icons++;
  }

  draw_draw_square(x_start + (icon_size + icon_spacing) * highlighted_icon - 4.0f, y_pos - 4.0f, icon_size + 8, themes[theme_current].highlight_color, &txr_highlight);

#undef LIST_ADJUST
}

static void draw_game_meta(void) {
  /* Icons if we have them*/

  /* Then text after */
  font_bmf_begin_draw();
  font_bmf_set_height_default();
  font_bmf_draw_auto_size(316, 92 - 20, themes[theme_current].text_color, list_current[current_selected_item]->name, 640 - 316 - 10);

  const char *synopsis;
  if (current_meta) {
    /* success! */
    synopsis = current_meta->description;
    font_bmf_draw_sub_wrap(316, 136 - 20 - 8, themes[theme_current].text_color, synopsis, 640 - 316 - 10); /* was 316,128 */

    font_bmf_set_height(12.0f);
    font_bmf_draw_centered(326 + (20 / 2), 282 + 12, themes[theme_current].text_color, db_format_nplayers_str(current_meta->num_players));
    //font_bmf_draw_centered(396 + (16 / 2), 282 + 12, themes[theme_current].text_color, "VMU");
    font_bmf_draw_centered(396 + (16 / 2), 282 + 12, themes[theme_current].text_color, db_format_vmu_blocks_str(current_meta->vmu_blocks));
    font_bmf_draw_centered(460 + (34 / 2), 282 + 12, themes[theme_current].text_color, "Jump Pack");
    font_bmf_draw_centered(534 + (22 / 2), 282 + 12, themes[theme_current].text_color, "Modem");

    /* Draw Icons */
    {
      //318x282
      const dimen_RECT uv_controller = {.x = 0, .y = 0, .w = 42, .h = 42};
      draw_draw_sub_image(326, 254 + 12, 20, 20, COLOR_WHITE, txr_icons_current, &uv_controller);  // 20x20
    }
    {
      //388x282
      const dimen_RECT uv_vmu = {.x = 42, .y = 0, .w = 26, .h = 42};
      draw_draw_sub_image(396, 254 + 12, 16, 22, COLOR_WHITE, txr_icons_current, &uv_vmu);  //16x22
    }
    {
      //448x282
      const dimen_RECT uv_rumble = {.x = 0, .y = 42, .w = 64, .h = 44};
      draw_draw_sub_image(458, 254 + 12, 34, 24, COLOR_WHITE, txr_icons_current, &uv_rumble);  //34x24
    }
    {
      //524x282
      const dimen_RECT uv_modem = {.x = 0, .y = 86, .w = 42, .h = 42};
      draw_draw_sub_image(534, 254 + 12, 22, 22, COLOR_WHITE, txr_icons_current, &uv_modem);  //22x22
    }
  }
}

static void menu_changed_item(void) {
  frames_focused = 0;
  db_get_meta(list_current[current_selected_item]->product, &current_meta);
}

static void menu_decrement(int amount) {
  if (navigate_timeout > 0) {
    navigate_timeout--;
    return;
  }
  current_selected_item -= amount;
  if (current_selected_item < 0) {
    current_selected_item = 0;
  }
  navigate_timeout = INPUT_TIMEOUT;
  menu_changed_item();
}

static void menu_increment(int amount) {
  if (navigate_timeout > 0) {
    navigate_timeout--;
    return;
  }
  current_selected_item += amount;
  if (current_selected_item >= list_len) {
    current_selected_item = list_len - 1;
  }
  navigate_timeout = INPUT_TIMEOUT;
  menu_changed_item();
}

static void menu_accept(void) {
  dreamcast_rungd(list_current[current_selected_item]->slot_num);
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
    case DATE:
      list_current = list_get_sort_date();
      break;
    case PRODUCT:
      list_current = list_get_sort_product();
      break;
    case DEFAULT:
    default:
      list_current = list_get_sort_default();
      break;
  }
  current_selected_item = 0;
  navigate_timeout = INPUT_TIMEOUT;
  frames_focused = 0;
}

static void menu_theme_cycle(void) {
  if (navigate_timeout > 0) {
    navigate_timeout--;
    return;
  }
  theme_current++;
  if (theme_current == THEME_END) {
    theme_current = NTSC_U;
  }
  LIST_DESC_init();
  navigate_timeout = INPUT_TIMEOUT;
}

static void menu_cycle_ui(void) {
  if (navigate_timeout > 0) {
    navigate_timeout--;
    return;
  }
  ui_cycle_next();
  navigate_timeout = INPUT_TIMEOUT;
}

static void update_data(void) {
  if (frames_focused > FOCUSED_HIRES_FRAMES) {
    txr_get_large(list_current[current_selected_item]->product, &txr_focus);
    if (txr_focus.texture == img_empty_boxart.texture) {
      txr_get_small(list_current[current_selected_item]->product, &txr_focus);
    }
  } else {
    txr_get_small(list_current[current_selected_item]->product, &txr_focus);
  }

  frames_focused++;
}

/* Base UI Methods */

FUNCTION(UI_NAME, init) {
  texman_clear();

  /* on user for now, may change */
  unsigned int temp = texman_create();
  draw_load_texture_buffer("EMPTY.PVR", &img_empty_boxart, texman_get_tex_data(temp));
  texman_reserve_memory(img_empty_boxart.width, img_empty_boxart.height, 2 /* 16Bit */);

  temp = texman_create();
  draw_load_texture_buffer("THEME/SHARED/HIGHLIGHT.PVR", &txr_highlight, texman_get_tex_data(temp));
  texman_reserve_memory(txr_highlight.width, txr_highlight.height, 2 /* 16Bit */);

  temp = texman_create();
  draw_load_texture_buffer(themes[theme_current].bg_left, &txr_bg_left, texman_get_tex_data(temp));
  texman_reserve_memory(txr_bg_left.width, txr_bg_left.height, 2 /* 16Bit */);

  temp = texman_create();
  draw_load_texture_buffer(themes[theme_current].bg_right, &txr_bg_right, texman_get_tex_data(temp));
  texman_reserve_memory(txr_bg_right.width, txr_bg_right.height, 2 /* 16Bit */);

  temp = texman_create();
  draw_load_texture_buffer("THEME/SHARED/ICON_BLACK.PVR", &txr_icons_black, texman_get_tex_data(temp));
  texman_reserve_memory(txr_icons_black.width, txr_icons_black.height, 2 /* 16Bit */);

  temp = texman_create();
  draw_load_texture_buffer("THEME/SHARED/ICON_WHITE.PVR", &txr_icons_white, texman_get_tex_data(temp));
  texman_reserve_memory(txr_icons_white.width, txr_icons_white.height, 2 /* 16Bit */);

  txr_icons_current = themes[theme_current].icon_set;

  font_bmf_init("FONT/BASILEA.FNT", "FONT/BASILEA_W.PVR");

  printf("Texture scratch free: %d/%d KB (%d/%d bytes)\n", texman_get_space_available() / 1024, (1024 * 1024) / 1024, texman_get_space_available(), (1024 * 1024));
}

FUNCTION(UI_NAME, setup) {
  list_current = list_get();
  list_len = list_length();

  current_selected_item = 0;
  sort_current = DEFAULT;
  frames_focused = 0;

  navigate_timeout = INPUT_TIMEOUT;
  menu_changed_item();
}

FUNCTION_INPUT(UI_NAME, handle_input) {
  enum control input_current = button;
  switch (input_current) {
    case LEFT:
      menu_decrement(1);
      break;
    case RIGHT:
      menu_increment(1);
      break;
    case UP:
      menu_decrement(10);
      break;
    case DOWN:
      menu_increment(10);
      break;
    case TRIG_L:
      menu_decrement(25);
      break;
    case TRIG_R:
      menu_increment(25);
      break;
    case A:
      menu_accept();
      break;
    case START:
      menu_swap_sort();
      break;
    case Y:
      menu_cycle_ui();
      break;
    case B:
      menu_theme_cycle();
      break;

      /* These dont do anything */
    case X:
      break;
    /* Always nothing */
    case NONE:
    default:
      break;
  }
}

FUNCTION(UI_NAME, drawOP) {
  update_data();

  draw_bg_layers();
  draw_small_boxes();
  draw_big_box();
}

FUNCTION(UI_NAME, drawTR) {
  draw_small_box_highlight();
  draw_game_meta();
#if 0
  const char *text = NULL;
  switch (sort_current) {
    case ALPHA:
      text = "Name";
      break;
    case DATE:
      text = "Date";
      break;
    case PRODUCT:
      text = "Product ID";
      break;
    case DEFAULT:
    default:
      text = "SD Card Order";
      break;
  }
  //font_bmf_draw_main(4, 440, themes[theme_current].text_color, text);
#endif
}
