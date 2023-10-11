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

/* Scaling */
#define X_SCALE_4_3 ((float)1.0f)
#define X_SCALE_16_9 ((float)0.74941452f)
#define ICON_BASE_SIZE ((int)68)

/* List managment */
#define INPUT_TIMEOUT (10)
#define FOCUSED_HIRES_FRAMES (60 * 1) /* 1 second load in */

/* Tile parameters */
/* Basic Info */
static float X_SCALE;
static float FONT_SYNOP_SIZE;
static short SCR_WIDTH;
static short SCR_HEIGHT;
static short ICON_SPACING;
static short ICON_AREA_WIDTH;
static short ICON_AREA_UNDERHANG;
static short NUM_ICONS;
static short HIGHLIGHT_OVERHANG;
/* Calculated params */
static short ICON_SIZE_X;
static short ICON_SIZE_Y;

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

static void recalculate_aspect(CFG_ASPECT aspect) {
  if (aspect == ASPECT_NORMAL) {
    SCR_WIDTH = (640);
    X_SCALE = (X_SCALE_4_3);
    ICON_SPACING = (8);
    ICON_AREA_WIDTH = (684);  // 640+22pixel over hang on edges
    ICON_AREA_UNDERHANG = (-24);
    NUM_ICONS = (9);
    FONT_SYNOP_SIZE = (16.0f);
  } else {
    X_SCALE = (X_SCALE_16_9);
    SCR_WIDTH = (854);
    ICON_SPACING = (8);
    ICON_AREA_WIDTH = (SCR_WIDTH);  // 854 no over hang on edges
    ICON_AREA_UNDERHANG = (10);
    NUM_ICONS = (11);
    FONT_SYNOP_SIZE = (18.0f);
  }
  SCR_HEIGHT = (480);
  HIGHLIGHT_OVERHANG = (4);
  ICON_SIZE_X = (ICON_BASE_SIZE);
  ICON_SIZE_Y = (ICON_BASE_SIZE);
}

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
  const int y_pos = 72;
  const int width = (232) * X_SCALE;
  const int height = 232;
  const int right_edge = ((SCR_WIDTH / 2) - (SCR_WIDTH * 0.03125)) * X_SCALE;
  // centers (640x480) = 320, (854x480) = 427, 50%
  // 4:3  (640x480) right edge at 300, 20 pixels left of center or 3.125%
  // 16:9 (854x480) right edge at 400, 26 pixels left of center or 3.125%

  draw_draw_image(right_edge - width, y_pos, width, height, COLOR_WHITE, &txr_focus);
}

static void draw_small_boxes(void) {
  int i;
  int num_icons = NUM_ICONS;
  int starting_icon_idx = current_selected_item - (NUM_ICONS / 2);
  float x_start = ICON_AREA_UNDERHANG;
  float y_pos = 350.0f;

  /* possible change how many we draw based on if we are not quite at the 5th item in the list */
  if (current_selected_item < (NUM_ICONS / 2)) {
    num_icons = (NUM_ICONS / 2) + current_selected_item;
    x_start += (((NUM_ICONS / 2)) - current_selected_item) * (ICON_SIZE_X + ICON_SPACING);
    starting_icon_idx = 0;
    num_icons++;
  }

  for (i = 0; (i < num_icons) && (i + starting_icon_idx < list_len); i++) {
    txr_get_small(list_current[starting_icon_idx + i]->product, &txr_icon_list[i]);
    draw_draw_image((x_start + (ICON_SIZE_X + ICON_SPACING) * i) * X_SCALE, y_pos, ICON_SIZE_X * X_SCALE, ICON_SIZE_Y, COLOR_WHITE, &txr_icon_list[i]);
  }
}

static void draw_small_box_highlight(void) {
  float x_start = ICON_AREA_UNDERHANG;
  float y_pos = 350.0f;
  int highlighted_icon = (NUM_ICONS) / 2;  // Middle

  draw_draw_image((x_start + (ICON_SIZE_X + ICON_SPACING) * highlighted_icon - 4.0f) * X_SCALE, y_pos - 4.0f, (ICON_SIZE_X + 8) * X_SCALE, ICON_SIZE_Y + 8, current_theme_colors->highlight_color, &txr_highlight);
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
  font_bmf_set_height(16.0f);
  if (list_len <= 0) {
    font_bmf_draw_auto_size((SCR_WIDTH / 2 - 4) * X_SCALE, 92 - 20, current_theme_colors->text_color, "Empty Game List", (SCR_WIDTH / 2 - 10) * X_SCALE);
    return;
  }
  font_bmf_draw_auto_size((SCR_WIDTH / 2 - 4) * X_SCALE, 92 - 20, current_theme_colors->text_color, list_current[current_selected_item]->name, (SCR_WIDTH / 2 - 10) * X_SCALE);

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
    font_bmf_set_height(FONT_SYNOP_SIZE);
    font_bmf_draw_sub_wrap(316, 136 - 20 - 8, current_theme_colors->text_color, synopsis, 640 - 316 - 10);

    font_bmf_set_height(14.0f);
    font_bmf_draw_centered(326 + (20 / 2), 282 + 12, current_theme_colors->text_color, db_format_nplayers_str(current_meta->num_players));
    
    if(current_meta->vmu_blocks) {
		font_bmf_draw_centered(396 + (16 / 2), 282 + 12, current_theme_colors->text_color, db_format_vmu_blocks_str(current_meta->vmu_blocks));
	}
	
	if(current_meta->accessories & ACCESORIES_JUMP_PACK) {
		font_bmf_draw_centered(460 + (34 / 2), 282 + 12, current_theme_colors->text_color, "Jump Pack");
	}
	
	if(current_meta->accessories & (ACCESORIES_BBA | ACCESORIES_MODEM)) {
		font_bmf_draw_centered(534 + (22 / 2), 282 + 12, current_theme_colors->text_color, "Modem");
	}

    /* Draw Icons */
    {
      // 318x282
      const dimen_RECT uv_controller = {.x = 0, .y = 0, .w = 42, .h = 42};
      draw_draw_sub_image(326, 254 + 12, 20 * X_SCALE, 20, current_theme_colors->icon_color, txr_icons_current, &uv_controller);  // 20x20
    }
    
    if(current_meta->vmu_blocks) {
      // 388x282
      const dimen_RECT uv_vmu = {.x = 42, .y = 0, .w = 26, .h = 42};
      draw_draw_sub_image(396, 254 + 12, 16 * X_SCALE, 22, current_theme_colors->icon_color, txr_icons_current, &uv_vmu);  // 16x22
    }
    
    if(current_meta->accessories & ACCESORIES_JUMP_PACK) {
      // 448x282
      const dimen_RECT uv_rumble = {.x = 0, .y = 42, .w = 64, .h = 44};
      draw_draw_sub_image(458, 254 + 12, 34 * X_SCALE, 24, current_theme_colors->icon_color, txr_icons_current, &uv_rumble);  // 34x24
    }
    
    if(current_meta->accessories & (ACCESORIES_BBA | ACCESORIES_MODEM)) {
      // 524x282
      const dimen_RECT uv_modem = {.x = 0, .y = 86, .w = 42, .h = 42};
      draw_draw_sub_image(534, 254 + 12, 22 * X_SCALE, 22, current_theme_colors->icon_color, txr_icons_current, &uv_modem);  // 22x22
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
    current_selected_item = list_len - 1;
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
    current_selected_item = 0;
  }
  navigate_timeout = INPUT_TIMEOUT;
  menu_changed_item();
}

static void menu_cb(void) {
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

  if (!strncmp(list_current[current_selected_item]->disc, "PS1", 3)) {
    return;
  }
  
  dreamcast_launch_cb(list_current[current_selected_item]);
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

  if (!strncmp(list_current[current_selected_item]->disc, "PS1", 3)) {
    bleem_launch(list_current[current_selected_item]);
  }
  else {
    dreamcast_launch_disc(list_current[current_selected_item]);
  }
}

static void menu_settings(void) {
  if (navigate_timeout > 0) {
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

static void menu_exit(void) {
  if (navigate_timeout > 0) {
    return;
  }

  draw_current = DRAW_EXIT;
  popup_setup(&draw_current, &region_themes[region_current].colors, &navigate_timeout);
}

/* Base UI Methods */

FUNCTION(UI_NAME, init) {
  texman_clear();

  /* Set region from preferences */
  openmenu_settings *settings = settings_get();
  region_current = settings->region;
  recalculate_aspect(settings->aspect);

  /* Get the current themes, original + custom */
  region_themes = theme_get_default(settings->aspect, &num_default_themes);
  custom_themes = theme_get_custom(&num_custom_themes);

  /* Enable custom theme if needed */
  int use_custom_theme = settings->custom_theme;
  if (use_custom_theme) {
    int custom_theme_num = settings->custom_theme_num;
    region_current = REGION_END + 1 + custom_theme_num;
  }

  /* on user for now, may change */
  uint32_t temp = texman_create();
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
    case Y:
      menu_exit();
      break;

      /* These dont do anything */
    case B:
      menu_cb();
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
