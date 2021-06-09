
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
#include "ui/common.h"
#include "ui/dc/input.h"
#include "ui/draw_prototypes.h"

/* UI Collection */
#include "ui/ui_grid.h"
#undef UI_NAME
#include "ui/ui_line_desc.h"
#undef UI_NAME

#include "texture/txr_manager.h"

void (*current_ui_init)(void);
void (*current_ui_setup)(void);
void (*current_ui_draw)(void);
void (*current_ui_handle_input)(unsigned int);

typedef struct ui_template {
  void (*init)(void);
  void (*setup)(void);
  void (*draw)(void);
  void (*handle_input)(unsigned int);
} ui_template;

#define UI_TEMPLATE(name)                                                                                                                         \
  (ui_template) {                                                                                                                                 \
    .init = FUNC_NAME(name, init), .setup = FUNC_NAME(name, setup), .draw = FUNC_NAME(name, draw), .handle_input = FUNC_NAME(name, handle_input), \
  }

static ui_template ui_choices[] = {
    UI_TEMPLATE(GRID_3),
    UI_TEMPLATE(LIST_DESC),
};

static const int num_ui_choices = sizeof(ui_choices) / sizeof(ui_template);
static int ui_choice_current = 0;

void ui_set_choice(int choice) {
  if (choice >= 0 && choice < num_ui_choices) {
    current_ui_init = ui_choices[choice].init;
    current_ui_setup = ui_choices[choice].setup;
    current_ui_draw = ui_choices[choice].draw;
    current_ui_handle_input = ui_choices[choice].handle_input;

    ui_choice_current = choice;

    /* Call setup but not init */
    (*current_ui_setup)();
  }
}

void ui_set_default(void) {
  ui_set_choice(0);
}

void ui_cycle_next(void) {
  ui_choice_current++;
  if (ui_choice_current > num_ui_choices) {
    ui_choice_current = 0;
  }
  ui_set_choice(ui_choice_current);
}

static int init(void) {
  int ret = 0;

  ret += txr_create_small_pool();
  ret += txr_create_large_pool();
  ret += txr_load_DATs();
  ret += list_read();

  draw_init();

  ui_set_default();

  (*current_ui_init)();
  (*current_ui_setup)();

  //GRID_3_init();
  //GRID_3_setup();

  //LIST_DESC_init();
  //LIST_DESC_setup();

  return ret;
}

static void draw(void) {
  pvr_wait_ready();
  pvr_scene_begin();

  pvr_list_begin(PVR_LIST_TR_POLY);

  (*current_ui_draw)();
  //GRID_3_draw();
  //LIST_DESC_draw();

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
    (*current_ui_handle_input)(input);
    //GRID_3_handle_input(input);
    //LIST_DESC_handle_input(input);
    draw();
  }

  return 0;
}
