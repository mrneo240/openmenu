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
#include "global_settings.h"
#include "ui_menu_credits.h"

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
static image txr_icons_white /*, txr_icons_black*/;
static image *txr_icons_current;

extern image img_empty_boxart;

/* Our actual gdemu items */
static const gd_item **list_current;
static int list_len;

static theme_region *region_themes;
static theme_custom *custom_themes;
static int num_default_themes;
static int num_custom_themes;
static theme_color *current_theme_colors;

static region region_current = REGION_NTSC_U;
static enum draw_state draw_current = DRAW_UI;

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

  draw_draw_square(x_start + (icon_size + icon_spacing) * highlighted_icon - 4.0f, y_pos - 4.0f, icon_size + 8, current_theme_colors->highlight_color, &txr_highlight);

#undef LIST_ADJUST
}

static void draw_game_meta(void) {
  /* grab the disc number and if there is more than one */
  int disc_num = list_current[current_selected_item]->disc[0] - '0';
  int disc_set = list_current[current_selected_item]->disc[2] - '0';

  /* Get multidisc settings */
  openmenu_settings *settings = settings_get();
  int hide_multidisc = settings->multidisc;

  /* Game Title */
  font_bmf_begin_draw();
  font_bmf_set_height_default();
  if (list_len <= 0) {
    font_bmf_draw_auto_size(316, 92 - 20, current_theme_colors->text_color, "Empty Game List!", 640 - 316 - 10);
    return;
  }
  font_bmf_draw_auto_size(316, 92 - 20, current_theme_colors->text_color, list_current[current_selected_item]->name, 640 - 316 - 10);

  /* Disc # above name, position 316x33 */
  {
    if (disc_set > 1) {
      if (!hide_multidisc) {
        char disc_str[8];
        snprintf(disc_str, 8, "Disc %d", disc_num);
        font_bmf_draw_sub(316 + 22 + 4, 36, current_theme_colors->text_color, disc_str);
      } else {
        /* Draw multiple discs and how many */
        char disc_str[8];
        snprintf(disc_str, 8, "%d Discs", disc_set);
        font_bmf_draw_sub(316 + 22 + 4, 36, current_theme_colors->text_color, disc_str);
      }
    }
  }

  const char *synopsis;
  if (current_meta) {
    /* success! */
    synopsis = current_meta->description;
    font_bmf_draw_sub_wrap(316, 136 - 20 - 8, current_theme_colors->text_color, synopsis, 640 - 316 - 10); /* was 316,128 */

    font_bmf_set_height(12.0f);
    font_bmf_draw_centered(326 + (20 / 2), 282 + 12, current_theme_colors->text_color, db_format_nplayers_str(current_meta->num_players));
    // font_bmf_draw_centered(396 + (16 / 2), 282 + 12, region_themes[region_current].text_color, "VMU");
    font_bmf_draw_centered(396 + (16 / 2), 282 + 12, current_theme_colors->text_color, db_format_vmu_blocks_str(current_meta->vmu_blocks));
    font_bmf_draw_centered(460 + (34 / 2), 282 + 12, current_theme_colors->text_color, "Jump Pack");
    font_bmf_draw_centered(534 + (22 / 2), 282 + 12, current_theme_colors->text_color, "Modem");

    /* Draw Icons */
    {
      // 318x282
      const dimen_RECT uv_controller = {.x = 0, .y = 0, .w = 42, .h = 42};
      draw_draw_sub_image(326, 254 + 12, 20, 20, current_theme_colors->icon_color, txr_icons_current, &uv_controller);  // 20x20
    }
    {
      // 388x282
      const dimen_RECT uv_vmu = {.x = 42, .y = 0, .w = 26, .h = 42};
      draw_draw_sub_image(396, 254 + 12, 16, 22, current_theme_colors->icon_color, txr_icons_current, &uv_vmu);  // 16x22
    }
    {
      // 448x282
      const dimen_RECT uv_rumble = {.x = 0, .y = 42, .w = 64, .h = 44};
      draw_draw_sub_image(458, 254 + 12, 34, 24, current_theme_colors->icon_color, txr_icons_current, &uv_rumble);  // 34x24
    }
    {
      // 524x282
      const dimen_RECT uv_modem = {.x = 0, .y = 86, .w = 42, .h = 42};
      draw_draw_sub_image(534, 254 + 12, 22, 22, current_theme_colors->icon_color, txr_icons_current, &uv_modem);  // 22x22
    }
  }

  if (disc_set > 1) {
    const dimen_RECT uv_disc = {.x = 42, .y = 86, .w = 42, .h = 42};
    draw_draw_sub_image(314, 33, 22, 22, current_theme_colors->text_color, txr_icons_current, &uv_disc);  // 22x22
  }
}

static void menu_changed_item(void) {
  frames_focused = 0;
  db_get_meta(list_current[current_selected_item]->product, &current_meta);
}

static void menu_decrement(int amount) {
  if (navigate_timeout > 0) {
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
  if ((navigate_timeout > 0) || (list_len <= 0)) {
    return;
  }

  /* grab the disc number and if there is more than one */
  int disc_set = list_current[current_selected_item]->disc[2] - '0';

  /* Get multidisc settings */
  openmenu_settings *settings = settings_get();
  int hide_multidisc = settings->multidisc;

  /* prepare to show multidisc chooser menu */
  if (hide_multidisc && (disc_set > 1)) {
    draw_current = DRAW_MULTIDISC;
    popup_setup(&draw_current, &region_themes[region_current].colors, &navigate_timeout);
    list_set_multidisc(list_current[current_selected_item]->product);
    return;
  }

  dreamcast_launch_disc(list_current[current_selected_item]);
}

static void menu_settings(void) {
  if ((navigate_timeout > 0) || (list_len <= 0)) {
    return;
  }

  draw_current = DRAW_MENU;
  menu_setup(&draw_current, &region_themes[region_current].colors, &navigate_timeout);
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

  /* Set region from preferences */
  openmenu_settings *settings = settings_get();
  region_current = settings->region;

  /* Get the current themes, original + custom */
  region_themes = theme_get_default(settings->aspect, &num_default_themes);
  custom_themes = theme_get_custom(&num_custom_themes);

  /* on user for now, may change */
  unsigned int temp = texman_create();
  draw_load_texture_buffer("EMPTY.PVR", &img_empty_boxart, texman_get_tex_data(temp));
  texman_reserve_memory(img_empty_boxart.width, img_empty_boxart.height, 2 /* 16Bit */);

  temp = texman_create();
  draw_load_texture_buffer("THEME/SHARED/HIGHLIGHT.PVR", &txr_highlight, texman_get_tex_data(temp));
  texman_reserve_memory(txr_highlight.width, txr_highlight.height, 2 /* 16Bit */);

  temp = texman_create();
  draw_load_texture_buffer("THEME/SHARED/ICON_WHITE.PVR", &txr_icons_white, texman_get_tex_data(temp));
  texman_reserve_memory(txr_icons_white.width, txr_icons_white.height, 2 /* 16Bit */);

  if ((int)region_current >= num_default_themes) {
    region_current -= num_default_themes;
    current_theme_colors = &custom_themes[region_current].colors;

    temp = texman_create();
    draw_load_texture_buffer(custom_themes[region_current].bg_left, &txr_bg_left, texman_get_tex_data(temp));
    texman_reserve_memory(txr_bg_left.width, txr_bg_left.height, 2 /* 16Bit */);

    temp = texman_create();
    draw_load_texture_buffer(custom_themes[region_current].bg_right, &txr_bg_right, texman_get_tex_data(temp));
    texman_reserve_memory(txr_bg_right.width, txr_bg_right.height, 2 /* 16Bit */);
  } else {
    current_theme_colors = &region_themes[region_current].colors;

    temp = texman_create();
    draw_load_texture_buffer(region_themes[region_current].bg_left, &txr_bg_left, texman_get_tex_data(temp));
    texman_reserve_memory(txr_bg_left.width, txr_bg_left.height, 2 /* 16Bit */);

    temp = texman_create();
    draw_load_texture_buffer(region_themes[region_current].bg_right, &txr_bg_right, texman_get_tex_data(temp));
    texman_reserve_memory(txr_bg_right.width, txr_bg_right.height, 2 /* 16Bit */);
  }

  txr_icons_current = &txr_icons_white;

#if 0
  temp = texman_create();
  draw_load_texture_buffer("THEME/SHARED/ICON_BLACK.PVR", &txr_icons_black, texman_get_tex_data(temp));
  texman_reserve_memory(txr_icons_black.width, txr_icons_black.height, 2 /* 16Bit */);
#endif

  font_bmf_init("FONT/BASILEA.FNT", "FONT/BASILEA_W.PVR", settings->aspect);

  printf("Texture scratch free: %d/%d KB (%d/%d bytes)\n", texman_get_space_available() / 1024, (1024 * 1024) / 1024, texman_get_space_available(), (1024 * 1024));
}

static void handle_input_ui(enum control input) {
  switch (input) {
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
      menu_settings();
      break;
    case Y: {
      extern void arch_menu(void);
      arch_menu();
    } break;

      /* These dont do anything */
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

FUNCTION(UI_NAME, setup) {
  list_current = list_get();
  list_len = list_length();

  current_selected_item = 0;
  frames_focused = 0;
  draw_current = DRAW_UI;

  navigate_timeout = INPUT_TIMEOUT * 2;
  menu_changed_item();
}

FUNCTION_INPUT(UI_NAME, handle_input) {
  enum control input_current = button;
  switch (draw_current) {
    case DRAW_MENU: {
      handle_input_menu(input_current);
    } break;
    case DRAW_CREDITS: {
      handle_input_credits(input_current);
    } break;
    case DRAW_MULTIDISC: {
      handle_input_multidisc(input_current);
    } break;
    case DRAW_EXIT: {
      handle_input_exit(input_current);
    } break;
    default:
    case DRAW_UI: {
      handle_input_ui(input_current);
    } break;
  }
  navigate_timeout--;
}

FUNCTION(UI_NAME, drawOP) {
  update_data();
  draw_bg_layers();

  switch (draw_current) {
    case DRAW_MENU: {
      /* Menu on top */
      draw_menu_op();
    } break;
    case DRAW_CREDITS: {
      /* Credits on top */
      draw_credits_op();
    } break;
    case DRAW_MULTIDISC: {
      /* Multidisc choice on top */
      draw_multidisc_op();
    } break;
    case DRAW_EXIT: {
      /* Exit popup on top */
      draw_exit_op();
    } break;
    default:
    case DRAW_UI: {
      /* always drawn */
    } break;
  }
}

FUNCTION(UI_NAME, drawTR) {
  draw_game_meta();
  if (list_len > 0) {
    draw_small_box_highlight();
    draw_small_boxes();
    draw_big_box();
  }

  switch (draw_current) {
    case DRAW_MENU: {
      /* Menu on top */
      draw_menu_tr();
    } break;
    case DRAW_CREDITS: {
      /* Credits on top */
      draw_credits_tr();
    } break;
    case DRAW_MULTIDISC: {
      /* Multidisc choice on top */
      draw_multidisc_tr();
    } break;
    case DRAW_EXIT: {
      /* Exit popup on top */
      draw_exit_tr();
    } break;
    default:
    case DRAW_UI: {
      /* always drawn */
    } break;
  }
}
