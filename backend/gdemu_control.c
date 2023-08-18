#include <arch/arch.h>
#include <dc/sound/sound.h>
#include <kos.h>
#include <kos/thread.h>

#include "backend/gd_item.h"
#include "gdemu_sdk.h"
#include "gdmenu_binary.h"

void dreamcast_launch_disc(gd_item *disc) 
{
  ldr_params_t param;
  param.region_free = 1;
  param.force_vga = 1;
  param.IGR = 1;
  param.boot_intro = 1;
  param.sega_license = 1;
  
  if (!strncmp(disc->region, "JUE", 3))
  {
	  param.game_region = (int)(((uint8_t *) 0x8C000072)[0] & 7);
  }
  else
  {
	  switch (disc->region[0])
	  {
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
  thd_sleep(200);
  
  if (cdrom_reinit() != ERR_OK)
  {
	  return;
  }
  
  int status = 0, disc_type = 0;
  
  cdrom_get_status(&status, &disc_type);
  
  param.disc_type = disc_type == CD_GDROM;
  
  if (!strncmp(disc->name, "PSO VER.2", 9) || !strncmp(disc->name, "SONIC ADVENTURE 2", 18))
  {
	 param. need_game_fix = 1;
  }
  else
  {
	  param. need_game_fix = 0;
  }
  
  if (!strncmp((char*)0x8c0007CC, "1.004", 5))
  {
	  ((uint32_t *) 0xAC000E20)[0] = 0;
  }
  else if (!strncmp((char*)0x8c0007CC, "1.01d", 5))
  {
	  ((uint32_t *) 0xAC000E1C)[0] = 0;
  }
  
  memcpy((void*)0xACCFFF00, &param, 32);
  arch_exec(gdmenu_loader, gdmenu_loader_length);
  //run_game(disc->region, disc->product);
}

