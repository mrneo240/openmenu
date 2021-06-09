/*
 * File: common.h
 * Project: ui
 * File Created: Thursday, 20th May 2021 12:53:34 am
 * Author: Hayden Kowalchuk
 * -----
 * Copyright (c) 2021 Hayden Kowalchuk, Hayden Kowalchuk
 * License: BSD 3-clause "New" or "Revised" License, http://www.opensource.org/licenses/BSD-3-Clause
 */

#pragma once

#include <sys/stat.h>

enum control { NONE = 0,
               LEFT,
               RIGHT,
               UP,
               DOWN,
               A,
               B,
               X,
               Y,
               START };

static inline int file_exists(const char *path) {
  struct stat buffer;
  return (stat(path, &buffer) == 0);
}