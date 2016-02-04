/*
 * hdmi_cec_ctrl.c
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


#include <linux/io.h>
#include <linux/slab.h>
#include <linux/platform_device.h>
#include <linux/irqreturn.h>
#include <linux/stddef.h>

#include "cec_reg.h"
#include "cec.h"

#define CEC_MESSAGE_BROADCAST_MASK	0x0F
#define CEC_MESSAGE_BROADCAST		0x0F


static struct resource	*cec_mem;
void __iomem		*cec_base;

struct cec_rx_struct cec_rx_struct;
struct cec_tx_struct cec_tx_struct;

void hdmi_cec_hw_init(void)
{
	u32 reg;

	reg = readl(cec_base + HDMI_CECCR);
	reg &= 0xffffff;	        //clear bit31:24
    reg |= (1<<28);	//PreDiv = 33;timerdiv = 0.8MHz/0.04MHz;disable DAC test
	writel(reg, cec_base + HDMI_CECCR);
	
	reg = readl(cec_base + HDMI_CECRTCR);
    reg &= 0xfffffff0;
    reg |= 5;
    writel(reg,cec_base + HDMI_CECRTCR);    
    
    writel(0x8cbc2a51,cec_base + HDMI_CECRXTCR);
    
    
    //CECTxTCR Register
    writel(0x9420,cec_base + HDMI_CECTXTCR0); 
    writel(0x182424,cec_base + HDMI_CECTXTCR1);
    
    //enable CEC mode
    reg = readl(cec_base + HDMI_CECCR);
    reg |= (1<<30);
    reg |= (4<<24);
    writel(reg,cec_base + HDMI_CECCR);

    reg = readl(cec_base + CEC_DDC_HPD);
    reg |= 0x10;
    reg &= 0xfffffffe;
    writel(reg,cec_base + CEC_DDC_HPD);			//enable cec internal pull up resistor   
}

void hdmi_cec_enable_rx(void)
{
	u32 reg;

	reg = readl(cec_base + HDMI_CECRXCR);
	reg |= CES_RX_CTRL_ENABLE;
	writel(reg, cec_base + HDMI_CECRXCR);
}

/*we can't disable rx ,used reset replace disable rx*/
void hdmi_cec_disable_rx(void)
{
	u32 reg;	
	reg = readl(cec_base + HDMI_CECRXCR);

	reg |= CES_RX_CTRL_RESET;
	
	writel(reg, cec_base + HDMI_CECRXCR);
	
	while((readl(cec_base + HDMI_CECRXCR) & 0x4000) != 0);
}

void hdmi_cec_disable_tx(void)
{
	u32 reg;
	reg = readl(cec_base + HDMI_CECTXCR);
	reg &= ~CES_TX_CTRL_ENABLE;
	writel(reg, cec_base + HDMI_CECTXCR);
}

void hdmi_cec_enable_tx(void)
{
	u32 reg;
	reg = readl(cec_base + HDMI_CECTXCR);
	reg |= CES_TX_CTRL_ENABLE;
	writel(reg, cec_base + HDMI_CECTXCR);
}

void hdmi_cec_mask_rx_interrupts(void)
{
	u32 reg;

	reg = readl(cec_base + HDMI_CECRXCR);
	reg &= ~CES_RX_IRQ_ENABLE;		/*close cec rx interrupt*/
	reg &= ~CES_RX_IRQ_PENDDING;		
	writel(reg, cec_base + HDMI_CECRXCR);
}

void hdmi_cec_unmask_rx_interrupts(void)
{
	u32 reg;
	reg = readl(cec_base + HDMI_CECRXCR);
	reg |= CES_RX_IRQ_ENABLE;		/*enable cec rx interrupt*/
	reg |= CES_RX_IRQ_PENDDING;		/*set 1 reset cec rx interrupt*/
	writel(reg, cec_base + HDMI_CECRXCR);
}

void hdmi_cec_mask_tx_interrupts(void)
{
	u32 reg;
	reg = readl(cec_base + HDMI_CECTXCR);
	reg &= ~CES_TX_IRQ_ENABLE;
	reg &= ~CES_TX_IRQ_PENDDING;
	writel(reg, cec_base + HDMI_CECTXCR);

}

void hdmi_cec_unmask_tx_interrupts(void)
{
	u32 reg;
	reg = readl(cec_base + HDMI_CECTXCR);
	reg |= CES_TX_IRQ_ENABLE;
	reg |= CES_TX_IRQ_PENDDING;
	writel(reg, cec_base + HDMI_CECTXCR);
}

void hdmi_cec_reset(void)
{
	u32 reg;

	reg = readl(cec_base + HDMI_CECRXCR);
	
	reg |= CES_RX_CTRL_RESET;
	
	writel(reg, cec_base + HDMI_CECRXCR);
	
	while(readl(cec_base + HDMI_CECRXCR) & 0x4000);
	
	reg = readl(cec_base + HDMI_CECTXCR);
	
	reg |= CES_TX_CTRL_RESET;
	
	writel(reg, cec_base + HDMI_CECTXCR);
	
	while(readl(cec_base + HDMI_CECTXCR) & 0x4000);

}

void hdmi_cec_tx_reset(void)
{
	u32 reg;

	reg = readl(cec_base + HDMI_CECTXCR);
	
	reg |= CES_TX_CTRL_RESET;
	
	writel(reg, cec_base + HDMI_CECTXCR);
	
	while(readl(cec_base + HDMI_CECTXCR) & 0x4000);
}

void hdmi_cec_rx_reset(void)
{
	u32 reg;
	
	reg = readl(cec_base + HDMI_CECRXCR);
	
	reg |= CES_RX_CTRL_RESET;
	
	writel(reg, cec_base + HDMI_CECRXCR);
	
	while(readl(cec_base + HDMI_CECRXCR) & 0x4000);
}

void hdmi_cec_set_tx_state(enum cec_state state)
{
	atomic_set(&cec_tx_struct.state, state);
}

void hdmi_cec_set_rx_state(enum cec_state state)
{
	atomic_set(&cec_rx_struct.state, state);
}

void hdmi_cec_copy_packet(char *data, size_t count)
{
	int i = 1;
	u8 initiator;
	u8 destination;
	u32 reg;	
	
	initiator = ((data[0] >> 4) & 0x0f);
	destination = (data[0] & 0x0f);
	
	hdmi_cec_dbg(" %s count %d %d initiator %d destination %d \n",__FUNCTION__,count,__LINE__,initiator,destination);
	
	/*reset TX */
	hdmi_cec_tx_reset();	
	
	/*write data */
	while (i < count) {
		writel(data[i], cec_base + HDMI_CECTXDR);
		i++;
	}

    /*config destination*/
	reg = readl(cec_base + HDMI_CECTXCR);	
	
	reg &= ~CES_TX_CTRL_BCAST;
		
	reg |= (destination << 8);
		
	writel(reg, cec_base + HDMI_CECTXCR);
	
	/*config initiator*/
	reg = readl(cec_base + HDMI_CECCR);
	
	reg &= ~ (0x0f << 24);
	reg |= ((initiator & 0x0F) << 24);	
	
	writel(reg, cec_base + HDMI_CECCR);
	
	/*set count*/
	reg = readl(cec_base + HDMI_CECTXCR);
	reg &= ~0x0f;
	reg |= count;	
	writel(reg, cec_base + HDMI_CECTXCR);
	
	/*set tx */
	hdmi_cec_set_tx_state(STATE_TX);
	
	/*enable tx*/
	hdmi_cec_enable_tx();
	
	/*enable tx interrupts*/	
	hdmi_cec_unmask_tx_interrupts();	
}

void hdmi_cec_set_addr(u32 addr)
{
	u32 reg;
	hdmi_cec_dbg("hdmi_cec_set_addr addr %d \n ",addr);
	reg = readl(cec_base + HDMI_CECCR);
	
	reg &= ~ (0x0f << 24);
	reg |= ((addr & 0x0F) << 24);
	
	writel(reg, cec_base + HDMI_CECCR);
	
}

u32 hdmi_cec_get_status(void)
{
	u32 status = 0;

	status = (readl(cec_base + HDMI_CECTXCR) & 0xff);
	status |= (readl(cec_base + HDMI_CECRXCR) & 0xff) << 8;

	if((status & CES_TX_IRQ_PENDDING) != 0){
		status |= CES_TX_IRQ_PENDDING;
		if(!((readl(cec_base + HDMI_CECCR) >> 29) & 0x1))
		{
			status |= CES_TX_FIFO_RRROR;
		}
	}
	return status;
}

void hdmi_cec_clr_pending_tx(void)
{
	u32 reg;

	reg = readl(cec_base + HDMI_CECTXCR);

	reg |= CES_TX_IRQ_PENDDING;
	
	writel(reg,	cec_base + HDMI_CECTXCR);
}

void hdmi_cec_clr_pending_rx(void)
{

	u32 reg;
	reg = readl(cec_base + HDMI_CECRXCR);

	reg |= CES_RX_IRQ_PENDDING;
	
	writel(reg,	cec_base + HDMI_CECRXCR);
}
u8 hdmi_cec_get_rx_header(void)
{
	u8 initiator;
	u8 destination;
	u8 rx_header;

	initiator = (readl(cec_base + HDMI_CECRXCR) >> 8) & 0xf;
	
	if((readl(cec_base + HDMI_CECRXCR) >> 16) & 0x1)
		destination = 0xf;
		else
			destination = (readl(cec_base + HDMI_CECCR) >> 24) & 0xf;
			
			initiator = (readl(cec_base + HDMI_CECRXCR) >> 8) & 0xf;
	rx_header = (initiator << 4) | destination;
	hdmi_cec_dbg("rx_header: %x\n", rx_header);
	return rx_header;
}
void hdmi_cec_get_rx_buf(u32 size, u8 *buffer)
{
	u32 i = 1;
	u32 reg;
	u8 count = size + 1;
	while(readl(cec_base + HDMI_CECRXCR) & 0x1F)
	{
		if(i < count){
			buffer[i] = readl(cec_base + HDMI_CECRXDR);
			i++;
		}
	}
	
#if 0
	hdmi_cec_dbg("hdmi_cec_get_rx_buf size 0x%x: ",size);
	for(i = 0 ; i < size ; i++)
	{
		hdmi_cec_dbg(" 0x%x ",buffer[i]);
	}
#endif
	
	if ((readl(cec_base + HDMI_CECRXCR)>>7) & 0x1)                  //Rx EOM 
	{   
		reg = readl(cec_base + HDMI_CECRXCR);
	
		reg |= 1<<14;
		
		writel(reg, cec_base + HDMI_CECRXCR);

		while((readl(cec_base + HDMI_CECRXCR) & 0x4000) != 0);
	}	   
}

int __init hdmi_cec_mem_probe(struct platform_device *pdev)
{
	struct resource *cec_mem;
	int	ret = 0;
	hdmi_cec_dbg("%s %d \n",__FUNCTION__,__LINE__);
	dev_dbg(&pdev->dev, "%s\n", __func__);

	cec_mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);

	if (cec_mem == NULL) {
		dev_err(&pdev->dev,
			"failed to get memory region resource for cec\n");
		return -ENOENT;
	}	

	cec_base = ioremap(cec_mem->start, resource_size(cec_mem));

	if (cec_base == NULL) {
		dev_err(&pdev->dev,
			"failed to ioremap address region for cec\n");
		return -ENOENT;
	}

	return ret;
}

int __init hdmi_cec_mem_release(struct platform_device *pdev)
{
	iounmap(cec_base);

	if (cec_mem != NULL) {
		if (release_resource(cec_mem))
			dev_err(&pdev->dev,
				"Can't remove tvout drv !!\n");

		kfree(cec_mem);

		cec_mem = NULL;
	}

	return 0;
}
