/*
 * File: example.c
 * Project: ini_parse
 * File Created: Wednesday, 19th May 2021 2:50:50 pm
 * Author: Hayden Kowalchuk
 * -----
 * Copyright (c) 2021 Hayden Kowalchuk, Hayden Kowalchuk
 * License: BSD 3-clause "New" or "Revised" License, http://www.opensource.org/licenses/BSD-3-Clause
 */

#ifdef COSMO
#include "../tools/cosmo/cosmopolitan.h"
#else
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#endif

#include "../external/ini.h"
#include "db_item.h"
#include "db_list.h"
#include "gd_item.h"
#include "gd_list.h"

#ifdef _arch_dreamcast
#include "../gdrom/gdrom_fs.h"
#define PATH_PREFIX DISC_PREFIX
#else
#define PATH_PREFIX ""
typedef FILE *FD_TYPE;
#endif

#ifndef STANDALONE_BINARY
#include "../ui/global_settings.h"
#endif

/* Base internal original */
static int num_items_BASE = -1;
static int num_items_read = 0;
static gd_item *gd_slots_BASE = NULL;

/* Current client facing pointer copy, may be sorted/filtered */
static int num_items_temp = -1;
static gd_item **list_temp = NULL;

/* Temporary list for holding all multidisc games in a set */
#define MULTIDISC_MAX_GAMES_PER_SET (4)
static int num_items_multidisc = -1;
static gd_item *list_multidisc[MULTIDISC_MAX_GAMES_PER_SET] = {NULL};

static inline long int filelength(FD_TYPE f) {
  long int end;
  fseek(f, 0, SEEK_END);
  end = ftell(f);
  fseek(f, 0, SEEK_SET);

  return end;
}

static int read_openmenu_ini(void *user, const char *section, const char *name, const char *value) {
  /* unused */
  (void)user;

  if ((strcmp(section, "OPENMENU") == 0) && (strcmp(name, "num_items") == 0)) {
    num_items_BASE = atoi(value) /* It can occur that GDMenuCardManager under reports by 1 */;
    num_items_temp = num_items_BASE - 1;
    gd_slots_BASE = malloc((num_items_BASE + 1) * sizeof(struct gd_item));
    list_temp = malloc((num_items_BASE + 1) * sizeof(struct gd_item *));

    memset(gd_slots_BASE, '\0', (num_items_BASE + 1) * sizeof(struct gd_item));
    memset(list_temp, '\0', (num_items_BASE + 1) * sizeof(struct gd_item *));
    memset(list_multidisc, '\0', MULTIDISC_MAX_GAMES_PER_SET * sizeof(struct gd_item *));
  } else {
    /* Parsing games */
    char slot_string[4] = {0, 0, 0, 0};
    uintptr_t seperator = (uintptr_t)strchr(name, '.');
    if (seperator) {
      size_t temp_len = (size_t)(seperator - (uintptr_t)name);
      memcpy(slot_string, name, temp_len);
      int slot = atoi(slot_string);
      num_items_read = slot;

      gd_item *item = &gd_slots_BASE[slot - 1];
      if (!item->slot_num) {
        item->slot_num = slot;
      }

      const char *plain_name = name + temp_len + 1;

      // printf("[%s] %s: %s\n", section, plain_name, value);

      if (0)
        ;
#define CFG(s, n, default) else if (strcasecmp(section, #s) == 0 && \
                                    strcasecmp(plain_name, #n) == 0) strcpy(item->n, value);
#include "gd_item.def"

    } else {
      /* error */
      printf("INI:Error unknown [%s] %s: %s\n", section, name, value);
    }
  }
  return 1;
}

void list_print_slots(void) {
  for (int i = 0; i < num_items_BASE; i++) {
    printf("slot %d\n", i);
    gd_item *item = &gd_slots_BASE[i];
#define CFG(s, n, default) printf("%s = %s\n", #n, item->n);
#include "gd_item.def"
    printf("\n");
  }
}

void list_print_temp(void) {
  for (int i = 0; i < num_items_temp; i++) {
    printf("slot %d\n", i);
    gd_item *item = list_temp[i];
#define CFG(s, n, default) printf("%s = %s\n", #n, item->n);
#include "gd_item.def"
    printf("\n");
  }
}

/* Might need a rework */
void list_print(const gd_item **list) {
  for (int i = 0; i < num_items_temp; i++) {
    // printf("slot %d\n", i);
    const gd_item *item = list[i];
#define CFG(s, n, default) printf("%s = %s\n", #n, item->n);
#include "gd_item.def"
    printf("\n");
  }
  printf("\n");
}

static void list_temp_reset(void) {
  int base_idx, temp_idx = 0;

#ifdef _arch_dreamcast
  int hide_multidisc = settings_get()->multidisc;
#else
  int hide_multidisc = 0;
#endif

  /* Skip openMenu itself */
  for (base_idx = 1; base_idx < num_items_BASE; base_idx++) {
    int disc_num = gd_slots_BASE[base_idx].disc[0] - '0';
    int disc_set = gd_slots_BASE[base_idx].disc[2] - '0';
    if (hide_multidisc && disc_num > 1 && disc_set > 1)
      continue;

    list_temp[temp_idx++] = &gd_slots_BASE[base_idx];
  }
  num_items_temp = temp_idx;
}

static int struct_cmp_by_name(const void *a, const void *b) {
  const gd_item *ia = *(const gd_item **)a;
  const gd_item *ib = *(const gd_item **)b;
  return strcmp(ia->name, ib->name);
}

static int struct_cmp_by_date(const void *a, const void *b) {
  const gd_item *ia = *(const gd_item **)a;
  const gd_item *ib = *(const gd_item **)b;
  return strcmp(ia->date, ib->date);
}

static int struct_cmp_by_product(const void *a, const void *b) {
  const gd_item *ia = *(const gd_item **)a;
  const gd_item *ib = *(const gd_item **)b;
  return strcmp(ia->product, ib->product);
}

const gd_item **list_get_sort_name(void) {
  list_temp_reset();

  /* Sort according to name alphabetically */
  qsort(list_temp, num_items_temp, sizeof(gd_item *), struct_cmp_by_name);
  return (const gd_item **)list_temp;
}

const gd_item **list_get_sort_date(void) {
  list_temp_reset();

  /* Sort according to name alphabetically */
  qsort(list_temp, num_items_temp, sizeof(gd_item *), struct_cmp_by_date);
  return (const gd_item **)list_temp;
}

const gd_item **list_get_sort_product(void) {
  list_temp_reset();

  /* Sort according to name alphabetically */
  qsort(list_temp, num_items_temp, sizeof(gd_item *), struct_cmp_by_product);
  return (const gd_item **)list_temp;
}

const gd_item **list_get_sort_default(void) {
  list_temp_reset();

  return (const gd_item **)list_temp;
}

const struct gd_item **list_get(void) {
  return (const gd_item **)list_temp;
}

const struct gd_item **list_get_multidisc(void) {
  return (const gd_item **)list_multidisc;
}

const struct gd_item **list_get_genre(int genre) {
#if !defined(STANDALONE_BINARY)
  FLAGS_GENRE matching_genre = (1 << genre);

  int base_idx, temp_idx = 0;

  int hide_multidisc = settings_get()->multidisc;

  /* Skip openMenu itself */
  for (base_idx = 1; base_idx < num_items_BASE; base_idx++) {
    int disc_num = gd_slots_BASE[base_idx].disc[0] - '0';
    int disc_set = gd_slots_BASE[base_idx].disc[2] - '0';
    if (hide_multidisc && disc_num > 1 && disc_set > 1)
      continue;

    gd_item *temp_item = &gd_slots_BASE[base_idx];
    db_item *temp_meta;
    if (!db_get_meta(temp_item->product, &temp_meta)) {
      if (temp_meta->genre & matching_genre) {
        list_temp[temp_idx++] = temp_item;
      }
    }
  }

  num_items_temp = temp_idx;
  return (const gd_item **)list_temp;
#endif
}

const struct gd_item **list_get_genre_sort(int genre, int sort) {
  list_get_genre(genre);

  switch (sort) {
    case 1:
      qsort(list_temp, num_items_temp, sizeof(gd_item *), struct_cmp_by_name);
      break;
    case 2:
      qsort(list_temp, num_items_temp, sizeof(gd_item *), struct_cmp_by_date);
      break;
    case 3:
      qsort(list_temp, num_items_temp, sizeof(gd_item *), struct_cmp_by_product);
      break;

    default:
    case 0:
      /* @Note: no sort, strange codeflow */
      break;
  }

  return (const gd_item **)list_temp;
}

void list_set_multidisc(const char *product_id) {
  int base_idx, temp_idx = 0;

  /* Skip openMenu itself */
  for (base_idx = 1; base_idx < num_items_BASE; base_idx++) {
    if (strcmp(gd_slots_BASE[base_idx].product, product_id))
      continue;

    list_multidisc[temp_idx++] = &gd_slots_BASE[base_idx];
  }
  num_items_multidisc = temp_idx;
}

int list_length(void) {
  return num_items_temp;
}

int list_multidisc_length(void) {
  return num_items_multidisc;
}

static void fix_sega_serials(void) {
  /* fixing Sega serial issues... */

  /* Skip openMenu itself */
  for (int base_idx = 1; base_idx < num_items_BASE; base_idx++) {
    gd_item *item = &gd_slots_BASE[base_idx];

    /* Fix Alone in the Dark (PAL) overlapping Alone in the Dark (USA) */
    if (!strcmp(item->product, "T15117N") && !strcmp(item->date, "20010423")) {
      strcpy(item->product, "T15112D05");
    }
    /* Fix Crazy Taxi (PAL) overlapping Crazy Taxi (USA) */
    if (!strcmp(item->product, "MK51035") && !strcmp(item->date, "20000120")) {
      strcpy(item->product, "MK5103550");
    }
    /* Fix Disney's Donald Duck: Goin' Quackers (USA) overlapping Disney's Donald Duck: Quack Attack (PAL) */
    if (!strcmp(item->product, "T17714D50") && !strcmp(item->date, "20001116")) {
      strcpy(item->product, "T17719N");
    }
    /* Fix Floigan Bros (PAL) overlapping Floigan Bros (USA) */
    if (!strcmp(item->product, "MK51114") && !strcmp(item->date, "20010920")) {
      strcpy(item->product, "MK5111450");
    }
    /* Fix Legacy of Kain: Soul Reaver (PAL) overlapping Legacy of Kain: Soul Reaver (USA) */
    if (!strcmp(item->product, "T36802N") && !strcmp(item->date, "19991220")) {
      strcpy(item->product, "T36803D05");
    }
    /* Fix NBA2K2 (PAL) overlapping NBA2K2 (USA) */
    if (!strcmp(item->product, "MK51178") && !strcmp(item->date, "20011129")) {
      strcpy(item->product, "MK5117850");
    }
    /* Fix NBA Showtime (PAL) overlapping 4 Wheel Thunder (PAL) */
    if (!strcmp(item->product, "T9706D50") && !strcmp(item->date, "19991201")) {
      strcpy(item->product, "T9705D50");
    }
    /* Fix Nightmare Creatures II (USA) overlapping Dancing Blade 2 (JAP) */
    if (!strcmp(item->product, "T9504M") && !strcmp(item->date, "20000407")) {
      strcpy(item->product, "T9504N");
    }
    /* Fix Plasma Sword (PAL) overlapping Street Fighter Alpha 3 (PAL) */
    if (!strcmp(item->product, "T7005D") && !strcmp(item->date, "20000711")) {
      strcpy(item->product, "T7003D");
    }
    /* Fix Skies of Arcadia (PAL) overlapping Skies of Arcadia (USA) */
    if (!strcmp(item->product, "MK51052") && !strcmp(item->date, "20010306")) {
      strcpy(item->product, "MK5105250");
    }
    /* Fix Spider-Man (PAL) overlapping Spider-Man (USA) */
    if (!strcmp(item->product, "T13008N") && !strcmp(item->date, "20010402")) {
      strcpy(item->product, "T13011D50");
    }
    /* Fix TNN Motorsports (USA) overlapping Metal Slug 6 (AW) */
    if (!strcmp(item->product, "T0000M") && !strcmp(item->date, "19990813")) {
      strcpy(item->product, "T13701N");
    }

    /* Fix Maximum Speed (AW) overlapping Dolphin Blue (AW) */
    if (!strcmp(item->product, "T0006M") && !strcmp(item->date, "20030609")) {
      strcpy(item->product, "T0010M");
    }

    /* Fix Fist of North Star (AW) overlapping Rumble Fish (AW) */
    if (!strcmp(item->product, "T0009M") && strstr(item->name, "orth")) {
      strcpy(item->product, "T0026M");
    }
  }
}

int list_read(const char *filename) {
  /* Always LD/cdrom */
  FD_TYPE ini = fopen(filename, "rb");
  if (!ini) {
    printf("INI:Error opening %s!\n", filename);
    fflush(stdout);
    /*exit or something */
    return -1;
  }

  printf("INI:Open %s\n", filename);

  size_t ini_size = filelength(ini);
  char *ini_buffer = malloc(ini_size + 2) /* adjust for adding newline at end always */;
  fread(ini_buffer, ini_size, 1, ini);
  fclose(ini);
  /* Add newline */
  ini_buffer[ini_size + 0] = '\n';
  ini_buffer[ini_size + 1] = '\0';

  if (ini_parse_string(ini_buffer, read_openmenu_ini, NULL) < 0) {
    printf("INI:Error Parsing %s!\n", filename);
    fflush(stdout);
    /*exit or something */
    return -1;
  }
  free(ini_buffer);

  printf("Info: Loaded %d items from %d\n", num_items_read, num_items_BASE);
  /* Trim list if over reported */
  if (num_items_read != num_items_BASE) {
    num_items_BASE = num_items_read;
    num_items_temp = num_items_read - 1;
  }

  fix_sega_serials();

  printf("INI:Parse success (%d items)!\n", num_items_BASE);
  list_temp_reset();
  fflush(stdout);

  return 0;
}

int list_read_default(void) {
  return list_read(PATH_PREFIX "OPENMENU.INI");
}

void list_destroy(void) {
  num_items_BASE = -1;
  num_items_temp = -1;
  free(gd_slots_BASE);
  free(list_temp);
  gd_slots_BASE = NULL;
  list_temp = NULL;
}

const gd_item *list_item_get(int idx) {
  if ((idx >= 0) && (idx < num_items_temp))
    return (const gd_item *)list_temp[idx];

  return NULL;
}