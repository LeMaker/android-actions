/*
 * hdmi_cecc
 *
 * HDMI OWL IP driver Library
 *
 * Copyright (C) 2014 Actions Corporation
 * Author: HaiYu Huang  <huanghaiyu@actions-semi.com>
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

#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/poll.h>
#include <linux/miscdevice.h>
#include <linux/clk.h>
#include <linux/sched.h>
#include <linux/platform_device.h>
#include <linux/interrupt.h>

#include "cec.h"
#include "cec_reg.h"

#define CEC_IOC_MAGIC        'c'
#define CEC_IOC_SETLADDR     _IOW(CEC_IOC_MAGIC, 0, unsigned int)

#define VERSION   "1.0" /* Driver version number */
#define CEC_MINOR 243	/* Major 10, Minor 242, /dev/cec */


#define CEC_STATUS_TX_BYTES         (1<<0)
#define CEC_STATUS_TX_ERROR         (1<<5)
#define CEC_STATUS_TX_DONE          (1<<6)
#define CEC_STATUS_TX_TRANSFERRING  (1<<7)

#define CEC_STATUS_RX_BYTES         (1<<8)
#define CEC_STATUS_RX_ERROR         (1<<13)
#define CEC_STATUS_RX_DONE          (1<<14)
#define CEC_STATUS_RX_TRANSFERRING  (1<<15)



/* CEC Rx buffer size */
#define CEC_RX_BUFF_SIZE            16
/* CEC Tx buffer size */
#define CEC_TX_BUFF_SIZE            16

static atomic_t hdmi_on = ATOMIC_INIT(0);
static DEFINE_MUTEX(cec_lock);
struct clk *hdmi_cec_clk;

static int hdmi_cec_open(struct inode *inode, struct file *file)
{
	int ret = 0;

	mutex_lock(&cec_lock);
	clk_enable(hdmi_cec_clk);

	if (atomic_read(&hdmi_on)) {
		hdmi_cec_dbg("do not allow multiple open for tvout cec\n");
		ret = -EBUSY;
		goto err_multi_open;
	} else
		atomic_inc(&hdmi_on);
	
	hdmi_cec_hw_init();
	
	hdmi_cec_reset();
	
	hdmi_cec_set_rx_state(STATE_RX);
	
	hdmi_cec_unmask_rx_interrupts();
	
	hdmi_cec_enable_rx();

err_multi_open:
	mutex_unlock(&cec_lock);

	return ret;
}

static int hdmi_cec_release(struct inode *inode, struct file *file)
{
	atomic_dec(&hdmi_on);

	hdmi_cec_mask_tx_interrupts();
	hdmi_cec_mask_rx_interrupts();

	clk_disable(hdmi_cec_clk);
	clk_put(hdmi_cec_clk);

	return 0;
}

static ssize_t hdmi_cec_read(struct file *file, char __user *buffer,
			size_t count, loff_t *ppos)
{
	ssize_t retval;
	int i = 0;
	unsigned long spin_flags;
	
	if (wait_event_interruptible(cec_rx_struct.waitq,
			atomic_read(&cec_rx_struct.state) == STATE_DONE)) {
		return -ERESTARTSYS;
	}
	spin_lock_irqsave(&cec_rx_struct.lock, spin_flags);

	if (cec_rx_struct.size > count) {
		spin_unlock_irqrestore(&cec_rx_struct.lock, spin_flags);

		return -1;
	}

#if 0
	hdmi_cec_dbg("hdmi_cec_read size 0x%x: ",cec_rx_struct.size);
	for(i = 0 ; i < cec_rx_struct.size ; i++)
	{
		printk("0x%x ",cec_rx_struct.buffer[i]);
	}
#endif
 
	if (copy_to_user(buffer, cec_rx_struct.buffer, cec_rx_struct.size)) {
		spin_unlock_irqrestore(&cec_rx_struct.lock, spin_flags);
		printk(KERN_ERR " copy_to_user() failed!\n");
		return -EFAULT;
	}

	retval = cec_rx_struct.size;

	hdmi_cec_set_rx_state(STATE_RX);
	
	spin_unlock_irqrestore(&cec_rx_struct.lock, spin_flags);

	return retval;
}

static ssize_t hdmi_cec_write(struct file *file, const char __user *buffer,
			size_t count, loff_t *ppos)
{
	int i = 0;
	char *data;

	/* check data size */
	if (count > CEC_TX_BUFF_SIZE || count == 0){
		printk(KERN_ERR" hdmi_cec_write count -- %d line %d error \n",count,__LINE__);
		return -1;
	}

	data = kmalloc(count, GFP_KERNEL);

	if (!data) {
		printk(KERN_ERR " kmalloc() failed!\n");
		return -1;
	}

	if (copy_from_user(data, buffer, count)) {
		printk(KERN_ERR " copy_from_user() failed!\n");
		kfree(data);
		return -EFAULT;
	}
#if 0
	hdmi_cec_dbg("hdmi_cec_write size 0x%x: ",count);
	for(i = 0 ; i < count ; i++)
	{
		printk("0x%x\n",buffer[i]);
	}
#endif	
	/*disable rx  switch to tx mode*/
	hdmi_cec_disable_rx();

	hdmi_cec_copy_packet(data, count);
	
	kfree(data);

	/* wait for interrupt */

	if (wait_event_interruptible(cec_tx_struct.waitq,
		atomic_read(&cec_tx_struct.state)
		!= STATE_TX)) {

		return -ERESTARTSYS;
	}

	hdmi_cec_disable_tx();
	
	/*cec_reset*/
	hdmi_cec_reset();
	
	/*switch to rx mode*/	
	hdmi_cec_unmask_rx_interrupts(); 	
		
	hdmi_cec_enable_rx();
	
	if (atomic_read(&cec_tx_struct.state) == STATE_ERROR){
		//printk(KERN_ERR"cec_tx_struct.state : STATE_ERROR \n");
		return -1;
	}

	return count;
}
static long hdmi_cec_ioctl(struct file *file, unsigned int cmd,
						unsigned long arg)

{
	u32 laddr;
	hdmi_cec_dbg(" hdmi_cec_ioctl  cmd 0x%x \n",cmd);
	switch (cmd) {
	case CEC_IOC_SETLADDR:
		
		if (get_user(laddr, (u32 __user *) arg))
			return -EFAULT;
		hdmi_cec_dbg(" hdmi_cec_ioctl  CEC_IOC_SETLADDR 0x%x \n",
					laddr);
		hdmi_cec_set_addr(laddr);
		break;

	default:
		return -EINVAL;
	}

	return 0;
}

static u32 hdmi_cec_poll(struct file *file, poll_table *wait)
{
	poll_wait(file, &cec_rx_struct.waitq, wait);
	if (atomic_read(&cec_rx_struct.state) == STATE_DONE)
		{
			hdmi_cec_dbg("owl_hdmi_cec_poll ok \n");
		return POLLIN | POLLRDNORM;
		}
	return 0;
}

static const struct file_operations cec_fops = {
	.owner   = THIS_MODULE,
	.open    = hdmi_cec_open,
	.release = hdmi_cec_release,
	.read    = hdmi_cec_read,
	.write   = hdmi_cec_write,
	.unlocked_ioctl = hdmi_cec_ioctl,
	.poll    = hdmi_cec_poll,
};

static struct miscdevice cec_misc_device = {
	.minor = CEC_MINOR,
	.name  = "CEC",
	.fops  = &cec_fops,
};

void hdmi_cec_irq_handler()
{

	u32 status = 0;
	status = hdmi_cec_get_status();
	if (status & CEC_STATUS_TX_DONE) {
		if (status & CEC_STATUS_TX_ERROR) {
			hdmi_cec_dbg(" CEC_STATUS_TX_ERROR!\n");
			hdmi_cec_set_tx_state(STATE_ERROR);
		} else {
			hdmi_cec_dbg(" CEC_STATUS_TX_DONE!\n");
			hdmi_cec_set_tx_state(STATE_DONE);
		}

		hdmi_cec_clr_pending_tx();

		wake_up_interruptible(&cec_tx_struct.waitq);
	}

	if (status & CEC_STATUS_RX_DONE) {
		if (status & CEC_STATUS_RX_ERROR) {
			hdmi_cec_dbg(" CEC_STATUS_RX_ERROR!\n");
			hdmi_cec_rx_reset();
			hdmi_cec_unmask_rx_interrupts();
			hdmi_cec_enable_rx();
		} else {
			u32 size;
			u8 header;

			hdmi_cec_dbg(" CEC_STATUS_RX_DONE!\n");

			/* copy data from internal buffer */
			size = (status >> 8) & 0x0f;

			header = hdmi_cec_get_rx_header();
			cec_rx_struct.buffer[0] = header;

			spin_lock(&cec_rx_struct.lock);
			
			hdmi_cec_get_rx_buf(size, cec_rx_struct.buffer);

			cec_rx_struct.size = size + 1;/*includ 1 byte header*/

			hdmi_cec_set_rx_state(STATE_DONE);

			spin_unlock(&cec_rx_struct.lock);
			
			/*after receive data reset rx*/
			hdmi_cec_rx_reset();
			hdmi_cec_unmask_rx_interrupts(); 
			hdmi_cec_enable_rx();
		}

		/* clear interrupt pending bit */
		hdmi_cec_clr_pending_rx();

		wake_up_interruptible(&cec_rx_struct.waitq);
	}

	return IRQ_HANDLED;
}
EXPORT_SYMBOL(hdmi_cec_irq_handler);
static int hdmi_cec_probe(struct platform_device *pdev)
{
	struct s5p_platform_cec *pdata;
	u8 *buffer;
	int ret;
	struct resource *res;

	hdmi_cec_mem_probe(pdev);	
	if (misc_register(&cec_misc_device)) {
		printk(KERN_WARNING " Couldn't register device 10, %d.\n",
			CEC_MINOR);
			return -EBUSY;
		}	

	spin_lock_init(&cec_rx_struct.lock);
	init_waitqueue_head(&cec_rx_struct.waitq);	
	init_waitqueue_head(&cec_tx_struct.waitq);

	buffer = kmalloc(CEC_TX_BUFF_SIZE, GFP_KERNEL);

	if (!buffer) {
		printk(KERN_ERR " kmalloc() failed!\n");
		misc_deregister(&cec_misc_device);
		return -EIO;
	}

	cec_rx_struct.buffer = buffer;

	cec_rx_struct.size   = 0;

	dev_info(&pdev->dev, "probe successful\n");

	return 0;
}

static int hdmi_cec_remove(struct platform_device *pdev)
{
	return 0;
}

#ifdef CONFIG_PM
static int hdmi_cec_suspend(struct platform_device *dev, pm_message_t state)
{
	return 0;
}

static int hdmi_cec_resume(struct platform_device *dev)
{
	return 0;
}
#else
#define hdmi_cec_suspend NULL
#define hdmi_cec_resume NULL
#endif

static struct of_device_id owl_hdmi_cec_of_match[] = {
	{
		.compatible = "actions,atm7059a-hdmi-cec",
	},
	{},
};

static struct platform_driver hdmi_cec_driver = {
	.probe		= hdmi_cec_probe,
	.remove		= hdmi_cec_remove,
	.suspend	= hdmi_cec_suspend,
	.resume		= hdmi_cec_resume,
	.driver		= {
		.name	= "atm7059a-hdmi-cec",
		.owner	= THIS_MODULE,
		.of_match_table = owl_hdmi_cec_of_match,
	},
};

static char banner[] __initdata =
	"Actions CEC for OWL Driver, (c) 2009 ACTIONS Electronics\n";

static int __init hdmi_cec_init(void)
{
	int ret;

	printk(banner);
	ret = platform_driver_register(&hdmi_cec_driver);
	if (ret) {
		printk(KERN_ERR "Platform Device Register Failed %d\n", ret);

		return -1;
	}
	hdmi_cec_dbg("platform_driver_register hdmi-cec \n");
	return 0;
}

static void __exit hdmi_cec_exit(void)
{
	kfree(cec_rx_struct.buffer);

	platform_driver_unregister(&hdmi_cec_driver);
}

module_init(hdmi_cec_init);
module_exit(hdmi_cec_exit);

