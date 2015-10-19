/*
 * linux/drivers/video/owl/dss/de_atm7059.c
 *
 * Copyright (C) 2014 Actions Corporation
 * Author: lipeng<lipeng@actions-semi.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published by
 * the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#define DSS_SUBSYS_NAME "MMU"

#include <linux/types.h>
#include <linux/dma-mapping.h>
#include <linux/slab.h>
#include <linux/mm.h>
#include <linux/pagemap.h>
#include <linux/hugetlb.h> 
#include <linux/vmalloc.h>

#include <asm/page.h>

#include <video/owldss.h>

#include "dss.h"
#include "de.h"
static DEFINE_MUTEX(MMU_lock);
#define MMU_TABLE_SIZE		(4 * 1024 * 1024)

/*
 * MMU entry used to convert virtaul address or ION fd
 * to DE device address.
 * 
 * --va, virtual address
 * --vsize, virtual address size
 * --num_pages_mapped, the number of mapped pages,
 *	which have valid physical memory
 * --page_offset, the offset in page
 *
 * --buffer_id, ion_handle ID
 *
 * --da, device address used by DE MMU
 * --dsize, device memory size
 * --num_pages, the number of pages coverted by this MMU entry
 *
 * --pages, a page array used to get physcal memory address
 *
 * --addr, virtual address in MMU table, which stores physical
 *	address used by DE(DA-->PA)
 *
 * --time_stamp, a time stamp to record MMU operate counters
 */
/* system physical address */
#define OWL_ADDRSPACE_PHYSADDR_BITS 32

typedef struct _OWL_SYS_PHYADDR
{
	/* variable sized type (32,64) */
#if OWL_ADDRSPACE_PHYSADDR_BITS == 32
	/* variable sized type (32,64) */
	u32 uiAddr;
#else
	u64 uiAddr;
#endif
} OWL_SYS_PHYADDR;

typedef struct _mmu_entry_ {
	/* only used by VA-->DA mapping */
	u64			va;
	u64			vsize;
	u32			num_pages_mapped;
	u32			page_offset;

#ifdef CONFIG_VIDEO_OWL_MMU_ION_SUPPORT
	/* only used by ION-->DA mapping */
	u64         buffer_id;
#endif
	
	u32			da;
	u32			dsize;	
	
	u32			num_pages;
	
	/* only used by VA-->DA mapping */
	struct page		**pages;

	u32			*addr;
	
	u64			time_stamp;
	
	struct _mmu_entry_	*next;	
	
} mmu_entry_t;


/* 
 * free space in MMU table, which maybe discontinuous.
 * --addr, virtual address of this free space
 * --size, free space's size
 */
typedef struct _mmu_free_info_ {
	void __iomem		*addr;
	u32			size;
	struct _mmu_free_info_	*next;		
} mmu_free_info_t;


/* 
 * DE MMU table.
 * --base, base virtual address
 * --base_phys, base physical address
 * --remain_size, remain size in byte
 * --free_list, free memory list in table
 * --entry_list, a list to hold all allocated MMU entried
 * --time_stamp, a time stamp to record MMU operate counters
 */
struct mmu_table {
	void __iomem		*base;
	u64			base_phys;
	u32			remain_size;
	
	mmu_free_info_t		*free_list;	
	mmu_entry_t		*entry_list;

	u64			time_stamp;
};

static struct mmu_table 	mmu;


/* allocate memory from MMU's free list */
static int mmu_alloc_memory(int size, u64 **addr){
	void __iomem *addr_tmp = NULL;
	mmu_free_info_t *free_info = mmu.free_list;

	DSSDBG("%s, size = %d\n", __func__, size);

	do {
		if (free_info == NULL || mmu.remain_size < size)
			break;

		if (free_info->size >= size) {
			addr_tmp = free_info->addr;
			free_info->addr += size;
			free_info->size -= size;
			mmu.remain_size -= size;
			DSSDBG("MMU Alloc: ADDR 0x%p size 0x%x HeapFree 0x%x \n",addr_tmp, size,mmu.remain_size);
			break;			
		}
								
		free_info = free_info->next;
			
	} while (free_info != NULL);
	
	if (addr_tmp == NULL) {
		DSSERR("fail to allocate de memory \n");
		return -1;
	}


	*addr = addr_tmp;
	return 0;
}

static int mmu_free_memroy(void __iomem *addr, int size)
{
	bool is_combination = false;
	
	mmu_free_info_t *free_info = mmu.free_list;
	mmu_free_info_t *last_free_info  = NULL;

	do {
		if (free_info == NULL)
			break;

		last_free_info = free_info;
		if (free_info->addr + free_info->size == addr) {
			free_info->size += size;
			is_combination = true;
			break;			
		}
		
		if(addr + size == free_info->addr) {
			free_info->addr = addr;
			free_info->size += size;
			is_combination = true;
			break;	
		}		
		free_info = free_info->next;	
	} while(free_info != NULL);
	
	/* insert new entry for free mem Info */
	if (!is_combination) {
		free_info = kmalloc(sizeof(mmu_free_info_t), GFP_KERNEL);
		free_info->addr = addr;
		free_info->size = size;
		free_info->next = NULL;
		last_free_info->next = free_info;		
	}
	mmu.remain_size += size;
		
	if (addr == 0) {
		DSSERR("fail to allocate de memory \n");
	}
	DSSDBG("MMU Free: 0x%p size 0x%x HeapFree 0x%x \n",addr, size,mmu.remain_size);
	return 0;
}
/* find MMU entry in device memory using vitrual address and its size */
static bool mmu_find_entry_by_vaddr(mmu_entry_t **found, u64 va, u32 length)
{
	bool is_found = false;
	mmu_entry_t *entry = mmu.entry_list;

	do {
		if (entry == NULL)
			break;

		if (entry->va == va && entry->vsize == length) {
			*found = entry;
			is_found = true;
			break;
		}	
		entry = entry->next;
	} while (entry != NULL);
	
	return is_found;
}

#ifdef CONFIG_VIDEO_OWL_MMU_ION_SUPPORT
/* find MMU entry in device memory using ion handle id */
static bool mmu_find_entry_by_id_firstone(mmu_entry_t **found, u64 id)
{
	bool is_found = false;
	mmu_entry_t *entry = mmu.entry_list;

	do {
		if (entry == NULL)
			break;

		if (entry->buffer_id == id) {
			if(*found != NULL){
				if((*found)->time_stamp > entry->time_stamp){
					*found = entry;
				}
			}else{
				*found = entry;
			}
			is_found = true;
			//break;
		}
		entry = entry->next;
	} while (entry != NULL);
	
	return is_found;
}
/* find MMU entry in device memory using ion handle id */
static bool mmu_find_entry_by_fd_lastone(mmu_entry_t **found, u64 fd)
{
	bool is_found = false;
	mmu_entry_t *entry = mmu.entry_list;

	do {
		if (entry == NULL)
			break;

		if (entry->buffer_id == fd) {
			if(*found != NULL){
				if((*found)->time_stamp < entry->time_stamp){
					*found = entry;
				}
			}else{
				*found = entry;
			}
			is_found = true;
			//break;
		}
		entry = entry->next;
	} while (entry != NULL);
	
	return is_found;
}
#endif

static int mmu_add_entry(mmu_entry_t *new_entry)
{
	int ret = 0;
	mmu_entry_t *entry = mmu.entry_list;
	mmu_entry_t *last_entry = NULL;
	
	do {
		if (entry == NULL)
			break;

		last_entry = entry;		
		entry = entry->next;			
	} while(entry != NULL);
	
	if (last_entry == NULL) {
		mmu.entry_list = new_entry;
		new_entry->next = NULL;
	} else {
		last_entry->next = new_entry;
		new_entry->next = NULL;
	}

	return ret;
}

static int mmu_delete_entry(mmu_entry_t * target_entry)
{
	int ret = 0;
	bool found = false;
	mmu_entry_t *entry = mmu.entry_list;
	mmu_entry_t *pre_entry =  mmu.entry_list;	
	do {
		if (entry == NULL)
			break;
		if(target_entry->buffer_id == entry->buffer_id){
			if (pre_entry->buffer_id == target_entry->buffer_id) {
				mmu.entry_list = pre_entry->next;
			} else {
				pre_entry->next =  target_entry->next;				
			}
			kfree(target_entry);
			found = true;
			break;
		}
		pre_entry = entry;
		entry = entry->next;			
	} while(entry != NULL);
	
	if(!found){
		DSSERR("fail to delete entry id is %llu )\n",target_entry->buffer_id);
		return -ENOMEM;
	}
	return ret;
}

/*=======================================================================
 *			APIs to others
 *=====================================================================*/
int mmu_init(void)
{
	void __iomem *vaddr;
	u64 paddr;
	mmu_free_info_t *free_info;
	
	vaddr = dma_alloc_coherent(NULL, MMU_TABLE_SIZE,
					(dma_addr_t *)&paddr, GFP_KERNEL);
	
	if (!vaddr) {
		DSSERR("fail to allocate mmu table mem (size: %dK))\n",
			MMU_TABLE_SIZE / 1024);
		return -ENOMEM;
	}

	//SetPageReserved(pfn_to_page((paddr) >> PAGE_SHIFT));
	
	mmu.base = vaddr;
	mmu.base_phys = paddr;
	mmu.remain_size = MMU_TABLE_SIZE;
	memset(vaddr,0,MMU_TABLE_SIZE);

	free_info = kmalloc(sizeof(mmu_free_info_t), GFP_KERNEL);
	free_info->addr = mmu.base ;
	free_info->size = MMU_TABLE_SIZE ;
	free_info->next = NULL;
		
	mmu.free_list = free_info;
	mmu.entry_list = NULL;
	
	DSSINFO("%s: base 0x%p base_phys 0x%llx free_info->addr 0x%p\n",
		__func__, mmu.base, mmu.base_phys, free_info->addr);

	owl_de_mmu_config(mmu.base_phys);

	return 0;
}
int mmu_va_to_da(u64 va, u32 length, u32 *da)
{
	u64 start_addr_orig = va;
	u32 addr_range_orig = length;

	u64 beyond_end_addr_orig = start_addr_orig + addr_range_orig;
	
	u64 start_addr = start_addr_orig & PAGE_MASK;
	u64 beyond_end_addr = PAGE_ALIGN(beyond_end_addr_orig);
	u32 addr_range = beyond_end_addr - start_addr;

	mmu_entry_t *entry = NULL;

	u64 *device_addr;
	int i = 0;
	int ret = 0;
	
	DSSDBG("mmu_va_to_da: va 0x%llx length 0x%x\n", va, length);
	
	if (va == 0)
		goto error_exit1;
	
	if (beyond_end_addr <= start_addr)
		goto error_exit1;
	
	DSSDBG("%s: va 0x%llx length 0x%x\n", __func__, va, length);
	if (mmu_find_entry_by_vaddr(&entry, va, length))
		goto ok_exit; 
	else
		entry = NULL;
	
	mmu.time_stamp++;
	
	DSSDBG("alloc MMU entry for mem record\n");
	entry = kmalloc(sizeof(*entry), GFP_KERNEL);
	if (entry == NULL) {
		DSSERR("Failed to alloc memory for MMU entry\n");
		goto error_exit1;
	}
	memset(entry, 0, sizeof(*entry));
	
	entry->num_pages = (u32)(addr_range >> PAGE_SHIFT);
	entry->page_offset = (u32)(start_addr_orig & ~PAGE_MASK);
	
	DSSDBG("alloc device addr from de mmu heap entry->num_pages 0x%x,\
		entry->page_offset 0x%x\n",
		entry->num_pages, entry->page_offset);
	ret = mmu_alloc_memory(entry->num_pages * sizeof(u32), &device_addr);
	
	if (ret) {
		DSSERR("Failed to alloc memory for devices\n");
		ret = -2;	
		goto error_exit2;
	}
	entry->da = (u32)(device_addr - (u64 *)mmu.base);
	entry->dsize = entry->num_pages * sizeof(u32);
	entry->addr = (u32 *)device_addr;
	DSSDBG(" entry->da 0x%x, entry->dsize 0x%x,\
		entry->addr 0x%p\n", entry->da,
		entry->dsize, entry->addr);
	memset(entry->addr, 0, entry->dsize);
	
	DSSDBG("Allocate page array \n");
	entry->pages = kmalloc((size_t)entry->num_pages
					* sizeof(*entry->pages), GFP_KERNEL);
	if (entry->pages == NULL) {
		DSSERR("mmu_va_to_da: Couldn't allocate page array");		
		ret = -3;	
		goto error_exit3;
	}
	memset(entry->pages, 0, (size_t)entry->num_pages
					* sizeof(*entry->pages));
	
	/* Lock down user memory */
	DSSDBG("Lock down user memory \n");
	down_read(&current->mm->mmap_sem);
	
	DSSDBG("start_addr 0x%llx, entry->pages 0x%p \n",
		start_addr, entry->pages);
	entry->num_pages_mapped = get_user_pages(current, current->mm, start_addr,
				entry->num_pages, 1, 0, entry->pages, NULL);
	DSSDBG("mmu_va_to_da: map all the pages needed\
		(wanted: %d, got %d) entry->addr %p \n",
		entry->num_pages, entry->num_pages_mapped,entry->addr);
	if (entry->num_pages_mapped >= 0) {
		/* See if we got all the pages we wanted */
		if (entry->num_pages_mapped != entry->num_pages) {
			printk("mmu_va_to_da: Couldn't map all\
				the pages needed (wanted: %d, got %d) \n",
				entry->num_pages, entry->num_pages_mapped);
				ret = -4;	
			goto error_exit4;
		}
		
		/* Build list of physical page addresses */
		for (i = 0; i < entry->num_pages; i++) {
			u32 ui32PFN = page_to_pfn(entry->pages[i]);
			
			entry->addr[i] = ui32PFN << PAGE_SHIFT;
			
			if ((entry->addr[i] >> PAGE_SHIFT) != ui32PFN) {
				DSSERR( "mmu_va_to_da: PFN out of range(%x)\n",
					ui32PFN);
				ret = -4;	
				goto error_exit4;
			}
			DSSDBG("entry->addr[%d] 0x%x \n",
				i, entry->addr[i]);
		}

	}

	entry->va = va;
	entry->vsize = length;
	
	DSSDBG("mmu_add_entry \n");
	ret = mmu_add_entry(entry);
	if (ret) {
		DSSERR( "mmu_add_entry failed \n");
		ret = -4;	
		goto error_exit4;
	}
	
	/* Lock up user memory */
	DSSDBG("Lock up user memory \n");
	up_read(&current->mm->mmap_sem);
	
	goto ok_exit;
		
error_exit4:
	kfree(entry->pages);	
	up_read(&current->mm->mmap_sem);
error_exit3:
	mmu_free_memroy(entry->addr, entry->num_pages * sizeof(u32));
error_exit2:
	kfree(entry);	
error_exit1:
ok_exit:
	if(entry != NULL) {
		int i = 0;
		u32 *mmu_table = (u32 *)mmu.base;
		*da = ((entry->da / 4) << PAGE_SHIFT)
				| entry->page_offset;
		DSSDBG("mmu_va_to_da va 0x%llx length 0x%x \
			da 0x%x  entry->da 0x%x \
			entry->page_offset 0x%x \n",
			va, length, *da,
			entry->da, entry->page_offset);
		entry->time_stamp = mmu.time_stamp;
		DSSDBG("mmu 0x%p \n", entry->addr);
		
		for (i = 0; i < (MMU_TABLE_SIZE - mmu.remain_size) / 4 + 5;
			i++) {
			DSSDBG("0x%x  ", *(mmu_table + i));
		}
		DSSDBG(" \n");
		DSSDBG("mmu_va_to_da va 0x%llx length 0x%x \
			da 0x%x  entry->da 0x%x  \
			entry->page_offset 0x%x \n",
			va, length, *da,
			entry->da, entry->page_offset);
	}

	return ret;
}

#ifdef CONFIG_VIDEO_OWL_MMU_ION_SUPPORT
int mmu_fd_to_da(u64 buffer_id, u32 *da)
{
	int ret = 0;
    mmu_entry_t *entry;
    
	mutex_lock(&MMU_lock);
	entry = NULL;

	if (!mmu_find_entry_by_fd_lastone(&entry, buffer_id)) {		
		entry = NULL;
	} 
		
	if(entry != NULL){
		*da = ((entry->da / 4) << PAGE_SHIFT);
	}else{
		*da = 0;
		ret = -1;
	}
	mutex_unlock(&MMU_lock);
	if(ret != 0)
		printk("mmu_fd_to_da (buffer_id %llu  da 0x%x) ret %d \n",buffer_id,*da,ret);
		
	return ret;
}
int owl_mmu_import_buffer_on_dc(u64 buffer_id , OWL_SYS_PHYADDR * psSysPhysAddr, u32 page_count,u32 page_offset)
{
	int i = 0;
	int ret = 0;
	u64 * device_addr;
	mmu_entry_t *entry;
	
	mutex_lock(&MMU_lock);
	entry = NULL;
	
	if (mmu_find_entry_by_id_firstone(&entry, buffer_id)) {
		DSSDBG("entry is already here  %lld \n", buffer_id);
		entry = NULL;
		//goto  error_exit1;
	} else {
		entry = NULL;
	}
	
	mmu.time_stamp++;

	entry = kmalloc(sizeof(*entry), GFP_KERNEL);
	if (entry == NULL) {
		DSSERR("Failed to alloc MMU entry\n");
		goto error_exit1;
	}
	memset(entry, 0, sizeof(*entry));
	
	entry->num_pages = page_count;

	ret = mmu_alloc_memory(entry->num_pages * sizeof(u32), &device_addr);
	
	if (ret) {
		DSSERR("Failed to alloc memory for devices \n");
		ret = -2;	
		goto error_exit2;
	}
	entry->da = (u32)((void *)device_addr - (void *)mmu.base);
	entry->dsize = entry->num_pages * sizeof(u32);

	entry->addr = (u32*)device_addr;
	memset(entry->addr, 0, entry->dsize);

	/* Build list of physical page addresses */
	for(i = 0 ; i < page_count; i++) {
		entry->addr[i] = psSysPhysAddr[i].uiAddr;
	}	
	
	entry->buffer_id = buffer_id;

	ret = mmu_add_entry(entry);
	
	if (ret) {
		DSSERR( "mmu_add_entry failed \n");
		ret = -4;	
		goto error_exit3;
	}
	DSSDBG("MMU Import: %lld  page_count %d entry->da 0x%x \n",buffer_id,page_count,entry->da);	
	goto ok_exit;
		
error_exit3:
	mmu_free_memroy(entry->addr, entry->num_pages * sizeof(u32));
error_exit2:
	kfree(entry);	
error_exit1:
ok_exit:
	if (entry != NULL) {
		entry->time_stamp = mmu.time_stamp;
		DSSDBG("mmu 0x%p \n", entry->addr);
	}
	mutex_unlock(&MMU_lock);
	return ret;
}
EXPORT_SYMBOL(owl_mmu_import_buffer_on_dc);
int owl_mmu_unimport_buffer_on_dc(u64 buffer_id)
{
	int ret = 0;
	mmu_entry_t *entry;
	
	mutex_lock(&MMU_lock);
	entry = NULL;
	DSSDBG("MMU UnImport: buffer_id %lld \n",buffer_id);		
	if (mmu_find_entry_by_id_firstone(&entry, buffer_id)) {
		mmu_free_memroy(entry->addr,entry->num_pages * sizeof(u32));
		
		ret = mmu_delete_entry(entry);
	} 
	mutex_unlock(&MMU_lock);
	return ret;
}
EXPORT_SYMBOL(owl_mmu_unimport_buffer_on_dc);
#else
int mmu_fd_to_da(u64 buffer_id, u32 *da)
{
	return -1;
}
#endif

