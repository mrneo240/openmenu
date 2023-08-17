
/*
 * File: main.c
 * Project: kos_pvr_texture_load
 * File Created: Wednesday, 23rd January 2019 8:07:09 pm
 * Author: Hayden Kowalchuk (hayden@hkowsoftware.com)
 * -----
 * Copyright (c) 2019 Hayden Kowalchuk
 */

#include <dc/cdrom.h>
#include <dc/flashrom.h>
#include <dc/maple.h>
#include <dc/maple/controller.h>
#include <dc/pvr.h>
#include <dc/video.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arch/exec.h>

#include "backend/db_list.h"
#include "backend/gd_list.h"
#include "ui/common.h"
#include "ui/dc/input.h"
#include "ui/draw_prototypes.h"
#include "ui/global_settings.h"

/* UI Collection */
#include "ui/ui_grid.h"
#undef UI_NAME
#include "ui/ui_line_desc.h"
#undef UI_NAME
#include "ui/ui_gdmenu.h"
#undef UI_NAME

#include "texture/txr_manager.h"

#include "inc/bloader.h"

void (*current_ui_init)(void);
void (*current_ui_setup)(void);
void (*current_ui_draw_OP)(void);
void (*current_ui_draw_TR)(void);
void (*current_ui_handle_input)(unsigned int);

typedef struct ui_template {
  void (*init)(void);
  void (*setup)(void);
  void (*drawOP)(void);
  void (*drawTR)(void);
  void (*handle_input)(unsigned int);
} ui_template;

#define UI_TEMPLATE(name)                                                                                                                                                                \
  (ui_template) {                                                                                                                                                                        \
    .init = FUNC_NAME(name, init), .setup = FUNC_NAME(name, setup), .drawOP = FUNC_NAME(name, drawOP), .drawTR = FUNC_NAME(name, drawTR), .handle_input = FUNC_NAME(name, handle_input), \
  }

static ui_template ui_choices[] = {
    UI_TEMPLATE(LIST_DESC),
    UI_TEMPLATE(GRID_3),
    UI_TEMPLATE(GDMENU_EMU),
};

static const int num_ui_choices = sizeof(ui_choices) / sizeof(ui_template);
static int ui_choice_current = 0;

static void ui_set_choice(int choice) {
  if (choice < UI_START || choice >= num_ui_choices) {
    choice = UI_START;
  }
  current_ui_init = ui_choices[choice].init;
  current_ui_setup = ui_choices[choice].setup;
  current_ui_draw_OP = ui_choices[choice].drawOP;
  current_ui_draw_TR = ui_choices[choice].drawTR;
  current_ui_handle_input = ui_choices[choice].handle_input;

  ui_choice_current = choice;

  /* Call init & setup */
  (*current_ui_init)();
  (*current_ui_setup)();
}

int round( float x) {
  if (x < 0.0f)
    return (int)(x - 0.5f);
  else
    return (int)(x + 0.5f);
}

void reload_ui(void) {
  openmenu_settings *settings = settings_get();
  ui_set_choice(settings->ui);
}

static int init(void) {
  int ret = 0;

  /* Load settings */
  settings_init();

  ret += txr_create_small_pool();
  ret += txr_create_large_pool();
  ret += txr_load_DATs();
  ret += list_read_default();
  ret += db_load_DAT();
  ret += theme_manager_load();

  /* setup internal memory zones */
  draw_init();

  /* Load UI */
  reload_ui();

  return ret;
}

static void draw(void) {
  pvr_wait_ready();
  pvr_scene_begin();

  draw_set_list(PVR_LIST_OP_POLY);
  pvr_list_begin(PVR_LIST_OP_POLY);

  (*current_ui_draw_OP)();

  pvr_list_finish();

  draw_set_list(PVR_LIST_TR_POLY);
  pvr_list_begin(PVR_LIST_TR_POLY);

  (*current_ui_draw_TR)();

  pvr_list_finish();

  pvr_scene_finish();
}

static void processInput(void) {
  inputs _input;
  unsigned int buttons;

  maple_device_t *cont;
  cont_state_t *state;

  cont = maple_enum_type(0, MAPLE_FUNC_CONTROLLER);
  if (!cont)
    return;
  state = (cont_state_t *)maple_dev_status(cont);

  buttons = state->buttons;

  /*  Reset Everything */
  memset(&_input, 0, sizeof(inputs));

  /* DPAD */
  _input.dpad = (state->buttons >> 4) & ~240;  //mrneo240 ;)

  /* BUTTONS */
  _input.btn_a = (uint8_t) !!(buttons & CONT_A);
  _input.btn_b = (uint8_t) !!(buttons & CONT_B);
  _input.btn_x = (uint8_t) !!(buttons & CONT_X);
  _input.btn_y = (uint8_t) !!(buttons & CONT_Y);
  _input.btn_start = (uint8_t) !!(buttons & CONT_START);

  /* ANALOG */
  _input.axes_1 = ((uint8_t)(state->joyx) + 128);
  _input.axes_2 = ((uint8_t)(state->joyy) + 128);

  /* TRIGGERS */
  if (!strncmp("Dreamcast Fishing Controller", cont->info.product_name, 28))
  {
	 _input.trg_left = 0;
	 _input.trg_right = 0;
  }
  else
  {
	 _input.trg_left = (uint8_t)state->ltrig & 255;
	 _input.trg_right = (uint8_t)state->rtrig & 255;
  }

  INPT_ReceiveFromHost(_input);
}

static int translate_input(void) {
  processInput();
  if (INPT_DPADDirection(DPAD_LEFT)) {
    return LEFT;
  }
  if (INPT_DPADDirection(DPAD_RIGHT)) {
    return RIGHT;
  }
  if (INPT_DPADDirection(DPAD_UP)) {
    return UP;
  }
  if (INPT_DPADDirection(DPAD_DOWN)) {
    return DOWN;
  }

  if (INPT_AnalogI(AXES_X) < 128 - 24) {
    return LEFT;
  }
  if (INPT_AnalogI(AXES_X) > 128 + 24) {
    return RIGHT;
  }

  if (INPT_AnalogI(AXES_Y) < 128 - 24) {
    return UP;
  }
  if (INPT_AnalogI(AXES_Y) > 128 + 24) {
    return DOWN;
  }

  if (INPT_Button(BTN_A)) {
    return A;
  }
  if (INPT_Button(BTN_B)) {
    return B;
  }
  if (INPT_Button(BTN_X)) {
    return X;
  }
  if (INPT_Button(BTN_Y)) {
    return Y;
  }
  if (INPT_Button(BTN_START)) {
    return START;
  }

  /* Triggers */
  if (INPT_TriggerPressed(TRIGGER_L)) {
    return TRIG_L;
  }
  if (INPT_TriggerPressed(TRIGGER_R)) {
    return TRIG_R;
  }

  return NONE;
}

void reset_gdrom_drive(void) {
  int status;
  int disc_type;

  do {
    cdrom_get_status(&status, &disc_type);

    if (status == CD_STATUS_PAUSED || status == CD_STATUS_STANDBY || status == CD_STATUS_PLAYING) {
      break;
    }
  } while (1);

  cdrom_init();
  void gd_reset_handles(void);
  gd_reset_handles();

  do {
    cdrom_get_status(&status, &disc_type);

    if (status == CD_STATUS_PAUSED || status == CD_STATUS_STANDBY || status == CD_STATUS_PLAYING) {
      break;
    }
  } while (1);
}

static void init_gfx_pvr(void) {
  /* BlueCrab (c) 2014,
    This assumes that the video mode is initialized as KOS
   normally does, that is to 640x480 NTSC IL or 640x480 VGA */
  int dc_region, ct;

  dc_region = flashrom_get_region();
  ct = vid_check_cable();

  /* Prompt the user for whether to run in PAL50 or PAL60 if the flashrom says
       the Dreamcast is European and a VGA Box is not hooked up. */
  if (dc_region == FLASHROM_REGION_EUROPE && ct != CT_VGA) {
    if (/*pal_menu()*/ 1 == 1)
      vid_set_mode(DM_640x480_NTSC_IL, PM_RGB565);
    else
      vid_set_mode(DM_640x480_PAL_IL, PM_RGB565);
  }

  pvr_init_params_t params = {
      /* Enable opaque and translucent polygons with size 32 and 32 */
      {PVR_BINSIZE_32, PVR_BINSIZE_0, PVR_BINSIZE_32, PVR_BINSIZE_0, PVR_BINSIZE_0}, /* Only TR */
      256 * 1024,                                                                    /* 256kb Vertex buffer  */
      0,                                                                             /* No DMA, but maybe? */
      0,                                                                             /* No FSAA */
      0                                                                              /* Disable TR autosort */
  };

  pvr_init(&params);
  draw_set_list(PVR_LIST_OP_POLY);
}

int main(int argc, char *argv[]) {
  /* unused */
  (void)argc;
  (void)argv;

  fflush(stdout);
  setbuf(stdout, NULL);
  init_gfx_pvr();

  //reset_gdrom_drive();

  if (init()) {
    puts("Init error.");
    return 1;
  }

  for (;;) {
    z_reset();
    enum control input = translate_input();
    (*current_ui_handle_input)(input);
    draw();
  }

  return 0;
}

void exit_to_bios(void)
{
	openmenu_settings* cur = settings_get();
	
	bloader_config->enable_wide  = cur->aspect;
	bloader_config->enable_3d = 1;
	
	arch_exec_at(bloader_data, bloader_size, 0xacf00000);
}

