/*
 * File: theme_manager.c
 * Project: ui
 * File Created: Tuesday, 27th July 2021 12:09:21 pm
 * Author: Hayden Kowalchuk
 * -----
 * Copyright (c) 2021 Hayden Kowalchuk, Hayden Kowalchuk
 * License: BSD 3-clause "New" or "Revised" License, http://www.opensource.org/licenses/BSD-3-Clause
 */

#include "theme_manager.h"

#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include "../external/ini.h"
#include "../gdrom/gdrom_fs.h"
#include "draw_prototypes.h"

/* Missing on sh-elf-gcc 9.1 ? */
char *strdup(const char *s);
char *strsep(char **stringp, const char *delim);

static theme_region region_themes[] = {
    {.bg_left = "THEME/NTSC_U/BG_U_L.PVR",
     .bg_right = "THEME/NTSC_U/BG_U_R.PVR",
     .colors = {.icon_color = COLOR_WHITE,
                .text_color = COLOR_WHITE,
                .highlight_color = COLOR_ORANGE_U,
                .menu_text_color = COLOR_WHITE,
                .menu_highlight_color = COLOR_ORANGE_U,
                .menu_bkg_color = COLOR_BLACK,
                .menu_bkg_border_color = COLOR_WHITE}},
    {.bg_left = "THEME/NTSC_J/BG_J_L.PVR",
     .bg_right = "THEME/NTSC_J/BG_J_R.PVR",
     .colors = {.icon_color = COLOR_BLACK,
                .text_color = COLOR_BLACK,
                .highlight_color = COLOR_ORANGE_J,
                .menu_text_color = COLOR_BLACK,
                .menu_highlight_color = COLOR_ORANGE_J,
                .menu_bkg_color = COLOR_WHITE,
                .menu_bkg_border_color = COLOR_BLACK}},
    {.bg_left = "THEME/PAL/BG_E_L.PVR",
     .bg_right = "THEME/PAL/BG_E_R.PVR",
     .colors = {.icon_color = COLOR_BLACK,
                .text_color = COLOR_BLACK,
                .highlight_color = COLOR_BLUE,
                .menu_text_color = COLOR_BLACK,
                .menu_highlight_color = COLOR_BLUE,
                .menu_bkg_color = COLOR_WHITE,
                .menu_bkg_border_color = COLOR_BLACK}},
};

static theme_custom custom_themes[10];
static int num_custom_themes = 0;

static void select_art_by_aspect(CFG_ASPECT aspect) {
  if (aspect == ASPECT_NORMAL) {
    region_themes[REGION_NTSC_U].bg_left = "THEME/NTSC_U/BG_U_L.PVR";
    region_themes[REGION_NTSC_U].bg_right = "THEME/NTSC_U/BG_U_R.PVR";

    region_themes[REGION_NTSC_J].bg_left = "THEME/NTSC_J/BG_J_L.PVR";
    region_themes[REGION_NTSC_J].bg_right = "THEME/NTSC_J/BG_J_R.PVR";

    region_themes[REGION_PAL].bg_left = "THEME/PAL/BG_E_L.PVR";
    region_themes[REGION_PAL].bg_right = "THEME/PAL/BG_E_R.PVR";
  } else {
    region_themes[REGION_NTSC_U].bg_left = "THEME/NTSC_U/BG_U_L.PVR";
    region_themes[REGION_NTSC_U].bg_right = "THEME/NTSC_U/BG_U_R.PVR";

    region_themes[REGION_NTSC_J].bg_left = "THEME/NTSC_J/BG_J_L_WIDE.PVR";
    region_themes[REGION_NTSC_J].bg_right = "THEME/NTSC_J/BG_J_R_WIDE.PVR";

    region_themes[REGION_PAL].bg_left = "THEME/PAL/BG_E_L_WIDE.PVR";
    region_themes[REGION_PAL].bg_right = "THEME/PAL/BG_E_R_WIDE.PVR";
  }
}

static inline long int filelength(FD_TYPE f) {
  long int end;
  fseek(f, 0, SEEK_END);
  end = ftell(f);
  fseek(f, 0, SEEK_SET);

  return end;
}

static uint32_t str2argb(const char *str) {
  char *token, *temp, *tofree, *endptr;
  int rgb[3] = {0, 0, 0};
  int *col = &rgb[0];

  tofree = temp = strdup(str);
  while ((token = strsep(&temp, ","))) *col++ = (int)strtol(token, &endptr, 0);
  free(tofree);
  return PVR_PACK_ARGB(0xFF, rgb[0], rgb[1], rgb[2]);
}

static int read_theme_ini(void *user, const char *section, const char *name, const char *value) {
  /* unused */
  (void)user;
  if (strcmp(section, "THEME") == 0) {
    theme_custom *new_theme = (theme_custom *)user;
    theme_color *new_color = &new_theme->colors;
    if (strcasecmp(name, "NAME") == 0) {
      strncpy(new_theme->name, value, sizeof(new_theme->name) - 1);
    } else if (strcasecmp(name, "ICON_COLOR") == 0) {
      new_color->icon_color = str2argb(value);
    } else if (strcasecmp(name, "TEXT_COLOR") == 0) {
      new_color->text_color = str2argb(value);
    } else if (strcasecmp(name, "HIGHLIGHT_COLOR") == 0) {
      new_color->highlight_color = str2argb(value);
    } else if (strcasecmp(name, "MENU_TEXT_COLOR") == 0) {
      new_color->menu_text_color = str2argb(value);
    } else if (strcasecmp(name, "MENU_HIGHLIGHT_COLOR") == 0) {
      new_color->menu_highlight_color = str2argb(value);
    } else if (strcasecmp(name, "MENU_BKG_COLOR") == 0) {
      new_color->menu_bkg_color = str2argb(value);
    } else if (strcasecmp(name, "MENU_BKG_BORDER_COLOR") == 0) {
      new_color->menu_bkg_border_color = str2argb(value);
    } else if (strcasecmp(name, "STYLE") == 0) {
      new_theme->style = atol(value);
    } else {
      printf("Unknown theme value: %s\n", name);
    }
  } else {
    /* error */
    printf("INI:Error unknown [%s] %s: %s\n", section, name, value);
  }
  return 1;
}

static int theme_read(const char *filename, theme_custom *theme) {
  FD_TYPE ini = fopen(filename, "rb");
  if (!FD_IS_OK(ini)) {
    printf("INI:Error opening %s!\n", filename);
    fflush(stdout);
    /*exit or something */
    return -1;
  }

  size_t ini_size = filelength(ini);
  char *ini_buffer = malloc(ini_size);
  fread(ini_buffer, ini_size, 1, ini);
  fclose(ini);

  if (ini_parse_string(ini_buffer, read_theme_ini, (void *)theme) < 0) {
    printf("INI:Error Parsing %s!\n", filename);
    fflush(stdout);
    /*exit or something */
    return -1;
  }
  free(ini_buffer);

  return 0;
}

static void load_themes(char *basePath) {
  char path[128];
  DIRENT_TYPE dp;
  DIR_TYPE dir = opendir(basePath);

  if (!dir) {
    return;
  }

  while ((dp = readdir(dir)) != NULL) {
	  //printf("load_themes: %s\n", dp->d_name);
    if (strcmp(dp->d_name, ".") != 0 && strcmp(dp->d_name, "..") != 0) {
      if (strncasecmp(dp->d_name, "CUST_", 5) == 0 || strncasecmp(dp->d_name, "GDMENU_", 6) == 0) {
        int theme_num = dp->d_name[5] - '0';

        strcpy(path, basePath);
        strcat(path, "/");
        strcat(path, dp->d_name);
        strcat(path, "/");

        printf("theme #%d: %s @ %s\n", theme_num, dp->d_name, path);

        /* Add the theme */
        strcpy(custom_themes[num_custom_themes].bg_left, path+4);
        strcat(custom_themes[num_custom_themes].bg_left, "BG_L.PVR");
        strcpy(custom_themes[num_custom_themes].bg_right, path+4);
        strcat(custom_themes[num_custom_themes].bg_right, "BG_R.PVR");

        /* dummy colors */
        custom_themes[num_custom_themes].colors = (theme_color){.text_color = COLOR_WHITE,
                                                                .highlight_color = COLOR_ORANGE_U,
                                                                .menu_text_color = COLOR_WHITE,
                                                                .menu_bkg_color = COLOR_BLACK,
                                                                .menu_bkg_border_color = COLOR_WHITE};

        custom_themes[num_custom_themes].style = 0;
        /* dummy name */
        sprintf(custom_themes[num_custom_themes].name, "CUSTOM #%d", theme_num);

        /* load INI if available, for name & colors */
        strcat(path, "THEME.INI");
        theme_read(path, &custom_themes[num_custom_themes]);

        num_custom_themes++;
      }
    }
  }

  closedir(dir);
}

int theme_manager_load(void) {
  /* Original themes are statically loaded */

  /* Load custom themes if they exist */
  load_themes("/cd/THEME");
  return 0;
}

theme_region *theme_get_default(CFG_ASPECT aspect, int *num_themes) {
  select_art_by_aspect(aspect);
  *num_themes = 3;
  return region_themes;
}

theme_custom *theme_get_custom(int *num_themes) {
  *num_themes = num_custom_themes;
  return custom_themes;
}
