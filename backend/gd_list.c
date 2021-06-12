/*
 * File: example.c
 * Project: ini_parse
 * File Created: Wednesday, 19th May 2021 2:50:50 pm
 * Author: Hayden Kowalchuk
 * -----
 * Copyright (c) 2021 Hayden Kowalchuk, Hayden Kowalchuk
 * License: BSD 3-clause "New" or "Revised" License, http://www.opensource.org/licenses/BSD-3-Clause
 */
/*

gcc example.c ini.c -Os -s  -Xlinker -Map=output.map -ffunction-sections -fdata-sections -Xlinker --gc-sections  -o example

*/

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#define stricmp strcasecmp

#include "gd_item.h"
#include "ini.h"

/* Used to read from GDROM instead of cdrom */
#define GDROM_FS (1)
#include "../gdrom/gdrom_fs.h"

#ifdef _arch_dreamcast
#define PATH_PREFIX DISC_PREFIX
#else
#define PATH_PREFIX ""
#endif

static int num_items = -1;
static gd_item* gd_slots_BASE = NULL;

static gd_item** list_temp = NULL;

static inline long int filelength(FD_TYPE f) {
  long int end;
  fseek(f, 0, SEEK_END);
  end = ftell(f);
  fseek(f, 0, SEEK_SET);

  return end;
}

static int read_openmenu_ini(void* user, const char* section, const char* name, const char* value) {
  if ((strcmp(section, "OPENMENU") == 0) && (strcmp(name, "num_items") == 0)) {
    num_items = atoi(value);
    gd_slots_BASE = malloc(num_items * sizeof(struct gd_item));
    list_temp = malloc(num_items * sizeof(struct gd_item*));

    memset(gd_slots_BASE, '\0', num_items * sizeof(struct gd_item));
    memset(list_temp, '\0', num_items * sizeof(struct gd_item*));
  } else {
    /* Parsing games */
    char slot_string[4] = {0, 0, 0, 0};
    uintptr_t seperator = (uintptr_t)strchr(name, '.');
    if (seperator) {
      size_t temp_len = (size_t)(seperator - (uintptr_t)name);
      memcpy(slot_string, name, temp_len);
      int slot = atoi(slot_string);

      gd_item* item = &gd_slots_BASE[slot - 1];
      if (!item->slot_num) {
        item->slot_num = slot;
      }

      const char* plain_name = name + temp_len + 1;

      //printf("[%s] %s: %s\n", section, plain_name, value);

      if (0)
        ;
#define CFG(s, n, default) else if (stricmp(section, #s) == 0 && \
                                    stricmp(plain_name, #n) == 0) strcpy(item->n, value);
#include "gd_item.def"
    } else {
      /* error */
      printf("UNKNOWN! [%s] %s: %s\n", section, name, value);
    }
  }
  //fflush(stdout);
  return 1;
}

void list_print_slots(void) {
  for (int i = 0; i < num_items; i++) {
    printf("slot %d\n", i);
    gd_item* item = &gd_slots_BASE[i];
#define CFG(s, n, default) printf("%s = %s\n", #n, item->n);
#include "gd_item.def"
    printf("\n");
  }
}

void list_print_temp(void) {
  for (int i = 0; i < num_items - 1; i++) {
    printf("slot %d\n", i);
    gd_item* item = list_temp[i];
#define CFG(s, n, default) printf("%s = %s\n", #n, item->n);
#include "gd_item.def"
    printf("\n");
  }
}

void list_print(const gd_item** list) {
  for (int i = 0; i < num_items - 1; i++) {
    //printf("slot %d\n", i);
    const gd_item* item = list[i];
#define CFG(s, n, default) printf("%s = %s\n", #n, item->n);
#include "gd_item.def"
    printf("\n");
  }
  printf("\n");
}

static void list_temp_reset(void) {
  for (int i = 0; i < num_items - 1; i++) {
    list_temp[i] = &gd_slots_BASE[i + 1];
  }
}

static int struct_cmp_by_name(const void* a, const void* b) {
  const gd_item* ia = *(const gd_item**)a;
  const gd_item* ib = *(const gd_item**)b;
  return strcmp(ia->name, ib->name);
}

static int struct_cmp_by_date(const void* a, const void* b) {
  const gd_item* ia = *(const gd_item**)a;
  const gd_item* ib = *(const gd_item**)b;
  return strcmp(ia->date, ib->date);
}

static int struct_cmp_by_product(const void* a, const void* b) {
  const gd_item* ia = *(const gd_item**)a;
  const gd_item* ib = *(const gd_item**)b;
  return strcmp(ia->product, ib->product);
}

const gd_item** list_get_sort_name(void) {
  list_temp_reset();

  /* Sort according to name alphabetically */
  qsort(list_temp, num_items - 1, sizeof(gd_item*), struct_cmp_by_name);
  return (const gd_item**)list_temp;
}

const gd_item** list_get_sort_date(void) {
  list_temp_reset();

  /* Sort according to name alphabetically */
  qsort(list_temp, num_items - 1, sizeof(gd_item*), struct_cmp_by_date);
  return (const gd_item**)list_temp;
}

const gd_item** list_get_sort_product(void) {
  list_temp_reset();

  /* Sort according to name alphabetically */
  qsort(list_temp, num_items - 1, sizeof(gd_item*), struct_cmp_by_product);
  return (const gd_item**)list_temp;
}

const gd_item** list_get_sort_default(void) {
  list_temp_reset();

  return (const gd_item**)list_temp;
}

const struct gd_item** list_get(void) {
  return (const gd_item**)list_temp;
}

int list_length(void) {
  return num_items - 1;
}

int list_read(void) {
  /* Always LD/cdrom */
  FD_TYPE ini = fopen(PATH_PREFIX "OPENMENU.INI", "r");
  if (!ini) {
    printf("Error opening!!\n");
    fflush(stdout);
    /*exit or something */
    return -1;
  }
  size_t ini_size = filelength(ini);
  char* ini_buffer = malloc(ini_size);
  fread(ini_buffer, ini_size, 1, ini);
  fclose(ini);

  if (ini_parse_string(ini_buffer, read_openmenu_ini, NULL) < 0) {
    printf("Error Parsing!!\n");
    fflush(stdout);
    /*exit or something */
    return -1;
  }
  printf("Parsed list successfully (%d items)!\n", num_items);
  list_temp_reset();
  fflush(stdout);

  return 0;
}

void list_destroy(void) {
  num_items = -1;
  free(gd_slots_BASE);
  free(list_temp);
  gd_slots_BASE = NULL;
  list_temp = NULL;
}

const gd_item* list_item_get(unsigned int idx) {
  if (idx < num_items - 1)
    return (const gd_item*)list_temp[idx];

  return NULL;
}