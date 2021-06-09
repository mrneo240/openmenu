/*
 * File: serial_sanitize.c
 * Project: texture
 * File Created: Tuesday, 8th June 2021 10:24:28 pm
 * Author: Hayden Kowalchuk
 * -----
 * Copyright (c) 2021 Hayden Kowalchuk, Hayden Kowalchuk
 * License: BSD 3-clause "New" or "Revised" License, http://www.opensource.org/licenses/BSD-3-Clause
 */

#include "../uthash.h"

// Disc serial will be the filename, e.g. T8119N.PVR
/* Name, IP Serial, Disc Serial */
// F355 Challenge: Passione Rossa, MK-0100, T-8119N

#define REMAP_ADD(ip, disc)              \
  (serial_remap) {                       \
    .disc_serial = disc, .ip_serial = ip \
  }

typedef struct serial_remap {
  const char* ip_serial;
  const char* disc_serial;
  UT_hash_handle hh; /* makes this structure hashable */
} serial_remap;

static serial_remap serial_remap_members[] = {
    REMAP_ADD("MK0100", "T8119N"),
};

static const int serials_added = sizeof(serial_remap_members) / sizeof(serial_remap);

static serial_remap* serial_remap_list = NULL;

const char* serial_santize(const char* id) {
  const serial_remap* item;
  const char* ret = id;

  HASH_FIND_STR(serial_remap_list, id, item);

  if (item) {
    ret = item->disc_serial;
  }
  return ret;
}

int serial_sanitizer_init(void) {
  for (int i = 0; i < serials_added; i++) {
    HASH_ADD_STR(serial_remap_list, ip_serial, &serial_remap_members[i]);
  }

  return 0;
}