/*
 * File: serial_sanitize.h
 * Project: texture
 * File Created: Monday, 28th June 2021 1:47:41 pm
 * Author: Hayden Kowalchuk
 * -----
 * Copyright (c) 2021 Hayden Kowalchuk, Hayden Kowalchuk
 * License: BSD 3-clause "New" or "Revised" License, http://www.opensource.org/licenses/BSD-3-Clause
 */

#pragma once

const char* serial_santize_art(const char* id);
const char* serial_santize_meta(const char* id);
int serial_sanitizer_init(void);
