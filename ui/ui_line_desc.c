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

#include "../backend/gd_item.h"
#include "../backend/gd_list.h"
#include "../texture/txr_manager.h"
#include "draw_prototypes.h"
#include "font_prototypes.h"

extern void ui_cycle_next(void);

/* List managment */
#define INPUT_TIMEOUT (10)
#define FOCUSED_HIRES_FRAMES (60 * 1) /* 1 second load in */

static int current_selected_item = 0;
static int navigate_timeout = INPUT_TIMEOUT;
static int frames_focused = 0;

/* For drawing */
static image txr_icon_list[16]; /* Lower list of 9 icons */
static image txr_focus;         /* current selected item, either lowres or hires */

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

static void draw_bg_layers() {
  draw_draw_image(0, 0, 640, 480, 1.0f, &txr_bg);
}

static void draw_big_box() {
  draw_draw_image(92, 92, 208, 208, 1.0f, &txr_focus);
}

static void draw_small_boxes() {
#define LIST_ADJUST (2)
  int i;
  int num_icons = 10; /* really 9..., the math is easier for 10 */
  int starting_icon_idx = current_selected_item - 4;
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
    starting_icon_idx = 0;
    num_icons++;
  }

  for (i = 0; (i < num_icons - 1) && (i + starting_icon_idx < list_len); i++) {
    txr_get_small(list_current[starting_icon_idx + i]->product, &txr_icon_list[i]);
    draw_draw_square(x_start + (icon_size + icon_spacing) * i, y_pos, icon_size, 1.0f, &txr_icon_list[i]);
  }
  draw_draw_square(x_start + (icon_size + icon_spacing) * highlighted_icon - 4.0f, y_pos - 4.0f, icon_size + 8, 1.0f, &txr_highlight);

#undef LIST_ADJUST
}

static void menu_decrement(void) {
  if (navigate_timeout > 0) {
    navigate_timeout--;
    return;
  }
  if (current_selected_item > 0) {
    current_selected_item--;
  }
  navigate_timeout = INPUT_TIMEOUT;
  frames_focused = 0;
}

static void menu_increment(void) {
  if (navigate_timeout > 0) {
    navigate_timeout--;
    return;
  }
  if (++current_selected_item >= list_len) {
    current_selected_item = list_len - 1;
  }
  navigate_timeout = INPUT_TIMEOUT;
  frames_focused = 0;
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
  /* Moved to global init */
  /*
  draw_load_texture("HIGHLIGHT.PVR", &txr_highlight);
  draw_load_texture("BG_RIGHT.PVR", &txr_bg);
  font_init();
  */
}

FUNCTION(UI_NAME, setup) {
  list_current = list_get();
  list_len = list_length();

  current_selected_item = 0;
  sort_current = DEFAULT;
  frames_focused = 0;

  /* Setup sensible defaults */
  txr_focus.format = (0 << 26) | (1 << 27); /* RGB565, Twiddled */
  txr_focus.width = 128;
  txr_focus.height = 128;
  txr_focus.texture = NULL;

  navigate_timeout = INPUT_TIMEOUT;
}

FUNCTION_INPUT(UI_NAME, handle_input) {
  enum control input_current = button;
  switch (input_current) {
    case LEFT:
      menu_decrement();
      break;
    case RIGHT:
      menu_increment();
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

    /* These dont do anything */
    case UP:
      break;
    case DOWN:
      break;
    case B:
      break;
    case X:
      break;
    /* Always nothing */
    case NONE:
    default:
      break;
  }
}

FUNCTION(UI_NAME, draw) {
  update_data();

  draw_bg_layers();
  draw_small_boxes();
  draw_big_box();

  font_bmf_begin_draw();
  font_bmf_draw_main(316, 92, 1.0f, list_current[current_selected_item]->name);
  font_bmf_draw_sub_wrap(316, 128, 1.0f, "Game Synopsis here...", 280);
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
  font_bmf_draw_main(4, 440, 1.0f, text);
}
