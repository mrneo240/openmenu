/*
 * File: txr_manager.h
 * Project: texture
 * File Created: Friday, 21st May 2021 2:30:41 pm
 * Author: Hayden Kowalchuk
 * -----
 * Copyright (c) 2021 Hayden Kowalchuk, Hayden Kowalchuk
 * License: BSD 3-clause "New" or "Revised" License, http://www.opensource.org/licenses/BSD-3-Clause
 */

#pragma once

struct image;

int txr_create_small_pool(void);
int txr_create_large_pool(void); /* unused for now */

int txr_get_small(const char *id, struct image *img);
int txr_get_large(const char *id, struct image *img); /* unused for now */