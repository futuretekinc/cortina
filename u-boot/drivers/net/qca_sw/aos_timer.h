
#ifndef _AOS_TIMER_H
#define _AOS_TIMER_H

#include "aos_types.h"
#ifdef KERNEL_MODULE
#include "aos_timer_pvt.h"
#else
#include "aos_timer_pvt.h"
#endif


typedef __aos_timer_t           aos_timer_t;


/*
 * Delay in microseconds
 */
static inline void
aos_udelay(int usecs)
{
    return __aos_udelay(usecs);
}

/*
 * Delay in milliseconds.
 */
static inline void
aos_mdelay(int msecs)
{
    return __aos_mdelay(msecs);
}


#endif

