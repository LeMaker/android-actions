#ifndef __OWL_IO_H__
#define __OWL_IO_H__
#include <asm/io.h>

static inline void owl_clrsetbits(unsigned long reg, unsigned int clear, 
		unsigned int set)
{
	unsigned int val;

	val = readl(reg);
	val &= ~clear;
	val |= set;
	writel(val, reg);
}

#endif /* __OWL_IO_H__ */
