/*
 * File: serial_sanitize.c
 * Project: texture
 * File Created: Tuesday, 8th June 2021 10:24:28 pm
 * Author: Hayden Kowalchuk
 * -----
 * Copyright (c) 2021 Hayden Kowalchuk, Hayden Kowalchuk
 * License: BSD 3-clause "New" or "Revised" License, http://www.opensource.org/licenses/BSD-3-Clause
 */

#include "../inc/uthash.h"

// Disc serial will be the filename, e.g. T8119N.PVR
/* Name, IP Serial, Disc Serial */
// F355 Challenge: Passione Rossa, MK-0100, T-8119N

enum REMAP_TYPE {
  REMAP_NONE = (0 << 0),  // 0
  REMAP_ART = (1 << 0),   // 1
  REMAP_META = (1 << 1),  // 2
};

#define REMAP_ADD_ART(ip, disc)                                    \
  (serial_remap) {                                                 \
    .ip_serial = ip, .art_serial = disc, .remap_choice = REMAP_ART \
  }

#define REMAP_ADD_META(ip, disc)                                     \
  (serial_remap) {                                                   \
    .ip_serial = ip, .meta_serial = disc, .remap_choice = REMAP_META \
  }

typedef struct serial_remap {
  const char* ip_serial;
  const char* art_serial;
  const char* meta_serial;
  UT_hash_handle hh; /* makes this structure hashable */
  const enum REMAP_TYPE remap_choice;
} serial_remap;

/* Generate from excel with:
find:     ^(.*)[\t](.*)[\n]
replace:  REMAP_ADD_META("$1", "$2"),\n
*/

static serial_remap serial_remap_members[] = {
    REMAP_ADD_ART("MK0100", "T8119N"),
    REMAP_ADD_META("T12501D61", "T12502N"),
    REMAP_ADD_META("T7011D50", "T40217N"),
    /* PAL Missing Meta */
    REMAP_ADD_META("T9701D61", "T9701N"),
    REMAP_ADD_META("T7020D50", "T1402N"),
    REMAP_ADD_META("MK5102250", "MK51012"),
    REMAP_ADD_META("T10001D50", "T10004N"),
    REMAP_ADD_META("MK5100453", "MK51004"),
    REMAP_ADD_META("MK5117850", "MK51178"),
    REMAP_ADD_META("T9713D61", "T9709N"),
    REMAP_ADD_META("T9705D50", "T9706N"),
    REMAP_ADD_META("T9703D50", "T9703N"),
    REMAP_ADD_META("T8102D05", "T8101N"),
    REMAP_ADD_META("MK5102589", "MK51025"),
    REMAP_ADD_META("T9502D50", "T9504N"),
    REMAP_ADD_META("MK5110250", "MK51102"),
    REMAP_ADD_META("T15103D61", "T15105N"),
    REMAP_ADD_META("MK5110050", "MK51100"),
    REMAP_ADD_META("MK5119350", "MK51193"),
    REMAP_ADD_META("T7003D50", "T1207N"),
    REMAP_ADD_META("T17710D50", "T17713N"),
    REMAP_ADD_META("T36801D64", "T1201N"),
    REMAP_ADD_META("T36812D61", "T1211N"),
    REMAP_ADD_META("T7022D50", "T1219N"),
    REMAP_ADD_META("T8106D05", "T31101N"),
    REMAP_ADD_META("MK5106150", "MK51061"),
    REMAP_ADD_META("T45006D50", "T17701N"),
    REMAP_ADD_META("17701D", "T17701D05"),
    REMAP_ADD_META("T17707D50", "T17704N"),
    REMAP_ADD_META("T8107D05", "T8109N"),
    REMAP_ADD_META("T9704D51", "T9704N"),
    REMAP_ADD_META("T9711D50", "T9717N"),
    REMAP_ADD_META("T7012D", "T40218N"),
    REMAP_ADD_META("MK5102150", "T40215N"),
    REMAP_ADD_META("T7004D", "T1205N"),
    REMAP_ADD_META("T7021D56", "T1220N"),
    REMAP_ADD_META("T36806D", "T1204N"),
    REMAP_ADD_META("T22901D05", "T22901N"),
    REMAP_ADD_META("MK5109250", "MK51092"),
    REMAP_ADD_META("T9709D61", "T9707N"),
    REMAP_ADD_META("MK5100605", "MK51006"),
    REMAP_ADD_META("MK5105350", "MK51053"),
    REMAP_ADD_META("MK5101950", "MK51019"),
    REMAP_ADD_META("T8104D05", "T8106N"),
    REMAP_ADD_META("MK5105950", "MK51059"),
    REMAP_ADD_META("T9505D76", "T9507N"),
    REMAP_ADD_META("T15109D91", "T15108N"),
    REMAP_ADD_META("MK5105250", "MK51052"),
    REMAP_ADD_META("T15104D59", "T15106N"),
    REMAP_ADD_META("T17722D50", "T40207N"),
    REMAP_ADD_META("T17726D50", "T40212N"),
    REMAP_ADD_META("MK5100053", "MK51000"),
    REMAP_ADD_META("MK5111750", "MK51117"),
    REMAP_ADD_META("MK5106050", "MK51060"),
    REMAP_ADD_META("T1401D50", "T1401N"),
    REMAP_ADD_META("T41401D61", "T41401N"),
    REMAP_ADD_META("T8105D50", "T8105N"),
    REMAP_ADD_META("T8112D05", "T8116N"),
    REMAP_ADD_META("MK5105150", "MK51051"),
    REMAP_ADD_META("T36816D05", "T1216N"),
    REMAP_ADD_META("T45004D50", "T41704N"),
    REMAP_ADD_META("T17702D50", "T17702N"),
    REMAP_ADD_META("T17713D50", "T17718N"),
    REMAP_ADD_META("T13011D05", "T13008N"),
    REMAP_ADD_META("T8117D59", "T8118N"),
    REMAP_ADD_META("T23001D", "T23001N"),
    REMAP_ADD_META("T13010D05", "T23003N"),
    REMAP_ADD_META("T17723D50", "T40209N"),
    REMAP_ADD_META("T7005D50", "T1203N"),
    REMAP_ADD_META("T7013D50", "T1213N"),
    REMAP_ADD_META("T7006D50", "T1210N"),
    REMAP_ADD_META("T17711D71", "T17708N"),
    REMAP_ADD_META("T40206D50", "T40206N"),
    REMAP_ADD_META("T17721D50", "T40216N"),
    REMAP_ADD_META("T17703D05", "T17703N"),
    REMAP_ADD_META("T36807D05", "T36805N"),
    REMAP_ADD_META("T36808D05", "T36808N"),
    REMAP_ADD_META("T7009D50", "T1208N"),
    REMAP_ADD_META("T8108D05", "T8108N"),
    REMAP_ADD_META("T9503D76", "T9512N"),
    REMAP_ADD_META("MK5100250", "MK51002"),
    REMAP_ADD_META("T36805D09", "T36807N"),
    REMAP_ADD_META("MK5101153", "MK51011"),
    REMAP_ADD_META("T40201D50", "T40202N"),
    REMAP_ADD_META("T17724D50", "T40211N"),
    REMAP_ADD_META("T45001D05", "T40401N"),
    REMAP_ADD_META("T45002D61", "T40402N"),
    REMAP_ADD_META("T36815D05", "T36812N"),
    REMAP_ADD_META("T36804D05", "T36806N"),
    REMAP_ADD_META("T13008D05", "T13006N"),
    REMAP_ADD_META("T40204D50", "T40205N"),
    REMAP_ADD_META("MK5102050", "MK57020"),
    REMAP_ADD_META("T8101D05", "T8102N"),
    REMAP_ADD_META("T40203D50", "T40204N"),
    REMAP_ADD_META("T15113D50", "T15125N"),
    REMAP_ADD_META("T36810D50", "T36810N"),
    REMAP_ADD_META("T8110D05", "T8110N"),
    REMAP_ADD_META("T13002D71", "T13002N"),
    REMAP_ADD_META("MK5109450", "T44301N"),
    REMAP_ADD_META("MK5100150", "MK51001"),
    REMAP_ADD_META("MK5102850", "MK51028"),
    REMAP_ADD_META("MK5105450", "MK51054"),
    REMAP_ADD_META("T15106D05", "T15113N"),
    REMAP_ADD_META("T36809D50", "T36804N"),
    REMAP_ADD_META("T40504D", "T8111N"),
    REMAP_ADD_META("T40601D79", "T40601N"),
    REMAP_ADD_META("T7016D50", "T22904N"),
    REMAP_ADD_META("T8103D50", "T8103N"),
    REMAP_ADD_META("T10003D50", "T10005N"),
};

static const int serials_added = sizeof(serial_remap_members) / sizeof(serial_remap);
static serial_remap* serial_remap_list = NULL;

const char* serial_santize_art(const char* id) {
  const serial_remap* item;
  const char* ret = id;

  HASH_FIND_STR(serial_remap_list, id, item);

  if (item && (item->remap_choice & REMAP_ART)) {
    ret = item->art_serial;
  }
  return ret;
}

const char* serial_santize_meta(const char* id) {
  const serial_remap* item;
  const char* ret = id;

  HASH_FIND_STR(serial_remap_list, id, item);

  if (item && (item->remap_choice & REMAP_META)) {
    ret = item->meta_serial;
  }
  return ret;
}

int serial_sanitizer_init(void) {
  for (int i = 0; i < serials_added; i++) {
    HASH_ADD_STR(serial_remap_list, ip_serial, &serial_remap_members[i]);
  }

  return 0;
}