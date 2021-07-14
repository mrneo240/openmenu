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
  REGION_NTSC_U = 0,
  REGION_NTSC_J,
  REGION_PAL,
  REGION_END,
} CFG_REGION;

typedef enum CFG_ASPECT {
  ASPECT_NORMAL = 0,
  ASPECT_WIDE,
} CFG_ASPECT;

typedef enum CFG_UI {
  UI_START = 0,
  UI_LINE_DESC = 0,
  UI_GRID3,
  UI_GDMENU,
  UI_END
} CFG_UI;

typedef enum CFG_SORT {
  SORT_START = 0,
  SORT_DEFAULT = SORT_START,
  SORT_NAME,
  SORT_DATE,
  SORT_PRODUCT
} CFG_SORT;

typedef enum CFG_FILTER {
  FILTER_ALL,
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
  FILTER_MUSIC
} CFG_FILTER;

typedef struct openmenu_settings {
  CFG_REGION region;
  CFG_ASPECT aspect;
  CFG_UI ui;
  CFG_SORT sort;
  CFG_FILTER filter;
} openmenu_settings;

typedef CFG_REGION region;

enum draw_state { DRAW_UI = 0,
                  DRAW_MENU,
                  DRAW_CREDITS };

void settings_init(void);
void settings_load(void);
void settings_save(void);
openmenu_settings* settings_get(void);
