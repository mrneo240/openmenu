/*
 * File: ui_menu_credits.c
 * Project: ui
 * File Created: Monday, 12th July 2021 11:34:23 pm
 * Author: Hayden Kowalchuk
 * -----
 * Copyright (c) 2021 Hayden Kowalchuk, Hayden Kowalchuk
 * License: BSD 3-clause "New" or "Revised" License, http://www.opensource.org/licenses/BSD-3-Clause
 */

#include "ui_menu_credits.h"

#include <string.h>

#include "../backend/db_item.h"
#include "../backend/gd_item.h"
#include "../backend/gd_list.h"
#include "draw_kos.h"
#include "draw_prototypes.h"
#include "font_prototypes.h"

#pragma region Settings_Menu

static const char* menu_choice_text[] = {"Style", "Theme", "Aspect", "Beep", "Sort", "Filter", "Multidisc"};
static const char* theme_choice_text[] = {"LineDesc", "Grid3", "Scroll"};
static const char* region_choice_text[] = {"NTSC-U", "NTSC-J", "PAL"};
static const char* region_choice_text1[] = {"GDMENU"};
static const char* aspect_choice_text[] = {"4:3", "16:9"};
static const char* beep_choice_text[] = {"Off", "On"};
static const char* sort_choice_text[] = {"Default", "Name", "Date", "Product"};
static const char* filter_choice_text[] = {
    "All", "Action", "Racing", "Simulation", "Sports", "Lightgun",
    "Fighting", "Shooter", "Survival", "Adventure", "Platformer", "RPG",
    "Shmup", "Strategy", "Puzzle", "Arcade", "Music"};
static const char* multidisc_choice_text[] = {"Show", "Hide"};
static const char* save_choice_text[] = {"Save", "Apply"};
static const char* credits_text[] = {"Credits"};

const char* custom_theme_text[10] = {0};
static theme_custom* custom_themes;
static theme_scroll* custom_scroll;
static int num_custom_themes;

#define MENU_OPTIONS ((int)(sizeof(menu_choice_text) / sizeof(menu_choice_text)[0]))
#define MENU_CHOICES (MENU_OPTIONS) /* Only those with selectable options */
#define THEME_CHOICES (sizeof(theme_choice_text) / sizeof(theme_choice_text)[0])
static int REGION_CHOICES = (sizeof(region_choice_text) / sizeof(region_choice_text)[0]);
#define ASPECT_CHOICES (sizeof(aspect_choice_text) / sizeof(aspect_choice_text)[0])
#define BEEP_CHOICES (sizeof(beep_choice_text) / sizeof(beep_choice_text)[0])
#define SORT_CHOICES (sizeof(sort_choice_text) / sizeof(sort_choice_text)[0])
#define FILTER_CHOICES (sizeof(filter_choice_text) / sizeof(filter_choice_text)[0])
#define MULTIDISC_CHOICES (sizeof(multidisc_choice_text) / sizeof(multidisc_choice_text)[0])

typedef enum MENU_CHOICE {
  CHOICE_START,
  CHOICE_THEME = CHOICE_START,
  CHOICE_REGION,
  CHOICE_ASPECT,
  CHOICE_BEEP,
  CHOICE_SORT,
  CHOICE_FILTER,
  CHOICE_MULTIDISC,
  CHOICE_SAVE,
  CHOICE_CREDITS,
  CHOICE_END = CHOICE_CREDITS
} MENU_CHOICE;

#define INPUT_TIMEOUT (10)

static int choices[MENU_CHOICES + 1];
static int choices_max[MENU_CHOICES + 1] = {THEME_CHOICES, 3, ASPECT_CHOICES, BEEP_CHOICES, SORT_CHOICES, FILTER_CHOICES, MULTIDISC_CHOICES, 2 /* Apply/Save */};
static const char** menu_choice_array[MENU_CHOICES] = {theme_choice_text, region_choice_text, aspect_choice_text, beep_choice_text, sort_choice_text, filter_choice_text, multidisc_choice_text};
static int current_choice = CHOICE_START;
static int* input_timeout_ptr = NULL;

#pragma endregion Settings_Menu

#pragma region Credits_Menu
typedef struct credit_pair {
  const char* contributor;
  const char* role;
} credit_pair;

static const credit_pair credits[] = {
    (credit_pair){"megavolt85", "gdemu sdk, coder"},
    (credit_pair){"u/westhinksdifferent/", "UI Mockups"},
    (credit_pair){"FlorreW", "Metadata DB"},
    (credit_pair){"hasnopants", "Metadata DB"},
    (credit_pair){"Roareye", "Metadata DB"},
    (credit_pair){"sonik-br", "GDMENUCardManager"},
    (credit_pair){"protofall", "Crayon_VMU"},
    (credit_pair){"TheLegendOfXela", "Boxart (Customs)"},
    (credit_pair){"marky-b-1986", "Theming Ideas"},
    (credit_pair){"Various Testers", "Breaking Things"},
    (credit_pair){"Kofi Supporters", "Coffee+Hardware"},
    (credit_pair){"mrneo240", "Author"},
};
static const int num_credits = sizeof(credits) / sizeof(credit_pair);

#pragma endregion Credits_Menu

static enum draw_state* state_ptr = NULL;
static uint32_t text_color;
static uint32_t highlight_color;
static uint32_t menu_bkg_color;
static uint32_t menu_bkg_border_color;
static openmenu_settings* settings = NULL;

static void common_setup(enum draw_state* state, theme_color* _colors, int* timeout_ptr) {
  /* Ensure color themeing is consistent */
  text_color = _colors->menu_text_color;
  highlight_color = _colors->menu_highlight_color;
  menu_bkg_color = _colors->menu_bkg_color;
  menu_bkg_border_color = _colors->menu_bkg_border_color;

  /* So we can modify the shared state and input timeout */
  state_ptr = state;
  input_timeout_ptr = timeout_ptr;
  *input_timeout_ptr = (30 * 1) /* half a second */;
}

void menu_setup(enum draw_state* state, theme_color* _colors, int* timeout_ptr) {
  common_setup(state, _colors, timeout_ptr);

  /* Keep local pointer to reference */
  settings = settings_get();

  choices[CHOICE_THEME] = settings->ui;
  choices[CHOICE_REGION] = settings->region;
  choices[CHOICE_ASPECT] = settings->aspect;
  choices[CHOICE_SORT] = settings->sort;
  choices[CHOICE_FILTER] = settings->filter;
  choices[CHOICE_BEEP] = settings->beep;
  choices[CHOICE_MULTIDISC] = settings->multidisc;
  
  if (choices[CHOICE_THEME] != UI_SCROLL) {
    menu_choice_array[CHOICE_REGION] = region_choice_text;
    REGION_CHOICES = (sizeof(region_choice_text) / sizeof(region_choice_text)[0]);
    choices_max[CHOICE_REGION] = REGION_CHOICES;
    /* Grab custom themes if we have them */
    custom_themes = theme_get_custom(&num_custom_themes);
    if (num_custom_themes > 0) {
      for (int i = 0; i < num_custom_themes; i++) {
	    choices_max[CHOICE_REGION]++;
        custom_theme_text[i] = custom_themes[i].name;
      }
    }
  }
  else
  {
    menu_choice_array[CHOICE_REGION] = region_choice_text1;
    REGION_CHOICES = 1;
    choices_max[CHOICE_REGION] = 1;
    custom_scroll = theme_get_scroll(&num_custom_themes);
    if (num_custom_themes > 0) {
      for (int i = 0; i < num_custom_themes; i++) {
	    choices_max[CHOICE_REGION]++;
        custom_theme_text[i] = custom_scroll[i].name;
      }
      if (settings->custom_theme == THEME_ON) {
		  choices[CHOICE_REGION] = settings->custom_theme_num + 1;
	  }
    }
  }
  
  if (choices[CHOICE_REGION] >= choices_max[CHOICE_REGION]) {
	choices[CHOICE_REGION] = choices_max[CHOICE_REGION] - 1;
  }
}

void popup_setup(enum draw_state* state, theme_color* _colors, int* timeout_ptr) {
  common_setup(state, _colors, timeout_ptr);

  current_choice = CHOICE_START;
}

static void menu_leave(void) {
  if (*input_timeout_ptr > 0) {
    return;
  }
  *state_ptr = DRAW_UI;
  *input_timeout_ptr = (30 * 1) /* half a second */;
}

static void credits_leave(void) {
  if (*input_timeout_ptr > 0) {
    return;
  }
  *state_ptr = DRAW_MENU;
  *input_timeout_ptr = INPUT_TIMEOUT;
}

static void menu_accept(void) {
  if (*input_timeout_ptr > 0) {
    return;
  }
  if (current_choice == CHOICE_SAVE) {
    /* update Global Settings */
    settings->ui = choices[CHOICE_THEME];
    settings->region = choices[CHOICE_REGION];
    settings->aspect = choices[CHOICE_ASPECT];
    settings->sort = choices[CHOICE_SORT];
    settings->filter = choices[CHOICE_FILTER];
    settings->beep = choices[CHOICE_BEEP];
    settings->multidisc = choices[CHOICE_MULTIDISC];
    if (choices[CHOICE_THEME] != UI_SCROLL && settings->region > REGION_END) {
      settings->custom_theme = THEME_ON;
      int num_default_themes = 0;
      theme_get_default(settings->aspect, &num_default_themes);
      settings->custom_theme_num = settings->region - num_default_themes;
    } else if (choices[CHOICE_THEME] == UI_SCROLL && settings->region > 0) {
      settings->custom_theme = THEME_ON;
      settings->custom_theme_num = settings->region - 1;
    } else {
      settings->custom_theme = THEME_OFF;
    }

    /* If not filtering, then plain sort */
    if (!choices[CHOICE_FILTER]) {
      switch ((CFG_SORT)choices[CHOICE_SORT]) {
        case SORT_NAME:
          list_get_sort_name();
          break;
        case SORT_DATE:
          list_get_sort_date();
          break;
        case SORT_PRODUCT:
		  list_get_sort_product();
		  break;
        default:
        case SORT_DEFAULT:
          list_get_sort_default();
          break;
      }
    } else {
      /* If filtering, filter down to only genre then sort */
      list_get_genre_sort((FLAGS_GENRE)choices[CHOICE_FILTER] - 1, choices[CHOICE_SORT]);
    }

    if (choices[CHOICE_SAVE] == 0 /* Save */){
      settings_save();
    }
    extern void reload_ui(void);
    reload_ui();
  }
  if (current_choice == CHOICE_CREDITS) {
    *state_ptr = DRAW_CREDITS;
    *input_timeout_ptr = (60 * 1) /* 1 seconds */;
  }
}

static void menu_choice_prev(void) {
  if (*input_timeout_ptr > 0) {
    return;
  }
  current_choice--;
  if (current_choice < CHOICE_START) {
    current_choice = CHOICE_START;
  }
  *input_timeout_ptr = INPUT_TIMEOUT;
}

static void menu_choice_next(void) {
  if (*input_timeout_ptr > 0) {
    return;
  }
  current_choice++;
  if (current_choice > CHOICE_END) {
    current_choice = CHOICE_END;
  }
  *input_timeout_ptr = INPUT_TIMEOUT;
}

static void menu_region_adj(void) {
  if (choices[CHOICE_THEME] != UI_SCROLL) {
    menu_choice_array[CHOICE_REGION] = region_choice_text;
    REGION_CHOICES = (sizeof(region_choice_text) / sizeof(region_choice_text)[0]);
    choices_max[CHOICE_REGION] = REGION_CHOICES;
    /* Grab custom themes if we have them */
    custom_themes = theme_get_custom(&num_custom_themes);
    if (num_custom_themes > 0) {
      for (int i = 0; i < num_custom_themes; i++) {
	    choices_max[CHOICE_REGION]++;
        custom_theme_text[i] = custom_themes[i].name;
      }
    }
  }
  else {
    menu_choice_array[CHOICE_REGION] = region_choice_text1;
    REGION_CHOICES = (sizeof(region_choice_text1) / sizeof(region_choice_text1)[0]);
    choices_max[CHOICE_REGION] = REGION_CHOICES;
    custom_scroll = theme_get_scroll(&num_custom_themes);
    if (num_custom_themes > 0) {
      for (int i = 0; i < num_custom_themes; i++) {
	    choices_max[CHOICE_REGION]++;
        custom_theme_text[i] = custom_scroll[i].name;
      }
    }
  }
  
  if (choices[CHOICE_REGION] >= choices_max[CHOICE_REGION]) {
	choices[CHOICE_REGION] = choices_max[CHOICE_REGION] - 1;
  }
}

static void menu_choice_left(void) {
  if ((*input_timeout_ptr > 0) || (current_choice >= CHOICE_CREDITS)) {
    return;
  }
  choices[current_choice]--;
  if (choices[current_choice] < 0) {
    choices[current_choice] = 0;
  }
  if (current_choice == CHOICE_THEME) {
	  menu_region_adj();
  }
  *input_timeout_ptr = INPUT_TIMEOUT;
}

static void menu_choice_right(void) {
  if ((*input_timeout_ptr > 0) || (current_choice >= CHOICE_CREDITS)) {
    return;
  }
  choices[current_choice]++;
  if (choices[current_choice] >= choices_max[current_choice]) {
    choices[current_choice]--;
  }
  if (current_choice == CHOICE_THEME) {
	  menu_region_adj();
  }
  *input_timeout_ptr = INPUT_TIMEOUT;
}

static void menu_multidisc_prev(void) {
  if (*input_timeout_ptr > 0) {
    return;
  }
  current_choice--;
  if (current_choice < 0) {
    current_choice = 0;
  }
  *input_timeout_ptr = INPUT_TIMEOUT;
}

static void menu_multidisc_next(void) {
  if (*input_timeout_ptr > 0) {
    return;
  }
  current_choice++;
  int multidisc_len = list_multidisc_length();
  if (current_choice >= multidisc_len) {
    current_choice--;
  }
  *input_timeout_ptr = INPUT_TIMEOUT;
}

static void menu_accept_multidisc(void) {
  if (*input_timeout_ptr > 0) {
    return;
  }
  const gd_item** list_multidisc = list_get_multidisc();
  dreamcast_launch_disc(list_multidisc[current_choice]);
}

static void menu_exit(void) {
  if (*input_timeout_ptr > 0) {
    return;
  }
  
  /* Probably should change this */
  exit_to_bios();
}

void handle_input_menu(enum control input) {
  switch (input) {
    case LEFT:
      menu_choice_left();
      break;
    case RIGHT:
      menu_choice_right();
      break;
    case UP:
      menu_choice_prev();
      break;
    case DOWN:
      menu_choice_next();
      break;
    case START:
    case B:
      menu_leave();
      break;
    case A:
      menu_accept();
      break;
    default:
      break;
  }
}

void handle_input_credits(enum control input) {
  switch (input) {
    case A:
    case B:
    case START:
      credits_leave();
      break;
    default:
      break;
  }
}

void handle_input_multidisc(enum control input) {
  switch (input) {
    case UP:
      menu_multidisc_prev();
      break;
    case DOWN:
      menu_multidisc_next();
      break;
    case B:
      menu_leave();
      break;
    case A:
      menu_accept_multidisc();
      break;
    default:
      break;
  }
}

void handle_input_exit(enum control input) {
  switch (input) {
    case B:
      menu_leave();
      break;
    case A:
      menu_exit();
      break;
    default:
      break;
  }
}

void draw_menu_op(void) {
  /* might be useless */
}

static void string_outer_concat(char* out, const char* left, const char* right, int len) {
  const int input_len = strlen(left) + strlen(right);
  strcpy(out, left);
  for (int i = 0; i < len - input_len; i++)
    strcat(out, " ");
  strcat(out, right);
}

static void draw_popup_menu(int x, int y, int width, int height) {
  const int border_width = 2;
  draw_draw_quad(x - border_width, y - border_width, width + (2 * border_width), height + (2 * border_width), menu_bkg_border_color);
  draw_draw_quad(x, y, width, height, menu_bkg_color);

  if (settings == NULL) {
    settings = settings_get();
  }
  
  if (settings->ui == UI_SCROLL) {
    /* Top header */
    draw_draw_quad(x, y, width, 20, menu_bkg_border_color);
  }
}

void draw_menu_tr(void) {
  z_set_cond(205.0f);
  if (settings->ui == UI_SCROLL) {
    /* Menu size and placement */
    const int line_height = 24;
    const int width = 320;
    const int height = (MENU_OPTIONS + 3) * line_height + (line_height / 2);
    const int x = (640 / 2) - (width / 2);
    const int y = (480 / 2) - (height / 2);
    const int x_item = x + 4;

    char line_buf[65];

    /* Draw a popup in the middle of the screen */
    draw_popup_menu(x, y, width, height);

    /* overlay our text on top with options */
    int cur_y = y + 2;
    font_bmp_begin_draw();
    font_bmp_set_color(highlight_color);

    font_bmp_draw_main(width - (8 * 8 / 2), cur_y, "Settings");

    cur_y += line_height / 2;
    for (int i = 0; i < MENU_CHOICES; i++) {
      cur_y += line_height;
      if (i == current_choice) {
        font_bmp_set_color(highlight_color);
      } else {
        font_bmp_set_color(text_color);
      }
      if (i == CHOICE_REGION && (choices[i] >= REGION_CHOICES)) {
        string_outer_concat(line_buf, menu_choice_text[i], custom_theme_text[(int)choices[i] - REGION_CHOICES], 39);
      } else {
        string_outer_concat(line_buf, menu_choice_text[i], menu_choice_array[i][(int)choices[i]], 39);
      }
      font_bmp_draw_main(x_item, cur_y, line_buf);
    }

    /* Draw save or apply choice, highlight the current one */
    uint32_t save_color = ((current_choice == CHOICE_SAVE) && (choices[CHOICE_SAVE] == 0) ? highlight_color : text_color);
    uint32_t apply_color = ((current_choice == CHOICE_SAVE) && (choices[CHOICE_SAVE] == 1) ? highlight_color : text_color);
    font_bmp_set_color(save_color);
    cur_y += line_height;
    font_bmp_draw_main(640 / 2 - (8 * (5 + 7)), cur_y, save_choice_text[0]);
    font_bmp_set_color(apply_color);
    font_bmp_draw_main(640 / 2 + (8 * 8), cur_y, save_choice_text[1]);

    if (current_choice == CHOICE_CREDITS) {
      font_bmp_set_color(highlight_color);
    } else {
      font_bmp_set_color(text_color);
    }
    cur_y += line_height;
    font_bmp_draw_main(640 / 2 - (8 * 2), cur_y, credits_text[0]);

  } else {
    /* Menu size and placement */
    const int line_height = 32;
    const int width = 400;
    const int height = (MENU_OPTIONS + 3) * line_height + (line_height / 2);
    const int x = (640 / 2) - (width / 2);
    const int y = (480 / 2) - (height / 2);
    const int x_item = x + 4;
    const int x_choice = 344 + 24 + 20 + 25; /* magic :( */

    /* Draw a popup in the middle of the screen */
    draw_popup_menu(x, y, width, height);

    /* overlay our text on top with options */
    int cur_y = y + 2;
    font_bmf_begin_draw();
    font_bmf_set_height_default();

    font_bmf_draw(x_item, cur_y, text_color, "Settings");

    cur_y += line_height / 4;
    for (int i = 0; i < MENU_CHOICES; i++) {
      cur_y += line_height;
      uint32_t temp_color = text_color;
      if (i == current_choice) {
        temp_color = highlight_color;
      }
      font_bmf_draw(x_item, cur_y, temp_color, menu_choice_text[i]);

      if (i == CHOICE_REGION && (choices[i] >= REGION_CHOICES)) {
        font_bmf_draw_centered(x_choice, cur_y, temp_color, custom_theme_text[(int)choices[i] - REGION_CHOICES]);
      } else {
        font_bmf_draw_centered(x_choice, cur_y, temp_color, menu_choice_array[i][(int)choices[i]]);
      }
    }

    /* Draw save or apply choice, highlight the current one */
    uint32_t save_color = ((current_choice == CHOICE_SAVE) && (choices[CHOICE_SAVE] == 0) ? highlight_color : text_color);
    uint32_t apply_color = ((current_choice == CHOICE_SAVE) && (choices[CHOICE_SAVE] == 1) ? highlight_color : text_color);
    cur_y += line_height;
    font_bmf_draw_centered(640 / 2 - (width / 4), cur_y, save_color, save_choice_text[0]);
    font_bmf_draw_centered(640 / 2 + (width / 4), cur_y, apply_color, save_choice_text[1]);

    uint32_t temp_color = ((current_choice == CHOICE_CREDITS) ? highlight_color : text_color);
    cur_y += line_height;
    font_bmf_draw_centered(640 / 2, cur_y, temp_color, credits_text[0]);
  }
}

void draw_credits_op(void) {
  /* Again nothing... */
}

void draw_credits_tr(void) {
  z_set_cond(205.0f);

  if (settings->ui == UI_SCROLL) {
    /* Menu size and placement */
    const int line_height = 24;
    const int width = 320;
    const int height = (num_credits + 1) * line_height + (line_height / 2);
    const int x = (640 / 2) - (width / 2);
    const int y = (480 / 2) - (height / 2);
    const int x_item = x + 4;

    char line_buf[65];
    /* Draw a rectangle in the middle of the screen */

    /* Draw a popup in the middle of the screen */
    draw_popup_menu(x, y, width, height);

    /* overlay our text on top with options */
    int cur_y = y + 4;
    font_bmp_begin_draw();
    font_bmp_set_color(text_color);

    font_bmp_draw_main(width - (8 * 8 / 2), cur_y, "Credits");
    font_bmp_set_color(highlight_color);

    cur_y += line_height / 2;
    for (int i = 0; i < num_credits; i++) {
      cur_y += line_height;
      string_outer_concat(line_buf, credits[i].contributor, credits[i].role, 39);
      font_bmp_draw_main(x_item, cur_y, line_buf);
    }

  } else {
    /* Menu size and placement */
    const int line_height = 26;
    const int width = 560;
    const int height = (num_credits + 2) * line_height;
    const int x = (640 / 2) - (width / 2);
    const int y = (480 / 2) - (height / 2);
    const int x_choice = 344 + 24 + 60; /* magic :( */
    const int x_item = x + 4;

    /* Draw a popup in the middle of the screen */
    draw_popup_menu(x, y, width, height);

    /* overlay our text on top with options */
    int cur_y = y + 2;
    font_bmf_begin_draw();
    font_bmf_set_height(24.0f);

    font_bmf_draw(x_item, cur_y, text_color, "Credits");

    cur_y += line_height / 4;
    for (int i = 0; i < num_credits; i++) {
      cur_y += line_height;
      font_bmf_draw(x_item, cur_y, highlight_color, credits[i].contributor);
      font_bmf_draw_centered(x_choice, cur_y, highlight_color, credits[i].role);
    }
  }
}

void draw_multidisc_op(void) {
  /* Again nothing...Still... */
}

void draw_multidisc_tr(void) {
  const gd_item** list_multidisc = list_get_multidisc();
  int multidisc_len = list_multidisc_length();

  z_set_cond(205.0f);
  if (settings->ui == UI_SCROLL) {
    /* Menu size and placement */
    const int line_height = 24;
    const int width = 320;
    const int height = (multidisc_len + 1) * line_height + (line_height / 2);
    const int x = (640 / 2) - (width / 2);
    const int y = (480 / 2) - (height / 2);
    const int x_item = x + 4;
    char line_buf[65];
    char temp_game_name[36];
    char temp_game_num[4];

    /* Draw a popup in the middle of the screen */
    draw_popup_menu(x, y, width, height);

    /* overlay our text on top with options */
    int cur_y = y + 2;
    font_bmp_begin_draw();
    font_bmp_set_color(text_color);

    font_bmp_draw_main(width - (10 * 8 / 2), cur_y, "Multidisc");

    cur_y += line_height / 2;
    for (int i = 0; i < multidisc_len; i++) {
      cur_y += line_height;
      if (i == current_choice) {
        font_bmp_set_color(highlight_color);
      } else {
        font_bmp_set_color(text_color);
      }
      const int disc_num = list_multidisc[i]->disc[0] - '0';
      strncpy(temp_game_name, list_multidisc[i]->name, sizeof(temp_game_name) - 1);
      temp_game_name[sizeof(temp_game_name) - 1] = '\0';
      snprintf(temp_game_num, sizeof(temp_game_name), "#%d", disc_num);
      string_outer_concat(line_buf, temp_game_name, temp_game_num, 39);
      font_bmp_draw_main(x_item, cur_y, line_buf);
    }
  } else {
    /* Menu size and placement */
    const int line_height = 32;
    const int width = 300;
    const int height = (multidisc_len + 1) * line_height + (line_height / 2);
    const int x = (640 / 2) - (width / 2);
    const int y = (480 / 2) - (height / 2);
    const int x_item = x + 4;
    char line_buf[65];
    char temp_game_name[62];

    /* Draw a popup in the middle of the screen */
    draw_popup_menu(x, y, width, height);

    /* overlay our text on top with options */
    int cur_y = y + 2;
    font_bmf_begin_draw();
    font_bmf_set_height_default();

    font_bmf_draw_centered(x+width/2, cur_y, text_color, "Multidisc");

    cur_y += line_height / 4;

    for (int i = 0; i < multidisc_len; i++) {
      cur_y += line_height;
      uint32_t temp_color = text_color;
      if (i == current_choice) {
        temp_color = highlight_color;
      }
      const int disc_num = list_multidisc[i]->disc[0] - '0';
      strncpy(temp_game_name, list_multidisc[i]->name, sizeof(temp_game_name) - 1);
      temp_game_name[sizeof(temp_game_name) - 1] = '\0';
      snprintf(line_buf, 65, "%s #%d", temp_game_name, disc_num);
      font_bmf_draw_auto_size(x_item, cur_y, temp_color, line_buf, width - 4);
    }
  }
}

void draw_exit_op(void) {
  /* Again nothing...Still...Ugh... */
}

void draw_exit_tr(void) {
	z_set_cond(205.0f);
	
	/* Draw a popup in the middle of the screen */
    draw_popup_menu(160, 120, 180, 80);
    
	if (settings->ui == UI_SCROLL) {
		font_bmp_begin_draw();
		font_bmp_set_color(text_color);

		font_bmp_draw_main(200, 122, "Exit to BIOS");
		font_bmp_set_color(highlight_color);
		font_bmp_draw_main(168, 158, "A - exit, B - cancel");
	}
	else {
		font_bmf_begin_draw();
		font_bmf_set_height(24.0);

		font_bmf_draw(200, 122, text_color, "Exit to BIOS");
		font_bmf_draw(168, 158, highlight_color, "A - exit, B - cancel");
	}
}

