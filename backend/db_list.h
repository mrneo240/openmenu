/*
 * File: db_list.h
 * Project: backend
 * File Created: Wednesday, 16th June 2021 10:32:48 pm
 * Author: Hayden Kowalchuk
 * -----
 * Copyright (c) 2021 Hayden Kowalchuk, Hayden Kowalchuk
 * License: BSD 3-clause "New" or "Revised" License, http://www.opensource.org/licenses/BSD-3-Clause
 */

#pragma once

#include "db_item.h"

int db_load_DAT(void);
int db_get_meta(const char *id, struct db_item **item);

const char *db_format_nplayers_str(int nplayers);
const char *db_format_vmu_blocks_str(int num_blocks);