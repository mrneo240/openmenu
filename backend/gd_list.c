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

#include "db_item.h"
#include "db_list.h"
#include "gd_item.h"
#include "gd_list.h"
#include "ini.h"

#ifdef _arch_dreamcast
#define PATH_PREFIX DISC_PREFIX
#include "../gdrom/gdrom_fs.h"
#else
#define PATH_PREFIX ""
#define FD_TYPE FILE *
#endif

/* Base internal original */
static int num_items_BASE = -1;
static gd_item *gd_slots_BASE = NULL;

/* Current client facing pointer copy, may be sorted/filtered */
static int num_items_temp = -1;
static gd_item **list_temp = NULL;

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
    num_items_BASE = atoi(value);
    num_items_temp = num_items_BASE - 1;
    gd_slots_BASE = malloc(num_items_BASE * sizeof(struct gd_item));
    list_temp = malloc(num_items_BASE * sizeof(struct gd_item *));

    memset(gd_slots_BASE, '\0', num_items_BASE * sizeof(struct gd_item));
    memset(list_temp, '\0', num_items_BASE * sizeof(struct gd_item *));
  } else {
    /* Parsing games */
    char slot_string[4] = {0, 0, 0, 0};
    uintptr_t seperator = (uintptr_t)strchr(name, '.');
    if (seperator) {
      size_t temp_len = (size_t)(seperator - (uintptr_t)name);
      memcpy(slot_string, name, temp_len);
      int slot = atoi(slot_string);

      gd_item *item = &gd_slots_BASE[slot - 1];
      if (!item->slot_num) {
        item->slot_num = slot;
      }

      const char *plain_name = name + temp_len + 1;

      //printf("[%s] %s: %s\n", section, plain_name, value);

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
  //fflush(stdout);
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
    //printf("slot %d\n", i);
    const gd_item *item = list[i];
#define CFG(s, n, default) printf("%s = %s\n", #n, item->n);
#include "gd_item.def"
    printf("\n");
  }
  printf("\n");
}

static void list_temp_reset(void) {
  for (int i = 0; i < num_items_BASE - 1; i++) {
    list_temp[i] = &gd_slots_BASE[i + 1];
  }
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
  num_items_temp = num_items_BASE - 1;

  /* Sort according to name alphabetically */
  qsort(list_temp, num_items_temp, sizeof(gd_item *), struct_cmp_by_name);
  return (const gd_item **)list_temp;
}

const gd_item **list_get_sort_date(void) {
  list_temp_reset();
  num_items_temp = num_items_BASE - 1;

  /* Sort according to name alphabetically */
  qsort(list_temp, num_items_temp, sizeof(gd_item *), struct_cmp_by_date);
  return (const gd_item **)list_temp;
}

const gd_item **list_get_sort_product(void) {
  list_temp_reset();
  num_items_temp = num_items_BASE - 1;

  /* Sort according to name alphabetically */
  qsort(list_temp, num_items_temp, sizeof(gd_item *), struct_cmp_by_product);
  return (const gd_item **)list_temp;
}

const gd_item **list_get_sort_default(void) {
  list_temp_reset();
  num_items_temp = num_items_BASE - 1;

  return (const gd_item **)list_temp;
}

const struct gd_item **list_get(void) {
  return (const gd_item **)list_temp;
}

const struct gd_item **list_get_genre(int genre) {
#if !defined(STANDALONE_BINARY)
  FLAGS_GENRE matching_genre = (1 << genre);
  int idx = 0;
  for (int i = 0; i < num_items_BASE - 1; i++) {
    gd_item *temp_item = &gd_slots_BASE[i + 1];
    db_item *temp_meta;
    if (!db_get_meta(temp_item->product, &temp_meta)) {
      if (temp_meta->genre & matching_genre) {
        list_temp[idx++] = temp_item;
      }
    }
  }
  num_items_temp = idx;
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

int list_length(void) {
  return num_items_temp;
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
  char *ini_buffer = malloc(ini_size);
  fread(ini_buffer, ini_size, 1, ini);
  fclose(ini);

  if (ini_parse_string(ini_buffer, read_openmenu_ini, NULL) < 0) {
    printf("INI:Error Parsing %s!\n", filename);
    fflush(stdout);
    /*exit or something */
    return -1;
  }
  free(ini_buffer);

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