/*
 * Simple synchronous userspace interface to SPI devices
 *
 * Copyright (C) 2006 SWAPP
 *  Andrea Paterniani <a.paterniani@swapp-eng.it>
 * Copyright (C) 2007 David Brownell (simplification, cleanup)
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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/ioctl.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/list.h>
#include <linux/errno.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/compat.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/delay.h>
#include <linux/spi/spi.h>
#include <linux/spi/spidev.h>

#include <asm/uaccess.h>


/*
 * This supports access to SPI devices using normal userspace I/O calls.
 * Note that while traditional UNIX/POSIX I/O semantics are half duplex,
 * and often mask message boundaries, full SPI support requires full duplex
 * transfers.  There are several kinds of internal message boundaries to
 * handle chipselect management and other protocol options.
 *
 * SPI has a character major number assigned.  We allocate minor numbers
 * dynamically using a bitmask.  You must use hotplug tools, such as udev
 * (or mdev with busybox) to create and destroy the /dev/spidevB.C device
 * nodes, since there is no fixed association of minor numbers with any
 * particular SPI bus or device.
 */
#define SPIDEV_MAJOR            153 /* assigned */
#define N_SPI_MINORS            32  /* ... up to 256 */

static DECLARE_BITMAP(minors, N_SPI_MINORS);


/* Bit masks for spi_device.mode management.  Note that incorrect
 * settings for some settings can cause *lots* of trouble for other
 * devices on a shared bus:
 *
 *  - CS_HIGH ... this device will be active when it shouldn't be
 *  - 3WIRE ... when active, it won't behave as it should
 *  - NO_CS ... there will be no explicit message boundaries; this
 *  is completely incompatible with the shared bus model
 *  - READY ... transfers may proceed when they shouldn't.
 *
 * REVISIT should changing those flags be privileged?
 */
#define SPI_MODE_MASK       (SPI_CPHA | SPI_CPOL | SPI_CS_HIGH \
                | SPI_LSB_FIRST | SPI_3WIRE | SPI_LOOP \
                | SPI_NO_CS | SPI_READY)

#define SPI_TEST_MODE     0

#define HK25L6406
#ifdef  HK25L6406
#define CMD_SZ 4
#else
#define CMD_SZ 2
#endif

#define CHIP_ERASE        0
#define BLOCK_ERASE   1
#define SECTOR_ERASE      2

#ifdef  HK25L6406
#define FLASH_PAGE_SIZE    256 
#define FLASH_CAPACITY     0x7FFFFF
#define TEST_ADDR      0x400

#define CMD_READ_ID        0x90
#define CMD_CHIP_ERASE     0x60
#define CMD_SECTOR_ERASE   0x20
#define CMD_BLOCK_ERASE    0x52

#define CMD_WRITE_ENABLE   0x06
#define CMD_WRITE_DISABLE  0x04
#define CMD_RDSR       0x05   /* Read status Register */
#define CMD_READ_BYTE      0x03
#define CMD_PAGE_PROGRAM   0x02

#define SR_WIP         0x1    /* erase/write in progress */
#define SR_WEL         0x2    /* write enable latch */

#define MAX_READY_WAIT_TIME   0x50  /* wait 50 *10 ms */

#define MANUFACTURER_ID    0xC2
#define DEVICE_ID          0x16

#define BLOCK_NUM_MAX      127
#define SECTOR_NUM_MAX     0x7FF
#define BLOCK_BASE_ADDR    0x10000
#define SECTOR_BASE_ADDR   0x1000
#endif
struct spidev_data {
    dev_t           devt;
    spinlock_t      spi_lock;
    struct spi_device   *spi;
    struct list_head    device_entry;

    /* buffer is NULL unless this device is open (users > 0) */
    struct mutex        buf_lock;
    unsigned        users;
    u8          *buffer;
    char            cmd[CMD_SZ];
};

static LIST_HEAD(device_list);
static DEFINE_MUTEX(device_list_lock);

static unsigned bufsiz = 4096*2;
module_param(bufsiz, uint, S_IRUGO);
MODULE_PARM_DESC(bufsiz, "data bytes in biggest supported SPI message");

/*-------------------------------------------------------------------------*/

/*
 * We can't use the standard synchronous wrappers for file I/O; we
 * need to protect against async removal of the underlying spi_device.
 */
static void spidev_complete(void *arg)
{
    complete(arg);
}

static ssize_t
spidev_sync(struct spidev_data *spidev, struct spi_message *message)
{
    DECLARE_COMPLETION_ONSTACK(done);
    int status;

    message->complete = spidev_complete;
    message->context = &done;

    spin_lock_irq(&spidev->spi_lock);
    if (spidev->spi == NULL)
        status = -ESHUTDOWN;
    else
        status = spi_async(spidev->spi, message);
    spin_unlock_irq(&spidev->spi_lock);

    if (status == 0) {
        wait_for_completion(&done);
        status = message->status;
        if (status == 0)
            status = message->actual_length;
    }
    return status;
}

#if SPI_TEST_MODE
static int check_test(struct spidev_data   *spidev)
{
      char buf[3] = {1};
      char cmd[3] = {0xa,1,2};
      return spi_write_then_read( spidev->spi, cmd, 3, buf, 3 );
}
#endif
static int check_id(struct spidev_data  *spidev)
{
    char buf[2] = {0};

        spidev->cmd[0] = CMD_READ_ID;
    
    spi_write_then_read(spidev->spi, spidev->cmd, CMD_SZ, buf, 2);
        if((buf[0] != MANUFACTURER_ID) || (buf[1] != DEVICE_ID)){
        printk("check id = [%x%x] fail !!!\n",buf[0],buf[1]);
        return -1;
    }

        return 0;
}

static int read_status(struct spidev_data  *spidev)
{
    int ret = -1;
    char status[2] = {0xff};

        spidev->cmd[0] = CMD_RDSR;
        ret = spi_write_then_read(spidev->spi, spidev->cmd, 1, status, 2);
    if(ret < 0){
        printk(" error %d reading SR", ret);
        return ret;
    }
    return status[0];
}

static int wait_till_ready(struct spidev_data  *spidev)
{
    int timeout,status;

    for(timeout = 0; timeout < MAX_READY_WAIT_TIME; timeout++)
    {
        status = read_status(spidev);
        if(status < 0)
            break;

        if(!(status & SR_WIP))
            return 0;

        msleep(10);
    }
    printk("---wait_till_ready : Busy !!!\n");
    return 1;
}

static int write_allow_check(struct spidev_data *spidev)
{
    int count, status;

        for(count = 0; count < 10; count++)
    {
                status = read_status(spidev);
                if(status & SR_WEL)
                        break;

                msleep(10);
        }
        if(count != 10)
                return 0;

    printk("disable write: write enable bit is 0 \n");
    return -1;
}

static inline int write_enable(struct spidev_data  *spidev)
{
    spidev->cmd[0] = CMD_WRITE_ENABLE;

    return spi_write(spidev->spi, spidev->cmd, 1);
}

static inline int write_disable(struct spidev_data  *spidev)
{
        spidev->cmd[0] = CMD_WRITE_DISABLE;

        return spi_write(spidev->spi, spidev->cmd, 1);
}

static int write_ready(struct spidev_data *spidev)
{
    int ret;
    
    write_enable(spidev);

    ret = write_allow_check(spidev);
    if(ret < 0)
        return -1;

    return 0;   
}

static int chip_erase(struct spidev_data  *spidev)
{
    int ret;

    /* wait until finished previous write commands */
    if(wait_till_ready(spidev))
        return -1;

    /* Send write enable, then erase commands */
        ret = write_ready(spidev);
    if(ret < 0)
        return -1;

    spidev->cmd[0] = CMD_CHIP_ERASE;
        return spi_write(spidev->spi, spidev->cmd, 1);  
}

static int block_erase(struct spidev_data  *spidev, int addr)
{
    int ret;

        /* wait until finished previous write commands */
        if(wait_till_ready(spidev))
                return -1;

        /* Send write enable, then erase commands */
        ret = write_ready(spidev);
    if(ret < 0)
        return -1;

        spidev->cmd[0] = CMD_BLOCK_ERASE;
    spidev->cmd[1] = ((u8)(addr >> 16));
        spidev->cmd[2] = ((u8)(addr >> 8));
        spidev->cmd[3] = ((u8)(addr));
        return spi_write(spidev->spi, spidev->cmd, 4);
}

static int sector_erase(struct spidev_data  *spidev, int addr)
{
        int ret;

        /* wait until finished previous write commands */
        if(wait_till_ready(spidev))
                return -1;

        /* Send write enable, then erase commands */
        ret = write_ready(spidev);
        if(ret < 0)
                return -1;

        spidev->cmd[0] = CMD_SECTOR_ERASE;
        spidev->cmd[1] = ((u8)(addr >> 16));
        spidev->cmd[2] = ((u8)(addr >> 8));
        spidev->cmd[3] = ((u8)(addr));
        return spi_write(spidev->spi, spidev->cmd, 4);
}

static int erase_flash(struct spidev_data  *spidev, int num, int cmd)
{
    int addr;
    int ret = -1;

    switch(cmd){
    case CHIP_ERASE:
        ret = chip_erase(spidev);
        break;
    case BLOCK_ERASE:
        if(num > BLOCK_NUM_MAX){
            printk("block num[%d] out of range[0~%d] !!!\n",num, BLOCK_NUM_MAX);
            return -1;  
        } 
        else{
            addr = num *  BLOCK_BASE_ADDR;
            ret = block_erase(spidev,addr);
        }
        break;
    case SECTOR_ERASE:
        if(num > SECTOR_NUM_MAX){
            printk("block num [%d] out of range [0~%d] !!!\n",num, SECTOR_NUM_MAX);
            return -1;
        }
        else {
            addr = num * SECTOR_BASE_ADDR;
            ret = sector_erase(spidev,addr);
        }
        break;
    default:
        printk("cmd[%d] is invalid \n", cmd);
        break;
    }

    return ret;
}
static int HK25L6406_read(struct spidev_data  *spidev, size_t len, loff_t addr)
{
    struct spi_transfer st[2];
    struct spi_message  msg;
    int ret;

        if(wait_till_ready(spidev))
                return -1;

    ret = write_ready(spidev);
    if(ret < 0)
        return -1;

    spi_message_init(&msg);
    memset(st, 0, sizeof(st));

    spidev->cmd[0] = CMD_READ_BYTE;
    spidev->cmd[1] = ((u8)(addr >> 16));
    spidev->cmd[2] = ((u8)(addr >> 8));
    spidev->cmd[3] = ((u8)(addr));

    st[0].tx_buf = spidev->cmd;
    st[0].len = CMD_SZ;
    spi_message_add_tail(&st[0], &msg);

    st[1].rx_buf = spidev->buffer;
    st[1].len = len;
    spi_message_add_tail(&st[1], &msg);

    return spidev_sync(spidev, &msg);
}

static int HK25L6406_write(struct spidev_data  *spidev, size_t len, loff_t addr)
{
    struct spi_transfer st[2];
    struct spi_message  msg;
    int ret;

        if(wait_till_ready(spidev))
                return -1;

    ret = write_ready(spidev);
        if(ret < 0)
                return -1;

    spi_message_init(&msg);
    memset(st, 0, sizeof(st));

    spidev->cmd[0] = CMD_PAGE_PROGRAM;
    spidev->cmd[1] = ((u8)(addr >> 16));
    spidev->cmd[2] = ((u8)(addr >> 8));
    spidev->cmd[3] = ((u8)(addr));

    st[0].tx_buf = spidev->cmd;
    st[0].len = CMD_SZ;
    spi_message_add_tail(&st[0], &msg);

    st[1].tx_buf = spidev->buffer;
    st[1].len = len;
    spi_message_add_tail(&st[1], &msg);

    return spidev_sync(spidev, &msg); 
}

/*-------------------------------------------------------------------------*/

/* Read-only message with current device setup */
static ssize_t
spidev_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
    struct spidev_data  *spidev;
    ssize_t         status = 0;
    unsigned int        addr = TEST_ADDR;

    /* chipselect only toggles at start or end of operation */
    if (count > bufsiz)
        return -EMSGSIZE;

    if(addr+count > FLASH_CAPACITY){
        printk("invalid addr:0x%x > flash capacity",addr);
        return -EFAULT;
    }
  
    spidev = filp->private_data;

    mutex_lock(&spidev->buf_lock);
    status = HK25L6406_read(spidev, count, addr);
    if (status > 0) {
        unsigned long   missing;

        missing = copy_to_user(buf, spidev->buffer, status);
        if (missing == status)
            status = -EFAULT;
        else
            status = status - missing;
    }
    mutex_unlock(&spidev->buf_lock);

    return status;
}

/* Write-only message with current device setup */
static ssize_t
spidev_write(struct file *filp, const char __user *buf,
        size_t count, loff_t *f_pos)
{
    struct spidev_data  *spidev;
    ssize_t         status = 0;
    unsigned int        addr = TEST_ADDR;
    unsigned long       missing;
    unsigned int        w_count = count;
    unsigned int        page_offset;
    unsigned int        num;

    /* chipselect only toggles at start or end of operation */
    if (count > bufsiz)
        return -EMSGSIZE;

    if(addr > FLASH_CAPACITY){
        printk("invalid addr:0x%x > flash capacity \n",addr);
        return -EFAULT;
    }
  
    if((addr + count) > FLASH_CAPACITY){
        printk("addr add len beyond the scope of flash capacity !!!\n");
        return -EFAULT;
    }
  
    spidev = filp->private_data;
    
    /* erase flash befor write data */
    if((count+addr%SECTOR_BASE_ADDR)<= SECTOR_BASE_ADDR)
    {
        num = addr / SECTOR_BASE_ADDR;
        status = erase_flash(spidev, num, SECTOR_ERASE);
    } else if((count+addr%BLOCK_BASE_ADDR) <= BLOCK_BASE_ADDR){
        num = addr / BLOCK_BASE_ADDR;
        status = erase_flash(spidev, num, BLOCK_ERASE);
    } else {
        status = erase_flash(spidev, 0, CHIP_ERASE);
    }
    if(status < 0){
        printk("flash erase fail !!!\n");
        return -1;
    }
    ssize_t write_size=0;

    mutex_lock(&spidev->buf_lock);
    missing = copy_from_user(spidev->buffer, buf, count);
    if (missing == 0) {
        page_offset = addr % FLASH_PAGE_SIZE;
        if((page_offset + count) <= FLASH_PAGE_SIZE){
            status = HK25L6406_write(spidev, count, addr);
			write_size+=status;
            if(status < 0)
                goto write_fail;
        } 
        else {
            w_count = FLASH_PAGE_SIZE - page_offset;
            status = HK25L6406_write(spidev, w_count, addr);
			write_size+=status;
            if(status < 0)
                goto write_fail;
            do{
                addr += w_count;
                spidev->buffer += w_count;
                count -= w_count;
                if( count >= FLASH_PAGE_SIZE )
                    w_count = FLASH_PAGE_SIZE;
                else
                    w_count = count;
                
                status = HK25L6406_write(spidev, w_count, addr);
				write_size+=status;
                if(status < 0)
                    goto write_fail;
            }while( (count - w_count) > 0 );
        }
    } else 
        status = -EFAULT;

write_fail:
    mutex_unlock(&spidev->buf_lock);
	if(status<0)
    return status;
	return write_size;
}

static int spidev_message(struct spidev_data *spidev,
        struct spi_ioc_transfer *u_xfers, unsigned n_xfers)
{
    struct spi_message  msg;
    struct spi_transfer *k_xfers;
    struct spi_transfer *k_tmp;
    struct spi_ioc_transfer *u_tmp;
    unsigned        n, total;
    u8          *buf;
    int         status = -EFAULT;

    spi_message_init(&msg);
    k_xfers = kcalloc(n_xfers, sizeof(*k_tmp), GFP_KERNEL);
    if (k_xfers == NULL)
        return -ENOMEM;

    /* Construct spi_message, copying any tx data to bounce buffer.
     * We walk the array of user-provided transfers, using each one
     * to initialize a kernel version of the same transfer.
     */
    buf = spidev->buffer;
    total = 0;
    for (n = n_xfers, k_tmp = k_xfers, u_tmp = u_xfers;
            n;
            n--, k_tmp++, u_tmp++) {
        k_tmp->len = u_tmp->len;

        total += k_tmp->len;
        if (total > bufsiz) {
            status = -EMSGSIZE;
            goto done;
        }

        if (u_tmp->rx_buf) {
            k_tmp->rx_buf = buf;
            if (!access_ok(VERIFY_WRITE, (u8 __user *)
                        (uintptr_t) u_tmp->rx_buf,
                        u_tmp->len))
                goto done;
        }
        if (u_tmp->tx_buf) {
            k_tmp->tx_buf = buf;
            if (copy_from_user(buf, (const u8 __user *)
                        (uintptr_t) u_tmp->tx_buf,
                    u_tmp->len))
                goto done;
        }
        buf += k_tmp->len;

        k_tmp->cs_change = !!u_tmp->cs_change;
        k_tmp->bits_per_word = u_tmp->bits_per_word;
        k_tmp->delay_usecs = u_tmp->delay_usecs;
        k_tmp->speed_hz = u_tmp->speed_hz;
#ifdef VERBOSE
        dev_dbg(&spidev->spi->dev,
            "  xfer len %zd %s%s%s%dbits %u usec %uHz\n",
            u_tmp->len,
            u_tmp->rx_buf ? "rx " : "",
            u_tmp->tx_buf ? "tx " : "",
            u_tmp->cs_change ? "cs " : "",
            u_tmp->bits_per_word ? : spidev->spi->bits_per_word,
            u_tmp->delay_usecs,
            u_tmp->speed_hz ? : spidev->spi->max_speed_hz);
#endif
        spi_message_add_tail(k_tmp, &msg);
    }

    status = spidev_sync(spidev, &msg);
    if (status < 0)
        goto done;

    /* copy any rx data out of bounce buffer */
    buf = spidev->buffer;
    for (n = n_xfers, u_tmp = u_xfers; n; n--, u_tmp++) {
        if (u_tmp->rx_buf) {
            if (__copy_to_user((u8 __user *)
                    (uintptr_t) u_tmp->rx_buf, buf,
                    u_tmp->len)) {
                status = -EFAULT;
                goto done;
            }
        }
        buf += u_tmp->len;
    }
    status = total;

done:
    kfree(k_xfers);
    return status;
}

static long
spidev_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    int         err = 0;
    int         retval = 0;
    struct spidev_data  *spidev;
    struct spi_device   *spi;
    u32         tmp;
    unsigned        n_ioc;
    struct spi_ioc_transfer *ioc;

    /* Check type and command number */
    if (_IOC_TYPE(cmd) != SPI_IOC_MAGIC)
        return -ENOTTY;

    /* Check access direction once here; don't repeat below.
     * IOC_DIR is from the user perspective, while access_ok is
     * from the kernel perspective; so they look reversed.
     */
    if (_IOC_DIR(cmd) & _IOC_READ)
        err = !access_ok(VERIFY_WRITE,
                (void __user *)arg, _IOC_SIZE(cmd));
    if (err == 0 && _IOC_DIR(cmd) & _IOC_WRITE)
        err = !access_ok(VERIFY_READ,
                (void __user *)arg, _IOC_SIZE(cmd));
    if (err)
        return -EFAULT;

    /* guard against device removal before, or while,
     * we issue this ioctl.
     */
    spidev = filp->private_data;
    spin_lock_irq(&spidev->spi_lock);
    spi = spi_dev_get(spidev->spi);
    spin_unlock_irq(&spidev->spi_lock);

    if (spi == NULL)
        return -ESHUTDOWN;

    /* use the buffer lock here for triple duty:
     *  - prevent I/O (from us) so calling spi_setup() is safe;
     *  - prevent concurrent SPI_IOC_WR_* from morphing
     *    data fields while SPI_IOC_RD_* reads them;
     *  - SPI_IOC_MESSAGE needs the buffer locked "normally".
     */
    mutex_lock(&spidev->buf_lock);

    switch (cmd) {
    /* read requests */
    case SPI_IOC_RD_MODE:
        retval = __put_user(spi->mode & SPI_MODE_MASK,
                    (__u8 __user *)arg);
        break;
    case SPI_IOC_RD_LSB_FIRST:
        retval = __put_user((spi->mode & SPI_LSB_FIRST) ?  1 : 0,
                    (__u8 __user *)arg);
        break;
    case SPI_IOC_RD_BITS_PER_WORD:
        retval = __put_user(spi->bits_per_word, (__u8 __user *)arg);
        break;
    case SPI_IOC_RD_MAX_SPEED_HZ:
        retval = __put_user(spi->max_speed_hz, (__u32 __user *)arg);
        break;

    /* write requests */
    case SPI_IOC_WR_MODE:
        retval = __get_user(tmp, (u8 __user *)arg);
        if (retval == 0) {
            u8  save = spi->mode;

            if (tmp & ~SPI_MODE_MASK) {
                retval = -EINVAL;
                break;
            }

            tmp |= spi->mode & ~SPI_MODE_MASK;
            spi->mode = (u8)tmp;
            retval = spi_setup(spi);
            if (retval < 0)
                spi->mode = save;
            else
                dev_dbg(&spi->dev, "spi mode %02x\n", tmp);
        }
        break;
    case SPI_IOC_WR_LSB_FIRST:
        retval = __get_user(tmp, (__u8 __user *)arg);
        if (retval == 0) {
            u8  save = spi->mode;

            if (tmp)
                spi->mode |= SPI_LSB_FIRST;
            else
                spi->mode &= ~SPI_LSB_FIRST;
            retval = spi_setup(spi);
            if (retval < 0)
                spi->mode = save;
            else
                dev_dbg(&spi->dev, "%csb first\n",
                        tmp ? 'l' : 'm');
        }
        break;
    case SPI_IOC_WR_BITS_PER_WORD:
        retval = __get_user(tmp, (__u8 __user *)arg);
        if (retval == 0) {
            u8  save = spi->bits_per_word;

            spi->bits_per_word = tmp;
            retval = spi_setup(spi);
            if (retval < 0)
                spi->bits_per_word = save;
            else
                dev_dbg(&spi->dev, "%d bits per word\n", tmp);
        }
        break;
    case SPI_IOC_WR_MAX_SPEED_HZ:
        retval = __get_user(tmp, (__u32 __user *)arg);
        if (retval == 0) {
            u32 save = spi->max_speed_hz;

            spi->max_speed_hz = tmp;
            retval = spi_setup(spi);
            if (retval < 0)
                spi->max_speed_hz = save;
            else
                dev_dbg(&spi->dev, "%d Hz (max)\n", tmp);
        }
        break;

    default:
        /* segmented and/or full-duplex I/O request */
        if (_IOC_NR(cmd) != _IOC_NR(SPI_IOC_MESSAGE(0))
                || _IOC_DIR(cmd) != _IOC_WRITE) {
            retval = -ENOTTY;
            break;
        }

        tmp = _IOC_SIZE(cmd);
        if ((tmp % sizeof(struct spi_ioc_transfer)) != 0) {
            retval = -EINVAL;
            break;
        }
        n_ioc = tmp / sizeof(struct spi_ioc_transfer);
        if (n_ioc == 0)
            break;

        /* copy into scratch area */
        ioc = kmalloc(tmp, GFP_KERNEL);
        if (!ioc) {
            retval = -ENOMEM;
            break;
        }
        if (__copy_from_user(ioc, (void __user *)arg, tmp)) {
            kfree(ioc);
            retval = -EFAULT;
            break;
        }

        /* translate to spi_message, execute */
        retval = spidev_message(spidev, ioc, n_ioc);
        kfree(ioc);
        break;
    }

    mutex_unlock(&spidev->buf_lock);
    spi_dev_put(spi);
    return retval;
}

#ifdef CONFIG_COMPAT
static long
spidev_compat_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    return spidev_ioctl(filp, cmd, (unsigned long)compat_ptr(arg));
}
#else
#define spidev_compat_ioctl NULL
#endif /* CONFIG_COMPAT */

static int spidev_open(struct inode *inode, struct file *filp)
{
    struct spidev_data  *spidev;
    int         status = -ENXIO;

    mutex_lock(&device_list_lock);

    list_for_each_entry(spidev, &device_list, device_entry) {
        if (spidev->devt == inode->i_rdev) {
            status = 0;
            break;
        }
    }
    if (status == 0) {
        if (!spidev->buffer) {
            spidev->buffer = kmalloc(bufsiz, GFP_KERNEL);
            if (!spidev->buffer) {
                dev_dbg(&spidev->spi->dev, "open/ENOMEM\n");
                status = -ENOMEM;
            }
        }
        if (status == 0) {
            spidev->users++;
            filp->private_data = spidev;
            nonseekable_open(inode, filp);
        }
    } else {
        pr_debug("spidev: nothing for minor %d\n", iminor(inode));
        mutex_unlock(&device_list_lock);
        return status;
        }

    status = check_id(spidev);
    if(status == 0)
        printk("flash check id : ok !!!\n");    

    mutex_unlock(&device_list_lock);

    return status;
}

static int spidev_release(struct inode *inode, struct file *filp)
{
    struct spidev_data  *spidev;
    int         status = 0;

    mutex_lock(&device_list_lock);
    spidev = filp->private_data;
    filp->private_data = NULL;

    /* last close? */
    spidev->users--;
    if (!spidev->users) {
        int     dofree;

        kfree(spidev->buffer);
        spidev->buffer = NULL;

        /* ... after we unbound from the underlying device? */
        spin_lock_irq(&spidev->spi_lock);
        dofree = (spidev->spi == NULL);
        spin_unlock_irq(&spidev->spi_lock);

        if (dofree)
            kfree(spidev);
    }
    mutex_unlock(&device_list_lock);

    return status;
}

static const struct file_operations spidev_fops = {
    .owner =    THIS_MODULE,
    /* REVISIT switch to aio primitives, so that userspace
     * gets more complete API coverage.  It'll simplify things
     * too, except for the locking.
     */
    .write =    spidev_write,
    .read =     spidev_read,
    .unlocked_ioctl = spidev_ioctl,
    .compat_ioctl = spidev_compat_ioctl,
    .open =     spidev_open,
    .release =  spidev_release,
    .llseek =   no_llseek,
};

/*-------------------------------------------------------------------------*/

/* The main reason to have this class is to make mdev/udev create the
 * /dev/spidevB.C character device nodes exposing our userspace API.
 * It also simplifies memory management.
 */
static struct class *spidev_class;

/*-------------------------------------------------------------------------*/
static int spidev_probe(struct spi_device *spi)
{
    struct spidev_data  *spidev;
    int         status;
    unsigned long       minor;

    /* Allocate driver data */
    spidev = kzalloc(sizeof(*spidev), GFP_KERNEL);
    if (!spidev)
        return -ENOMEM;

    /* Initialize the driver data */
    spidev->spi = spi;
    spin_lock_init(&spidev->spi_lock);
    mutex_init(&spidev->buf_lock);

    INIT_LIST_HEAD(&spidev->device_entry);

    /* If we can allocate a minor number, hook up this device.
     * Reusing minors is fine so long as udev or mdev is working.
     */
    mutex_lock(&device_list_lock);
    minor = find_first_zero_bit(minors, N_SPI_MINORS);
    if (minor < N_SPI_MINORS) {
        struct device *dev;

        spidev->devt = MKDEV(SPIDEV_MAJOR, minor);
        dev = device_create(spidev_class, &spi->dev, spidev->devt,
                    spidev, "spidev%d.%d",
                    spi->master->bus_num, spi->chip_select);
        status = PTR_RET(dev);
    } else {
        dev_dbg(&spi->dev, "no minor number available!\n");
        status = -ENODEV;
    }
    if (status == 0) {
        set_bit(minor, minors);
        list_add(&spidev->device_entry, &device_list);
    }
    mutex_unlock(&device_list_lock);

    if (status == 0)
        spi_set_drvdata(spi, spidev);
    else
        kfree(spidev);

#if SPI_TEST_MODE
       minor = 100;
       do {
           status = check_test(spidev);
           if(status < 0)
               printk("[leo_zt] ---- spi fail ---\n");
           msleep(10);
       }while(minor --);
#endif     
    return status;
}

static int spidev_remove(struct spi_device *spi)
{
    struct spidev_data  *spidev = spi_get_drvdata(spi);

    /* make sure ops on existing fds can abort cleanly */
    spin_lock_irq(&spidev->spi_lock);
    spidev->spi = NULL;
    spi_set_drvdata(spi, NULL);
    spin_unlock_irq(&spidev->spi_lock);

    /* prevent new opens */
    mutex_lock(&device_list_lock);
    list_del(&spidev->device_entry);
    device_destroy(spidev_class, spidev->devt);
    clear_bit(MINOR(spidev->devt), minors);
    if (spidev->users == 0)
        kfree(spidev);
    mutex_unlock(&device_list_lock);

    return 0;
}

static const struct of_device_id spidev_dt_ids[] = {
    { .compatible = "actions,spidev" },
    {},
};

MODULE_DEVICE_TABLE(of, spidev_dt_ids);

static struct spi_driver spidev_spi_driver = {
    .driver = {
        .name =     "spidev",
        .owner =    THIS_MODULE,
        .of_match_table = of_match_ptr(spidev_dt_ids),
    },
    .probe =    spidev_probe,
    .remove =   spidev_remove,

    /* NOTE:  suspend/resume methods are not necessary here.
     * We don't do anything except pass the requests to/from
     * the underlying controller.  The refrigerator handles
     * most issues; the controller driver handles the rest.
     */
};

/*-------------------------------------------------------------------------*/

static int __init spidev_init(void)
{
    int status;

    /* Claim our 256 reserved device numbers.  Then register a class
     * that will key udev/mdev to add/remove /dev nodes.  Last, register
     * the driver which manages those device numbers.
     */
    BUILD_BUG_ON(N_SPI_MINORS > 256);
    status = register_chrdev(SPIDEV_MAJOR, "spi", &spidev_fops);
    if (status < 0)
        return status;

    spidev_class = class_create(THIS_MODULE, "spidev");
    if (IS_ERR(spidev_class)) {
        unregister_chrdev(SPIDEV_MAJOR, spidev_spi_driver.driver.name);
        return PTR_ERR(spidev_class);
    }

    status = spi_register_driver(&spidev_spi_driver);
    if (status < 0) {
        class_destroy(spidev_class);
        unregister_chrdev(SPIDEV_MAJOR, spidev_spi_driver.driver.name);
    }
    return status;
}
module_init(spidev_init);

static void __exit spidev_exit(void)
{
    spi_unregister_driver(&spidev_spi_driver);
    class_destroy(spidev_class);
    unregister_chrdev(SPIDEV_MAJOR, spidev_spi_driver.driver.name);
}
module_exit(spidev_exit);

MODULE_AUTHOR("leo zhang, <leo.zhang@actekmicro.com>");
MODULE_DESCRIPTION("ActduinoTest spi test driver");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:ActduinoTest");
