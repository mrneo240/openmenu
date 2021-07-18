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
#include "../backend/gd_list.h"
#include "draw_kos.h"
#include "draw_prototypes.h"
#include "font_prototypes.h"

#pragma region Settings_Menu

const char* menu_choice_text[] = {"Theme", "Region", "Aspect", "Beep", "Sort", "Filter"};
const char* theme_choice_text[] = {"LineDesc", "Grid3", "GDMENU"};
const char* region_choice_text[] = {"NTSC-U", "NTSC-J", "PAL"};
const char* aspect_choice_text[] = {"4:3", "16:9"};
const char* beep_choice_text[] = {"Off", "On"};
const char* sort_choice_text[] = {"Default", "Name", "Date", "Product"};
const char* filter_choice_text[] = {
    "All", "Action", "Racing", "Simulation", "Sports", "Lightgun",
    "Fighting", "Shooter", "Survival", "Adventure", "Platformer", "RPG",
    "Shmup", "Strategy", "Puzzle", "Arcade", "Music"};
const char* save_choice_text[] = {"Save", "Apply"};
const char* credits_text[] = {"Credits"};

#define MENU_OPTIONS ((int)(sizeof(menu_choice_text) / sizeof(menu_choice_text)[0]))
#define MENU_CHOICES (MENU_OPTIONS) /* Only those with selectable options */
#define THEME_CHOICES (sizeof(theme_choice_text) / sizeof(theme_choice_text)[0])
#define REGION_CHOICES (sizeof(region_choice_text) / sizeof(region_choice_text)[0])
#define ASPECT_CHOICES (sizeof(aspect_choice_text) / sizeof(aspect_choice_text)[0])
#define BEEP_CHOICES (sizeof(beep_choice_text) / sizeof(beep_choice_text)[0])
#define SORT_CHOICES (sizeof(sort_choice_text) / sizeof(sort_choice_text)[0])
#define FILTER_CHOICES (sizeof(filter_choice_text) / sizeof(filter_choice_text)[0])

typedef enum MENU_CHOICE {
  CHOICE_START,
  CHOICE_THEME = CHOICE_START,
  CHOICE_REGION,
  CHOICE_ASPECT,
  CHOICE_BEEP,
  CHOICE_SORT,
  CHOICE_FILTER,
  CHOICE_SAVE,
  CHOICE_CREDITS,
  CHOICE_END = CHOICE_CREDITS
} MENU_CHOICE;

#define INPUT_TIMEOUT (10)

static int choices[MENU_CHOICES + 1];
static int choices_max[MENU_CHOICES + 1] = {THEME_CHOICES, REGION_CHOICES, ASPECT_CHOICES, BEEP_CHOICES, SORT_CHOICES, FILTER_CHOICES, 2 /* Apply/Save */};
static const char** menu_choice_array[MENU_CHOICES] = {theme_choice_text, region_choice_text, aspect_choice_text, beep_choice_text, sort_choice_text, filter_choice_text};
static int current_choice = CHOICE_START;
static int navigate_timeout;

#pragma endregion Settings_Menu

#pragma region Credits_Menu
typedef struct credit_pair {
  const char* contributor;
  const char* role;
} credit_pair;

static const credit_pair credits[] = {
    (credit_pair){"megavolt85", "gdemu sdk"},
    (credit_pair){"u/westhinksdifferent/", "UI Mockups"},
    (credit_pair){"FlorreW", "Metadata DB"},
    (credit_pair){"hasnopants", "Metadata DB"},
    (credit_pair){"Roareye", "Metadata DB"},
    (credit_pair){"sonik-br", "GDMENUCardManager"},
    (credit_pair){"protofall", "Crayon_VMU"},
    (credit_pair){"TheLegendOfXela", "Boxart (Customs)"},
    (credit_pair){"Various Testers", "Breaking Things"},
    (credit_pair){"Kofi Supporters", "Coffee+Hardware"},
    (credit_pair){"mrneo240", "Author"},
};
static const int num_credits = sizeof(credits) / sizeof(credit_pair);

#pragma endregion Credits_Menu

static enum draw_state* state_ptr = NULL;
static uint32_t text_color;
static uint32_t highlight_color;
static openmenu_settings* settings;

void menu_setup(enum draw_state* state, uint32_t _text_color, uint32_t _highlight_color) {
  /* Keep local pointer to reference */
  settings = settings_get();

  choices[CHOICE_THEME] = settings->ui;
  choices[CHOICE_REGION] = settings->region;
  choices[CHOICE_ASPECT] = settings->aspect;
  choices[CHOICE_SORT] = settings->sort;
  choices[CHOICE_FILTER] = settings->filter;
  choices[CHOICE_BEEP] = settings->beep;

  /* So we can modify the state back to normal */
  state_ptr = state;
  text_color = _text_color;
  highlight_color = _highlight_color;

  navigate_timeout = INPUT_TIMEOUT;
}

static void menu_leave(void) {
  if (navigate_timeout > 0) {
    return;
  }
  *state_ptr = DRAW_UI;
  navigate_timeout = INPUT_TIMEOUT;
}

static void credits_leave(void) {
  if (navigate_timeout > 0) {
    return;
  }
  *state_ptr = DRAW_MENU;
  navigate_timeout = INPUT_TIMEOUT;
}

static void menu_accept(void) {
  if (navigate_timeout > 0) {
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

    /* If not filtering, then plain sort */
    if (!choices[CHOICE_FILTER]) {
      switch ((CFG_SORT)choices[CHOICE_SORT]) {
        case SORT_NAME:
          list_get_sort_name();
          break;
        case SORT_DATE:
          list_get_sort_date();
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

    if (choices[CHOICE_SAVE] == 0 /* Save */)
      settings_save();
    extern void reload_ui(void);
    reload_ui();
    //navigate_timeout = (60 * 1) /* 1 second */;
  }
  if (current_choice == CHOICE_CREDITS) {
    *state_ptr = DRAW_CREDITS;
    navigate_timeout = (60 * 1) /* 1 seconds */;
  }
}

static void menu_choice_prev(void) {
  if (navigate_timeout > 0) {
    return;
  }
  current_choice--;
  if (current_choice < CHOICE_START) {
    current_choice = CHOICE_START;
  }
  navigate_timeout = INPUT_TIMEOUT;
}

static void menu_choice_next(void) {
  if (navigate_timeout > 0) {
    return;
  }
  current_choice++;
  if (current_choice > CHOICE_END) {
    current_choice = CHOICE_END;
  }
  navigate_timeout = INPUT_TIMEOUT;
}

static void menu_choice_left(void) {
  if ((navigate_timeout > 0) || (current_choice >= CHOICE_CREDITS)) {
    return;
  }
  choices[current_choice]--;
  if (choices[current_choice] < 0) {
    choices[current_choice] = 0;
  }
  navigate_timeout = INPUT_TIMEOUT;
}
static void menu_choice_right(void) {
  if ((navigate_timeout > 0) || (current_choice >= CHOICE_CREDITS)) {
    return;
  }
  choices[current_choice]++;
  if (choices[current_choice] >= choices_max[current_choice]) {
    choices[current_choice]--;
  }
  navigate_timeout = INPUT_TIMEOUT;
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
    case B:
      menu_leave();
      break;
    case A:
      menu_accept();
      break;
    default:
      break;
  }
  navigate_timeout--;
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
  navigate_timeout--;
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

void draw_menu_tr(void) {
  z_set(205.0f);
  if (settings->ui == UI_GDMENU) {
    /* Menu size and placement */
    const int line_height = 24;
    const int width = 320;
    const int height = (MENU_OPTIONS + 3) * line_height + (line_height / 2);
    const int x = (640 / 2) - (width / 2);
    const int y = (480 / 2) - (height / 2);
    const int x_item = x + 4;
    const int border_width = 2;

    char line_buf[65];
    /* Draw a rectangle in the middle of the screen */

    uint32_t menu_bkg_color = COLOR_BLACK;
    uint32_t menu_bkg_border_color = COLOR_WHITE;
    if (text_color == COLOR_BLACK) {
      menu_bkg_color = COLOR_WHITE;
      menu_bkg_border_color = COLOR_BLACK;
    }

    draw_draw_quad(x - border_width, y - border_width, width + (2 * border_width), height + (2 * border_width), menu_bkg_border_color);
    draw_draw_quad(x, y, width, height, menu_bkg_color);

    /* overlay our text on top with options */
    int cur_y = y + 2;
    font_bmp_begin_draw();
    font_bmp_set_color(text_color);

    font_bmp_draw_main(x_item, cur_y, "Settings");

    cur_y += line_height / 2;
    for (int i = 0; i < MENU_CHOICES; i++) {
      cur_y += line_height;
      if (i == current_choice) {
        font_bmp_set_color(highlight_color);
      } else {
        font_bmp_set_color(text_color);
      }
      string_outer_concat(line_buf, menu_choice_text[i], menu_choice_array[i][(int)choices[i]], 39);
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
    const int width = 300;
    const int height = (MENU_OPTIONS + 3) * line_height + (line_height / 2);
    const int x = (640 / 2) - (width / 2);
    const int y = (480 / 2) - (height / 2);
    const int x_item = x + 4;
    const int x_choice = 344 + 24 + 20; /* magic :( */
    const int border_width = 2;

    font_bmf_set_height(24.0f);

    /* Draw a rectangle in the middle of the screen */
    uint32_t menu_bkg_color = COLOR_BLACK;
    uint32_t menu_bkg_border_color = COLOR_WHITE;
    if (text_color == COLOR_BLACK) {
      menu_bkg_color = COLOR_WHITE;
      menu_bkg_border_color = COLOR_BLACK;
    }

    draw_draw_quad(x - border_width, y - border_width, width + (2 * border_width), height + (2 * border_width), menu_bkg_border_color);
    draw_draw_quad(x, y, width, height, menu_bkg_color);

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
      font_bmf_draw_centered(x_choice, cur_y, temp_color, menu_choice_array[i][(int)choices[i]]);
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
  z_set(205.0f);

  if (settings->ui == UI_GDMENU) {
    /* Menu size and placement */
    const int line_height = 24;
    const int width = 320;
    const int height = (num_credits + 1) * line_height + (line_height / 2);
    const int x = (640 / 2) - (width / 2);
    const int y = (480 / 2) - (height / 2);
    const int x_item = x + 4;
    const int border_width = 2;

    char line_buf[65];
    /* Draw a rectangle in the middle of the screen */

    uint32_t menu_bkg_color = COLOR_BLACK;
    uint32_t menu_bkg_border_color = COLOR_WHITE;
    if (text_color == COLOR_BLACK) {
      menu_bkg_color = COLOR_WHITE;
      menu_bkg_border_color = COLOR_BLACK;
    }

    draw_draw_quad(x - border_width, y - border_width, width + (2 * border_width), height + (2 * border_width), menu_bkg_border_color);
    draw_draw_quad(x, y, width, height, menu_bkg_color);

    /* overlay our text on top with options */
    int cur_y = y + 4;
    font_bmp_begin_draw();
    font_bmp_set_color(text_color);

    font_bmp_draw_main(x_item, cur_y, "Credits");
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
    const int border_width = 2;

    /* Draw a rectangle in the middle of the screen */
    uint32_t menu_bkg_color = COLOR_BLACK;
    uint32_t menu_bkg_border_color = COLOR_WHITE;
    if (text_color == COLOR_BLACK) {
      menu_bkg_color = COLOR_WHITE;
      menu_bkg_border_color = COLOR_BLACK;
    }

    draw_draw_quad(x - border_width, y - border_width, width + (2 * border_width), height + (2 * border_width), menu_bkg_border_color);
    draw_draw_quad(x, y, width, height, menu_bkg_color);

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
