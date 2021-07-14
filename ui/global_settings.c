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
#include "openmenu_lcd.h"
#include "openmenu_pal.h"
#include "openmenu_vmu.h"

static crayon_savefile_details_t savefile_details;
static openmenu_settings savedata;

static void settings_defaults(void) {
  savedata.ui = UI_LINE_DESC;
  savedata.region = REGION_NTSC_U;
  savedata.aspect = ASPECT_NORMAL;
  savedata.sort = SORT_DEFAULT;
  savedata.filter = FILTER_ALL;
}

static void settings_create(void) {
  //If we don't already have a savefile, choose a VMU
  if (savefile_details.valid_memcards) {
    for (int iter = 0; iter <= 3; iter++) {
      for (int jiter = 1; jiter <= 2; jiter++) {
        if (crayon_savefile_get_vmu_bit(savefile_details.valid_memcards, iter, jiter)) {  //Use the left most VMU
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
                                        (uint8_t *)&savedata, sizeof(openmenu_settings), 1, 1,
                                        "openMenu Preferences\0", "openMenu Config\0", "openMenuPref\0", "OPENMENU.CFG\0");

  savefile_details.icon = openmenu_icon;
  savefile_details.icon_palette = (unsigned short *)openmenu_pal;

  crayon_vmu_display_icon(savefile_details.valid_vmu_screens, openmenu_lcd);

  //Find the first savefile (if it exists)
  for (int iter = 0; iter <= 3; iter++) {
    for (int jiter = 1; jiter <= 2; jiter++) {
      if (crayon_savefile_get_vmu_bit(savefile_details.valid_saves, iter, jiter)) {  //Use the left most VMU
        savefile_details.savefile_port = iter;
        savefile_details.savefile_slot = jiter;
        goto Exit_loop_1;
      }
    }
  }
Exit_loop_1:

  settings_load();
}

void settings_load(void) {
  //Try and load savefile
  crayon_savefile_load(&savefile_details);

  //No savefile yet
  if (savefile_details.valid_memcards && savefile_details.savefile_port == -1 && savefile_details.savefile_slot == -1) {
    settings_create();
  }
}

void settings_save(void) {
  if (savefile_details.valid_memcards) {
    crayon_savefile_save(&savefile_details);
    savefile_details.valid_saves = crayon_savefile_get_valid_saves(&savefile_details);
    maple_device_t *vmu;
    if ((vmu = maple_enum_dev(savefile_details.savefile_port, savefile_details.savefile_slot))) {
      vmu_beep_raw(vmu, 0x000065f0);
    }
  }
}

openmenu_settings *settings_get(void) {
  return &savedata;
}