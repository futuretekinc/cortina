/*
 *  linux/arch/arm/kernel/smp_scu.c
 *
 *  Copyright (C) 2002 ARM Ltd.
 *  All Rights Reserved
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/init.h>
#include <linux/io.h>

#include <asm/smp_scu.h>
#include <asm/cacheflush.h>
#include <asm/cputype.h>


#define SCU_CTRL		0x00
#define SCU_CONFIG		0x04
#define SCU_CPU_STATUS		0x08
#define SCU_INVALIDATE		0x0c
#define SCU_FPGA_REVISION	0x10

/*
 * Get the number of CPU cores from the SCU configuration
 */
unsigned int __init scu_get_core_count(void __iomem *scu_base)
{
	unsigned int ncores = __raw_readl(scu_base + SCU_CONFIG);
	return (ncores & 0x03) + 1;
}

/*
 * Enable the SCU
 */
void __init scu_enable(void __iomem *scu_base)
{
	u32 scu_ctrl;

	/* added by hkou 02/22/2012 */
#ifdef CONFIG_ARM_ERRATA_764369
	/* Cortex-A9 only */
	if ((read_cpuid(CPUID_ID) & 0xff0ffff0) == 0x410fc090) {
		scu_ctrl = __raw_readl(scu_base + 0x30);
		if (!(scu_ctrl & 1))
			__raw_writel(scu_ctrl | 0x1, scu_base + 0x30);
	}
#endif

	scu_ctrl = __raw_readl(scu_base + SCU_CTRL);
	/* already enabled? */
	if (scu_ctrl & 1)
		return;

	/* updated by hkou & ted 02/22/2012 */
	scu_ctrl |= 0x29;
	__raw_writel(scu_ctrl, scu_base + SCU_CTRL);

	/*
	 * Ensure that the data accessed by CPU0 before the SCU was
	 * initialised is visible to the other CPUs.
	 */
	flush_cache_all();
}
