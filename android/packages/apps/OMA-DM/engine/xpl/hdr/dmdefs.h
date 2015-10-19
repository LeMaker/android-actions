#ifndef DM_DEFS_H
#define DM_DEFS_H

/************** HEADER FILE INCLUDES *****************************************/

#include "dmMemory.h"


#define DIM(array) (sizeof(array)/sizeof((array)[0]))

#define FreeAndSetNull(_x) \
	if ((_x)!=NULL) { DmFreeMem(_x); (_x)=NULL; }


#endif /* DMDEFS_H */

