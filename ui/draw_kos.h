/*
 * File: draw_kos.h
 * Project: ui
 * File Created: Wednesday, 19th May 2021 11:54:58 pm
 * Author: Hayden Kowalchuk
 * -----
 * Copyright (c) 2021 Hayden Kowalchuk, Hayden Kowalchuk
 * License: BSD 3-clause "New" or "Revised" License, http://www.opensource.org/licenses/BSD-3-Clause
 */

#pragma once

#include <dc/fmath.h>
#include <dc/pvr.h>
#include <stdint.h>

#include "dc/pvr_texture.h"

#ifndef PVR_PACK_ARGB
#define PVR_PACK_ARGB(a, r, g, b)( \
    (((uint8_t)((a))) << 24) |  \
    (((uint8_t)((r))) << 16) |  \
    (((uint8_t)((g))) << 8) |   \
    (((uint8_t)((b))) << 0))
#endif
