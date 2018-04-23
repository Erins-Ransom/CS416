/*
  Copyright (C) 2012 Joseph J. Pfeiffer, Jr., Ph.D. <pfeiffer@cs.nmsu.edu>

  This program can be distributed under the terms of the GNU GPLv3.
  See the file COPYING.

  There are a couple of symbols that need to be #defined before
  #including all the headers.
*/

#ifndef _PARAMS_H_
#define _PARAMS_H_

// The FUSE API has been changed a number of times.  So, our code
// needs to define the version of the API that we assume.  As of this
// writing, the most current API version is 26
#define FUSE_USE_VERSION 26

// need this to get pwrite().  I have to use setvbuf() instead of
// setlinebuf() later in consequence.
#define _XOPEN_SOURCE 500

// maintain bbfs state in here
#include <limits.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>

/***** struct stat *****/
 //dev_t     st_dev;         /* ID of device containing file, ignored */
 //ino_t     st_ino;         /* inode number, ignored */
 //mode_t    st_mode;        /* protection */
 //nlink_t   st_nlink;       /* number of hard links */
 //uid_t     st_uid;         /* user ID of owner */
 //gid_t     st_gid;         /* group ID of owner */
 //dev_t     st_rdev;        /* device ID (if special file), ignored */
 //off_t     st_size;        /* total size, in bytes */
 //blksize_t st_blksize;     /* blocksize for filesystem I/O, ignored */
 //blkcnt_t  st_blocks;      /* number of 512B blocks allocated */

/***** MACROS *****/
#define MAX_FILES 128
#define MAX_PATH_LEN 255

/*
 * mapping of file path to
 * inode number
 */
typedef struct path_map {
	int st_ino;
	char path[MAX_PATH_LEN];
} path_map_t;

/*
 * linked list of disk blocks for file
 */
typedef struct block_list {
	int block_num;		// number of current block
	struct block_ptr *next;	// pointer to next block
} block_list_t;

/*
 * virtual inode structure
 * holds all file metadata
 * represents single entry in
 * inode table
 */
typedef struct inode {
	struct stat stat;	// metadata
	block_list_t *data;	// pointer to list of disk blocks
} inode_t;

struct sfs_state {
    FILE *logfile;
    char *diskfile;
};

#define SFS_DATA ((struct sfs_state *) fuse_get_context()->private_data)
#endif
