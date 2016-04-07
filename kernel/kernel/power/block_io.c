/*
 * This file provides functions for block I/O operations on swap/file.
 *
 * Copyright (C) 1998,2001-2005 Pavel Machek <pavel@ucw.cz>
 * Copyright (C) 2006 Rafael J. Wysocki <rjw@sisk.pl>
 *
 * This file is released under the GPLv2.
 */

#include <linux/bio.h>
#include <linux/kernel.h>
#include <linux/pagemap.h>
#include <linux/swap.h>
#include <linux/export.h>
#include <mach/power.h>

#include "power.h"

/* #define USE_DRIVER_RW_INTERFACE */
#define RESUME_DISK_FIRST_LBA 16384

read_block_fn *read_block_handler;
write_block_fn *write_block_handler;

void register_swap_rw_handler(read_block_fn *read, write_block_fn *write)
{
	read_block_handler = read;
	write_block_handler = write;
}
EXPORT_SYMBOL(register_swap_rw_handler);

int __init block_io_init(void)
{
	read_block_handler = NULL;
	write_block_handler = NULL;
	return 0;
}
late_initcall(block_io_init);

/**
 *	submit - submit BIO request.
 *	@rw:	READ or WRITE.
 *	@off	physical offset of page.
 *	@page:	page we're reading or writing.
 *	@bio_chain: list of pending biod (for async reading)
 *
 *	Straight from the textbook - allocate and initialize the bio.
 *	If we're reading, make sure the page is marked as dirty.
 *	Then submit it and, if @bio_chain == NULL, wait.
 */
static int submit(int rw, struct block_device *bdev, sector_t sector,
		struct page *page, struct bio **bio_chain)
{
	const int bio_rw = rw | REQ_SYNC;
	struct bio *bio;

	bio = bio_alloc(__GFP_WAIT | __GFP_HIGH, 1);
	bio->bi_sector = sector;
	bio->bi_bdev = bdev;
	bio->bi_end_io = end_swap_bio_read;

	if (bio_add_page(bio, page, PAGE_SIZE, 0) < PAGE_SIZE) {
		pr_err("PM: Adding page to bio failed at %llu\n",
			(unsigned long long)sector);
		bio_put(bio);
		return -EFAULT;
	}

	lock_page(page);
	bio_get(bio);

	if (bio_chain == NULL) {
		submit_bio(bio_rw, bio);
		wait_on_page_locked(page);
		if (rw == READ)
			bio_set_pages_dirty(bio);
		bio_put(bio);
	} else {
		if (rw == READ)
			get_page(page);	/* These pages are freed later */
		bio->bi_private = *bio_chain;
		*bio_chain = bio;
		submit_bio(bio_rw, bio);
	}
	return 0;
}

int hib_bio_read_page(pgoff_t page_off, void *addr, struct bio **bio_chain)
{
#ifdef USE_DRIVER_RW_INTERFACE
	if (read_block_handler == NULL) {
		pr_err("PM: No swap read handler\n");
		return -1;
	} else {
		read_block_handler(RESUME_DISK_FIRST_LBA + page_off * 8,
							8, addr);
		return 0;
	}
#else
	return submit(READ, hib_resume_bdev, page_off * (PAGE_SIZE >> 9),
			virt_to_page(addr), bio_chain);
#endif
}

int hib_bio_write_page(pgoff_t page_off, void *addr, struct bio **bio_chain)
{
#ifdef USE_DRIVER_RW_INTERFACE
	if (write_block_handler == NULL) {
		pr_err("PM: No swap write handler\n");
		return -1;
	} else {
		write_block_handler(RESUME_DISK_FIRST_LBA + page_off * 8,
							8, addr);
		return 0;
	}
#else
	return submit(WRITE, hib_resume_bdev, page_off * (PAGE_SIZE >> 9),
			virt_to_page(addr), bio_chain);
#endif
}

int hib_wait_on_bio_chain(struct bio **bio_chain)
{
	struct bio *bio;
	struct bio *next_bio;
	int ret = 0;

#ifdef USE_DRIVER_RW_INTERFACE
	return 0;
#endif

	if (bio_chain == NULL)
		return 0;

	bio = *bio_chain;
	if (bio == NULL)
		return 0;
	while (bio) {
		struct page *page;

		next_bio = bio->bi_private;
		page = bio->bi_io_vec[0].bv_page;
		wait_on_page_locked(page);
		if (!PageUptodate(page) || PageError(page))
			ret = -EIO;
		put_page(page);
		bio_put(bio);
		bio = next_bio;
	}
	*bio_chain = NULL;
	return ret;
}
