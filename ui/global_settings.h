/*
 * File: global_settings.h
 * Project: ui
 * File Created: Monday, 12th July 2021 10:33:26 am
 * Author: Hayden Kowalchuk
 * -----
 * Copyright (c) 2021 Hayden Kowalchuk, Hayden Kowalchuk
 * License: BSD 3-clause "New" or "Revised" License, http://www.opensource.org/licenses/BSD-3-Clause
 */

#pragma once

#include <stdint.h>
#include <stdio.h>

typedef enum CFG_REGION {
  REGION_START = 0,
  REGION_NTSC_U = REGION_START,
  REGION_NTSC_J,
  REGION_PAL,
  REGION_END = REGION_PAL,
} CFG_REGION;

typedef enum CFG_ASPECT {
  ASPECT_START = 0,
  ASPECT_NORMAL = ASPECT_START,
  ASPECT_WIDE,
  ASPECT_END = ASPECT_WIDE
} CFG_ASPECT;

typedef enum CFG_UI {
  UI_START = 0,
  UI_LINE_DESC = UI_START,
  UI_GRID3,
  UI_GDMENU,
  UI_END = UI_GDMENU
} CFG_UI;

typedef enum CFG_SORT {
  SORT_START = 0,
  SORT_DEFAULT = SORT_START,
  SORT_NAME,
  SORT_DATE,
  SORT_PRODUCT,
  SORT_END = SORT_PRODUCT
} CFG_SORT;

typedef enum CFG_FILTER {
  FILTER_START = 0,
  FILTER_ALL = FILTER_START,
  FILTER_ACTION,
  FILTER_RACING,
  FILTER_SIMULATION,
  FILTER_SPORTS,
  FILTER_LIGHTGUN,
  FILTER_FIGHTING,
  FILTER_SHOOTER,
  FILTER_SURVIVAL,
  FILTER_ADVENTURE,
  FILTER_PLATFORMER,
  FILTER_RPG,
  FILTER_SHMUP,
  FILTER_STRATEGY,
  FILTER_PUZZLE,
  FILTER_ARCADE,
  FILTER_MUSIC,
  FILTER_END = FILTER_MUSIC
} CFG_FILTER;

typedef enum CFG_BEEP {
  BEEP_START = 0,
  BEEP_OFF = BEEP_START,
  BEEP_ON,
  BEEP_END = BEEP_ON
} CFG_BEEP;

typedef enum CFG_MULTIDISC {
  MULTIDISC_START = 0,
  MULTIDISC_SHOW = MULTIDISC_START,
  MULTIDISC_HIDE,
  MULTIDISC_END = MULTIDISC_HIDE
} CFG_MULTIDISC;

typedef enum CFG_CUSTOM_THEME {
  THEME_START = 0,
  THEME_OFF = THEME_START,
  THEME_ON,
  THEME_END = THEME_ON
} CFG_CUSTOM_THEME;

typedef enum CFG_CUSTOM_THEME_NUM {
  THEME_NUM_START = 0,
  THEME_0 = THEME_NUM_START,
  THEME_1,
  THEME_2,
  THEME_3,
  THEME_4,
  THEME_5,
  THEME_6,
  THEME_7,
  THEME_8,
  THEME_9,
  THEME_NUM_END = THEME_9
} CFG_CUSTOM_THEME_NUM;

typedef struct openmenu_settings {
  char identifier[2]; /* OM */
  uint8_t version;    /* 0x01 */
  uint8_t padding;    /* 0x00 */
  CFG_REGION region;
  CFG_ASPECT aspect;
  CFG_UI ui;
  CFG_SORT sort;
  CFG_FILTER filter;
  CFG_BEEP beep;
  CFG_MULTIDISC multidisc;
  CFG_CUSTOM_THEME custom_theme;
  CFG_CUSTOM_THEME_NUM custom_theme_num;
} openmenu_settings;

typedef CFG_REGION region;

enum draw_state { DRAW_UI = 0,
                  DRAW_MULTIDISC,
                  DRAW_EXIT,
                  DRAW_MENU,
                  DRAW_CREDITS };

void settings_init(void);
void settings_load(void);
void settings_save(void);
void settings_validate(void);
openmenu_settings* settings_get(void);
