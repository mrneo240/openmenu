/*
 * File: ui_scroll.c
 * Project: ui
 * File Created: Sunday, 13th June 2021 12:32:47 pm
 * Author: Hayden Kowalchuk
 * -----
 * Copyright (c) 2021 Hayden Kowalchuk, Hayden Kowalchuk
 * License: BSD 3-clause "New" or "Revised" License, http://www.opensource.org/licenses/BSD-3-Clause
 */

#include "ui_scroll.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../backend/gd_item.h"
#include "../backend/gd_list.h"
#include "../texture/txr_manager.h"
#include "draw_prototypes.h"
#include "font_prototypes.h"
#include "global_settings.h"
#include "ui_menu_credits.h"

#define UNUSED __attribute__((unused))

static image txr_bg_left, txr_bg_right;

/* Info taken from megavolt85 and RazorX */
/* GDMENU Default Colors */
//static const uint32_t UNUSED color_window_title = PVR_PACK_ARGB(255, 103, 193, 245);
	//static const uint32_t UNUSED color_options_default = PVR_PACK_ARGB(255, 164, 158, 152);
	//static const uint32_t UNUSED color_options_highlight = PVR_PACK_ARGB(255, 103, 193, 245);
//static const uint32_t UNUSED color_options_save = PVR_PACK_ARGB(255, 152, 158, 164);
//static const uint32_t UNUSED color_sysinfo_default = PVR_PACK_ARGB(255, 152, 158, 164);
//static const uint32_t UNUSED color_about_default = PVR_PACK_ARGB(255, 152, 158, 164);
//static const uint32_t UNUSED color_exit_default = PVR_PACK_ARGB(255, 152, 158, 164);
//static const uint32_t UNUSED color_exit_highlight = PVR_PACK_ARGB(255, 103, 193, 245);
//static const uint32_t UNUSED color_cb_default = PVR_PACK_ARGB(255, 152, 158, 164);
//static const uint32_t UNUSED color_cb_highlight = PVR_PACK_ARGB(255, 103, 193, 245);
	//static const uint32_t UNUSED color_main_default = PVR_PACK_ARGB(255, 152, 158, 164);
	//static const uint32_t UNUSED color_main_highlight = PVR_PACK_ARGB(255, 103, 193, 245);
//static const uint32_t UNUSED color_update_default = PVR_PACK_ARGB(255, 152, 158, 164);
//static const uint32_t UNUSED color_image_info_default = PVR_PACK_ARGB(255, 152, 158, 164);
//static const uint32_t UNUSED color_image_name_default = PVR_PACK_ARGB(255, 152, 158, 164);
//static const uint32_t UNUSED color_image_name_highlight = PVR_PACK_ARGB(255, 103, 193, 245);
//static const uint32_t UNUSED color_cursor_default = PVR_PACK_ARGB(255, 33, 56, 82); /* unsure */
	//static const uint32_t UNUSED color_cursor_actual = PVR_PACK_ARGB(255, 29, 44, 66);
	//static const uint32_t UNUSED color_menu_background = PVR_PACK_ARGB(255, 13, 44, 70);
	//static const uint32_t UNUSED color_gdmenu_green = PVR_PACK_ARGB(255, 100, 255, 225);

/* GDMENU Default positions */
//static const int cursor_width = 404;
//static const int cursor_height = 20;
//static const int items_per_page = 20;

//static const int pos_gameslist_x = 3;
//static const int pos_gameslist_y = 14;

//static const int pos_gameinfo_x = 424;
//static const int pos_gameinfo_region_y = 85;
//static const int pos_gameinfo_vga_y = 109;
//static const int pos_gameinfo_disc_y = 133;
//static const int pos_gameinfo_date_y = 157;
//static const int pos_gameinfo_version_y = 181;

//static const int pos_gametxr_x = 420;
//static const int pos_gametxr_y = 213;

static const theme_scroll default_theme = {
  "THEME/SCROLL/BG_L.PVR",
  "THEME/SCROLL/BG_R.PVR",
  "GDMENU",
  {
	PVR_PACK_ARGB(255, 152, 158, 164),
	PVR_PACK_ARGB(255, 103, 193, 245),
	PVR_PACK_ARGB(255, 164, 158, 152),
	PVR_PACK_ARGB(255, 103, 193, 245),
	COLOR_BLACK,
	PVR_PACK_ARGB(255, 13, 44, 70),
	COLOR_WHITE
  },
  "FONT/GDMNUFNT.PVR",
  PVR_PACK_ARGB(255, 29, 44, 66),
  PVR_PACK_ARGB(255, 100, 255, 225),
  404,
  20,
  20,
  3,
  14,
  424,
  85,
  109,
  133,
  157,
  181,
  420,
  213
};

static theme_scroll *cur_theme = NULL;
static theme_scroll *custom = NULL;
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

/*static theme_color gdemu_colors = {
    .text_color = color_main_default,
    .highlight_color = color_main_highlight,
    .menu_text_color = color_options_default,
    .menu_highlight_color = color_options_highlight,
    .menu_bkg_color = COLOR_BLACK,
    .menu_bkg_border_color = color_menu_background,
    .icon_color = COLOR_WHITE,
};*/

static image txr_focus;
extern image img_empty_boxart;

#define INPUT_TIMEOUT (5)

static int current_selected_item = 0;
static int current_starting_index = 0;
static int navigate_timeout = INPUT_TIMEOUT;
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

static void draw_gamelist(void) {
  char buffer[128];
  const int X_ADJUST_TEXT = 7;
  const int Y_ADJUST_TEXT = 4;
  const int Y_ADJUST_CRSR = 3; /* 2 pixels higher than text */
  font_bmp_begin_draw();
  if (list_len <= 0) {
    draw_draw_quad(cur_theme->pos_gameslist_x, cur_theme->pos_gameslist_y + Y_ADJUST_TEXT + (0 * 21) - Y_ADJUST_CRSR, cur_theme->cursor_width, cur_theme->cursor_height, cur_theme->cursor_color);
    font_bmp_set_color(cur_theme->colors.highlight_color);
    font_bmp_draw_main(cur_theme->pos_gameslist_x + X_ADJUST_TEXT, cur_theme->pos_gameslist_y + Y_ADJUST_TEXT + (0 * 21), "Empty Game List");
  }

  for (int i = 0; i < cur_theme->items_per_page; i++) {
    /* Break before issues */
    if ((current_starting_index + i) >= list_len) {
      break;
    }

    sprintf(buffer, "%02d %s", current_starting_index + i + 1, list_current[current_starting_index + i]->name);
    if ((current_starting_index + i) == current_selected_item) {
      /* grab the disc number and if there is more than one */
      int disc_set = list_current[current_selected_item]->disc[2] - '0';

      /* Get multidisc settings */
      openmenu_settings *settings = settings_get();
      int hide_multidisc = settings->multidisc;

      uint32_t highlight_text_color = cur_theme->colors.highlight_color;
      if (hide_multidisc && (disc_set > 1)) {
        highlight_text_color = cur_theme->multidisc_color;
      }
      
      draw_draw_quad(cur_theme->pos_gameslist_x, cur_theme->pos_gameslist_y + Y_ADJUST_TEXT + (i * 21) - Y_ADJUST_CRSR, 
					 cur_theme->cursor_width, cur_theme->cursor_height, cur_theme->cursor_color);
      font_bmp_set_color(highlight_text_color);
    } else {
      font_bmp_set_color(cur_theme->colors.text_color);
    }

    font_bmp_draw_main(cur_theme->pos_gameslist_x + X_ADJUST_TEXT, cur_theme->pos_gameslist_y + Y_ADJUST_TEXT + (i * 21), buffer);
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
    default:
      return STR_FREE;
  }
  return STR_FREE;
}

static void draw_gameinfo(void) {
  if ((current_selected_item >= list_len) || (list_len <= 0)) {
    return;
  }

  char line_buf[26];
  char date_buf[11];

  font_bmp_begin_draw();
  font_bmp_set_color(cur_theme->colors.highlight_color); /* Unsure */
  // Region
  string_outer_concat(line_buf, STR_REGION, region_code_to_readable(list_current[current_selected_item]->region), INFO_STR_LEN);
  font_bmp_draw_main(cur_theme->pos_gameinfo_x, cur_theme->pos_gameinfo_region_y, line_buf);
  // VGA
  string_outer_concat(line_buf, STR_VGA, (list_current[current_selected_item]->vga[0] == '1' ? STR_YES : STR_NO), INFO_STR_LEN);
  font_bmp_draw_main(cur_theme->pos_gameinfo_x, cur_theme->pos_gameinfo_vga_y, line_buf);
  // DISC
  string_outer_concat(line_buf, STR_DISC, list_current[current_selected_item]->disc, INFO_STR_LEN);
  font_bmp_draw_main(cur_theme->pos_gameinfo_x, cur_theme->pos_gameinfo_disc_y, line_buf);
  // DATE
  string_outer_concat(line_buf, STR_DATE, transform_date_readable(date_buf, list_current[current_selected_item]->date), INFO_STR_LEN);
  font_bmp_draw_main(cur_theme->pos_gameinfo_x, cur_theme->pos_gameinfo_date_y, line_buf);
  // VERSION
  string_outer_concat(line_buf, STR_VERSION, list_current[current_selected_item]->version, INFO_STR_LEN);
  font_bmp_draw_main(cur_theme->pos_gameinfo_x, cur_theme->pos_gameinfo_version_y, line_buf);
}

static void draw_gameart(void) {
  if ((current_selected_item >= list_len) || (list_len <= 0)) {
    return;
  }

  txr_get_large(list_current[current_selected_item]->product, &txr_focus);
  if (txr_focus.texture == img_empty_boxart.texture) {
    txr_get_small(list_current[current_selected_item]->product, &txr_focus);
  }
  if (txr_focus.texture == img_empty_boxart.texture) {
    /* Only draw if image is present */
    return;
  }

  draw_draw_image(cur_theme->pos_gametxr_x, cur_theme->pos_gametxr_y, 210, 210, COLOR_WHITE, &txr_focus);
}

static void menu_decrement(int amount) {
  if (navigate_timeout > 0) {
    return;
  }
  if (current_selected_item < amount) {
    current_selected_item = 0;
  }
  else {
    current_selected_item -= amount;
  }
  if (current_selected_item < current_starting_index) {
    current_starting_index -= amount;
    if (current_starting_index < 0) {
      current_starting_index = 0;
    }
  }
  navigate_timeout = INPUT_TIMEOUT;
}

static void menu_increment(int amount) {
  if (navigate_timeout > 0) {
    return;
  }
  current_selected_item += amount;
  if (current_selected_item >= list_len) {
    current_selected_item = list_len - 1;
  }
  if (current_selected_item >= current_starting_index + cur_theme->items_per_page) {
    current_starting_index += amount;
  }
  navigate_timeout = INPUT_TIMEOUT;
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
    popup_setup(&draw_current, &cur_theme->colors, &navigate_timeout);
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
    popup_setup(&draw_current, &cur_theme->colors, &navigate_timeout);
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
  menu_setup(&draw_current, &cur_theme->colors, &navigate_timeout);
}

FUNCTION(UI_NAME, init) {
  texman_clear();
  /* @Note: these exist but do we really care? Naturally this will happen without forcing it and old data doesn't matter */
  // txr_empty_small_pool();
  // txr_empty_large_pool();
  openmenu_settings *settings = settings_get();
  if (settings->custom_theme) {
	int custom_theme_num = 0;
	custom = theme_get_scroll(&custom_theme_num);
	if ((int) settings->custom_theme_num > custom_theme_num) {
	  cur_theme = (theme_scroll *) &default_theme;
	} else {
	  cur_theme = &custom[settings->custom_theme_num];
	}
  }
  else {
    cur_theme = (theme_scroll *) &default_theme;
  }
  
  unsigned int temp = texman_create();
  draw_load_texture_buffer(cur_theme->bg_left, &txr_bg_left, texman_get_tex_data(temp));
  texman_reserve_memory(txr_bg_left.width, txr_bg_left.height, 2 /* 16Bit */);

  temp = texman_create();
  draw_load_texture_buffer(cur_theme->bg_right, &txr_bg_right, texman_get_tex_data(temp));
  texman_reserve_memory(txr_bg_right.width, txr_bg_right.height, 2 /* 16Bit */);

  font_bmp_init(cur_theme->font, 8, 16);

  printf("Texture scratch free: %d/%d KB (%d/%d bytes)\n", texman_get_space_available() / 1024, (1024 * 1024) / 1024, texman_get_space_available(), (1024 * 1024));
}

static void handle_input_ui(enum control input) {
  switch (input) {
    case UP:
      menu_decrement(1);
      break;
    case DOWN:
      menu_increment(1);
      break;
    case TRIG_L:
      menu_decrement(5);
      break;
    case TRIG_R:
      menu_increment(5);
      break;
    case A:
      menu_accept();
      break;
    case X:
      menu_settings();
      break;
    case Y: {
      exit_to_bios();
    } break;

      /* These dont do anything */
    case B:
	  menu_cb();
      break;

    /* Always nothing */
    case NONE:
    default:
      break;
  }
}

theme_scroll *get_def_scr_thm() {
  return (theme_scroll *) &default_theme;
}

FUNCTION(UI_NAME, setup) {
  list_current = list_get();
  list_len = list_length();

  current_selected_item = 0;
  current_starting_index = 0;
  navigate_timeout = INPUT_TIMEOUT * 2;
  draw_current = DRAW_UI;
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
  draw_gameart();

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
  draw_gamelist();
  draw_gameinfo();

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
