/*
 * Copyright (C) 2007 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
 /*
 *ActionsCode(author:liaotianyang, type:new_code)
 */
#define _LARGEFILE_SOURCE
#define _LARGEFILE64_SOURCE
#define _FILE_OFFSET_BITS 64
#include <fcntl.h>

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <dirent.h>
#include <limits.h>

#include "DirUtil.h"

typedef enum { DMISSING, DDIR, DILLEGAL } DirStatus;

static DirStatus
getPathDirStatus(const char *path)
{
    struct stat st;
    int err;

    err = stat(path, &st);
    if (err == 0) {
        /* Something's there; make sure it's a directory.
         */
        if (S_ISDIR(st.st_mode)) {
            return DDIR;
        }
        errno = ENOTDIR;
        return DILLEGAL;
    } else if (errno != ENOENT) {
        /* Something went wrong, or something in the path
         * is bad.  Can't do anything in this situation.
         */
        return DILLEGAL;
    }
    return DMISSING;
}

int
dirCreateHierarchy(const char *path, int mode,
        const struct utimbuf *timestamp, bool stripFileName,
        struct selabel_handle *sehnd)
{
    DirStatus ds;

    /* Check for an empty string before we bother
     * making any syscalls.
     */
    if (path[0] == '\0') {
        errno = ENOENT;
        return -1;
    }

    /* Allocate a path that we can modify; stick a slash on
     * the end to make things easier.
     */
    size_t pathLen = strlen(path);
    char *cpath = (char *)malloc(pathLen + 2);
    if (cpath == NULL) {
        errno = ENOMEM;
        return -1;
    }
    memcpy(cpath, path, pathLen);
    if (stripFileName) {
        /* Strip everything after the last slash.
         */
        char *c = cpath + pathLen - 1;
        while (c != cpath && *c != '/') {
            c--;
        }
        if (c == cpath) {
//xxx test this path
            /* No directory component.  Act like the path was empty.
             */
            errno = ENOENT;
            free(cpath);
            return -1;
        }
        c[1] = '\0';    // Terminate after the slash we found.
    } else {
        /* Make sure that the path ends in a slash.
         */
        cpath[pathLen] = '/';
        cpath[pathLen + 1] = '\0';
    }

    /* See if it already exists.
     */
    ds = getPathDirStatus(cpath);
    if (ds == DDIR) {
        return 0;
    } else if (ds == DILLEGAL) {
        return -1;
    }

    /* Walk up the path from the root and make each level.
     * If a directory already exists, no big deal.
     */
    char *p = cpath;
    while (*p != '\0') {
        /* Skip any slashes, watching out for the end of the string.
         */
        while (*p != '\0' && *p == '/') {
            p++;
        }
        if (*p == '\0') {
            break;
        }

        /* Find the end of the next path component.
         * We know that we'll see a slash before the NUL,
         * because we added it, above.
         */
        while (*p != '/') {
            p++;
        }
        *p = '\0';

        /* Check this part of the path and make a new directory
         * if necessary.
         */
        ds = getPathDirStatus(cpath);
        if (ds == DILLEGAL) {
            /* Could happen if some other process/thread is
             * messing with the filesystem.
             */
            free(cpath);
            return -1;
        } else if (ds == DMISSING) {
            int err;

            char *secontext = NULL;

            if (sehnd) {
                selabel_lookup(sehnd, &secontext, cpath, mode);
                setfscreatecon(secontext);
            }

            err = mkdir(cpath, mode);

            if (secontext) {
                freecon(secontext);
                setfscreatecon(NULL);
            }

            if (err != 0) {
                free(cpath);
                return -1;
            }
            if (timestamp != NULL && utime(cpath, timestamp)) {
                free(cpath);
                return -1;
            }
        }
        // else, this directory already exists.
        
        /* Repair the path and continue.
         */
        *p = '/';
    }
    free(cpath);

    return 0;
}

int
dirUnlinkHierarchy(const char *path)
{
    struct stat st;
    DIR *dir;
    struct dirent *de;
    int fail = 0;

    /* is it a file or directory? */
    if (lstat(path, &st) < 0) {
        return -1;
    }

    /* a file, so unlink it */
    if (!S_ISDIR(st.st_mode)) {
        return unlink(path);
    }

    /* a directory, so open handle */
    dir = opendir(path);
    if (dir == NULL) {
        return -1;
    }

    /* recurse over components */
    errno = 0;
    while ((de = readdir(dir)) != NULL) {
//TODO: don't blow the stack
        char dn[PATH_MAX];
        if (!strcmp(de->d_name, "..") || !strcmp(de->d_name, ".")) {
            continue;
        }
        snprintf(dn, sizeof(dn), "%s/%s", path, de->d_name);
        if (dirUnlinkHierarchy(dn) < 0) {
            fail = 1;
            break;
        }
        errno = 0;
    }
    /* in case readdir or unlink_recursive failed */
    if (fail || errno < 0) {
        int save = errno;
        closedir(dir);
        errno = save;
        return -1;
    }

    /* close directory handle */
    if (closedir(dir) < 0) {
        return -1;
    }

    /* delete target directory */
    return rmdir(path);
}
/*
 *support large file cpy
 *ActionsCode(author:liaotianyang, type:new_method)
 */
 int file_cpy(const char *dstfile , const char *srcflie)
{
    char buffer [4096];
    int rc , ret;
    int fd1, fd2 ;
    off64_t len, flen;
	struct stat s;
	
	//printf("1.cpy file %s to %s ....\n", srcflie, dstfile);
    if ((fd1 = open(srcflie, O_RDONLY)) < 0) { 
		printf("open =%s fail \n", srcflie);
		return -1;
    }
    if ((fd2 = creat(dstfile, 755)) < 0) {
		close(fd1);
		printf("creat =%s fail \n", dstfile);
		return -1;
    }
    flen = lseek64(fd1, 0, SEEK_END);
    lseek64(fd1, 0, SEEK_SET);
    len = 0;
    do{
		rc = read(fd1, buffer, 4096);
		if (rc < 0) {
			printf("file_cpy: failed to read %s\n", strerror(errno));   
			break;	
		}	
		ret = write(fd2, buffer, rc); 
		if ( ret != rc ) {
			printf("file_cpy: failed to write %s\n", strerror(errno)); 
			rc = -1;
		}
		len += rc;
		if ( flen == len )
			break;
    } while ( rc == 4096 );

	fstat(fd1,&s);
	chown(dstfile,s.st_uid,s.st_gid);
	chmod(dstfile,s.st_mode);
		    
    close(fd1);
    close(fd2);
        
    if ( rc >= 0 && flen == len) {   
		return 0;
	} 
	printf("cpy %s to %s fail,flen=%llu, wlen=%llu \n",srcflie, dstfile, flen, len);
    return -1;   
}
/*
 *busybox_cp
 *ActionsCode(author:liaotianyang, type:new_method)
 */
#include <sys/wait.h>
// /system/bin/busybox cp -a srcdir  dstdir
static int busybox_cp(const char *srcdir, const char *dstdir)
{   
    const char** args = (const char**)malloc(sizeof(char*) * 6);
    args[0] = "/sbin/busybox";
    args[1] = "cp";
    args[2] = "-a";
    args[3] = srcdir;
    args[4] = dstdir;
    args[5] = NULL; 

    printf("busybox cp: src=%s to dst=%s\n", srcdir, dstdir);
    pid_t pid = fork();
    if (pid == 0) {
        execv(args[0], (char* const*)args);
        fprintf(stdout, "Can't run %s (%s)\n", args[0], strerror(errno));
        _exit(-1);
    }
    int status;
    waitpid(pid, &status, 0);
    if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
        printf("busybox cp err (Status %d)\n",  WEXITSTATUS(status));
        return -1;
    }  
    printf("busybox cp OK\n");  
    return 0;
}
/*
 *copy dir
 *ActionsCode(author:liaotianyang, type:new_method)
 */
static int dir_copy(const char *spathname,const char *tpathname)
{
	struct stat s;
	struct dirent *s_p;
	DIR *dirs;
	int ret = 0;
	
	if (lstat(spathname, &s) < 0) {
		printf("d_copy:lstat %s fail\n", spathname);
	    return -1;
	}
    /* a file, so copy it */
    if (!S_ISDIR(s.st_mode)) {
        return busybox_cp(spathname, tpathname);
    }

	dirs = opendir(spathname);

	while((s_p = readdir(dirs))!= NULL ) {
		
		char sn[PATH_MAX];
		if (!strcmp(s_p->d_name, "..") || !strcmp(s_p->d_name, ".")) {
            continue;
        }   	
		snprintf(sn, sizeof(sn), "%s/%s", spathname, s_p->d_name);
		
        if ( lstat(sn, &s) < 0 ) {
        	printf("lstat %s fail\n", sn);
        	ret = -1;
       		break; 	
        }
        if ( busybox_cp(sn, tpathname) ) {
        	ret = -1;
       		break; 
		}
	}
	closedir(dirs);
	return ret;
}
/*
 *copy dir
 *ActionsCode(author:liaotianyang, type:new_method)
 */
int d_copy(const char *spathname,const char *tpathname)
{
    return dir_copy(spathname, tpathname);
}
