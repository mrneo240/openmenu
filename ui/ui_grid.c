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

static bool showing_large_art = false;

static bool direction_last = false;
static bool direction_current = false;
#define direction_held (direction_last & direction_current)

typedef struct vec2d {
  float x;
  float y;
} vec2d;

typedef struct anim2d {
  int frame_len;
  int frame_now;
  vec2d start;
  vec2d end;
  vec2d cur;
} anim2d;

static vec2d pos_highlight = (vec2d){.x = 0, .y = 0};
static anim2d anim_highlight = {0};
static bool anim_active = false;

#define ANIM_FRAMES (15)
#define TILE_X_POS(col) (100 - 4 + ((40 + 120) * (col)))
#define TILE_Y_POS(row) (20 - 4 + ((10 + 120) * (row)))

static void update_anim(anim2d *anim) {
  AHEasingFunction ease = CircularEaseOut;
  const float dt = (float)anim->frame_now / (float)anim->frame_len;
  const float dv = (*ease)(dt);
  anim->cur.x = anim->start.x + (anim->end.x - anim->start.x) * dv;
  anim->cur.y = anim->start.y + (anim->end.y - anim->start.y) * dv;
}

/* For drawing */
//static image txr_highlight, txr_bg; /* Highlight square and Background */
static image txr_icon_list[9]; /* Lower list of 9 icons */
static image txr_focus;

extern image txr_highlight, txr_bg; /* Highlight square and Background */
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

static void draw_bg_layers(void) {
  draw_draw_image(0, 0, 640, 480, 1.0f, &txr_bg);
}

static inline int current_selected(void) {
  return current_starting_index + (screen_row * items_per_row) + (screen_column);
}

static void draw_large_art(int x, int y, int width, int height) {
  if (showing_large_art) {
    txr_get_large(list_current[current_selected()]->product, &txr_focus);
    if (txr_focus.texture == img_empty_boxart.texture) {
      /* Only draw if large is present */
      return;
    }
    draw_draw_image(x, y, width, height, 1.0f, &txr_focus);
  }
}

static void setup_highlight_animation(void) {
  float start_x = pos_highlight.x;
  float start_y = pos_highlight.y;
  if (anim_active) {
    start_x = anim_highlight.cur.x;
    start_y = anim_highlight.cur.y;
  }
  anim_highlight.start.x = start_x;
  anim_highlight.start.y = start_y;
  anim_highlight.end.x = TILE_X_POS(screen_column);
  anim_highlight.end.y = TILE_Y_POS(screen_row);
  anim_highlight.frame_now = 0;
  anim_highlight.frame_len = ANIM_FRAMES;
  anim_active = true;
}

static void draw_static_highlight(int size) {
  draw_draw_square(pos_highlight.x, pos_highlight.y, size, 1.0f, &txr_highlight);
}

static void draw_animated_highlight(int size) {
  /* Always draw on top */
  float z = z_get();
  z_set(512.0f);
  draw_draw_square(anim_highlight.cur.x, anim_highlight.cur.y, size, 1.0f, &txr_highlight);
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
      draw_draw_square(x_pos, y_pos, tile_size, 1.0f, &txr_icon_list[idx]);

      /* Highlight */
      if ((current_starting_index + idx) == current_selected()) {
        if (anim_active) {
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
  draw_large_art(gutter_side - 4, gutter_top - 4, (tile_size * items_per_row) + (horizontal_spacing * (items_per_row - 1)) + 8, (tile_size * items_per_row) + (vertical_spacing * (rows - 1)) + 8);
}

static void update_time(void) {
  if (anim_active) {
    anim_highlight.frame_now++;
    if (anim_highlight.frame_now > anim_highlight.frame_len) {
      anim_active = false;
    }
    update_anim(&anim_highlight);
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

static void menu_up(void) {
  if (direction_held && navigate_timeout > 0) {
    navigate_timeout--;
    return;
  }
  menu_row_up();

  setup_highlight_animation();

  frames_focused = 0;
  navigate_timeout = INPUT_TIMEOUT;
}

static void menu_down(void) {
  if (direction_held && navigate_timeout > 0) {
    navigate_timeout--;
    return;
  }
  menu_row_down();

  setup_highlight_animation();

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

static void menu_show_large_art(void) {
  showing_large_art = true;
}

/* Base UI Methods */

FUNCTION(UI_NAME, init) {
  /* Moved to global init */
  /*
  draw_load_texture("HIGHLIGHT.PVR", &txr_highlight);
  draw_load_texture("BG_RIGHT.PVR", &txr_bg);
  font_init();
  */
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
  showing_large_art = false;

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
      menu_up();
      break;
    case DOWN:
      direction_current = true;
      menu_down();
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
      break;

    /* These dont do anything */
    case B:
      break;
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

FUNCTION(UI_NAME, draw) {
  update_time();

  draw_bg_layers();
  draw_grid_boxes();

  font_begin_draw();
  font_draw_centered(320, 430, 1.0f, list_current[current_selected()]->name);
}
