#include <arch/arch.h>
#include <dc/sound/sound.h>
#include <kos.h>
#include <kos/thread.h>

#include "backend/gd_item.h"
#include "gdemu_sdk.h"
#include "gdmenu_loader.h"

#ifndef GDROM_FS
void gd_reset_handles(void) {
}

void run_game(const char *region, const char *product) {
  (void)region;
  (void)product;
  void arch_menu(void) __attribute__((noreturn));
  arch_menu();
  __builtin_unreachable();
}
#endif

void dreamcast_launch_disc(gd_item *disc) {
  gdemu_set_img_num((uint16_t)disc->slot_num);
  thd_sleep(200);

  run_game(disc->region, disc->product);
}
