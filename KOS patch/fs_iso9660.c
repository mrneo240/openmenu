/* KallistiOS ##version##

   fs_iso9660.c
   Copyright (C) 2000, 2001, 2003 Megan Potter
   Copyright (C) 2001 Andrew Kieschnick
   Copyright (C) 2002 Bero
   Copyright (C) 2012, 2013, 2014, 2016 Lawrence Sebald

*/

/*

This module implements an ISO9660 file system for reading from a CDR or CD
in the DC's GD-Rom drive.

Rock Ridge support has now been implemented, thanks to Andrew Kieschnick
who donated the code. Thanks to Bero for the Joliet support here.

This FS is considerably simplified from what you'd find in a bigger kernel
like Linux or BSD, since we have the pleasure of working with only a single
device capable of ISO9660 at once =). So there are a number of things in here
that are global variables that might otherwise not be.

Some thanks are in order here to Marcus Comstedt for providing an ISO9660
implementation that was easy enough to understand without downloading the
full spec =). Thanks also in order to the creators of the BSD and Linux
ISO9660 systems, as these were used as references as well.

*/

#include <dc/fs_iso9660.h>
#include <dc/cdrom.h>
#include <dc/vblank.h>

#include <kos/thread.h>
#include <kos/mutex.h>
#include <kos/fs.h>
#include <kos/opts.h>

#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <strings.h>
#include <malloc.h>
#include <errno.h>

static int init_percd(void);
static int percd_done;
static int disc_type;

/********************************************************************************/
/* Low-level Joliet utils */

/* Joliet UCS is big endian */
static void utf2ucs(uint8 * ucs, const uint8 * utf) {
    int c;

    do {
        c = *utf++;

        if(c <= 0x7f) {
        }
        else if(c < 0xc0) {
            c = (c & 0x1f) << 6;
            c |= (*utf++) & 0x3f;
        }
        else {
            c = (c & 0x0f) << 12;
            c |= ((*utf++) & 0x3f) << 6;
            c |= (*utf++) & 0x3f;
        }

        *ucs++ = c >> 8;
        *ucs++ = c & 0xff;
    }
    while(c);
}

static void ucs2utfn(uint8 * utf, const uint8 * ucs, size_t len) {
    int c;

    len = len / 2;

    while(len) {
        len--;
        c = (*ucs++) << 8;
        c |= *ucs++;

        if(c == ';') break;

        if(c <= 0x7f) {
            *utf++ = c;
        }
        else if(c <= 0x7ff) {
            *utf++ = 0xc0 | (c >> 6);
            *utf++ = 0x80 | (c & 0x3f);
        }
        else {
            *utf++ = 0xe0 | (c >> 12);
            *utf++ = 0x80 | ((c >> 6) & 0x3f);
            *utf++ = 0x80 | (c & 0x3f);
        }
    }

    *utf = 0;
}

static int ucscompare(const uint8 * isofn, const uint8 * normalfn, int isosize) {
    int i, c0, c1 = 0;

    /* Compare ISO name */
    for(i = 0; i < isosize; i += 2) {
        c0 = ((int)isofn[i] << 8) | ((int)isofn[i + 1]);
        c1 = ((int)normalfn[i] << 8) | ((int)normalfn[i + 1]);

        if(c0 == ';') break;

        /* Otherwise, compare the chars normally */
        if(tolower(c0) != tolower(c1))
            return -1;
    }

    c1 = ((int)normalfn[i] << 8) | (normalfn[i + 1]);

    /* Catch ISO name shorter than normal name */
    if(c1 != '/' && c1 != '\0')
        return -1;
    else
        return 0;
}

static int isjoliet(char * p) {
    if(p[0] == '%' && p[1] == '/') {
        switch(p[2]) {
            case '@':
                return 1;
            case 'C':
                return 2;
            case 'E':
                return 3;
        }
    }

    return 0;
}

static int joliet;

/********************************************************************************/
/* Low-level ISO utils */

/* ISO Directory entry */
typedef struct {
    uint8   length;         /* 711 */
    uint8   ext_attr_length;    /* 711 */
    uint8   extent[8];      /* 733 */
    uint8   size[8];        /* 733 */
    uint8   date[7];        /* 7x711 */
    uint8   flags;
    uint8   file_unit_size;     /* 711 */
    uint8   interleave;     /* 711 */
    uint8   vol_sequence[4];    /* 723 */
    uint8   name_len;       /* 711 */
    char    name[1];
} iso_dirent_t;

/* Util function to reverse the byte order of a uint32 */
/* static uint32 ntohl_32(const void *data) {
    const uint8 *d = (const uint8*)data;
    return (d[0] << 24) | (d[1] << 16) | (d[2] << 8) | (d[3] << 0);
} */

/* This seems kinda silly, but it's important since it allows us
   to do unaligned accesses on a buffer */
static uint32 htohl_32(const void *data) {
    const uint8 *d = (const uint8*)data;
    return (d[0] << 0) | (d[1] << 8) | (d[2] << 16) | (d[3] << 24);
}

/* Read red-book section 7.1.1 number (8 bit) */
/* static uint8 iso_711(const uint8 *from) { return (*from & 0xff); } */

/* Read red-book section 7.3.3 number (32 bit LE / 32 bit BE) */
static uint32 iso_733(const uint8 *from) {
    return htohl_32(from);
}


/********************************************************************************/
/* Low-level block cacheing routines. This implements a simple queue-based
   LRU/MRU cacheing system. Whenever a block is requested, it will be placed
   on the MRU end of the queue. As more blocks are loaded than can fit in
   the cache, blocks are deleted from the LRU end. */

/* Holds the data for one cache block, and a pointer to the next one.
   As sectors are read from the disc, they are added to the front of
   this cache. As the cache fills up, sectors are removed from the end
   of it. */
typedef struct {
    uint32  sector;         /* CD sector */
    uint8   data[2048];     /* Sector data */
} cache_block_t;

/* List of cache blocks (ordered least recently used to most recently) */
#define NUM_CACHE_BLOCKS 16
static cache_block_t *icache[NUM_CACHE_BLOCKS];     /* inode cache */
static cache_block_t *dcache[NUM_CACHE_BLOCKS];     /* data cache */

/* Cache modification mutex */
static mutex_t cache_mutex;

/* Clears all cache blocks */
static void bclear_cache(cache_block_t **cache) {
    int i;

    mutex_lock(&cache_mutex);

    for(i = 0; i < NUM_CACHE_BLOCKS; i++)
        cache[i]->sector = (uint32)-1;

    mutex_unlock(&cache_mutex);
}

/* Graduate a block from its current position to the MRU end of the cache */
static void bgrad_cache(cache_block_t **cache, int block) {
    int     i;
    cache_block_t   *tmp;

    /* Don't try it with the end block */
    if(block < 0 || block >= (NUM_CACHE_BLOCKS - 1)) return;

    /* Make a copy and scoot everything down */
    tmp = cache[block];

    for(i = block; i < (NUM_CACHE_BLOCKS - 1); i++)
        cache[i] = cache[i + 1];

    cache[NUM_CACHE_BLOCKS - 1] = tmp;
}

/* Pulls the requested sector into a cache block and returns the cache
   block index. Note that the sector in question may already be in the
   cache, in which case it just returns the containing block. */
static void iso_break_all(void);
static int bread_cache(cache_block_t **cache, uint32 sector) {
    int i, j, rv;

    rv = -1;
    mutex_lock(&cache_mutex);

    /* Look for a pre-existing cache block */
    for(i = NUM_CACHE_BLOCKS - 1; i >= 0; i--) {
        if(cache[i]->sector == sector) {
            bgrad_cache(cache, i);
            rv = NUM_CACHE_BLOCKS - 1;
            goto bread_exit;
        }
    }

    /* If not, look for an open cache slot; if we find one, use it */
    for(i = 0; i < NUM_CACHE_BLOCKS; i++) {
        if(cache[i]->sector == (uint32)-1) break;
    }

    /* If we didn't find one, kick an LRU block out of cache */
    if(i >= NUM_CACHE_BLOCKS) {
        i = 0;
    }

    /* Load the requested block */
    j = cdrom_read_sectors(cache[i]->data, sector + 150, 1);

    if(j < 0) {
        //dbglog(DBG_ERROR, "fs_iso9660: can't read_sectors for %d: %d\n",
        //  sector+150, j);
        if(j == ERR_DISC_CHG || j == ERR_NO_DISC) {
            init_percd();
        }

        rv = -1;
        goto bread_exit;
    }

    cache[i]->sector = sector;

    /* Move it to the most-recently-used position */
    bgrad_cache(cache, i);
    rv = NUM_CACHE_BLOCKS - 1;

    /* Return the new cache block index */
bread_exit:
    mutex_unlock(&cache_mutex);
    return rv;
}

/* read data block */
static int bdread(uint32 sector) {
    return bread_cache(dcache, sector);
}

/* read inode block */
static int biread(uint32 sector) {
    return bread_cache(icache, sector);
}

/* Clear both caches */
static void bclear(void) {
    bclear_cache(dcache);
    bclear_cache(icache);
}

/********************************************************************************/
/* Higher-level ISO9660 primitives */

/* Root FS session location (in sectors) */
static uint32 session_base = 0;

/* Root directory extent and size in bytes */
static uint32 root_extent = 0, root_size = 0;

/* Root dirent */
static iso_dirent_t root_dirent;


/* Per-disc initialization; this is done every time it's discovered that
   a new CD has been inserted. */
static int init_percd() {
    int     i, blk;
    CDROM_TOC   toc;

    dbglog(DBG_NOTICE, "fs_iso9660: disc change detected\n");

    /* Start off with no cached blocks and no open files*/
    iso_reset();

    /* Locate the root session */
    if((i = cdrom_reinit()) != 0) {
        dbglog(DBG_ERROR, "fs_iso9660:init_percd: cdrom_reinit returned %d\n", i);
        return -1;
    }

    if((i = cdrom_read_toc(&toc, (disc_type == CD_GDROM))) != 0)
    {
        return i;
       }
	
	if(disc_type == CD_GDROM)
	{
		session_base = 45150;
	}
    else if(!(session_base = cdrom_locate_data_track(&toc)))
    {
        return -1;
	}
	
    /* Check for joliet extensions */
    joliet = 0;

    for(i = 1; i <= 3; i++) {
        blk = biread(session_base + i + 16 - 150);

        if(blk < 0) return blk;

        if(memcmp((char *)icache[blk]->data, "\02CD001", 6) == 0) {
            joliet = isjoliet((char *)icache[blk]->data + 88);
            dbglog(DBG_NOTICE, "  (joliet level %d extensions detected)\n", joliet);

            if(joliet) break;
        }
    }

    /* If that failed, go after standard/RockRidge ISO */
    if(!joliet) {
        /* Grab and check the volume descriptor */
        blk = biread(session_base + 16 - 150);

        if(blk < 0) return i;

        if(memcmp((char*)icache[blk]->data, "\01CD001", 6)) {
            dbglog(DBG_ERROR, "fs_iso9660: disc is not iso9660\r\n");
            return -1;
        }
    }

    /* Locate the root directory */
    memcpy(&root_dirent, icache[blk]->data + 156, sizeof(iso_dirent_t));
    root_extent = iso_733(root_dirent.extent);
    root_size = iso_733(root_dirent.size);

    return 0;
}

/* Compare an ISO9660 filename against a normal filename. This takes into
   account the version code on the end and is not case sensitive. Also
   takes into account the trailing period that some CD burning software
   adds. */
static int fncompare(const char *isofn, int isosize, const char *normalfn) {
    int i;

    /* Compare ISO name */
    for(i = 0; i < isosize; i++) {
        /* Weed out version codes */
        if(isofn[i] == ';') break;

        /* Deal with crap '.' at end of filenames */
        if(isofn[i] == '.' &&
                (i == (isosize - 1) || isofn[i + 1] == ';'))
            break;

        /* Otherwise, compare the chars normally */
        if(tolower((int)isofn[i]) != tolower((int)normalfn[i]))
            return -1;
    }

    /* Catch ISO name shorter than normal name */
    if(normalfn[i] != '/' && normalfn[i] != '\0')
        return -1;
    else
        return 0;
}

/* Locate an ISO9660 object in the given directory; this can be a directory or
   a file, it works fine for either one. Pass in:

   fn:      object filename (relative to the passed directory)
   dir:     0 if looking for a file, 1 if looking for a dir
   dir_extent:  directory extent to start with
   dir_size:    directory size (in bytes)

   It will return a pointer to a transient dirent buffer (i.e., don't
   expect this buffer to stay around much longer than the call itself).
 */
static iso_dirent_t *find_object(const char *fn, int dir,
                                 uint32 dir_extent, uint32 dir_size) {
    int     i, c;
    iso_dirent_t    *de;

    /* RockRidge */
    int     len;
    uint8       *pnt;
    char        rrname[NAME_MAX];
    int     rrnamelen;
    int     size_left;

    /* We need this to be signed for our while loop to end properly */
    size_left = (int)dir_size;

    /* Joliet */
    uint8       * ucsname = (uint8 *)rrname;

    /* If this is a Joliet CD, then UCSify the name */
    if(joliet)
        utf2ucs(ucsname, (uint8 *)fn);

    while(size_left > 0) {
        c = biread(dir_extent);

        if(c < 0) return NULL;

        for(i = 0; i < 2048 && i < size_left;) {
            /* Locate the current dirent */
            de = (iso_dirent_t *)(icache[c]->data + i);

            if(!de->length) break;

            /* Try the Joliet filename if the CD is a Joliet disc */
            if(joliet) {
                if(!ucscompare((uint8 *)de->name, ucsname, de->name_len)) {
                    if(!((dir << 1) ^ de->flags))
                        return de;
                }
            }
            else {
                /* Assume no Rock Ridge name */
                rrnamelen = 0;

                /* Check for Rock Ridge NM extension */
                len = de->length - sizeof(iso_dirent_t)
                      + sizeof(de->name) - de->name_len;
                pnt = (uint8*)de + sizeof(iso_dirent_t)
                      - sizeof(de->name) + de->name_len;

                if((de->name_len & 1) == 0) {
                    pnt++;
                    len--;
                }

                while((len >= 4) && ((pnt[3] == 1) || (pnt[3] == 2))) {
                    if(strncmp((char *)pnt, "NM", 2) == 0) {
                        rrnamelen = pnt[2] - 5;
                        strncpy(rrname, (char *)(pnt + 5), rrnamelen);
                        rrname[rrnamelen] = 0;
                    }

                    len -= pnt[2];
                    pnt += pnt[2];
                }

                /* Check the filename against the requested one */
                if(rrnamelen > 0) {
                    char *p = strchr(fn, '/');
                    int fnlen;

                    if(p)
                        fnlen = p - fn;
                    else
                        fnlen = strlen(fn);

                    if(!strncasecmp(rrname, fn, fnlen) && ! *(rrname + fnlen)) {
                        if(!((dir << 1) ^ de->flags))
                            return de;
                    }
                }
                else {
                    if(!fncompare(de->name, de->name_len, fn)) {
                        if(!((dir << 1) ^ de->flags))
                            return de;
                    }
                }
            }

            i += de->length;
        }

        dir_extent++;
        size_left -= 2048;
    }

    return NULL;
}

/* Locate an ISO9660 object anywhere on the disc, starting at the root,
   and expecting a fully qualified path name. This is analogous to find_object
   but it searches with the path in mind.

   fn:      object filename (relative to the passed directory)
   dir:     0 if looking for a file, 1 if looking for a dir
   dir_extent:  directory extent to start with
   dir_size:    directory size (in bytes)

   It will return a pointer to a transient dirent buffer (i.e., don't
   expect this buffer to stay around much longer than the call itself).
 */
static iso_dirent_t *find_object_path(const char *fn, int dir, iso_dirent_t *start) {
    char        *cur;

    /* If the object is in a sub-tree, traverse the trees looking
       for the right directory */
    while((cur = strchr(fn, '/'))) {
        if(cur != fn) {
            /* Note: trailing path parts don't matter since find_object
               only compares based on the FN length on the disc. */
            start = find_object(fn, 1, iso_733(start->extent), iso_733(start->size));

            if(start == NULL) return NULL;
        }

        fn = cur + 1;
    }

    /* Locate the file in the resulting directory */
    if(*fn) {
        start = find_object(fn, dir, iso_733(start->extent), iso_733(start->size));
        return start;
    }
    else {
        if(!dir)
            return NULL;
        else
            return start;
    }
}

/********************************************************************************/
/* File primitives */

/* File handles.. I could probably do this with a linked list, but I'm just
   too lazy right now. =) */
static struct {
    uint32      first_extent;   /* First sector */
    int     dir;        /* >0 if a directory */
    uint32      ptr;        /* Current read position in bytes */
    uint32      size;       /* Length of file in bytes */
    dirent_t    dirent;     /* A static dirent to pass back to clients */
    int     broken;     /* >0 if the CD has been swapped out since open */
} fh[FS_CD_MAX_FILES];

/* Mutex for file handles */
static mutex_t fh_mutex;

/* Break all of our open file descriptor. This is necessary when the disc
   is changed so that we don't accidentally try to keep on doing stuff
   with the old info. As files are closed and re-opened, the broken flag
   will be cleared. */
static void iso_break_all(void) {
    int i;

    mutex_lock(&fh_mutex);

    for(i = 0; i < FS_CD_MAX_FILES; i++)
        fh[i].broken = 1;

    mutex_unlock(&fh_mutex);
}

/* Open a file or directory */
static void * iso_open(vfs_handler_t * vfs, const char *fn, int mode) {
    file_t      fd;
    iso_dirent_t    *de;

    (void)vfs;

    /* Make sure they don't want to open things as writeable */
    if((mode & O_MODE_MASK) != O_RDONLY)
        return 0;

    /* Do this only when we need to (this is still imperfect) */
    if(!percd_done && init_percd() < 0)
        return 0;

    percd_done = 1;

    /* Find the file we want */
    de = find_object_path(fn, (mode & O_DIR) ? 1 : 0, &root_dirent);

    if(!de) return 0;

    /* Find a free file handle */
    mutex_lock(&fh_mutex);

    for(fd = 0; fd < FS_CD_MAX_FILES; fd++)
        if(fh[fd].first_extent == 0) {
            fh[fd].first_extent = -1;
            break;
        }

    mutex_unlock(&fh_mutex);

    if(fd >= FS_CD_MAX_FILES)
        return 0;

    /* Fill in the file handle and return the fd */
    fh[fd].first_extent = iso_733(de->extent);
    fh[fd].dir = (mode & O_DIR) ? 1 : 0;
    fh[fd].ptr = 0;
    fh[fd].size = iso_733(de->size);
    fh[fd].broken = 0;

    return (void *)fd;
}

/* Close a file or directory */
static int iso_close(void * h) {
    file_t fd = (file_t)h;

    /* Check that the fd is valid */
    if(fd < FS_CD_MAX_FILES) {
        /* No need to lock the mutex: this is an atomic op */
        fh[fd].first_extent = 0;
    }
    return 0;
}

/* Read from a file */
static ssize_t iso_read(void * h, void *buf, size_t bytes) {
    int rv, toread, thissect, c;
    uint8 * outbuf;
    file_t fd = (file_t)h;

    /* Check that the fd is valid */
    if(fd >= FS_CD_MAX_FILES || fh[fd].first_extent == 0 || fh[fd].broken)
        return -1;

    rv = 0;
    outbuf = (uint8 *)buf;

    /* Read zero or more sectors into the buffer from the current pos */
    while(bytes > 0) {
        /* Figure out how much we still need to read */
        toread = (bytes > (fh[fd].size - fh[fd].ptr)) ?
                 fh[fd].size - fh[fd].ptr : bytes;

        if(toread == 0) break;

        /* How much more can we read in the current sector? */
        thissect = 2048 - (fh[fd].ptr % 2048);

        /* If we're on a sector boundary and we have more than one
           full sector to read, then short-circuit the cache here
           and use the multi-sector reads from the CD unit (this
           should theoretically be a lot faster) */
        /* XXX This code isn't actually faster, but I'm leaving it
           here commented out in case we could find a better use
           for it later than speed (i.e., preventing thread context
           switches). */
        /* if(thissect == 2048 && toread >= 2048) {
            // Round it off to an even sector count
            thissect = toread / 2048;
            toread = thissect * 2048;

            printf("cdrom: short-circuit read for %d sectors\n",
                thissect);

            // Do the read
            if(cdrom_read_sectors(outbuf,
                fh[fd].first_extent + fh[fd].ptr/2048 + 150,
                thissect) < 0)
            {
                // Something went wrong...
                return -1;
            }
        } else { */
        toread = (toread > thissect) ? thissect : toread;

        /* Do the read */
        c = bdread(fh[fd].first_extent + fh[fd].ptr / 2048);

        if(c < 0) return -1;

        memcpy(outbuf, dcache[c]->data + (fh[fd].ptr % 2048), toread);
        /* } */

        /* Adjust pointers */
        outbuf += toread;
        fh[fd].ptr += toread;
        bytes -= toread;
        rv += toread;
    }

    return rv;
}

/* Seek elsewhere in a file */
static off_t iso_seek(void * h, off_t offset, int whence) {
    file_t fd = (file_t)h;

    /* Check that the fd is valid */
    if(fd >= FS_CD_MAX_FILES || fh[fd].first_extent == 0 || fh[fd].broken) {
        errno = EBADF;
        return -1;
    }

    /* Update current position according to arguments */
    switch(whence) {
        case SEEK_SET:
            if(offset < 0) {
                errno = EINVAL;
                return -1;
            }

            fh[fd].ptr = offset;
            break;

        case SEEK_CUR:
            if(offset < 0 && ((uint32)-offset) > fh[fd].ptr) {
                errno = EINVAL;
                return -1;
            }

            fh[fd].ptr += offset;
            break;

        case SEEK_END:
            if(offset < 0 && ((uint32)-offset) > fh[fd].size) {
                errno = EINVAL;
                return -1;
            }

            fh[fd].ptr = fh[fd].size + offset;
            break;

        default:
            errno = EINVAL;
            return -1;
    }

    /* Check bounds */
    if(fh[fd].ptr > fh[fd].size) fh[fd].ptr = fh[fd].size;

    return fh[fd].ptr;
}

/* Tell where in the file we are */
static off_t iso_tell(void * h) {
    file_t fd = (file_t)h;

    if(fd >= FS_CD_MAX_FILES || fh[fd].first_extent == 0 || fh[fd].broken)
        return -1;

    return fh[fd].ptr;
}

/* Tell how big the file is */
static size_t iso_total(void * h) {
    file_t fd = (file_t)h;

    if(fd >= FS_CD_MAX_FILES || fh[fd].first_extent == 0 || fh[fd].broken)
        return -1;

    return fh[fd].size;
}

/* Helper function for readdir: post-processes an ISO filename to make
   it a bit prettier. */
static void fn_postprocess(char *fnin) {
    char    * fn = fnin;

    while(*fn && *fn != ';') {
        *fn = tolower((int) * fn);
        fn++;
    }

    *fn = 0;

    /* Strip trailing dots */
    if(fn > fnin && fn[-1] == '.') {
        fn[-1] = 0;
    }
}

/* Read a directory entry */
static dirent_t *iso_readdir(void * h) {
    int     c;
    iso_dirent_t    *de;

    /* RockRidge */
    int     len;
    uint8       *pnt;

    file_t fd = (file_t)h;

    if(fd >= FS_CD_MAX_FILES || fh[fd].first_extent == 0 || !fh[fd].dir ||
       fh[fd].broken) {
        errno = EBADF;
        return NULL;
    }

    /* Scan forwards until we find the next valid entry, an
       end-of-entry mark, or run out of dir size. */
    c = -1;
    de = NULL;

    while(fh[fd].ptr < fh[fd].size) {
        /* Get the current dirent block */
        c = biread(fh[fd].first_extent + fh[fd].ptr / 2048);

        if(c < 0) return NULL;

        de = (iso_dirent_t *)(icache[c]->data + (fh[fd].ptr % 2048));

        if(de->length) break;

        /* Skip to the next sector */
        fh[fd].ptr += 2048 - (fh[fd].ptr % 2048);
    }

    if(fh[fd].ptr >= fh[fd].size) return NULL;

    /* If we're at the first, skip the two blank entries */
    if(!de->name[0] && de->name_len == 1) {
        fh[fd].ptr += de->length;
        de = (iso_dirent_t *)(icache[c]->data + (fh[fd].ptr % 2048));
        fh[fd].ptr += de->length;
        de = (iso_dirent_t *)(icache[c]->data + (fh[fd].ptr % 2048));

        if(!de->length) return NULL;
    }

    if(joliet) {
        ucs2utfn((uint8 *)fh[fd].dirent.name, (uint8 *)de->name, de->name_len);
    }
    else {
        /* Fill out the VFS dirent */
        strncpy(fh[fd].dirent.name, de->name, de->name_len);
        fh[fd].dirent.name[de->name_len] = 0;
        fn_postprocess(fh[fd].dirent.name);

        /* Check for Rock Ridge NM extension */
        len = de->length - sizeof(iso_dirent_t) + sizeof(de->name) - de->name_len;
        pnt = (uint8*)de + sizeof(iso_dirent_t) - sizeof(de->name) + de->name_len;

        if((de->name_len & 1) == 0) {
            pnt++;
            len--;
        }

        while((len >= 4) && ((pnt[3] == 1) || (pnt[3] == 2))) {
            if(strncmp((char *)pnt, "NM", 2) == 0) {
                strncpy(fh[fd].dirent.name, (char *)(pnt + 5), pnt[2] - 5);
                fh[fd].dirent.name[pnt[2] - 5] = 0;
            }

            len -= pnt[2];
            pnt += pnt[2];
        }
    }

    if(de->flags & 2) {
        fh[fd].dirent.size = -1;
        fh[fd].dirent.attr = O_DIR;
    }
    else {
        fh[fd].dirent.size = iso_733(de->size);
        fh[fd].dirent.attr = 0;
    }

    fh[fd].ptr += de->length;

    return &fh[fd].dirent;
}

static int iso_rewinddir(void * h) {
    file_t fd = (file_t)h;

    if(fd >= FS_CD_MAX_FILES || fh[fd].first_extent == 0 || !fh[fd].dir ||
       fh[fd].broken) {
        errno = EBADF;
        return -1;
    }

    /* Rewind to the beginning of the directory. */
    fh[fd].ptr = 0;
    return 0;
}

int iso_reset(void) {
    iso_break_all();
    bclear();
    percd_done = 0;
    return 0;
}

/* This handler will be called during every vblank. We have to
   be careful about modifying variables that are in use in the
   foreground, so instead we'll just set a "dead" flag and next
   time someone calls in it'll get reset. */
static int iso_last_status;
static int iso_vblank_hnd;
static void iso_vblank(uint32 evt) {
    int status;

    (void)evt;

    /* Get the status. This may fail if a CD operation is in
       progress in the foreground. */
    if(cdrom_get_status(&status, &disc_type) < 0)
        return;

    if(iso_last_status != status) {
        if(status == CD_STATUS_OPEN || status == CD_STATUS_NO_DISC)
            percd_done = 0;

        iso_last_status = status;
    }
}

static int iso_fcntl(void *h, int cmd, va_list ap) {
    file_t fd = (file_t)h;
    int rv = -1;

    (void)ap;

    if(fd >= FS_CD_MAX_FILES || !fh[fd].first_extent || fh[fd].broken) {
        errno = EBADF;
        return -1;
    }

    switch(cmd) {
        case F_GETFL:
            rv = O_RDONLY;

            if(fh[fd].dir)
                rv |= O_DIR;

            break;

        case F_SETFL:
        case F_GETFD:
        case F_SETFD:
            rv = 0;
            break;

        default:
            errno = EINVAL;
    }

    return rv;
}

static int iso_fstat(void *h, struct stat *st) {
    file_t fd = (file_t)h;

    if(fd >= FS_CD_MAX_FILES || !fh[fd].first_extent || fh[fd].broken) {
        errno = EBADF;
        return -1;
    }

    memset(st, 0, sizeof(struct stat));

    if(fh[fd].dir) {
        st->st_size = 0;
        st->st_dev = 'c' | ('d' << 8);
        st->st_mode = S_IFDIR | S_IRUSR | S_IRGRP | S_IROTH | S_IXUSR |
            S_IXGRP | S_IXOTH;
        st->st_nlink = 1;
        st->st_blksize = 512;
    }
    else {
        st->st_size = fh[fd].size;
        st->st_dev = 'c' | ('d' << 8);
        st->st_mode = S_IFREG | S_IRUSR | S_IRGRP | S_IROTH | S_IXUSR |
            S_IXGRP | S_IXOTH;
        st->st_nlink = 1;
        st->st_blksize = 512;
    }

    return 0;
}

/* Put everything together */
static vfs_handler_t vh = {
    /* Name handler */
    {
        "/cd",          /* name */
        0,              /* tbfi */
        0x00010000,     /* Version 1.0 */
        0,              /* flags */
        NMMGR_TYPE_VFS, /* VFS handler */
        NMMGR_LIST_INIT
    },

    0, NULL,            /* no cacheing, privdata */

    iso_open,
    iso_close,
    iso_read,
    NULL,
    iso_seek,
    iso_tell,
    iso_total,
    iso_readdir,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    iso_fcntl,
    NULL,               /* poll */
    NULL,               /* link */
    NULL,               /* symlink */
    NULL,               /* seek64 */
    NULL,               /* tell64 */
    NULL,               /* total64 */
    NULL,               /* readlink */
    iso_rewinddir,
    iso_fstat
};

/* Initialize the file system */
int fs_iso9660_init(void) {
    int i;

    /* Reset fd's */
    memset(fh, 0, sizeof(fh));

    /* Mark the first as active so we can have an error FD of zero */
    fh[0].first_extent = -1;

    /* Init thread mutexes */
    mutex_init(&cache_mutex, MUTEX_TYPE_NORMAL);
    mutex_init(&fh_mutex, MUTEX_TYPE_NORMAL);

    /* Allocate cache block space */
    for(i = 0; i < NUM_CACHE_BLOCKS; i++) {
        icache[i] = malloc(sizeof(cache_block_t));
        icache[i]->sector = -1;
        dcache[i] = malloc(sizeof(cache_block_t));
        dcache[i]->sector = -1;
    }

    percd_done = 0;
    iso_last_status = -1;

    /* Register with the vblank */
    iso_vblank_hnd = vblank_handler_add(iso_vblank);

    /* Register with VFS */
    return nmmgr_handler_add(&vh.nmmgr);
}

/* De-init the file system */
int fs_iso9660_shutdown(void) {
    int i;

    /* De-register with vblank */
    vblank_handler_remove(iso_vblank_hnd);

    /* Dealloc cache block space */
    for(i = 0; i < NUM_CACHE_BLOCKS; i++) {
        free(icache[i]);
        free(dcache[i]);
    }

    /* Free muteces */
    mutex_destroy(&cache_mutex);
    mutex_destroy(&fh_mutex);

    return nmmgr_handler_remove(&vh.nmmgr);
}
