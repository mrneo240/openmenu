/*
 * File: animation.h
 * Project: ui
 * File Created: Wednesday, 16th June 2021 10:23:23 pm
 * Author: Hayden Kowalchuk
 * -----
 * Copyright (c) 2021 Hayden Kowalchuk, Hayden Kowalchuk
 * License: BSD 3-clause "New" or "Revised" License, http://www.opensource.org/licenses/BSD-3-Clause
 */

#pragma once

#include <stdbool.h>

#include "../external/easing.h"

typedef struct vec2d {
  float x;
  float y;
} vec2d;

typedef struct AnimBare {
  int frame_len;
  int frame_now;
  bool active;
} AnimBare;

typedef struct anim2d {
  AnimBare time;
  vec2d start;
  vec2d end;
  vec2d cur;
} anim2d;

void anim_update_2d(anim2d *anim);
void anim_clear(anim2d *anim);

static inline bool anim_finished(AnimBare *anim) {
  return anim->frame_now == anim->frame_len;
}

static inline bool anim_active(AnimBare *anim) {
  return anim->active;
}

static inline bool anim_alive(AnimBare *anim) {
  return anim_active(anim) && (anim->frame_now <= anim->frame_len);
}

static inline void anim_tick(AnimBare *anim) {
  if (anim_alive(anim) && !anim_finished(anim))
    anim->frame_now++;
}

static inline void anim_tick_backward(AnimBare *anim) {
  if (anim_active(anim))
    anim->frame_now--;
  if (anim->frame_now <= 0) {
    anim->active = false;
    anim->frame_now = 0;
  }
}
