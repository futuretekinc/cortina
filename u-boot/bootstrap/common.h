#ifndef __COMMON_H__
#define __COMMON_H__
#include "registers.h"

#define _GETS_OK        1
#define _GETS_TIMEOUT   2
#define _GETS_CTRLC     3

#define IO_WRITE(addr, val) (*(volatile unsigned int *)(addr) = (val))
#define IO_READ(addr) (*(volatile unsigned int *)(addr))

#define wmb()	asm volatile ("" : : : "memory")

#define POST_DDR_MEM_TYPE    (0x0)
#define ddr_mem_p            ((unsigned int *)POST_DDR_MEM_TYPE)

void post_cli(void);
void do_show_version(void);
int cs75xx_sdram_init(int mem_type);
int cs75xx_mem_test(int mem_type);


int timer_init(void);
void reset_timer(void);
unsigned long get_timer(unsigned long base_ticks);
void udelay (unsigned long usec);
void reset_timer_masked (void);
unsigned long get_timer_masked (void);
void udelay_masked (unsigned long usec);
unsigned long long get_ticks(void);
unsigned long get_tbclk (void);
void mmu_setup(void);
void icache_enable(void);
void icache_disable(void);
void dcache_enable(void);
void dcache_disable(void);
int user_escape(void);
int post_cmd_in_function(char *command);

struct platform_clk {
	unsigned int	cpu_clk;
	unsigned int	apb_clk;
	unsigned int	axi_clk;
};

void get_platform_clk(struct platform_clk *clk);


typedef struct {
   char *cmd;
   char *help_msg;
   int (*func_exe)(int argc, char **argv);
} cmd_t;

#endif
