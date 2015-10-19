/*
 * arch/arm/mach-owl/include/mach/storage_access.h
 *
 * storage interface
 *
 * Copyright 2012 Actions Semi Inc.
 * Author: Actions Semi, Inc.
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 */

#ifndef __ASM_ARCH_STORAGE_ACCESS_H
#define __ASM_ARCH_STORAGE_ACCESS_H

/* storage data type */
#define STORAGE_DATA_TYPE_SN		0x736E0000
#define STORAGE_DATA_TYPE_DRM		0x64724D00
#define STORAGE_DATA_TYPE_HDCP		0x68646370
#define STORAGE_DATA_TYPE_DEVNUM	0x64657600

/* user interface*/
extern int owl_read_storage_data(int type, char * buf, int size);

#endif /* __ASM_ARCH_STORAGE_ACCESS_H */
