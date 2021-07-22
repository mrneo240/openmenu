#include <arch/arch.h>
#include <dc/sound/sound.h>
#include <kos.h>
#include <kos/thread.h>

#include "gdemu_sdk.h"
#include "gdmenu_loader.h"
#include "rungd.h"

#ifndef GDROM_FS
void gd_reset_handles(void){}
#endif

void dreamcast_rungd(unsigned int slot_num) {
  uint16_t image = (uint16_t)(slot_num & 0xFFFF);
  gdemu_set_img_num(image);
  thd_sleep(200);

#if __GNUC__ < 4
  arch_dtors();
#else
//  fini();
#endif

#ifdef USE_GDMENU_LOADER
  arch_exec(gdmenu_loader, gdmenu_loader_length);
#else
  ubc_disable_all();
  fs_dclsocket_shutdown();
  net_shutdown();
  irq_disable();
  snd_shutdown();
  timer_shutdown();
  la_shutdown();
  bba_shutdown();
  maple_shutdown();
  cdrom_shutdown();
  spu_dma_shutdown();
  spu_shutdown();
  pvr_shutdown();
  library_shutdown();
  fs_dcload_shutdown();
  fs_vmu_shutdown();
  vmufs_shutdown();
  fs_iso9660_shutdown();
  fs_ramdisk_shutdown();
  fs_romdisk_shutdown();
  fs_pty_shutdown();
  fs_shutdown();
  thd_shutdown();
  rtc_shutdown();
  irq_shutdown();

  gdplay_run_game((void*)rungd);
#endif
}
