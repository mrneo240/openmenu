/*
 * File: ui_grid.c
 * Project: ui
 * File Created: Saturday, 5th June 2021 10:07:38 pm
 * Author: Hayden Kowalchuk
 * -----
 * Copyright (c) 2021 Hayden Kowalchuk, Hayden Kowalchuk
 * License: BSD 3-clause "New" or "Revised" License, http://www.opensource.org/licenses/BSD-3-Clause
 */
#include "ui_grid.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../backend/gd_item.h"
#include "../backend/gd_list.h"
#include "../texture/txr_manager.h"
#include "animation.h"
#include "draw_prototypes.h"
#include "font_prototypes.h"

extern void ui_cycle_next(void);

/* List managment */
#define INPUT_TIMEOUT (10)
#define FOCUSED_HIRES_FRAMES (60 * 1) /* 1 second load in */

static int screen_row = 0;
static int screen_column = 0;
static int current_starting_index = 0;
static int navigate_timeout = INPUT_TIMEOUT;
static int frames_focused = 0;

static const int items_per_row = 3;
static const int rows = 3;

static bool boxart_button_held = false;

static bool direction_last = false;
static bool direction_current = false;
#define direction_held (direction_last & direction_current)

static vec2d pos_highlight = (vec2d){.x = 0, .y = 0};
static anim2d anim_highlight = {0};

static anim2d anim_large_art_pos = {0};
static anim2d anim_large_art_scale = {0};

#define ANIM_FRAMES (15)
#define HIGHLIGHT_X_POS(col) (100 - 4 + ((40 + 120) * (col)))
#define HIGHLIGHT_Y_POS(row) (20 - 4 + ((10 + 120) * (row)))
#define TILE_X_POS(col) (100 + ((40 + 120) * (col)))
#define TILE_Y_POS(row) (20 + ((10 + 120) * (row)))

/* For drawing */
static image txr_icon_list[9]; /* Lower list of 9 icons */
static image txr_focus;
static image txr_highlight; /* Highlight square */
static image txr_bg_left, txr_bg_right;
static image txr_icons_white, txr_icons_black;

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
};
enum theme { NTSC_U = 0,
             NTSC_J = 1,
             PAL = 2,
             THEME_END };
static enum theme theme_current = NTSC_U;

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

static inline int current_selected(void) {
  return current_starting_index + (screen_row * items_per_row) + (screen_column);
}

static void draw_large_art(void) {
  if (anim_active(&anim_large_art_scale.time)) {
    txr_get_large(list_current[current_selected()]->product, &txr_focus);
    if (txr_focus.texture == img_empty_boxart.texture) {
      /* Only draw if large is present */
      return;
    }
    /* Always draw on top */
    float z = z_get();
    z_set(512.0f);
    draw_draw_image_centered(anim_large_art_pos.cur.x, anim_large_art_pos.cur.y, anim_large_art_scale.cur.x, anim_large_art_scale.cur.y, COLOR_WHITE, &txr_focus);
    z_set(z);
  }
}

static void setup_highlight_animation(void) {
  float start_x = pos_highlight.x;
  float start_y = pos_highlight.y;
  if (anim_active(&anim_highlight.time)) {
    start_x = anim_highlight.cur.x;
    start_y = anim_highlight.cur.y;
  }
  anim_highlight.start.x = start_x;
  anim_highlight.start.y = start_y;
  anim_highlight.end.x = HIGHLIGHT_X_POS(screen_column);
  anim_highlight.end.y = HIGHLIGHT_Y_POS(screen_row);
  anim_highlight.time.frame_now = 0;
  anim_highlight.time.frame_len = ANIM_FRAMES;
  anim_highlight.time.active = true;
}

static void draw_static_highlight(int size) {
  draw_draw_square(pos_highlight.x, pos_highlight.y, size, themes[theme_current].highlight_color, &txr_highlight);
}

static void draw_animated_highlight(int size) {
  /* Always draw on top */
  float z = z_get();
  z_set(256.0f);
  draw_draw_square(anim_highlight.cur.x, anim_highlight.cur.y, size, themes[theme_current].highlight_color, &txr_highlight);
  z_set(z);
}

static void draw_grid_boxes(void) {
  const int tile_size = 120;
  const int gutter_side = 100;
  const int gutter_top = 20;
  const int horizontal_spacing = 40;
  const int vertical_spacing = 10;
  const int highlight_overhang = 4;

  for (int row = 0; row < rows; row++) {
    for (int column = 0; column < items_per_row; column++) {
      int idx = (row * items_per_row) + column;

      if (current_starting_index + idx < 0) {
        continue;
      }
      if (current_starting_index + idx >= list_len) {
        continue;
      }
      int x_pos = gutter_side + ((horizontal_spacing + tile_size) * column); /* 100 + ((40 + 120)*{0,1,2}) */
      int y_pos = gutter_top + ((vertical_spacing + tile_size) * row);       /* 20 + ((10 + 120)*{0,1,2}) */

      txr_get_small(list_current[current_starting_index + idx]->product, &txr_icon_list[idx]);
      draw_draw_square(x_pos, y_pos, tile_size, COLOR_WHITE, &txr_icon_list[idx]);

      /* Highlight */
      if ((current_starting_index + idx) == current_selected()) {
        if (anim_alive(&anim_highlight.time)) {
          draw_animated_highlight(tile_size + (highlight_overhang * 2));
        } else {
          pos_highlight.x = x_pos - highlight_overhang;
          pos_highlight.y = y_pos - highlight_overhang;
          draw_static_highlight(tile_size + (highlight_overhang * 2));
        }
      }
    }
  }

  /* If focused, draw large cover art */
  draw_large_art();
}

static void update_time(void) {
  if (anim_alive(&anim_highlight.time)) {
    anim_tick(&anim_highlight.time);
    anim_update_2d(&anim_highlight);
  }
  if (boxart_button_held && anim_alive(&anim_large_art_scale.time)) {
    /* Update scale and position */
    anim_tick(&anim_large_art_pos.time);
    anim_update_2d(&anim_large_art_pos);

    anim_tick(&anim_large_art_scale.time);
    anim_update_2d(&anim_large_art_scale);
  }
  if (!boxart_button_held && anim_alive(&anim_large_art_scale.time)) {
    /* Rewind Animation update scale and position */
    anim_tick_backward(&anim_large_art_pos.time);
    anim_update_2d(&anim_large_art_pos);

    anim_tick_backward(&anim_large_art_scale.time);
    anim_update_2d(&anim_large_art_scale);
  }
}

static void menu_row_up(void) {
  screen_row--;
  if (screen_row < 0) {
    screen_row = 0;
    current_starting_index -= items_per_row;
    if (current_starting_index < 0) {
      current_starting_index = 0;
    }
  }
}

static void menu_row_down(void) {
  screen_row++;
  if (screen_row >= rows) {
    screen_row--;
    current_starting_index += items_per_row;
    if (current_selected() > list_len) {
      current_starting_index -= items_per_row;
    }
  }
  while (current_selected() >= list_len) {
    screen_column--;
    if (screen_column < 0) {
      screen_column = items_per_row - 1;
      menu_row_up();
    }
  }
}

static void kill_large_art_animation(void) {
  anim_large_art_pos.time.active = false;
  anim_large_art_scale.time.active = false;
}

static void menu_up(int amount) {
  if (direction_held && navigate_timeout > 0) {
    navigate_timeout--;
    return;
  }

  while (amount--)
    menu_row_up();

  setup_highlight_animation();
  kill_large_art_animation();

  frames_focused = 0;
  navigate_timeout = INPUT_TIMEOUT;
}

static void menu_down(int amount) {
  if (direction_held && navigate_timeout > 0) {
    navigate_timeout--;
    return;
  }

  while (amount--)
    menu_row_down();

  setup_highlight_animation();
  kill_large_art_animation();

  frames_focused = 0;
  navigate_timeout = INPUT_TIMEOUT;
}

static void menu_left(void) {
  if (direction_held && navigate_timeout > 0) {
    navigate_timeout--;
    return;
  }

  screen_column--;
  if (current_selected() < 0) {
    screen_column = 0;
  }
  if (screen_column < 0) {
    screen_column = items_per_row - 1;
    menu_row_up();
  }

  setup_highlight_animation();
  kill_large_art_animation();

  frames_focused = 0;
  navigate_timeout = INPUT_TIMEOUT;
}

static void menu_right(void) {
  if (direction_held && navigate_timeout > 0) {
    navigate_timeout--;
    return;
  }
  screen_column++;
  if (current_selected() >= list_len) {
    screen_column--;
  }
  if (screen_column >= items_per_row) {
    screen_column = 0;
    menu_row_down();
  }

  setup_highlight_animation();
  kill_large_art_animation();

  frames_focused = 0;
  navigate_timeout = INPUT_TIMEOUT;
}

static void menu_accept(void) {
  dreamcast_rungd(list_current[current_selected()]->slot_num);
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

  frames_focused = 0;
  screen_column = screen_row = 0;
  current_starting_index = 0;
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

static void menu_theme_cycle(void) {
  if (navigate_timeout > 0) {
    navigate_timeout--;
    return;
  }
  theme_current++;
  if (theme_current == THEME_END) {
    theme_current = NTSC_U;
  }
  GRID_3_init();
  navigate_timeout = INPUT_TIMEOUT;
}

static void menu_show_large_art(void) {
  if (!boxart_button_held && !anim_active(&anim_large_art_scale.time)) {
    /* Setup positioning */
    {
      anim_large_art_pos.start.x = TILE_X_POS(screen_column) + (120 / 2);
      anim_large_art_pos.start.y = TILE_Y_POS(screen_row) + (120 / 2);
      anim_large_art_pos.end.x = TILE_X_POS(1) + (120 / 2);
      anim_large_art_pos.end.y = TILE_Y_POS(1) + (120 / 2);
      anim_large_art_pos.time.frame_now = 0;
      anim_large_art_pos.time.frame_len = 30;
      anim_large_art_pos.time.active = true;
    }
    /* Setup Scaling */
    {
      anim_large_art_scale.start.x = 120;
      anim_large_art_scale.start.y = 120;
      anim_large_art_scale.end.x = (120 * 3) + (40 * (3 - 1)) + 8;
      anim_large_art_scale.end.y = (120 * 3) + (10 * (3 - 1)) + 8;
      anim_large_art_scale.time.frame_now = 0;
      anim_large_art_scale.time.frame_len = 30;
      anim_large_art_scale.time.active = true;
    }
  }
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

#if 0
  temp = texman_create();
  draw_load_texture_buffer("THEME/SHARED/ICON_BLACK.PVR", &txr_icons_black, texman_get_tex_data(temp));
  texman_reserve_memory(txr_icons_black.width, txr_icons_black.height, 2 /* 16Bit */);

  draw_load_texture_buffer("THEME/SHARED/ICON_WHITE.PVR", &txr_icons_white, texman_get_tex_data(temp));
  texman_reserve_memory(txr_icons_white.width, txr_icons_white.height, 2 /* 16Bit */);
  //txr_icons_current = &txr_icons_white;
  txr_icons_current = themes[theme_current].icon_set;
#endif

  font_bmf_init("FONT/BASILEA.FNT", "FONT/BASILEA_W.PVR");

  printf("Texture scratch free: %d/%d KB (%d/%d bytes)\n", texman_get_space_available() / 1024, (1024 * 1024) / 1024, texman_get_space_available(), (1024 * 1024));
}

/* Reset variables sensibly */
FUNCTION(UI_NAME, setup) {
  list_current = list_get();
  list_len = list_length();

  screen_column = screen_row = 0;
  current_starting_index = 0;
  navigate_timeout = INPUT_TIMEOUT;
  sort_current = DEFAULT;
}

FUNCTION_INPUT(UI_NAME, handle_input) {
  direction_last = direction_current;
  direction_current = false;
  boxart_button_held = false;

  enum control input_current = button;
  switch (input_current) {
    case LEFT:
      direction_current = true;
      menu_left();
      break;
    case RIGHT:
      direction_current = true;
      menu_right();
      break;
    case UP:
      direction_current = true;
      menu_up(1);
      break;
    case DOWN:
      direction_current = true;
      menu_down(1);
      break;
    case TRIG_L:
      direction_current = true;
      menu_up(3);
      break;
    case TRIG_R:
      direction_current = true;
      menu_down(3);
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
    case X:
      menu_show_large_art();
      boxart_button_held = true;
      break;
    case B:
      menu_theme_cycle();
      break;

    /* These dont do anything */

    /* Always nothing */
    case NONE:
    default:
      break;
  }
  if (screen_row < 0) {
    screen_row = 0;
  }
  if (screen_column < 0) {
    screen_column = 0;
  }
}

FUNCTION(UI_NAME, drawOP) {
  draw_bg_layers();
}

FUNCTION(UI_NAME, drawTR) {
  update_time();

  draw_grid_boxes();

  font_bmf_begin_draw();
  font_bmf_draw_centered_auto_size(320, 430, themes[theme_current].text_color, list_current[current_selected()]->name, 640 - (10 * 2));
}
