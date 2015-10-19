#ifndef DMSTRINGUTIL_H
#define DMSTRINGUTIL_H


/************** HEADER FILE INCLUDES *****************************************/
#include "stdlib.h"
#include "string.h"
#include "stdio.h"

//#define DmStrlen(str) strlen(str)
#define DmStrlen(str) ((str!=NULL)? strlen(str):0)

#define DmStrcpy(target, source) strcpy(target, source)

#define DmStrncpy(target, source, count) strncpy(target, source, count)

#define DmStrcat(target, source) strcat(target, source)

#define DmStrncat(target, source, count) strncat(target, source, count)

#define DmStrcmp(target, source) strcmp(target, source)

#define DmStrncmp(target, source, count) strncmp(target, source, count)

#define DmStrchr(source, target) strchr(source, target)

#define DmStrrchr(source, target) strrchr(source, target)

#define DmStrstr(source, target) strstr(source, target)

#define DmTolower(source) tolower(source)

#define DmAtoi(source) atoi(source)

#define DmAtol(source) atol(source)

#define DmAtoll(source) atoll(source)

#define DmAtof(source) atof(source)

#define DmStrtol(source, end_ptr, radix) strtol( source, end_ptr, radix )

#define DmSprintf sprintf

#define DmSnprintf snprintf

#define DmSscanf sscanf

#define MAX_INT_STRING_LENGTH 14

#define MAX_FLOAT_STRING_LENGTH 20

#endif /* DMSTRINGUTIL_H */


