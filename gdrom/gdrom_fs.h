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
typedef DIR* DIR_TYPE;
typedef struct gd_dirent* DIRENT_TYPE;
#define DISC_PREFIX ""

#define fopen(path, b) gd_open(path, O_RDONLY)
#define fread(buf, size, num, fd) gd_read(fd, buf, (size) * (num))
#define fseek(fd, offset, origin) gd_seek(fd, offset, origin)
#define fclose(fd) gd_close(fd)
#define ftell(fd) gd_tell(fd)
#define opendir(path) gd_opendir(path)
#define readdir(fd) gd_readdir(fd)
#define closedir(fd) gd_closedir(fd)
#define FD_IS_OK(fd) (((fd) > 0))
#else
#ifdef COSMO
#include "../tools/cosmo/cosmopolitan.h"
#else
#include <dirent.h>
#include <stdio.h>
#include <sys/types.h>
#endif
typedef FILE* FD_TYPE;
typedef DIR* DIR_TYPE;
typedef struct dirent* DIRENT_TYPE;
#ifdef _arch_dreamcast
#define DISC_PREFIX "/cd/"
#endif
#define FD_IS_OK(fd) ((fd) != NULL)
#endif
