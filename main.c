
/*
 * File: main.c
 * Project: kos_pvr_texture_load
 * File Created: Wednesday, 23rd January 2019 8:07:09 pm
 * Author: Hayden Kowalchuk (hayden@hkowsoftware.com)
 * -----
 * Copyright (c) 2019 Hayden Kowalchuk
 */

#include <dc/maple.h>
#include <dc/maple/controller.h>
#include <dc/pvr.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "backend/gd_list.h"
//#include "ui/ui_line_large.h"
//#undef UI_NAME
#include "ui/common.h"
#include "ui/dc/input.h"
#include "ui/draw_prototypes.h"
#include "ui/ui_line_desc.h"
#undef UI_NAME

static int init() {
  int ret = 0;

  ret += list_read();

  draw_init();

  LIST_DESC_init();
  LIST_DESC_setup();

  return ret;
}

static void draw() {
  pvr_wait_ready();
  pvr_scene_begin();

  pvr_list_begin(PVR_LIST_TR_POLY);

  LIST_DESC_draw();

  pvr_list_finish();

  pvr_scene_finish();
}

static void processInput(void) {
  static inputs _input;
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
  _input.trg_left = (uint8_t)state->ltrig & 255;
  _input.trg_right = (uint8_t)state->rtrig & 255;

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

  return NONE;
}

int main(int argc, char *argv[]) {
  fflush(stdout);
  setbuf(stdout, NULL);
  pvr_init_defaults();

  if (init()) {
    puts("Init error.");
    return 1;
  }

  for (;;) {
    enum control input = translate_input();
    LIST_DESC_handle_input(input);
    draw();
  }

  return 0;
}
