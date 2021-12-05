/*
 * File: theme_manager.h
 * Project: ui
 * File Created: Tuesday, 27th July 2021 12:09:25 pm
 * Author: Hayden Kowalchuk
 * -----
 * Copyright (c) 2021 Hayden Kowalchuk, Hayden Kowalchuk
 * License: BSD 3-clause "New" or "Revised" License, http://www.opensource.org/licenses/BSD-3-Clause
 */

#pragma once

#include <stdint.h>
#include <string.h>

#include "global_settings.h"

struct image;

typedef struct theme_color {
  uint32_t text_color;
  uint32_t highlight_color;
  uint32_t menu_text_color;
  uint32_t menu_highlight_color;
  uint32_t menu_bkg_color;
  uint32_t menu_bkg_border_color;
  uint32_t icon_color;
} theme_color;

typedef struct theme_region {
  const char *bg_left;
  const char *bg_right;
  theme_color colors;
} theme_region;

typedef struct theme_custom {
  char bg_left[32];
  char bg_right[32];
  char name[20];
  theme_color colors;
} theme_custom;

int theme_manager_load(void);
theme_region *theme_get_default(CFG_ASPECT aspect, int *num_themes);
theme_custom *theme_get_custom(int *num_themes);
