/*
  Simple File System

  This code is derived from function prototypes found /usr/include/fuse/fuse.h
  Copyright (C) 2001-2007  Miklos Szeredi <miklos@szeredi.hu>
  His code is licensed under the LGPLv2.

*/

#include "params.h"

#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <fuse.h>
#include <libgen.h>
#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <time.h>

#ifdef HAVE_SYS_XATTR_H
#include <sys/xattr.h>
#endif

#include "log.h"
#include "block.h"

///////////////////////////////////////////////////////////
//
// Prototypes for all these functions, and the C-style comments,
// come indirectly from /usr/include/fuse.h
//

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
#define MAX_NODES 128
#define MAX_LINK 128
#define NUM_BLOCKS 2048
#define ALLOCD 1
#define FREE 0

/***** GLOBALS *****/	// all these need to be kept on disk
inode_t 	inode_table[MAX_NODES];
path_map_t	name_table[MAX_LINK];
short		disk_blocks[NUM_BLOCKS];

int get_inode() {
	int i, j;
	for(i = 0; i < MAX_NODES; i++) {
		if(inode_table[i].avail == FREE) {
			inode_table[i].avail = ALLOCD;
			for(j = 0; j < MAX_FILE_BLOCKS; j++) {
				inode_table[i].blocks[j] = -1;
			}
			return i;
		}
	}	
	return -1;
}

void free_inode(int st_ino) {
	inode_table[st_ino].stat.st_nlink--;
	if(inode_table[st_ino].stat.st_nlink > 0) {
		return;
	}

	memset( &(inode_table[st_ino].stat), 0, sizeof(struct stat) );
	int i;
	for(i = 0; i < MAX_FILE_BLOCKS; i++) {
                inode_table[st_ino].blocks[i] = -1;
        }

	inode_table[st_ino].avail = FREE;
	return;
}

int get_mapping() {
	int i;
        for(i = 0; i < MAX_LINK; i++) {
                if(name_table[i].avail == FREE) {
                        name_table[i].avail = ALLOCD;
                        return i;
                }
        }       
        return -1;	
}

void free_mapping(int st_ino) {
	memset( &(inode_table[st_ino]), 0, sizeof(inode_t) );
        inode_table[st_ino].avail = FREE;
        return;
}

int get_block() {
	int i;
	for(i = 0; i < NUM_BLOCKS; i++) {
		if(disk_blocks[i] == FREE) {
			disk_blocks[i] = ALLOCD;
			return i;
		}
	}
	return -1;
}

void free_block(int i) {
	disk_blocks[i] = FREE;
	return;
}

int path_lookup(const char *path) {
	int i;
	for(i = 0; i < MAX_LINK; i++) {
		if( strcmp(name_table[i].path, path) == 0 ) {
			return i;
		}
	}
	return -1;
}

void removeSubstring(char *s,const char *toremove) {
  	while( s = strstr(s, toremove) ) {
    		memmove( s, s+strlen(toremove), 1+strlen(s+strlen(toremove)) );
	}
	return;
}

/**
 * Initialize filesystem
 *
 * The return value will passed in the private_data field of
 * fuse_context to all file operations and as a parameter to the
 * destroy() method.
 *
 * Introduced in version 2.3
 * Changed in version 2.6
 */
void *sfs_init(struct fuse_conn_info *conn)
{
	fprintf(stderr, "in bb-init\n");
    	log_msg("\nsfs_init()\n");
    	log_conn(conn);
    	log_fuse_context(fuse_get_context());

	/* set diskfile based on sfs_state */
	disk_open((SFS_DATA)->diskfile);

	/* zero out all tables (add check for pre existing file system) */
	memset(inode_table, 0, sizeof(inode_t)*MAX_NODES);
	memset(name_table,  0, sizeof(path_map_t)*MAX_LINK);
	memset(disk_blocks, FREE, sizeof(short)*NUM_BLOCKS);

	// how to we check for a pre-existing file system?


	/* enter root directory into inode table */
	int root_inode = get_inode();
	inode_table[root_inode].stat.st_mode = S_IFDIR | S_IRWXU | S_IRWXG | S_IRWXO;
        inode_table[root_inode].stat.st_nlink = 2;
        inode_table[root_inode].stat.st_uid = getuid();
        inode_table[root_inode].stat.st_gid = getgid();
        inode_table[root_inode].stat.st_size = BLOCK_SIZE;
        inode_table[root_inode].stat.st_blocks = 1;
	inode_table[root_inode].stat.st_atime = time(NULL);
        inode_table[root_inode].stat.st_mtime = time(NULL);
        inode_table[root_inode].stat.st_ctime = time(NULL);
	
	int i;
	for(i = 0; i < MAX_FILE_BLOCKS; i++) {
		inode_table[root_inode].blocks[i] = -1;
	}

	/* enter root directory into name table */
	int root_mapping = get_mapping();
	sprintf(name_table[root_mapping].path, "/");
	name_table[root_mapping].st_ino = root_inode;

	/* allocate disk block for root */
	int root_block = get_block();
	inode_table[root_inode].blocks[0] = root_block;

	/* 
 	 * add . and .. as entries to root the syntax for directory data will
 	 * be <file name>/<inode #>/<other file name>/<other inode #>/...
 	 */
	void *buf = malloc(BLOCK_SIZE);
	memset(buf, 0, BLOCK_SIZE);
	block_write(0, buf);
	
	/* hard coded file */
	//block_write(0, "./0/../0/foo.txt/2");
	//inode_table[2].stat.st_mode = S_IFREG | S_IRWXU | S_IRWXG | S_IRWXO;

	block_write(0, "./0/../0");

    	return SFS_DATA;
}

/**
 * Clean up filesystem
 *
 * Called on filesystem exit.
 *
 * Introduced in version 2.3
 */
void sfs_destroy(void *userdata)
{	
	/* must write all tables to persistent memory */

	disk_close();
	log_msg("\nsfs_destroy(userdata=0x%08x)\n", userdata);
}

/** Get file attributes.
 *
 * Similar to stat().  The 'st_dev' and 'st_blksize' fields are
 * ignored.  The 'st_ino' field is ignored except if the 'use_ino'
 * mount option is given.
 */
int sfs_getattr(const char *path, struct stat *statbuf)
{
	int retstat = 0;
    	char fpath[PATH_MAX];
    	log_msg("\nsfs_getattr(path=\"%s\", statbuf=0x%08x)\n", path, statbuf);
	
	int mapping = path_lookup(path);
	
	if(mapping < 0) {	// path not found
		memset(statbuf, 0, sizeof(struct stat));
		statbuf->st_mode = S_IFREG | S_IRWXU | S_IRWXG | S_IRWXO;
		statbuf->st_nlink = 1;
		statbuf->st_uid = getuid();
		statbuf->st_gid = getgid();
		statbuf->st_size = 0;
		statbuf->st_blocks = 0;
		statbuf->st_atime = time(NULL);
		statbuf->st_mtime = time(NULL);
		statbuf->st_ctime = time(NULL);

		return -ENOENT;
	}
	
	
	memcpy( statbuf, &(inode_table[mapping].stat), sizeof(struct stat) );

    	return retstat;
}

/**
 * Create and open a file
 *
 * If the file does not exist, first create it with the specified
 * mode, and then open it.
 *
 * If this method is not implemented or under Linux kernel
 * versions earlier than 2.6.15, the mknod() and open() methods
 * will be called instead.
 *
 * Introduced in version 2.5
 */
int sfs_create(const char *path, mode_t mode, struct fuse_file_info *fi)
{
	int retstat = 0;
   	log_msg("\nsfs_create(path=\"%s\", mode=0%03o, fi=0x%08x)\n", path, mode, fi);

	/* set file handle */
	//memset(fi, 0, sizeof(struct fuse_file_info));
	fi->fh = get_inode();
	if(fi->fh < 0) {
		return -ENOMEM;
	}


	/* enter file  into inode table */
        inode_table[fi->fh].stat.st_mode = mode;
        inode_table[fi->fh].stat.st_nlink = 1;
        inode_table[fi->fh].stat.st_uid = getuid();
        inode_table[fi->fh].stat.st_gid = getgid();
        inode_table[fi->fh].stat.st_size = 0;
        inode_table[fi->fh].stat.st_blocks = 0;
	inode_table[fi->fh].stat.st_atime = time(NULL);
	inode_table[fi->fh].stat.st_mtime = time(NULL);
	inode_table[fi->fh].stat.st_ctime = time(NULL);

        /* enter file into name table */
	int mapping = get_mapping();        
	if(mapping < 0) {
		free_inode(fi->fh);
		return -ENOMEM;
	}
	sprintf(name_table[mapping].path, path);
        name_table[mapping].st_ino = fi->fh;

        /* allocate disk block for file */
        //disk_blocks[0] = ALLOCD;
        //inode_table[0].blocks[0] = 0;

	/* enter file into root directory  */
	void *buf = malloc(BLOCK_SIZE);
	memset(buf, 0, BLOCK_SIZE);
	block_read(inode_table[0].blocks[0], buf);
	char *ptr = (char*)buf;	
	while(*ptr != '\0') {	
		ptr++;
	}
	sprintf(ptr, "%s/%i", path, fi->fh);
	block_write(inode_table[0].blocks[0], buf);
	inode_table[0].stat.st_nlink++;

    	return retstat;
}

/** Remove a file */
int sfs_unlink(const char *path)
{
	int retstat = 0;
    	log_msg("sfs_unlink(path=\"%s\")\n", path);

	/* path lookup  */
	int mapping = path_lookup(path);
	if(mapping < 0) {
		return -ENOENT;
	}	

	/* update inode table */
	free_inode(name_table[mapping].st_ino);

	/* update name table  */
	free_mapping(mapping);

	/* update root directory */
	char *entry = malloc(MAX_PATH_LEN);
	memset(entry, 0, MAX_PATH_LEN);
	sprintf(entry, "%s/%i", path, name_table[mapping].st_ino);

	void *buf = malloc(BLOCK_SIZE);
	memset(buf, 0, BLOCK_SIZE);
	block_read(inode_table[0].blocks[0], buf);	

	removeSubstring((char*)buf, entry);

	block_write(inode_table[0].blocks[0], buf);
	inode_table[0].stat.st_nlink--;
 
    	return retstat;
}

/** File open operation
 *
 * No creation, or truncation flags (O_CREAT, O_EXCL, O_TRUNC)
 * will be passed to open().  Open should check if the operation
 * is permitted for the given flags.  Optionally open may also
 * return an arbitrary filehandle in the fuse_file_info structure,
 * which will be passed to all file operations.
 *
 * Changed in version 2.2
 */
int sfs_open(const char *path, struct fuse_file_info *fi)
{
    int retstat = 0;
    log_msg("\nsfs_open(path\"%s\", fi=0x%08x)\n",
	    path, fi);

    
    return retstat;
}

/** Release an open file
 *
 * Release is called when there are no more references to an open
 * file: all file descriptors are closed and all memory mappings
 * are unmapped.
 *
 * For every open() call there will be exactly one release() call
 * with the same flags and file descriptor.  It is possible to
 * have a file opened more than once, in which case only the last
 * release will mean, that no more reads/writes will happen on the
 * file.  The return value of release is ignored.
 *
 * Changed in version 2.2
 */
int sfs_release(const char *path, struct fuse_file_info *fi)
{
    int retstat = 0;
    log_msg("\nsfs_release(path=\"%s\", fi=0x%08x)\n",
	  path, fi);
    

    return retstat;
}

/** Read data from an open file
 *
 * Read should return exactly the number of bytes requested except
 * on EOF or error, otherwise the rest of the data will be
 * substituted with zeroes.  An exception to this is when the
 * 'direct_io' mount option is specified, in which case the return
 * value of the read system call will reflect the return value of
 * this operation.
 *
 * Changed in version 2.2
 */
int sfs_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
{
    int retstat = 0;
    log_msg("\nsfs_read(path=\"%s\", buf=0x%08x, size=%d, offset=%lld, fi=0x%08x)\n",
	    path, buf, size, offset, fi);

   
    return retstat;
}

/** Write data to an open file
 *
 * Write should return exactly the number of bytes requested
 * except on error.  An exception to this is when the 'direct_io'
 * mount option is specified (see read operation).
 *
 * Changed in version 2.2
 */
int sfs_write(const char *path, const char *buf, size_t size, off_t offset,
	     struct fuse_file_info *fi)
{
    int retstat = 0;
    log_msg("\nsfs_write(path=\"%s\", buf=0x%08x, size=%d, offset=%lld, fi=0x%08x)\n",
	    path, buf, size, offset, fi);
    
    
    return retstat;
}


/** Create a directory */
int sfs_mkdir(const char *path, mode_t mode)
{
    int retstat = 0;
    log_msg("\nsfs_mkdir(path=\"%s\", mode=0%3o)\n",
	    path, mode);
   
    
    return retstat;
}


/** Remove a directory */
int sfs_rmdir(const char *path)
{
    int retstat = 0;
    log_msg("sfs_rmdir(path=\"%s\")\n", path);
    
    
    return retstat;
}


/** Open directory
 *
 * This method should check if the open operation is permitted for
 * this  directory
 *
 * Introduced in version 2.3
 */
int sfs_opendir(const char *path, struct fuse_file_info *fi)
{
    int retstat = 0;
    log_msg("\nsfs_opendir(path=\"%s\", fi=0x%08x)\n", path, fi);
    
    
    return retstat;
}

/** Read directory
 *
 * This supersedes the old getdir() interface.  New applications
 * should use this.
 *
 * The filesystem may choose between two modes of operation:
 *
 * 1) The readdir implementation ignores the offset parameter, and
 * passes zero to the filler function's offset.  The filler
 * function will not return '1' (unless an error happens), so the
 * whole directory is read in a single readdir operation.  This
 * works just like the old getdir() method.
 *
 * 2) The readdir implementation keeps track of the offsets of the
 * directory entries.  It uses the offset parameter and always
 * passes non-zero offset to the filler function.  When the buffer
 * is full (or an error happens) the filler function will return
 * '1'.
 *
 * Introduced in version 2.3
 */
int sfs_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi)
{
    	int retstat = 0, st_ino = -1;

	if( strcmp(path, "/") == 0 ) {
		st_ino = 0;
	} else {
		st_ino = fi->fh;	// make sure this is set in another funtion
	}

	char *entry_list = malloc(BLOCK_SIZE), *tok;
	block_read(inode_table[st_ino].blocks[0] , entry_list);
	
	tok = strtok(entry_list, "/");

	while(tok != NULL) {
		filler( buf, tok, NULL, 0 );
		strtok(NULL, "/");
		tok = strtok(NULL, "/");
	}

	free(entry_list);
	
    	return retstat;
}

/** Release directory
 *
 * Introduced in version 2.3
 */
int sfs_releasedir(const char *path, struct fuse_file_info *fi)
{
    int retstat = 0;

    
    return retstat;
}

struct fuse_operations sfs_oper = {
  .init = sfs_init,
  .destroy = sfs_destroy,

  .getattr = sfs_getattr,
  .create = sfs_create,
  .unlink = sfs_unlink,
  .open = sfs_open,
  .release = sfs_release,
  .read = sfs_read,
  .write = sfs_write,

  .rmdir = sfs_rmdir,
  .mkdir = sfs_mkdir,

  .opendir = sfs_opendir,
  .readdir = sfs_readdir,
  .releasedir = sfs_releasedir
};

void sfs_usage()
{
    fprintf(stderr, "usage:  sfs [FUSE and mount options] diskFile mountPoint\n");
    abort();
}

int main(int argc, char *argv[])
{
    int fuse_stat;
    struct sfs_state *sfs_data;
    
    // sanity checking on the command line
    if ((argc < 3) || (argv[argc-2][0] == '-') || (argv[argc-1][0] == '-'))
	sfs_usage();

    sfs_data = malloc(sizeof(struct sfs_state));
    if (sfs_data == NULL) {
	perror("main calloc");
	abort();
    }

    // Pull the diskfile and save it in internal data
    sfs_data->diskfile = argv[argc-2];
    argv[argc-2] = argv[argc-1];
    argv[argc-1] = NULL;
    argc--;
    
    sfs_data->logfile = log_open();
    
    // turn over control to fuse
    fprintf(stderr, "about to call fuse_main, %s \n", sfs_data->diskfile);
    fuse_stat = fuse_main(argc, argv, &sfs_oper, sfs_data);
    fprintf(stderr, "fuse_main returned %d\n", fuse_stat);
    
    return fuse_stat;
}
