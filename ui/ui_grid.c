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
#include "global_settings.h"
#include "ui_menu_credits.h"

/* Scaling */
#define X_SCALE_4_3 ((float)1.0f)
#define X_SCALE_16_9 ((float)0.74941452f)

/* List managment */
#define INPUT_TIMEOUT (10)
#define FOCUSED_HIRES_FRAMES (60 * 1) /* 1 second load in */
#define ANIM_FRAMES (15)

/* Tile parameters */
/* Basic Info */
static float X_SCALE;
static short SCR_WIDTH;
static short SCR_HEIGHT;
static short TILE_AREA_WIDTH;
static short TILE_AREA_HEIGHT;
static short COLUMNS;
static short ROWS;
/* Calculated params */
static short GUTTER_SIDE;
static short GUTTER_TOP;
static short HORIZONTAL_SPACING;
static short VERTICAL_SPACING;
static short HIGHLIGHT_OVERHANG;
static short TILE_SIZE_X;
static short TILE_SIZE_Y;
/* Helpers */
static inline int HIGHLIGHT_X_POS(int col) {
  return (GUTTER_SIDE - HIGHLIGHT_OVERHANG + ((HORIZONTAL_SPACING + TILE_SIZE_X) * (col))) * X_SCALE;
}
static inline int HIGHLIGHT_Y_POS(int row) {
  return (GUTTER_TOP - HIGHLIGHT_OVERHANG + ((VERTICAL_SPACING + TILE_SIZE_Y) * (row)));
}
static inline int TILE_X_POS(int col) {
  return (GUTTER_SIDE + ((HORIZONTAL_SPACING + TILE_SIZE_X) * (col))) * X_SCALE;
}
static inline int TILE_Y_POS(int row) {
  return (GUTTER_TOP + ((VERTICAL_SPACING + TILE_SIZE_Y) * (row)));
}

static int screen_row = 0;
static int screen_column = 0;
static int current_starting_index = 0;
static int navigate_timeout = INPUT_TIMEOUT;
static int frames_focused = 0;

static bool boxart_button_held = false;

static bool direction_last = false;
static bool direction_current = false;
#define direction_held (direction_last & direction_current)

static vec2d pos_highlight = {.x = 0.f, .y = 0.f};
static anim2d anim_highlight;

static anim2d anim_large_art_pos;
static anim2d anim_large_art_scale;

/* For drawing */
static image txr_icon_list[/*ROWS * COLUMNS*/ 4 * 3 /* Assume the worst */]; /* Lower list of 9 or 12 icons */
static image txr_focus;
static image txr_highlight; /* Highlight square */
static image txr_bg_left, txr_bg_right;

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
    TILE_AREA_WIDTH = (440);
    TILE_AREA_HEIGHT = (380);
    COLUMNS = (3);
    ROWS = (3);
  } else {
    X_SCALE = (X_SCALE_16_9);
    SCR_WIDTH = (854);
    TILE_AREA_WIDTH = (600);
    TILE_AREA_HEIGHT = (380);
    COLUMNS = (4);
    ROWS = (3);
  }
  SCR_HEIGHT = (480);
  GUTTER_SIDE = ((SCR_WIDTH - TILE_AREA_WIDTH) / 2);
  GUTTER_TOP = ((SCR_HEIGHT - TILE_AREA_HEIGHT) / 2);
  HORIZONTAL_SPACING = (40);
  VERTICAL_SPACING = (10);
  HIGHLIGHT_OVERHANG = (4);
  TILE_SIZE_X = (((TILE_AREA_WIDTH - ((COLUMNS - 1) * HORIZONTAL_SPACING)) / COLUMNS));
  TILE_SIZE_Y = ((TILE_AREA_HEIGHT - ((ROWS - 1) * VERTICAL_SPACING)) / ROWS);
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

static inline int current_selected(void) {
  return current_starting_index + (screen_row * COLUMNS) + (screen_column);
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
  anim_highlight.end.x = (float)HIGHLIGHT_X_POS(screen_column);
  anim_highlight.end.y = (float)HIGHLIGHT_Y_POS(screen_row);
  anim_highlight.time.frame_now = 0;
  anim_highlight.time.frame_len = ANIM_FRAMES;
  anim_highlight.time.active = true;
}

static void draw_static_highlight(int width, int height) {
  draw_draw_image(pos_highlight.x, pos_highlight.y, width, height, current_theme_colors->highlight_color, &txr_highlight);
}

static void draw_animated_highlight(int width, int height) {
  /* Always draw on top */
  float z = z_get();
  z_set(200.0f);
  draw_draw_image(anim_highlight.cur.x, anim_highlight.cur.y, width, height, current_theme_colors->highlight_color, &txr_highlight);
  z_set(z);
}

static void draw_grid_boxes(void) {
  for (int row = 0; row < ROWS; row++) {
    for (int column = 0; column < COLUMNS; column++) {
      int idx = (row * COLUMNS) + column;

      if (current_starting_index + idx < 0) {
        continue;
      }
      if (current_starting_index + idx >= list_len) {
        break;
      }
      float x_pos = GUTTER_SIDE + ((HORIZONTAL_SPACING + TILE_SIZE_X) * column); /* 100 + ((40 + 120)*{0,1,2}) */
      float y_pos = GUTTER_TOP + ((VERTICAL_SPACING + TILE_SIZE_Y) * row);       /* 20 + ((10 + 120)*{0,1,2}) */

      x_pos *= X_SCALE;

      txr_get_small(list_current[current_starting_index + idx]->product, &txr_icon_list[idx]);
      draw_draw_image((int)x_pos, (int)y_pos, TILE_SIZE_X * X_SCALE, TILE_SIZE_Y, COLOR_WHITE, &txr_icon_list[idx]);

      /* Highlight */
      if ((current_starting_index + idx) == current_selected()) {
        if (anim_alive(&anim_highlight.time)) {
          draw_animated_highlight((TILE_SIZE_X + (HIGHLIGHT_OVERHANG * 2)) * X_SCALE, TILE_SIZE_Y + (HIGHLIGHT_OVERHANG * 2));
        } else {
          pos_highlight.x = x_pos - (HIGHLIGHT_OVERHANG * X_SCALE);
          pos_highlight.y = y_pos - (HIGHLIGHT_OVERHANG);
          draw_static_highlight((TILE_SIZE_X + (HIGHLIGHT_OVERHANG * 2)) * X_SCALE, TILE_SIZE_Y + (HIGHLIGHT_OVERHANG * 2));
        }
      }
    }
  }

  /* Get multidisc settings */
  openmenu_settings *settings = settings_get();
  int hide_multidisc = settings->multidisc;

  for (int row = 0; row < ROWS; row++) {
    for (int column = 0; column < COLUMNS; column++) {
      int idx = (row * COLUMNS) + column;

      if (current_starting_index + idx < 0) {
        continue;
      }
      if (current_starting_index + idx >= list_len) {
        break;
      }

      const int disc_set = list_current[current_selected()]->disc[2] - '0';

      /* Disc # above name, position 316x33 */
      if (((current_starting_index + idx) == current_selected()) && (hide_multidisc) && (disc_set > 1)) {
        float x_pos = GUTTER_SIDE + ((HORIZONTAL_SPACING + TILE_SIZE_X) * column) + 4;
        float y_pos = GUTTER_TOP + ((VERTICAL_SPACING + TILE_SIZE_Y) * row) + TILE_SIZE_Y - 24;
        int disc_set = list_current[current_selected()]->disc[2] - '0';

        x_pos *= X_SCALE;

        /* Draw multiple discs and how many */
        draw_draw_quad(x_pos, y_pos, TILE_SIZE_X * X_SCALE * 0.5f, 28, current_theme_colors->menu_bkg_color);
        char disc_str[8];
        snprintf(disc_str, 8, "%d Discs", disc_set);
        font_bmf_begin_draw();
        font_bmf_set_height(24);
        font_bmf_draw_sub(x_pos + 8, y_pos + 2, current_theme_colors->text_color, disc_str);
      }
    }
  }

  /* If focused, draw large cover art */
  draw_large_art();
}

static void draw_game_title(void) {
  font_bmf_begin_draw();
  if (list_len <= 0) {
    font_bmf_draw_centered_auto_size((SCR_WIDTH / 2) * X_SCALE, 434, current_theme_colors->text_color, "Empty Game List", (SCR_WIDTH - (10 * 2)) * X_SCALE);
    return;
  }
  font_bmf_draw_centered_auto_size((SCR_WIDTH / 2) * X_SCALE, 434, current_theme_colors->text_color, list_current[current_selected()]->name, (SCR_WIDTH - (10 * 2)) * X_SCALE);
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
    current_starting_index -= COLUMNS;
    if (current_starting_index < 0) {
      current_starting_index = 0;
    }
  }
}

static void menu_row_down(void) {
  screen_row++;
  if (screen_row >= ROWS) {
    screen_row = ROWS - 1;
    current_starting_index += COLUMNS;
    if (current_selected() > list_len) {
      current_starting_index -= COLUMNS;
    }
  }
  while (current_selected() >= list_len) {
    screen_column--;
    if (screen_column < 0) {
      screen_column = COLUMNS - 1;
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
    return;
  }

  screen_column--;
  if (current_selected() < 0) {
    //screen_column = 0;
    int temp = (list_len - (COLUMNS * ROWS)) / COLUMNS;
    if ((list_len - (COLUMNS * ROWS)) % COLUMNS) {
		temp++;
	}
    current_starting_index = temp * COLUMNS;
    
    if (current_starting_index < 0 || list_len <= (COLUMNS * ROWS)) {
		current_starting_index = 0;
	}
    
    screen_row = (list_len - 1 - current_starting_index) / COLUMNS;
    screen_column = (list_len - 1 - current_starting_index) % COLUMNS;
  }
  if (screen_column < 0) {
    screen_column = COLUMNS - 1;
    menu_row_up();
  }
  
  setup_highlight_animation();
  kill_large_art_animation();

  frames_focused = 0;
  navigate_timeout = INPUT_TIMEOUT;
}

static void menu_right(void) {
  if (direction_held && navigate_timeout > 0) {
    return;
  }
  screen_column++;
  if (current_selected() >= list_len) {
    //screen_column--;
    screen_row = screen_column = current_starting_index = 0;
  }
  if (screen_column >= COLUMNS) {
    screen_column = 0;
    menu_row_down();
  }
  
  setup_highlight_animation();
  kill_large_art_animation();

  frames_focused = 0;
  navigate_timeout = INPUT_TIMEOUT;
}

static void menu_cb(void) {
  if ((navigate_timeout > 0) || (list_len <= 0)) {
    return;
  }

  /* grab the disc number and if there is more than one */
  int disc_set = list_current[current_selected()]->disc[2] - '0';

  /* Get multidisc settings */
  openmenu_settings *settings = settings_get();
  int hide_multidisc = settings->multidisc;

  /* prepare to show multidisc chooser menu */
  if (hide_multidisc && (disc_set > 1)) {
    draw_current = DRAW_MULTIDISC;
    popup_setup(&draw_current, current_theme_colors, &navigate_timeout);
    list_set_multidisc(list_current[current_selected()]->product);
    return;
  }
  
  if (!strncmp(list_current[current_selected()]->disc, "PS1", 3)) {
    return;
  }
  
  dreamcast_launch_cb(list_current[current_selected()]);
}

static void menu_accept(void) {
  if ((navigate_timeout > 0) || (list_len <= 0)) {
    return;
  }

  /* grab the disc number and if there is more than one */
  int disc_set = list_current[current_selected()]->disc[2] - '0';

  /* Get multidisc settings */
  openmenu_settings *settings = settings_get();
  int hide_multidisc = settings->multidisc;

  /* prepare to show multidisc chooser menu */
  if (hide_multidisc && (disc_set > 1)) {
    draw_current = DRAW_MULTIDISC;
    popup_setup(&draw_current, current_theme_colors, &navigate_timeout);
    list_set_multidisc(list_current[current_selected()]->product);
    return;
  }
  
  if (!strncmp(list_current[current_selected()]->disc, "PS1", 3)) {
    bleem_launch(list_current[current_selected()]);
  }
  else {
    dreamcast_launch_disc(list_current[current_selected()]);
  }
}

static void menu_settings(void) {
  if (navigate_timeout > 0) {
    return;
  }

  draw_current = DRAW_MENU;
  menu_setup(&draw_current, current_theme_colors, &navigate_timeout);
}

static void menu_show_large_art(void) {
  if (list_len <= 0) {
    return;
  }
  if (!boxart_button_held && !anim_active(&anim_large_art_scale.time)) {
    /* Setup positioning */
    {
      anim_large_art_pos.start.x = TILE_X_POS(screen_column) + (TILE_SIZE_X / 2 * X_SCALE);
      anim_large_art_pos.start.y = TILE_Y_POS(screen_row) + (TILE_SIZE_Y / 2);
      anim_large_art_pos.end.x = (SCR_WIDTH / 2 * X_SCALE);
      anim_large_art_pos.end.y = (TILE_Y_POS(0) + ((TILE_Y_POS(ROWS - 1) - TILE_Y_POS(0)) / 2)) + (TILE_SIZE_Y / 2);
      anim_large_art_pos.time.frame_now = 0;
      anim_large_art_pos.time.frame_len = 30;
      anim_large_art_pos.time.active = true;
    }
    /* Setup Scaling */
    {
      anim_large_art_scale.start.x = TILE_SIZE_X * X_SCALE;
      anim_large_art_scale.start.y = TILE_SIZE_X;
      anim_large_art_scale.end.x = (TILE_AREA_HEIGHT + HIGHLIGHT_OVERHANG * 2) * X_SCALE;
      anim_large_art_scale.end.y = TILE_AREA_HEIGHT + HIGHLIGHT_OVERHANG * 2;
      anim_large_art_scale.time.frame_now = 0;
      anim_large_art_scale.time.frame_len = 30;
      anim_large_art_scale.time.active = true;
    }
  }
}

static void menu_exit(void) {
  if (navigate_timeout > 0) {
    return;
  }

  draw_current = DRAW_EXIT;
  popup_setup(&draw_current, current_theme_colors, &navigate_timeout);
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
  unsigned int temp = texman_create();
  draw_load_texture_buffer("EMPTY.PVR", &img_empty_boxart, texman_get_tex_data(temp));
  texman_reserve_memory(img_empty_boxart.width, img_empty_boxart.height, 2 /* 16Bit */);

  temp = texman_create();
  draw_load_texture_buffer("THEME/SHARED/HIGHLIGHT.PVR", &txr_highlight, texman_get_tex_data(temp));
  texman_reserve_memory(txr_highlight.width, txr_highlight.height, 2 /* 16Bit */);

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
  
  font_bmf_init("FONT/BASILEA.FNT", "FONT/BASILEA_W.PVR", settings->aspect);

  printf("Texture scratch free: %d/%d KB (%d/%d bytes)\n", texman_get_space_available() / 1024, (1024 * 1024) / 1024, texman_get_space_available(), (1024 * 1024));
}

static void handle_input_ui(enum control input) {
  direction_last = direction_current;
  direction_current = false;
  boxart_button_held = false;
  switch (input) {
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
      menu_up(ROWS);
      break;
    case TRIG_R:
      direction_current = true;
      menu_down(ROWS);
      break;
    case A:
      menu_accept();
      break;
    case START:
      menu_settings();
      break;
    case Y: {
      menu_exit();
    } break;
    case X:
      menu_show_large_art();
      boxart_button_held = true;
      break;

    /* These dont do anything */
    case B:
      menu_cb();
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

/* Reset variables sensibly */
FUNCTION(UI_NAME, setup) {
  list_current = list_get();
  list_len = list_length();

  screen_column = screen_row = 0;
  current_starting_index = 0;
  draw_current = DRAW_UI;

  navigate_timeout = INPUT_TIMEOUT * 2;

  anim_clear(&anim_highlight);
  anim_clear(&anim_large_art_pos);
  anim_clear(&anim_large_art_scale);
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
  update_time();

  draw_grid_boxes();
  draw_game_title();

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
