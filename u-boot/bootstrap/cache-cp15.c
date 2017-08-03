/*
 * (C) Copyright 2002
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <system.h>
#include <config.h>

static unsigned int __attribute__((aligned(16384))) page_table[4096];

#if !(defined(CONFIG_SYS_NO_ICACHE) && defined(CONFIG_SYS_NO_DCACHE))
static void cp_delay (void)
{
	volatile int i;

	/* copro seems to need some delay between reading and writing */
	for (i = 0; i < 100; i++)
		nop();
}

void mmu_setup(void)
{
	int i, j;
	unsigned int reg;

	/* Set up an identity-mapping for all 4GB, rw for everyone */
	for (i = 0; i < 4096; i++)
		page_table[i] = i << 20 | (3 << 10) | 0x12;

	/* Then, enable cacheable and bufferable for RAM only */
	for (j = 0; j < CONFIG_NR_DRAM_BANKS; j++) {
		for (i = PHYS_SDRAM_1 >> 20;
			i < (PHYS_SDRAM_1 + PHYS_SDRAM_1_SIZE) >> 20;
			i++) {
			page_table[i] = i << 20 | (3 << 10) | 0x1e; /* C=1 B=1 */
		}
	}

	/* Copy the page table address to cp15 */
	asm volatile("mcr p15, 0, %0, c2, c0, 0"
		     : : "r" (page_table) : "memory");
		     	
	/* Set the access control to all-supervisor */
	asm volatile("mcr p15, 0, %0, c3, c0, 0"
		     : : "r" (~0));
		     	
	/* and enable the mmu */
	reg = get_cr();	/* get control reg. */
	cp_delay();
	set_cr(reg | CR_M);
}

/* cache_bit must be either CR_I or CR_C */
static void cache_enable(unsigned int cache_bit)
{
	unsigned int reg;

	reg = get_cr();	/* get control reg. */
	cp_delay();
	set_cr(reg | cache_bit);
}

/* cache_bit must be either CR_I or CR_C */
static void cache_disable(unsigned int cache_bit)
{
	unsigned int reg;

	reg = get_cr();
	cp_delay();
	set_cr(reg & ~cache_bit);
}

#else

void mmu_setup(void)
{
	return;
}	

static void cache_enable(unsigned int cache_bit)
{
	return;
}

static void cache_disable(unsigned int cache_bit)
{
	return;
}

#endif

#ifdef CONFIG_SYS_NO_ICACHE
void icache_enable (void)
{
	return;
}

void icache_disable (void)
{
	return;
}

int icache_status (void)
{
	return 0;					/* always off */
}
#else
void icache_enable(void)
{
	cache_enable(CR_I);
}

void icache_disable(void)
{
	cache_disable(CR_I);
}

int icache_status(void)
{
	return (get_cr() & CR_I) != 0;
}
#endif

#ifdef CONFIG_SYS_NO_DCACHE
void dcache_enable (void)
{
	return;
}

void dcache_disable (void)
{
	return;
}

int dcache_status (void)
{
	return 0;					/* always off */
}
#else
void dcache_enable(void)
{
	cache_enable(CR_C);
}

void dcache_disable(void)
{
	cache_disable(CR_C);
}

int dcache_status(void)
{
	return (get_cr() & CR_C) != 0;
}
#endif
