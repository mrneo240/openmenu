/*
 * File: packer.c
 * Project: dat_builder
 * File Created: Tuesday, 8th June 2021 2:32:46 pm
 * Author: Hayden Kowalchuk
 * -----
 * Copyright (c) 2021 Hayden Kowalchuk, Hayden Kowalchuk
 * License: BSD 3-clause "New" or "Revised" License, http://www.opensource.org/licenses/BSD-3-Clause
 */

#ifdef COSMO
#include "cosmo/cosmopolitan.h"
#else
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <errno.h>
#include <ctype.h>
#endif

#include "../inc/dat_format.h"

/* Called:
./datpack FOLDER output.dat

packs the items in the folder into the output.bin
*/

#define NUM_ARGS (2)

#if defined(WIN32) || defined(WINNT)
#define PATH_SEP "\\"
#else
#define PATH_SEP "/"
#endif

typedef struct bin_item_raw
{
  char ID[12];
  uint32_t offset;
} bin_item_raw;

/* Locals */
static bin_header file_header;
static FILE *out_fd;
static bin_item_raw *bin_items;
static unsigned char *data_buf;

void open_output(const char *path)
{
  out_fd = fopen(path, "wb");
  if (!out_fd)
  {
    printf("ERR: unable to open %s for writing!\n", path);
  }
}

void write_bin_file(void)
{
  char *nul = calloc(1, file_header.chunk_size - sizeof(file_header) - (sizeof(bin_item_raw) * file_header.num_chunks));

  printf("Writing:");
  /* Write header */
  printf("header..");
  fwrite(&file_header, sizeof(file_header), 1, out_fd);
  /* Write file list */
  printf("item list..");
  fwrite(bin_items, sizeof(bin_item_raw), file_header.num_chunks, out_fd);
  /* Write padding out to first chunk offset */
  printf("padding..");
  fwrite(nul, file_header.chunk_size - sizeof(file_header) - (sizeof(bin_item_raw) * file_header.num_chunks), 1, out_fd);
  /* Write out all chunks */
  printf("chunks..");
  fwrite(data_buf, file_header.num_chunks * file_header.chunk_size, 1, out_fd);

  fclose(out_fd);
  printf("done!\n");
}

int add_bin_file(const char *path, const char *folder, struct stat *statptr)
{
  char temp_id[12];
  char temp_file[FILENAME_MAX];

  if (file_header.chunk_size == 0)
  {
    file_header.chunk_size = (uint32_t)statptr->st_size;
    data_buf = malloc(file_header.chunk_size * file_header.padding0); /* Temporarily use padding0 as num_files */
  }
  else
  {
    if (statptr->st_size != file_header.chunk_size)
    {
      printf("Err: Filesize mismatch for %s, found %ld vs %d!\n", path, statptr->st_size, file_header.chunk_size);
      return -1;
    }
  }
  /* Check if filename too long, dont try to reconcile, just skip */
  char *dot = strrchr(path, '.');
  if ((size_t)dot - (size_t)path > 11)
  {
    printf("Err: filename too long \"%s\", maxlength = 11!\n", path);
    return -1;
  }

  temp_file[0] = '\0';
  strcpy(temp_file, folder);
  strcat(temp_file, PATH_SEP);
  strcat(temp_file, path);
  FILE *temp_fd = fopen(temp_file, "rb");
  if (!temp_fd)
  {
    printf("ERR: cant read %s\n", temp_file);
    return -1;
  }
  fread(data_buf + (file_header.num_chunks * file_header.chunk_size), file_header.chunk_size, 1, temp_fd);
  fclose(temp_fd);

  /* Use filename as ID, remove extension */
  memcpy(temp_id, path, 11);
  char *end = strrchr(temp_id, '.');
  if (end)
    memset(end, '\0', sizeof(temp_id) - ((size_t)end - (size_t)temp_id));
  char *temp_start = temp_id;
  while (*temp_start)
    *temp_start++ = toupper(*temp_start);
  memcpy(&bin_items[file_header.num_chunks].ID, temp_id, sizeof(bin_items->ID));

  bin_items[file_header.num_chunks].offset = file_header.num_chunks + 1;
  (void)file_header.num_chunks++;

  printf("Added[%d] as %s\n", file_header.num_chunks, temp_id);
}

int print_cb(const char *path, const char *folder, struct stat *statptr)
{
  printf("%s\n", path);
}

int iterate_dir(const char *path, int (*file_cb)(const char *, const char *, struct stat *))
{
  struct dirent *dp;
  struct stat statbuf;
  char pathbuf[FILENAME_MAX];
  uint32_t num_files_found;

  DIR *dir = opendir(path);

  if (file_cb == NULL)
  {
    file_cb = print_cb;
  }

  // Unable to open directory stream
  if (!dir)
    return -1;

  /* Loop twice, first time to count then second time to add */
  num_files_found = 0;
  while ((dp = readdir(dir)) != NULL)
  {
    /* ignore these */
    if (strcmp(dp->d_name, ".") == 0 || strcmp(dp->d_name, "..") == 0)
      continue;

    /* create proper full path */
    getcwd(pathbuf, FILENAME_MAX);
    strcat(pathbuf, PATH_SEP);
    strcat(pathbuf, path);
    strcat(pathbuf, PATH_SEP);
    strcat(pathbuf, dp->d_name);
    if (stat(pathbuf, &statbuf) == -1)
    {
      printf("ERR: errno = %d\n", errno);
      return -1;
    }

    /* only check files */
    if (S_ISREG(statbuf.st_mode))
    {
      num_files_found++;
    }
  }
#ifdef COSMO
  closedir(dir);
  dir = opendir(path);
#else
  rewinddir(dir);
#endif

  file_header.padding0 = num_files_found;
  bin_items = malloc(sizeof(bin_item_raw) * num_files_found);

  while ((dp = readdir(dir)) != NULL)
  {
    /* ignore these */
    if (strcmp(dp->d_name, ".") == 0 || strcmp(dp->d_name, "..") == 0)
      continue;

    /* create proper full path */
    getcwd(pathbuf, FILENAME_MAX);
    strcat(pathbuf, PATH_SEP);
    strcat(pathbuf, path);
    strcat(pathbuf, PATH_SEP);
    strcat(pathbuf, dp->d_name);
    if (stat(pathbuf, &statbuf) == -1)
    {
      printf("ERR: errno = %d\n", errno);
      return -1;
    }

    /* only check files */
    if (S_ISREG(statbuf.st_mode))
    {
      (*file_cb)(dp->d_name, path, &statbuf);
    }
  }

  closedir(dir);
}

int main(int argc, char **argv)
{
  if (argc < NUM_ARGS + 1 /*binary itself*/)
  {
    printf("Incorrect usage!\n\t./datpack FOLDER output.dat\n");
    return 1;
  }

  /* Setup file constraints */
  file_header.chunk_size = 0;
  file_header.num_chunks = 0;
  memcpy(&file_header.magic.rich.alpha, "DAT", 3);
  file_header.magic.rich.version = 1;
  file_header.padding0 = 0;

  out_fd = NULL;

  open_output(argv[2]);
  iterate_dir(argv[1], add_bin_file);
  file_header.padding0 = 0;
  write_bin_file();
}