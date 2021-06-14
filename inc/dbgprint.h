/*
 * File: dbgprint.h
 * Project: ini_parse
 * File Created: Wednesday, 9th June 2021 8:49:15 pm
 * Author: Hayden Kowalchuk
 * -----
 * Copyright (c) 2021 Hayden Kowalchuk, Hayden Kowalchuk
 * License: BSD 3-clause "New" or "Revised" License, http://www.opensource.org/licenses/BSD-3-Clause
 */

#pragma once

#if DEBUG
#define DBG_PRINT(...) printf(__VA_ARGS__)
#define DBG_CHAR_INFO (1)
#define DBG_KERN_INFO (0)
#else
#define DBG_PRINT(...)
#define DBG_CHAR_INFO (0)
#define DBG_KERN_INFO (0)
#endif
