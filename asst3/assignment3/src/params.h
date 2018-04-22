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

/***** struct stat *****/
 //dev_t     st_dev;         /* ID of device containing file, ignored */
 //ino_t     st_ino;         /* inode number */
 //mode_t    st_mode;        /* protection */
 //nlink_t   st_nlink;       /* number of hard links */
 //uid_t     st_uid;         /* user ID of owner */
 //gid_t     st_gid;         /* group ID of owner */
 //dev_t     st_rdev;        /* device ID (if special file), ignored */
 //off_t     st_size;        /* total size, in bytes */
 //blksize_t st_blksize;     /* blocksize for filesystem I/O, ignored */
 //blkcnt_t  st_blocks;      /* number of 512B blocks allocated */

/***** MACROS *****/
#define FILE_NAME_LIM 255


/*
 * linked list of disk blocks for file
 */
typedef struct block_ptr {
	char* block;	// pointer to current block
	char* next;	// pointer to next block
} block_ptr_t;

/*
 * virtual inode structure
 * holds all file metadata
 * represents single entry in
 * inode table
 */
typedef struct inode {
 	ino_t     st_ino;         /* inode number */
 	mode_t    st_mode;        /* protection */
 	nlink_t   st_nlink;       /* number of hard links */
	uid_t     st_uid;         /* user ID of owner */
	gid_t     st_gid;         /* group ID of owner */
 	off_t     st_size;        /* total size, in bytes */
 	blkcnt_t  st_blocks;      /* number of 512B blocks allocated */
	struct block_ptr* data;   /* pointer to list of data blocks */
} inode_t;

/* 
 * file name to v_node mapping
 * stored by directories
 */
typedef struct link {
	char name[FILE_NAME_LIM];	/* file name */
	ino_t st_ino;			/* inode number */
} link_t;

/*
 * metadata for an open file
 * represents single entry 
 * in open file table
 */
typedef struct open_file {
	ino_t st_ino;   /* inode number */
	char* pos;	// current position in the file
	int refcnt;	// number of open file descriptors that point to this file
} open_file_t;

struct sfs_state {
    FILE *logfile;
    char *diskfile;
};
#define SFS_DATA ((struct sfs_state *) fuse_get_context()->private_data)
#endif
