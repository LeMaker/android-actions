/*
 * arch/arm/mach-leopard/include/mach/secure_storage.h
 *
 * secure storage interface
 *
 * Copyright 2012 Actions Semi Inc.
 * Author: Actions Semi, Inc.
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 */

#ifndef __ASM_ARCH_SECURE_STORAGE_H
#define __ASM_ARCH_SECURE_STORAGE_H

/* secure storage data type */
#define SECURE_STORAGE_DATA_TYPE_SN     0
#define SECURE_STORAGE_DATA_TYPE_DRM    1
#define SECURE_STORAGE_DATA_TYPE_HDCP   2
#define SECURE_STORAGE_DATA_TYPE_DEVNUM 3
#define SECURE_STORAGE_DATA_TYPE_EXT    4 

struct secure_storage
{
    const char *name;
    unsigned int size;      /* maxinum size: byte */
    int (*read_data)(int type, char * buf, int size);
    int (*write_data)(int type, char * buf, int size);
};

/* provider interface */
extern int owl_register_secure_storage(struct secure_storage *ss);
extern int owl_unregister_secure_storage(struct secure_storage *ss);

/* user interface*/
extern int owl_read_secure_storage_data(int type, char * buf, int size);
extern int owl_write_secure_storage_data(int type, char * buf, int size);

#endif /* __ASM_ARCH_SECURE_STORAGE_H */
