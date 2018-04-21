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

/*
 * inode structure containing metadata
 * about file stored on disk
 */
typedef struct inode {
	int       st_ino;     /* inode number */
	uid_t     st_uid;     /* user ID of owner */
        gid_t     st_gid;     /* group ID of owner */
	off_t     st_size;    /* total size, in bytes */
        blksize_t st_blksize; /* blocksize for file system I/O */
	char*     data;	      /* pointer to actual data */
} inode_t;

/*
 * representation of a filename to
 * inode mapping 
 */
typedef struct hard_link {
	char* filename;
	int st_ino;			// inode number
	struct hard_link* next; 	// next entry in a list of hard links
} hard_link_t;

/*
 * directory represented as a 
 * list of hard links
 */
typedef struct directory {
	int num_links;			// number of hard links
	hard_link_t* front;		// first entry in list of links		
} dir;

struct sfs_state {
    FILE *logfile;
    char *diskfile;
};
#define SFS_DATA ((struct sfs_state *) fuse_get_context()->private_data)

