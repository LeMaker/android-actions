/*	uvc_queue_asoc.c  --  USB Video Class driver - Buffers management
 *
 *      Copyright (C) 2005-2010
 *          Laurent Pinchart (laurent.pinchart@ideasonboard.com)
 *
 *      This program is free software; you can redistribute it and/or modify
 *      it under the terms of the GNU General Public License as published by
 *      the Free Software Foundation; either version 2 of the License, or
 *      (at your option) any later version.
 *
 */
#ifdef CONFIG_ASOC_CAMERA

#include <linux/atomic.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/list.h>
#include <linux/module.h>
#include <linux/usb.h>
#include <linux/videodev2.h>
#include <linux/vmalloc.h>
#include <linux/wait.h>
#include <media/videobuf2-core.h>
#include <media/videobuf2-dma-contig.h>
#include <media/videobuf2-memops.h>


#include "uvcvideo.h"  

/*BUGFIX: recover the mmap buf usage for android vender uvc (author:liyuan,Add_code)*/
extern  void *vb2_vmalloc_alloc(void *alloc_ctx, unsigned long size, gfp_t gfp_flags);
extern  void vb2_vmalloc_put(void *buf_priv);
extern  int vb2_vmalloc_mmap(void *buf_priv, struct vm_area_struct *vma);

struct asoc_vb2_ion_buf {
	void				*vaddr;
	dma_addr_t			dma_addr;
	unsigned long			size;
	struct vm_area_struct		*vma;
	atomic_t			refcount;
	struct vb2_vmarea_handler	handler;
};


static void *asoc_vb2_ion_get_userptr(void *alloc_ctx, unsigned long paddr,
					unsigned long size, int write)
{
	struct asoc_vb2_ion_buf *buf;
	int ret = 0;
	unsigned long pfn;
	void* vaddr=NULL;
	buf = kzalloc(sizeof *buf, GFP_KERNEL);
	if (!buf)
		return ERR_PTR(-ENOMEM);

	pfn =paddr>>PAGE_SHIFT;
	if (pfn_valid(pfn)) {
		vaddr = phys_to_virt(pfn << PAGE_SHIFT);
	} else {
		vaddr = ioremap(pfn << PAGE_SHIFT, PAGE_SIZE);
	}
	if (!vaddr)
		ret = -ENOMEM;
	if (ret) {
		printk(KERN_ERR "Failed acquiring vaddr 0x%08lx\n",
				(unsigned long)vaddr);
		kfree(buf);
		return ERR_PTR(ret);
	}

	buf->size = size;
	buf->vaddr = vaddr;
	buf->dma_addr = paddr;
	buf->vma = NULL;

	return buf;
}

static void asoc_vb2_ion_put_userptr(void *mem_priv)
{
	struct asoc_vb2_ion_buf *buf = mem_priv;
	void* vaddr;
	unsigned long pfn;

	if (!buf)
		return;
	pfn = buf->dma_addr>>PAGE_SHIFT;
	vaddr = buf->vaddr;
	if (!pfn_valid(pfn))
		iounmap(vaddr);
	kfree(buf);
}

static void *asoc_vb2_ion_vaddr(void *buf_priv)
{
	struct asoc_vb2_ion_buf *buf = buf_priv;
	if (!buf)
		return NULL;

	return buf->vaddr;
}

static void *asoc_vb2_ion_cookie(void *buf_priv)
{
	struct asoc_vb2_ion_buf *buf = buf_priv;
	if (!buf)
		return NULL;

	return &buf->dma_addr;
}

static unsigned int asoc_vb2_ion_num_users(void *buf_priv)
{
	struct asoc_vb2_ion_buf *buf = buf_priv;

	return atomic_read(&buf->refcount);
}



struct vb2_mem_ops asoc_vb2_ion_memops=
{
    .cookie		= asoc_vb2_ion_cookie,
	.vaddr		= asoc_vb2_ion_vaddr,
	.get_userptr	= asoc_vb2_ion_get_userptr,
	.put_userptr	= asoc_vb2_ion_put_userptr,
	.num_users	= asoc_vb2_ion_num_users,
	.alloc		= vb2_vmalloc_alloc,
	.put		= vb2_vmalloc_put,
	.mmap		= vb2_vmalloc_mmap,
};
#endif

