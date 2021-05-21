/*
 * File: gdemu_sdk.h
 * Project: openMenu
 * File Created: Sunday, 20th January 2019 8:22:56 pm
 * Author: megavolt85
 * -----
 *
 */

#ifndef GDEMU_SDK_H
#define GDEMU_SDK_H
#include <stdint.h>

/* return 8 byte: 00 00 09 01 00 00 14 05
 * 01 09 00 00 - internal bootloader version. don't used in early models
 * 05 14 00 00 - FW version (5.14.0)
 */
int gdemu_get_version(void *buffer, uint32_t *size);

/* param = 0x55 next img */
/* param = 0x44 prev img */
#define GDEMU_NEXT_IMG 0x55
#define GDEMU_PREV_IMG 0x44
int gdemu_img_cmd(uint8_t cmd);

/* 0 = reset to default img */
/* 1 to 999 = set image index */
int gdemu_set_img_num(uint16_t img_num);
#endif /* GDEMU_SDK_H */
