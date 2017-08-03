/***********************************************************************/
/* This file contains unpublished documentation and software           */
/* proprietary to Cortina Systems Incorporated. Any use or disclosure, */
/* in whole or in part, of the information in this file without a      */
/* written consent of an officer of Cortina Systems Incorporated is    */
/* strictly prohibited.                                                */
/* Copyright (c) 2009 by Cortina Systems Incorporated.                 */
/***********************************************************************/
#include "config.h"
#include "common.h"
#include "registers.h"

#define CONFIG_SYS_TIMERBASE	PER_TMR_LD1

/*
 * U-Boot expects a 32 bit timer, running at CONFIG_SYS_HZ
 * CONFIG_SYS_HZ is NOT configurable but constant 1000 Hz
 */

/* Keep total timer count to avoid losing decrements < div_timer */
static unsigned long long total_count = 0;
static unsigned long long lastdec;	 /* Timer reading at last call */

/* CONFIG_SYS_HZ = CONFIG_SYS_HZ_CLOCK/(div_clock * div_timer) */
static unsigned long timestamp;		/* U-Boot ticks since startup */

#define READ_TIMER (*(volatile unsigned long *)(CONFIG_SYS_TIMERBASE+8))
#define TIMER_LOAD_VAL	0xffffffff

/* all function return values in U-Boot ticks i.e. (1/CONFIG_SYS_HZ) sec
 *  - unless otherwise stated
 */

/* starts up a counter
 * - the Integrator/CP timer can be set up to issue an interrupt */
int timer_init(void)
{
	/* Load timer with initial value */
	*(volatile unsigned long *)(CONFIG_SYS_TIMERBASE + 0) = TIMER_LOAD_VAL;
	*(volatile unsigned long *)(CONFIG_SYS_TIMERBASE + 4) = 0x000000C0;

	/* init the timestamp */
	total_count = 0ULL;
	reset_timer_masked();

	return (0);
}

/*
 * timer without interrupts
 */
void reset_timer(void)
{
	reset_timer_masked();
}

/*
 * This function must return milliseconds
 */
unsigned long get_timer(unsigned long base_ticks)
{
	return get_timer_masked () - base_ticks;
}

void set_timer(unsigned long ticks)
{
	timestamp = ticks;
}

/* delay usec useconds */
void udelay (unsigned long usec)
{
	long tmo;
	unsigned long now, last = READ_TIMER;

	tmo  = usec * (CONFIG_SYS_HZ / 1000000);

	while (tmo > 0) {
		now = READ_TIMER;
		if (last < now) /* timer overflow */
			tmo -= TIMER_LOAD_VAL + last - now;
		else
			tmo -= last - now;
		last = now;
	}
}

void reset_timer_masked (void)
{
	/* capure current decrementer value    */
	lastdec = READ_TIMER / (CONFIG_SYS_HZ / CONFIG_SYS_HZ);
	/* start "advancing" time stamp from 0 */
	timestamp = 0L;
}

/* converts the timer reading to U-Boot ticks	       */
/* the timestamp is the number of ticks since reset    */

unsigned long get_timer_masked (void)
{
	/* current tick value @ 1 tick per msec */
	unsigned long now = READ_TIMER / (CONFIG_SYS_HZ / CONFIG_SYS_HZ);

    if (lastdec < now)    /* count down timer underflow */
	    timestamp+= TIMER_LOAD_VAL/(CONFIG_SYS_HZ/CONFIG_SYS_HZ)-now+lastdec;
    else
        timestamp += lastdec - now;

	lastdec = now;

	return timestamp/(CONFIG_SYS_HZ/1000);
}

/* waits specified delay value and resets timestamp */
void udelay_masked (unsigned long usec)
{
	udelay(usec);
#ifdef CONFIG_ARCH_CINTEGRATOR
	reset_timer_masked();
#endif
}

/*
 * This function is derived from PowerPC code (read timebase as long long).
 * On ARM it just returns the timer value.
 */
unsigned long long get_ticks(void)
{
	return (unsigned long long )get_timer(0);
}

/*
 * Return the timebase clock frequency
 * i.e. how often the timer decrements
 */
unsigned long get_tbclk (void)
{
	return CONFIG_SYS_HZ;
}

