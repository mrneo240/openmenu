#pragma once
#ifndef GD_MENU_LOADER_H
#define GD_MENU_LOADER_H

#include <stdint.h>

void run_game(const char *region, const char *product) __attribute__((noreturn));

typedef struct
{
  uint32_t region_free;    // 0 - off, 1 - on (patch bios region protection)
  uint32_t force_vga;      // 0 - off, 1 - on (patch vga flag bios check)
  uint32_t IGR;            // 0 - off, 1 - on (In-Game Reset)
  uint32_t boot_intro;     // 0 - skip, 1 - show (boot animation)
  uint32_t sega_license;   // 0 - skip, 1 - show (IP.BIN license screen)
  uint32_t game_region;    // 0 - J, 1 - U, 2 - E
  uint32_t disc_type;      // 0 - CD, 1 - GD
  uint32_t need_game_fix;  // 0 - no, 1 - yes
} ldr_params_t;

#endif
