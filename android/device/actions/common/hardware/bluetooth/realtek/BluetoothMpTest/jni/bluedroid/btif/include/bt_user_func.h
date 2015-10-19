#ifndef _BT_USER_FUNC_H
#define _BT_USER_FUNC_H
#include "bt_mp_base.h"

void
UserDefinedWaitMs(
	BASE_INTERFACE_MODULE *pBaseInterface,
	unsigned long WaitTimeMs
	) ;
unsigned int OsSleepSeconds(unsigned int seconds);

unsigned long GetTickCount();

#endif
