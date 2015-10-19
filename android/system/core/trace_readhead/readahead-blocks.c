/*
 * Copyright (C) 2007 Michael Opdenacker <michael@free-electrons.com>
 * Copyright (C) 2005 Robin Hugh Johnson <robbat2@orbis-terrarum.net>
 *
 * Version 0.1
 *
 * Home page and updates:
 * http://free-electrons.com/community/tools/readahead/
 *  
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software 
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

// Based on code originally written by Erich Schubert <erich@debian.org>.

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <sys/syscall.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <sched.h>
#include <string.h>
#include <errno.h>
#include <linux/fs.h>

#define MAXPATH 2048
#define MAXLINE 32768

static char* program_name = "readahead-blocks";

// flag options
static int flag_debug = 0;
static int flag_verbose = 0;
static int flag_version = 0;
static int flag_help = 0;

static struct option long_options[] = {
	{"verbose", 0, &flag_verbose, 1},
	{"debug", 0, &flag_debug, 1},
	{"version", 0, &flag_version, 1},
	{"help", 0, &flag_help, 1},
	{0, 0, 0, 0}
};
static char* short_options = "vdhV";


void process_file(char *fileblocks, int size) {

#define __RAFUNCTION__ "process_file"

	int fd;
	char *filename, *token;
    char *space=" ";
	int block_start, block_end;
	
	filename = strtok(fileblocks, space);
	
	if (!filename)
		return;
	
	if(flag_debug) {
		fprintf(stderr,"%s:%s:%d:Attempting to readahead file: %s\n",__FILE__,__RAFUNCTION__,__LINE__,filename);
	}
		
	fd = open(filename,O_RDONLY);
	if (fd<0) {
		if(flag_debug) {
			fprintf(stderr,"%s:%s:%d:failed to open file: %s\n",__FILE__,__RAFUNCTION__,__LINE__,filename);
		}
		return;
	}

	while (token = strtok(NULL, space)) {
	      block_start = atoi(token);
		  
		  if (! (token = strtok(NULL, space))) {
		     fprintf(stderr, "Error reading block data\n");
			 return;
	      }

	      block_end = atoi(token);
		
	      readahead(fd, (loff_t) block_start, (size_t) (block_end - block_start));

	      int readahead_errno = errno;
	      switch(readahead_errno) {
	              case 0: 
	        	      if(flag_debug) 
	        		      fprintf(stderr,"%s:%s:%d:Loaded %s %d %d\n",__FILE__,__RAFUNCTION__,__LINE__,filename, block_start, block_end);
	        	      if(flag_verbose) 
	        		      fprintf(stdout,"Loaded file:%s %d %d\n",filename, block_start, block_end);
	        	      break;
	              case EBADF:
	        	      if(flag_debug)
	        		      fprintf(stderr,"%s:%s:%d:Bad file: %s\n",__FILE__,__RAFUNCTION__,__LINE__,filename);
	              case EINVAL:
	        	      if(flag_debug)
	        		      fprintf(stderr,"%s:%s:%d:Invalid filetype for readhead: %s\n",__FILE__,__RAFUNCTION__,__LINE__,filename);
	        	      break;
	      }

	}

	close(fd);
	// be nice to other processes now 
	sched_yield();
	
#undef __RAFUNCTION__
}





void process_files(char* filename) {
#define __RAFUNCTION__ "process_files"
	int fd;
	char* file = NULL;
	struct stat statbuf;
	char buffer[MAXLINE+1];
	char* iter = NULL;


	if (!filename)
		return;
	
	if(flag_debug) {
		fprintf(stderr,"%s:%s:%d:Attempting to load list: %s\n",__FILE__,__RAFUNCTION__,__LINE__,filename);
	}



	fd = open(filename,O_RDONLY);

	if (fd<0) {
		if(flag_debug) {
			fprintf(stderr,"%s:%s:%d:failed to open list: %s\n",__FILE__,__RAFUNCTION__,__LINE__,filename);
		}
		return;
	}
	
	if (fstat(fd, &statbuf)<0) {
		if(flag_debug) {
			fprintf(stderr,"%s:%s:%d:failed to fstat list: %s\n",__FILE__,__RAFUNCTION__,__LINE__,filename);
		}
		return;
	}

	/* map the whole file */
	file = mmap(0, statbuf.st_size, PROT_READ, MAP_SHARED, fd, 0);
	if (!file || file == MAP_FAILED) {
		if(flag_debug) {
			fprintf(stderr,"%s:%s:%d:failed to mmap list: %s\n",__FILE__,__RAFUNCTION__,__LINE__,filename);
		}
		return;
	}
	if(flag_verbose) 
		fprintf(stdout,"Loaded list:%s\n",filename);

	iter = file;
	while (iter) {
		/* find next newline */
		char* next = memchr(iter,'\n',file + statbuf.st_size - iter);
		if (next) {
			// if the length is positive, and shorter than MAXLINE
			// then we process it
			if((next - iter) >= MAXLINE) {
				fprintf(stderr,"%s:%s:%d:item in list too long!\n",__FILE__,__RAFUNCTION__,__LINE__);
			} else if (next-iter > 1) {
				memcpy(buffer, iter, next-iter);
				// replace newline with string terminator
				buffer[next-iter]='\0';
				// we allow # as the first character in a line, to show comments
				if(buffer[0] != '#') {
					process_file(buffer, next-iter);
				}
			}
			iter = next + 1;
		} else {
			iter = NULL;
		}
	}
#undef __RAFUNCTION__
}

void skel_command_msg_exit(FILE * f, char* msg, unsigned char retval) {
	//fprintf(f, msg);
	exit(retval);
}

void error_exit(char* msg) {
	skel_command_msg_exit(stderr,msg,1);
}

void command_error() {
#define LEN 1024
	char s[LEN];
	snprintf(s,LEN,"Try `%s --help' for more information.\n",program_name);
#undef LEN
	error_exit(s);
}

void command_version() {
#define LEN 1024
	char s[LEN];
	snprintf(s,LEN,"%s: %s\n",program_name,"0.1");
#undef LEN
//G	fprintf(stdout,s);
}

void command_help() {
#define LEN 8192
	char s[LEN];
	snprintf(s,LEN,
			"Usage: %s [OPTION]... [FILE]...\n"\
			"Performs readahead(2) on each entry in a file-based list.\n"\
			"\n"\
			"Options:\n"\
			"  -v --verbose   Print name of each successfully loaded file.\n"\
			"  -d --debug     Print out status messages while processing.\n"\
			"  -h --help      Stop looking at me!\n"\
			"  -V --version   As the name says.\n"\
			,program_name);
#undef LEN
//	fprintf(stdout,s);
}





#if 0
//int main(int argc, char **argv) {
//#define __RAFUNCTION__ "main"
//	int i;
//	program_name = argv[0];
//	while(1) {
//		int long_index,c;
//		long_index = -1;
//		c = getopt_long(argc,argv,short_options,long_options,&long_index);
//		if (c == -1)
//			break;
//		switch(c) {
//			// is this a long option?
//			case 0:
//			case '-':
//				switch(long_index) {
//					// handled by getopt directly
//					case 0: // verbose
//					case 1: // debug
//					case 2: // version
//					case 3: // help
//						break;
//					default:
//						command_error();
//				}
//				break;
//			// nope, short option
//			case 'v':
//				flag_verbose = 1;
//				break;
//			case 'd':
//				flag_version = 1;
//				break;
//			case 'h':
//				flag_help = 1;
//				break;
//			case 'V':
//				flag_version = 1;
//				break;
//			default:
//				command_error();
//		}
//	}
//	if(flag_help) {
//		command_help();
//	}
//	if(flag_version) {
//		command_version();
//	}
//	if(flag_version || flag_help) {
//		exit(0);
//	}
//
//
//	// now do the work
//	for (i = optind; i < argc; i++) {
//
//
//		process_files(argv[i]);
//	}
//
//	return 0;
//}
#endif

// vim: ts=4 sw=4:
