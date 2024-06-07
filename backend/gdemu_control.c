#include <arch/arch.h>
#include <dc/sound/sound.h>
#include <kos.h>
#include <kos/thread.h>

#include "inc/vm2_api.h"
#include "backend/gd_item.h"
#include "gdemu_sdk.h"
#include "gdmenu_binary.h"
#include "cb_loader.h"
#include "controls.p1.h"
extern maple_device_t *vm2_dev;

static void wait_cd_ready(void) {
  for (int i = 0; i < 500; i++) {
	  if (cdrom_reinit() == ERR_OK) {
	     return;
	  }
	  thd_sleep(20);
  }
  return;
}

void bleem_launch(gd_item *disc) {
  file_t fd;
  uint32_t bleem_size;
  uint8_t *bleem_buf;
  
  fd = fs_open("/cd/BLEEM.BIN", O_RDONLY);
  
  if (fd == -1) {
	  printf("Can't open %s\n", "/cd/BLEEM.BIN");
	  return;
  }
  
  fs_seek(fd, 0, SEEK_END);
  bleem_size = fs_tell(fd);
  fs_seek(fd, 0, SEEK_SET);
  bleem_buf = (uint8_t*) malloc(bleem_size+32);
  bleem_buf = (uint8_t*) (((uint32_t) bleem_buf & 0xffffffe0) + 0x20);
  fs_read(fd, bleem_buf, bleem_size);
  fs_close(fd);
  
  gdemu_set_img_num((uint16_t)disc->slot_num);
  
  wait_cd_ready();
  
  /* Patch */
  ((uint16_t *) 0xAC000198)[0] = 0xFF86;
  
  for (int i = 0; i < altctrl_size; i++) {
    bleem_buf[i+0x7079C] = altctrl_data[i];
  }
  
  bleem_buf[0x49E6] = 0x06; // patch  restart emu A+B+X+Y+dpad down
  bleem_buf[0x49E7] = 0x0E; //       exit to menu A+B+X+Y+START
  
  bleem_buf[0x1CA70] = 1;
  
  arch_exec(bleem_buf, bleem_size);
}

void dreamcast_launch_disc(gd_item *disc) {
  ldr_params_t param;
  param.region_free = 1;
  param.force_vga = 1;
  param.IGR = 1;
  param.boot_intro = 1;
  param.sega_license = 1;
  
  if (!strncmp(disc->region, "JUE", 3)) {
	  param.game_region = (int)(((uint8_t *) 0x8C000072)[0] & 7);
  }
  else {
	  switch (disc->region[0]) {
		  case 'J':
			  param.game_region = 0;
			  break;
		  case 'U':
			  param.game_region = 1;
			  break;
		  case 'E':
			  param.game_region = 2;
			  break;
		  default:
			  param.game_region = (int)(((uint8_t *) 0x8C000072)[0] & 7);
			  break;
	  }
  }
  
  gdemu_set_img_num((uint16_t)disc->slot_num);
  //thd_sleep(500);
  
  if (vm2_dev)
  {
	  vm2_set_id(vm2_dev, disc->product, disc->name);
  }
  
  wait_cd_ready();
  
  int status = 0, disc_type = 0;
  
  cdrom_get_status(&status, &disc_type);
  
  param.disc_type = disc_type == CD_GDROM;
  
  if (!strncmp(disc->name, "PSO VER.2", 9) || !strncmp(disc->name, "SONIC ADVENTURE 2", 18)) {
	 param. need_game_fix = 1;
  }
  else {
	  param. need_game_fix = 0;
  }
  
  if (!strncmp((char*)0x8c0007CC, "1.004", 5)) {
	  ((uint32_t *) 0xAC000E20)[0] = 0;
  }
  else if (!strncmp((char*)0x8c0007CC, "1.01d", 5) ||
		   !strncmp((char*)0x8c0007CC, "1.01c", 5)) {
	  ((uint32_t *) 0xAC000E1C)[0] = 0;
  }
  
  ((uint32_t *) 0xAC0000E4)[0] = -3;
  
  memcpy((void*)0xACCFFF00, &param, 32);
  
  arch_exec(gdmenu_loader, gdmenu_loader_length);
}

void dreamcast_launch_cb(gd_item *disc) {
  file_t fd;
  uint32_t cb_size;
  uint8_t *cb_buf;
  uint32_t cheat_size = 0;
  uint8_t *cheat_buf;
  char cheat_name[32];
  
  fd = fs_open("/cd/PELICAN.BIN", O_RDONLY);
  
  if (fd == -1) {
	  return;
  }
  
  fs_seek(fd, 0, SEEK_END);
  cb_size = fs_tell(fd);
  fs_seek(fd, 0, SEEK_SET);
  cb_buf = (uint8_t*) malloc(cb_size+32);
  cb_buf = (uint8_t*) (((uint32_t) cb_buf & 0xffffffe0) + 0x20);
  fs_read(fd, cb_buf, cb_size);
  fs_close(fd);
  
  sprintf(cheat_name, "/cd/cheats/%s.bin", disc->product);
  
  if ((fd = fs_open(cheat_name, O_RDONLY)) == -1)
  {
	  fd = fs_open("/cd/cheats/FCDCHEATS.BIN", O_RDONLY);
  }
  
  if (fd != -1) {

	  fs_seek(fd, 0, SEEK_END);
	  cheat_size = fs_tell(fd);
	  fs_seek(fd, 0, SEEK_SET);
	  
	  cheat_buf = (uint8_t*) malloc(cheat_size+32);
	  cheat_buf = (uint8_t*) (((uint32_t) cheat_buf & 0xffffffe0) + 0x20);
	  
	  fs_read(fd, cheat_buf, 16);
	  
	  if (!strncmp((const char *) cheat_buf, "XploderDC Cheats", 16))
	  {
		  fs_seek(fd, 640, SEEK_SET);
		  cheat_size -= 640;
		  fs_read(fd, cheat_buf, cheat_size);
		  
		  if (!((uint32_t *) cheat_buf)[0])
		  {
			  cheat_size = 0;
		  }
	  }
	  else
	  {
		  cheat_size = 0;
	  }
	  
	  fs_close(fd);
  }
  
  gdemu_set_img_num((uint16_t)disc->slot_num);
  //thd_sleep(500);
  
  if (vm2_dev)
  {
	  vm2_set_id(vm2_dev, disc->product, disc->name);
  }
  
  wait_cd_ready();
  
  ((uint16_t *) 0xAC000198)[0] = 0xFF86;
  
  int status = 0, disc_type = 0;
  
  cdrom_get_status(&status, &disc_type);
  
  if (cheat_size)
  {
	  uint16_t *pelican = (uint16_t *) cb_buf;
	  
	  pelican[128] = 0;
	  pelican[129] = 0x90;
	  
	  pelican[10818] = (uint16_t) cheat_size;
	  pelican[10819] = (cheat_size >> 16);
	  
	  pelican[10820] = 0;
	  pelican[10821] = 0x8CD0;
	  
	  memcpy((void*)0xACD00000, cheat_buf, cheat_size);
  }
  
  if (disc_type != CD_GDROM) {
	  CDROM_TOC toc;
	  cdrom_read_toc(&toc, 0);
	  uint32_t lba = cdrom_locate_data_track(&toc);
	  
	  uint16_t *pelican = (uint16_t *) cb_buf;
	  
	  pelican[4067] = 0x711F;
	  pelican[4074] = 0xE500;
	  pelican[4302] = (uint16_t) lba;
	  pelican[4303] = (uint16_t) (lba >> 16);
	  pelican[472] = 0x0009;
	  pelican[4743] = 0x0018;
	  pelican[4745] = 0x0018;
	  pelican[5261] = 0x0008;
	  pelican[5433] = 0x0009;
	  pelican[5436] = 0x0009;
	  pelican[5438] = 0x0008;
	  pelican[5460] = 0x0009;
	  pelican[5472] = 0x0009;
	  pelican[5511] = 0x0008;
	  pelican[310573] = 0x64C3;
	  pelican[310648] = 0x0009;
	  pelican[310666] = 0x0009;
	  pelican[310708] = 0x0018;
	  pelican[310784] = 0x0000;
	  pelican[310785] = 0x8CE1;
	  
	  memcpy((void*)0xACE10000, cb_loader_data, cb_loader_size);
  }
  
  arch_exec(cb_buf, cb_size);
}

