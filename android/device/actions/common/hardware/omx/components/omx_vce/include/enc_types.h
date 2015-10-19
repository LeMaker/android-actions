#ifndef _ENC_TYPE_H_
#define _ENC_TYPE_H_

#include "stdlib.h"
#include "stdio.h"
#include "string.h"

#ifndef uint32_t
typedef unsigned uint32_t;
#endif

#ifndef int64_t
#define int64_t long long
#endif

#ifndef uint64_t
#define uint64_t unsigned long long
#endif

#define MIN(X,Y) ((X)>(Y)?(Y):(X))
#define MAX(X,Y) ((Y)>(X)?(Y):(X))
#define ABS(X) ((X)>(0)?(X):(-X))

#endif

