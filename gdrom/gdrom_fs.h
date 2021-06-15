/*
 * File: gdrom_fs.h
 * Project: gdrom
 * File Created: Wednesday, 9th June 2021 4:27:01 pm
 * Author: Hayden Kowalchuk
 * -----
 * Copyright (c) 2021 Hayden Kowalchuk, Hayden Kowalchuk
 * License: BSD 3-clause "New" or "Revised" License, http://www.opensource.org/licenses/BSD-3-Clause
 */

#pragma once

/* Used to control reading from GD-ROM or CD-ROM */

#ifdef GDROM_FS
#include "gdfs.h"
typedef int FD_TYPE;
#define DISC_PREFIX ""
#define fopen(path, b) gd_open(path, O_RDONLY)
#define fread(buf, size, num, fd) gd_read(fd, buf, (size) * (num))
#define fseek(fd, offset, origin) gd_seek(fd, offset, origin)
#define fclose(fd) gd_close(fd)
#define ftell(fd) gd_tell(fd)
#else
#include <stdio.h>
typedef FILE* FD_TYPE;
#define DISC_PREFIX "/cd/"
#endif
