/*
 * File: global_settings.c
 * Project: ui
 * File Created: Monday, 12th July 2021 10:33:21 am
 * Author: Hayden Kowalchuk
 * -----
 * Copyright (c) 2021 Hayden Kowalchuk, Hayden Kowalchuk
 * License: BSD 3-clause "New" or "Revised" License, http://www.opensource.org/licenses/BSD-3-Clause
 */

#include "global_settings.h"

#include <external/libcrayonvmu/savefile.h>
#include <external/libcrayonvmu/setup.h>

/* Images and such */
#if __has_include("openmenu_lcd.h") && __has_include("openmenu_pal.h") && __has_include("openmenu_vmu.h")
#include "openmenu_lcd.h"
#include "openmenu_pal.h"
#include "openmenu_vmu.h"

#define OPENMENU_ICON (openmenu_icon)
#define OPENMENU_LCD (openmenu_lcd)
#define OPENMENU_PAL (openmenu_pal)
#define OPENMENU_ICONS (1)
#else
#define OPENMENU_ICON (NULL)
#define OPENMENU_LCD (NULL)
#define OPENMENU_PAL (NULL)
#define OPENMENU_ICONS (0)
#endif

static crayon_savefile_details_t savefile_details;
static openmenu_settings savedata;

static void settings_defaults(void) {
  savedata.identifier[0] = 'O';
  savedata.identifier[1] = 'M';
  savedata.version = 1;
  savedata.padding = 0;
  savedata.ui = UI_LINE_DESC;
  savedata.region = REGION_NTSC_U;
  savedata.aspect = ASPECT_NORMAL;
  savedata.sort = SORT_DEFAULT;
  savedata.filter = FILTER_ALL;
  savedata.beep = BEEP_ON;
  savedata.multidisc = MULTIDISC_SHOW;
  savedata.custom_theme = THEME_OFF;
  savedata.custom_theme_num = THEME_0;
}

static void settings_create(void) {
  // If we don't already have a savefile, choose a VMU
  if (savefile_details.valid_memcards) {
    for (int iter = 0; iter <= 3; iter++) {
      for (int jiter = 1; jiter <= 2; jiter++) {
        if (crayon_savefile_get_vmu_bit(savefile_details.valid_memcards, iter, jiter)) {  // Use the left most VMU
          savefile_details.savefile_port = iter;
          savefile_details.savefile_slot = jiter;
          goto Exit_loop_2;
        }
      }
    }
  }
Exit_loop_2:;

  settings_defaults();

  settings_save();
}

void settings_init(void) {
  /* Mostly from CrayonVMU example */
  crayon_savefile_init_savefile_details(&savefile_details,
                                        (uint8_t *)&savedata, sizeof(openmenu_settings), OPENMENU_ICONS, 1,
                                        "openMenu Preferences\0", "openMenu Config\0", "openMenuPref\0", "OPENMENU.CFG\0");

  savefile_details.icon = OPENMENU_ICON;
  savefile_details.icon_palette = (unsigned short *)OPENMENU_PAL;

#if OPENMENU_ICONS
  crayon_vmu_display_icon(savefile_details.valid_vmu_screens, OPENMENU_LCD);
#endif

  // Find the first savefile (if it exists)
  for (int iter = 0; iter <= 3; iter++) {
    for (int jiter = 1; jiter <= 2; jiter++) {
      if (crayon_savefile_get_vmu_bit(savefile_details.valid_saves, iter, jiter)) {  // Use the left most VMU
        savefile_details.savefile_port = iter;
        savefile_details.savefile_slot = jiter;
        goto Exit_loop_1;
      }
    }
  }
Exit_loop_1:

  settings_load();
  settings_validate();
}

void settings_validate(void) {
  if (savedata.version != 1) {
    settings_defaults();
    settings_save();
    return;
  }

  if ((savedata.ui < UI_START) || (savedata.ui > UI_END)) {
    savedata.ui = UI_LINE_DESC;
  }

  if ((savedata.region < REGION_START) || (savedata.region > REGION_END)) {
    savedata.region = REGION_NTSC_U;
  }

  if ((savedata.aspect < ASPECT_START) || (savedata.aspect > ASPECT_END)) {
    savedata.aspect = ASPECT_NORMAL;
  }

  if ((savedata.sort < SORT_START) || (savedata.sort > SORT_END)) {
    savedata.sort = SORT_DEFAULT;
  }

  if ((savedata.filter < FILTER_START) || (savedata.filter > FILTER_END)) {
    savedata.filter = FILTER_ALL;
  }

  if ((savedata.beep < BEEP_START) || (savedata.beep > BEEP_END)) {
    savedata.beep = BEEP_ON;
  }
  if ((savedata.multidisc < MULTIDISC_START) || (savedata.multidisc > MULTIDISC_END)) {
    savedata.multidisc = MULTIDISC_SHOW;
  }

  if ((savedata.custom_theme < THEME_START) || (savedata.custom_theme > THEME_END)) {
    savedata.custom_theme_num = (CFG_CUSTOM_THEME_NUM) THEME_OFF;
  }

  if ((savedata.custom_theme_num < THEME_NUM_START) || (savedata.custom_theme_num > THEME_NUM_END)) {
    savedata.custom_theme_num = THEME_NUM_START;
  }

  if (savedata.custom_theme) {
    savedata.region = REGION_END + 1 + savedata.custom_theme_num;
  }
}

void settings_load(void) {
  // Try and load savefile
  crayon_savefile_load(&savefile_details);

  // No savefile yet
  if (savefile_details.valid_memcards && savefile_details.savefile_port == -1 && savefile_details.savefile_slot == -1) {
    settings_create();
  }
}

/* Beeps while saving if enabled */
void settings_save(void) {
  maple_device_t *vmu = NULL;
  if ((savedata.beep == BEEP_ON) && (vmu = maple_enum_dev(savefile_details.savefile_port, savefile_details.savefile_slot))) {
    vmu_beep_raw(vmu, 0x000065f0); /* Turn on Beep */
  }
  if (savefile_details.valid_memcards) {
    crayon_savefile_save(&savefile_details);
    savefile_details.valid_saves = crayon_savefile_get_valid_saves(&savefile_details);
    if ((savedata.beep == BEEP_ON) && (vmu)) {
      vmu_beep_raw(vmu, 0x00000000); /* Turn off Beep */
    }
  }
}

openmenu_settings *settings_get(void) {
  return &savedata;
}
