#include "config.h"
#include "utils.h"
#include "common.h"
#include "serial_cs752x.h"

#define DCACHE_SIZE      (16 * 1024)
#define CACHE_LINE_SIZE  32
#define CACHE_LINE_WORDS (CACHE_LINE_SIZE/sizeof(unsigned int))

#define ITERATION             10
#define MOD_SZ                20
#define BYTE_SIZE             1
#define WORD_SIZE             2

#define DRAM_BREAK			2
#define DRAM_FAIL			1
#define DRAM_SUCCESS		0

int addr_test_walk_one_nocache(unsigned long start, unsigned long size);
int addr_test_own_address(unsigned long start, unsigned long size);
int movinv1(unsigned long iter, unsigned long start, unsigned long size, unsigned long p1, unsigned long p2);
int movinv32(unsigned long iter, unsigned long start, unsigned long size, unsigned long p1, unsigned long lb,
             unsigned long hb, int sval, int off);
int modtst(int offset, int iter, unsigned long start, unsigned long size, unsigned long p1, unsigned long p2);
int dram_sequential_access(int type, unsigned long start, unsigned long size, unsigned short pattern);
int ssn_inc_byte_test(unsigned long start, unsigned long size);
int ssn_inc_word_test(unsigned long start, unsigned long size);

void run_print_meter (void)
{
    static int mi = 0;

    serial_putc('\b');

    if (mi < 8) {
	serial_putc('|');
    } else if (mi < 16) {
	serial_putc('/');
    } else if (mi < 24) {
	serial_putc('-');
    } else {
	serial_putc('\\');
    }

    mi++;
    mi %= 32;

    return;
}

void init_print_meter(void)
{
    serial_putc(' ');
    return;
}

void clear_print_meter(void)
{
    serial_putc('\b');
    serial_putc(' ');
    serial_putc('\b');
    return;
}

int do_high_load_test_imp(unsigned int start, unsigned int len, int display_time) {
/*
    unsigned int time1_ts0=0;
    unsigned int time1_ts1=0;
    unsigned int time2_ts0=0;
    unsigned int time2_ts1=0;
*/    
    int len_by_words;
    int i;
    register unsigned int *p;

    len_by_words = len / sizeof(unsigned int);

    for (i = 0; i < CACHE_LINE_WORDS; i++) {
	p = (unsigned int *) start;
	p += i;
	while (p < (unsigned int *) (start + len)) {
	    // 32 writes
	    *p = (unsigned int) p;
	    p += CACHE_LINE_WORDS;
	    *p = (unsigned int) p;
	    p += CACHE_LINE_WORDS;
	    *p = (unsigned int) p;
	    p += CACHE_LINE_WORDS;
	    *p = (unsigned int) p;
	    p += CACHE_LINE_WORDS;
	    *p = (unsigned int) p;
	    p += CACHE_LINE_WORDS;
	    *p = (unsigned int) p;
	    p += CACHE_LINE_WORDS;
	    *p = (unsigned int) p;
	    p += CACHE_LINE_WORDS;
	    *p = (unsigned int) p;
	    p += CACHE_LINE_WORDS;
	    *p = (unsigned int) p;
	    p += CACHE_LINE_WORDS;
	    *p = (unsigned int) p;
	    p += CACHE_LINE_WORDS;
	    *p = (unsigned int) p;
	    p += CACHE_LINE_WORDS;
	    *p = (unsigned int) p;
	    p += CACHE_LINE_WORDS;
	    *p = (unsigned int) p;
	    p += CACHE_LINE_WORDS;
	    *p = (unsigned int) p;
	    p += CACHE_LINE_WORDS;
	    *p = (unsigned int) p;
	    p += CACHE_LINE_WORDS;
	    *p = (unsigned int) p;
	    p += CACHE_LINE_WORDS;
	    *p = (unsigned int) p;
	    p += CACHE_LINE_WORDS;
	    *p = (unsigned int) p;
	    p += CACHE_LINE_WORDS;
	    *p = (unsigned int) p;
	    p += CACHE_LINE_WORDS;
	    *p = (unsigned int) p;
	    p += CACHE_LINE_WORDS;
	    *p = (unsigned int) p;
	    p += CACHE_LINE_WORDS;
	    *p = (unsigned int) p;
	    p += CACHE_LINE_WORDS;
	    *p = (unsigned int) p;
	    p += CACHE_LINE_WORDS;
	    *p = (unsigned int) p;
	    p += CACHE_LINE_WORDS;
	    *p = (unsigned int) p;
	    p += CACHE_LINE_WORDS;
	    *p = (unsigned int) p;
	    p += CACHE_LINE_WORDS;
	    *p = (unsigned int) p;
	    p += CACHE_LINE_WORDS;
	    *p = (unsigned int) p;
	    p += CACHE_LINE_WORDS;
	    *p = (unsigned int) p;
	    p += CACHE_LINE_WORDS;
	    *p = (unsigned int) p;
	    p += CACHE_LINE_WORDS;
	    *p = (unsigned int) p;
	    p += CACHE_LINE_WORDS;
	    *p = (unsigned int) p;
	    p += CACHE_LINE_WORDS;

	}
    }

/*
    if (display_time) {
	unsigned int total_access; // estimate between dcache and ddr
	total_access = len * ((CACHE_LINE_WORDS/2) + CACHE_LINE_WORDS) ;
	serial_puts("timing the burst w/r loop\n");
	serial_puts(" time1: ");
	serial_put_hex(time1_ts1);
	serial_puts(":");
	serial_put_hex(time1_ts0);

	serial_puts(" time2: ");
	serial_put_hex(time2_ts1);
	serial_puts(":");
	serial_put_hex(time2_ts0);
	serial_puts("\n");

	serial_puts("estimated total memory accesses in bytes in burst r/w loop:");
	serial_put_hex(total_access);
	serial_puts("\n");

    }
*/

    p = (unsigned int *) start;
    while (p < (unsigned int *) (start + len)) {
	/* touch 8 cache lines */
	if ((unsigned int)p != p[0]) {
	    goto err;
	}
	p += CACHE_LINE_WORDS;
	if ((unsigned int)p != p[0]) {
	    goto err;
	}
	p += CACHE_LINE_WORDS;
	if ((unsigned int)p != p[0]) {
	    goto err;
	}
	p += CACHE_LINE_WORDS;
	if ((unsigned int)p != p[0]) {
	    goto err;
	}
	p += CACHE_LINE_WORDS;
	if ((unsigned int)p != p[0]) {
	    goto err;
	}
	p += CACHE_LINE_WORDS;
	if ((unsigned int)p != p[0]) {
	    goto err;
	}
	p += CACHE_LINE_WORDS;
	if ((unsigned int)p != p[0]) {
	    goto err;
	}
	p += CACHE_LINE_WORDS;
	if ((unsigned int)p != p[0]) {
	    goto err;
	}
	p += CACHE_LINE_WORDS;

	// do all other words in the previous 8 cache lines
	register unsigned int *p1;
	p1 = p - (CACHE_LINE_WORDS << 3);
	while (p1 < p) {
	    if ((unsigned int)p1 != p1[0]) {
		goto err;
	    }
	    p1++;
	}
    }

/*
    if (display_time) {
	serial_puts("timing the fast read verifying loop\n");
	serial_puts("  time1: ");
	serial_put_hex(time1_ts1);
	serial_puts(":");
	serial_put_hex(time1_ts0);

	serial_puts(" time2: ");
	serial_put_hex(time2_ts1);
	serial_puts(":");
	serial_put_hex(time2_ts0);
	serial_puts("\n");
    }
*/
    return 0; // only do the fast test

 err:
    serial_puts("Error expected ");
    serial_put_hex((unsigned int)p);
    serial_puts(" got ");
    serial_put_hex(p[0]);
    serial_puts("\n");
    return 1;
}

int do_high_load_test_new(char **argv, int argc)
{
    unsigned int st_addr, len;
    unsigned int test_len = 0x10000; /* 64K */

    serial_puts("\nMemory Test St Address ");
    serial_puts(argv[1]);
    serial_puts(" Len ");
    serial_puts(argv[2]);
    serial_puts(": ");

    st_addr = (unsigned int) iros_strtol(argv[1]);
    len = (unsigned int) iros_strtol(argv[2]);

    init_print_meter();

    while (len >= test_len) {
	run_print_meter();
	if (do_high_load_test_imp(st_addr, test_len, 0)) {
	    clear_print_meter();
	    serial_puts("Failed\n");
	    return 0;
	}
	len -= test_len;
	st_addr += test_len;
    }
    clear_print_meter();
    serial_puts("Passed\n");

    return 1;
}


#ifdef CONFIG_CORTINA_DIAG

static unsigned int SEED_X;
static unsigned int SEED_Y;

unsigned long rand (void)
{
   static unsigned int a = 18000, b = 30903;

   SEED_X = a * (SEED_X & 65535) + (SEED_X>>16);
   SEED_Y = b * (SEED_Y & 65535) + (SEED_Y>>16);

   return ((SEED_X << 16) + (SEED_Y & 65535));
}


void rand_seed( unsigned int seed1, unsigned int seed2)
{
   SEED_X =+ seed1;
   SEED_Y =+ seed2;
}

void show_error(unsigned long address, unsigned long good, unsigned long bad)
{
    serial_puts(" the error address is ");
    serial_put_hex((unsigned int)address);
    serial_puts("\n");

    serial_puts(" the expect pattern is ");
    serial_put_hex((unsigned int)good);
    serial_puts("\n");

    serial_puts(" the error pattern is ");
    serial_put_hex((unsigned int)bad);
    serial_puts("\n");
}

void enable_cache(void)
{
	dcache_enable();
	return;
}

void disable_cache(void)
{
	dcache_disable();
	return;
}

int ram_item_test(char **argv, int arg)
{
    unsigned int 	item; // = (unsigned int) arg[0].value;
    unsigned long 	address; // = (unsigned long) arg[1].value;
    unsigned long 	size; // = (unsigned long) arg[2].value;
    unsigned long 	p0, p1, p2;
    unsigned short 	pattern;
	unsigned long 	sp1, sp2;
    int i, j, ret, cache;

    item = (unsigned int) iros_strtol(argv[1]);
    address = (unsigned int) iros_strtol(argv[2]);
    size = (unsigned int) iros_strtol(argv[3]);

    switch(item)
    {
	    case 0: /* Address test, walking ones */
		disable_cache();
		ret = addr_test_walk_one_nocache(address, size);
		enable_cache();
		break;

	    case 1: /* Address test, own address */
		disable_cache();
		    ret=addr_test_own_address(address, size);
		    enable_cache();
	    break;

	    /* Now do the testing according to the selected pattern */
	    case 2:	/* Moving inversions, ones and zeros sequential */
		cache = 0;
		while(cache<2)
		{
		    if(cache==0)
		    {
			disable_cache();
		    }
		    else
		    {
			enable_cache();
		    }
			p1 = 0;
			p2 = ~p1;
			ret = movinv1(ITERATION,address,size,p1,p2);
			if(ret == DRAM_FAIL)
			{
			goto test_done;
		    }
			/* Switch patterns */
			p2 = p1;
			p1 = ~p2;
			ret = movinv1(ITERATION,address,size,p1,p2);
			if(ret == DRAM_FAIL)
			{
			goto test_done;
		    }
			cache++;
		} // end of while loop
		break;

	    case 3: /* Moving inversions, 8 bit wide walking ones and zeros. */
		cache = 0;
		while(cache<2)
		{
		    if(cache==0)
		    {
			disable_cache();
		    }
		    else
		    {
			enable_cache();
		    }
			p0 = 0x80;
			for (i=0; i<8; i++, p0=p0>>1)
			{
				p1 = p0 | (p0<<8) | (p0<<16) | (p0<<24);
				p2 = ~p1;
				ret=movinv1(ITERATION,address,size,p1,p2);
			if(ret==DRAM_FAIL)
			{
			    goto test_done;
			}
				/* Switch patterns */
				p2 = p1;
				p1 = ~p2;
				ret=movinv1(ITERATION,address,size,p1,p2);
				if(ret==DRAM_FAIL)
				{
				    goto test_done;
				}
			}
			cache++;
		} // end of while loop
		break;

	    case 4: /* Moving inversions, 32 bit shifting pattern, very long */
		cache = 0;
		while(cache<2)
		{
		    if(cache==0)
		    {
			disable_cache();
		    }
		    else
		    {
			enable_cache();
		    }
			for (i=0, p1=1; p1; p1=p1<<1, i++)
			{
				ret=movinv32(ITERATION,address,size,p1,1,0x80000000,0,i);
				if(ret==DRAM_FAIL)
				{
				    goto test_done;
				}
				ret=movinv32(ITERATION,address,size,~p1,0xfffffffe,0x7fffffff,1,i);
				if(ret==DRAM_FAIL)
				{
				    goto test_done;
				}
			}
			cache++;
		    } // end of while loop
		break;

		case 5: /* Moving inversions, random pattern */
		sp1 = 521288629;
		sp2 = 362436069;
		    rand_seed(sp1, sp2);

			for (i=0; i < ITERATION*2 ; i++) {
				p1 = rand();
				p2 = ~p1;
				movinv1(2,address,size,p1,p2);
			}
			break;

	    case 6: /* Modulo X check, all ones and zeros */
		enable_cache();
		    p1=0;
		    for (i=0; i<MOD_SZ; i++)
		    {
			p2 = ~p1;
			ret=modtst(i,ITERATION,address,size,p1,p2);
		if(ret==DRAM_FAIL)
		{
		    goto test_done;
		}
			/* Switch patterns */
			p2 = p1;
			p1 = ~p2;
			ret = modtst(i,ITERATION,address,size,p1,p2);
			if(ret == DRAM_FAIL)
			{
				goto test_done;
			    }
		    }
		break;

	    case 7: /* Modulo X check, 8 bit pattern */
	    enable_cache();
		    p0 = 0x80;
		    for (j=0; j<8; j++, p0=p0>>1)
		    {
			p1 = p0 | (p0<<8) | (p0<<16) | (p0<<24);
			for (i=0; i<MOD_SZ; i++)
			{
				p2 = ~p1;
				ret = modtst(i,ITERATION,address,size,p1,p2);
				if(ret == DRAM_FAIL)
				{
				    goto test_done;
				}

				/* Switch patterns */
				p2 = p1;
				p1 = ~p2;
				ret = modtst(i,ITERATION,address,size,p1,p2);
				if(ret == DRAM_FAIL)
				{
				    goto test_done;
				}
			} // end of i loop
		    } // end of j loop
		break;

	    case 17:  /* byte sequence access */
		cache = 0;
		while(cache<2)
		{
		    if(cache==0)
		    {
			disable_cache();
		    }
		    else
		    {
			enable_cache();
		    }

		    pattern = 0x5555;
			ret = dram_sequential_access(BYTE_SIZE,address,size,pattern);
			if(ret == DRAM_FAIL)
			{
				goto test_done;
			    }
			    pattern = 0xAAAA;
			ret = dram_sequential_access(BYTE_SIZE,address,size,pattern);
			if(ret == DRAM_FAIL)
			{
				goto test_done;
			    }

			cache++;
		    } // end of while loop
		break;

	    case 18: /* word sequence access */
		cache = 0;
		while(cache<2)
		{
		    if(cache == 0)
		    {
			disable_cache();
		    }
		    else
		    {
			enable_cache();
		}
		    pattern = 0x5555;
			ret = dram_sequential_access(WORD_SIZE,address,size,pattern);
			if(ret == DRAM_FAIL)
			{
				goto test_done;
			    }
			    pattern = 0xAAAA;
			ret = dram_sequential_access(WORD_SIZE,address,size,pattern);
			if(ret == DRAM_FAIL)
			{
				goto test_done;
		    }
			cache++;
		    } // end of while loop
		break;

		case 8: /* Modulo X check, Random pattern (test #10) */
           	sp1 = 521288629;
           	sp2 = 362436069;
		    rand_seed(sp1, sp2);
			for (j=0; j<ITERATION; j++) {
				p1 = rand();
				for (i=0; i<MOD_SZ; i++) {
					p2 = ~p1;
					//modtst(i, 2, p1, p2);
				ret=modtst(i,2,address,size,p1,p2);
				if(ret==DRAM_FAIL)
				{
				    goto test_done;
				}

					/* Switch patterns */
					//modtst(i, 2, p2, p1);
				ret=modtst(i,2,address,size,p2,p1);
				if(ret==DRAM_FAIL)
				{
				    goto test_done;
				}
				}
			}
			break;

		case 10:
			ret = ssn_inc_byte_test(address,size);
			break;

		case 11:
			ret = ssn_inc_word_test(address,size);
			break;

		default:
			break;


    } // end of switch

test_done:

    return ret;
}

int addr_test_walk_one_nocache(unsigned long start, unsigned long size)
{
	unsigned long i, len;
	volatile unsigned long *p, back;
	volatile unsigned long pattern, bad;

	serial_puts("verify addr_test_walk_one_nocache \n");
	p = (unsigned long *)start;
	len = size / sizeof(unsigned long);

	for (i=0; i<len; i++, p++)
	{
		for(pattern=1;pattern!=0;pattern<<=1)
		{
			*p=pattern;
			back = *p;
			if(back != pattern)
			{
				show_error((unsigned long)p, pattern, bad);
				return DRAM_FAIL;
			}
		} // end of pattern loop
		if((i % 0x40000) == 0)
		{
			//printf("address is %x \n", (unsigned long)p);
			serial_puts("address is ");
			serial_put_hex((unsigned int)p);
			serial_puts(" \n");
		}
		if((i % 0x100) == 0){
			if (user_escape() == 1)
				return DRAM_BREAK;
		}
	} // end of i loop

    return DRAM_SUCCESS;
}

/*
 * Memory address test, own address
 */
int addr_test_own_address(unsigned long start, unsigned long length)
{
	unsigned long i, size, bad;
	volatile unsigned long *p;

	serial_puts("<<<<<verify addr_test_own_address : >>>>> \n");

	/* Initialize memory with the initial pattern.  */
	p = (unsigned long *)start;
	size = length/sizeof(unsigned long);
	for (i=0; i<size; i++, p++)
	{
		*p = (unsigned long)p;
		if((i % 0x1000) == 0){
			if (user_escape() == 1)
				return DRAM_BREAK;
		}
	}

	/* Each address should have its own address */
	p = (unsigned long *)start;
	for (i=0; i<size; i++, p++)
	{
		if((bad = *p) != (unsigned long)p)
		{
			show_error((unsigned long)p, (unsigned long)p, bad);
			return DRAM_FAIL;
		}
		if((i % 0x1000) == 0){
			if (user_escape() == 1)
				return DRAM_BREAK;
		}
	} // end of for loop

	return DRAM_SUCCESS;
}

int movinv1(unsigned long iter, unsigned long start, unsigned long size, unsigned long p1, unsigned long p2)
{
	unsigned long i, j, len;
	volatile unsigned long *p, bad;

//    printf("<<<<<verify movinv1 pattern %x %x :>>>>> \n", p1, p2);
	serial_puts("<<<<<verify movinv1 pattern :");
	serial_put_hex((unsigned int)p1);
	serial_puts(" ");
	serial_put_hex((unsigned int)p2);
	serial_puts(">>>>> \n");

	/* Initialize memory with the initial pattern.  */
	p = (unsigned long *)start;
	len = size / sizeof(long);
	for (i = 0; i < len; i++, p++)
	{
		*p = p1;
		if((i % 0x1000) == 0){
			if (user_escape() == 1)
				return DRAM_BREAK;
		}
	}

	/* Do moving inversions test. Check for initial pattern and then
	 * write the complement for each memory location. Test from bottom
	 * up and then from the top down.  */

	for (i=0; i<iter; i++)
	{
		// from low address
		p = (unsigned long *)start;
		len = size / sizeof(long);
		for (j=0; j < len; j++,p++)
		{
			if ((bad=*p) != p1)
			{
				show_error((unsigned long)p, p1, bad);
				return DRAM_FAIL;
			}
			*p = p2;
		} // end of for j loop

		// from high address
		p = (unsigned long *)(start+size);
		len = size / sizeof(long);
		for (j=0, --p; j < len; j++, p--)
		{
			if ((bad=*p) != p2)
			{
				show_error((unsigned long)p, p2, bad);
			return DRAM_FAIL;
			}
			*p = p1;
			if((j % 0x1000) == 0){
				if (user_escape() == 1)
					return DRAM_BREAK;
			}
		}
		//printf("<<<<<movinv1 iteration %x pass : >>>>> \n", i);
		serial_puts("<<<<<movinv1 iteration ");
		serial_put_hex((unsigned int)i);
		serial_puts(" pass : >>>>> \n");
	} // end of iteration loop

	return DRAM_SUCCESS;
}

int movinv32(unsigned long iter, unsigned long start, unsigned long size, unsigned long p1, unsigned long lb, unsigned long hb, int sval, int off)
{
	unsigned long i, j, len;
	unsigned long n = 0;
	volatile unsigned long *p, bad;
	unsigned long pat, p3;
	int k;

//    printf("<<<<<verify movinv32 pattern %x : >>>>> \n", p1);
	serial_puts("<<<<<verify movinv32 pattern ");
	serial_put_hex((unsigned int)p1);
	serial_puts(" : >>>>> \n");

	p3 = sval << 31;

	/* Initialize memory with the initial pattern.  */
	p = (unsigned long *)start;
	k = off;
	pat = p1;
	len = size/sizeof(long);
	for(i=0; i<len; i++, p++)
	{
		*p = pat;
		if (++k >= 32)
		{
			pat = lb;
			k = 0;
		}
		else
		{
			pat = pat << 1;
			pat |= sval;
		}
		if((i % 0x1000) == 0){
			if (user_escape() == 1)
				return DRAM_BREAK;
		}
	} // end of for i loop

	/* Do moving inversions test. Check for initial pattern and then
	 * write the complement for each memory location. Test from bottom
	 * up and then from the top down.  */
	for (i=0; i<iter; i++)
	{
		p = (unsigned long *)start;
		k = off;
		pat = p1;
		len = size/sizeof(long);
		// form low address
		for(j=0; j<len; j++, p++)
		{
			if ((bad=*p) != pat)
			{
				show_error((unsigned long)p, pat, bad);
				return DRAM_FAIL;
			}

			*p = ~pat;

			if (++k >= 32)
			{
				pat = lb;
				k = 0;
			}
			else
			{
				pat = pat << 1;
				pat |= sval;
			}
			if((j % 0x1000) == 0){
				if (user_escape() == 1)
					return DRAM_BREAK;
			}
		} // end of for j loop

		/* Since we already adjusted k and the pattern this
		* code backs both up one step
		*/
		if (--k < 0)
			k = 31;


		for (pat = lb, n = 0; n < k; n++)
		{
			pat = pat << 1;
			pat |= sval;
		}
		k++;

		// form high address
		p = (unsigned long *)(start+size);
		len = size/sizeof(long);

		for(j = 0, --p; j < len; j++, p--)
		{
			if ((bad=*p) != ~pat)
			{
				show_error((unsigned long)p, ~pat, bad);
				return DRAM_FAIL;
			}

			*p = pat;
			if (--k <= 0)
			{
				pat = hb;
				k = 32;
			}
			else
			{
				pat = pat >> 1;
				pat |= p3;
			}
			if((j % 0x1000) == 0){
				if (user_escape() == 1)
					return DRAM_BREAK;
			}
		} // end of j loop
//		printf("movinv32 iteration %x pass : \n", i);
		serial_puts("movinv32 iteration ");
		serial_put_hex((unsigned int)i);
		serial_puts(" pass. \n");
	} // end of for iteration loop

	return DRAM_SUCCESS;
}

/*
 * Test all of memory using modulo X access pattern.
 */
int modtst(int offset, int iter, unsigned long start, unsigned long size, unsigned long p1, unsigned long p2)
{
	unsigned long j, k, l, len;
	volatile unsigned long *p, bad;

//    printf("<<<<<verify modtst pattern %x %x >>>>> \n", p1, p2);
	serial_puts("<<<<<verify modtst pattern ");
	serial_put_hex((unsigned int)p1);
	serial_puts(" ");
	serial_put_hex((unsigned int)p2);
	serial_puts(" >>>>> \n");

	/* Write every nth location with pattern */
	p = (unsigned long *)start;
	p += offset;
	len = size/sizeof(long);
	for(j = 0; j < len; j += MOD_SZ, p += MOD_SZ)
	{
		*p = p1;
		if((j % 0x1000) == 0){
			if (user_escape() == 1)
				return DRAM_BREAK;
		}
	}
	/* Write the rest of memory "iter" times with the pattern complement */
	for (l=0; l<iter; l++)
	{
		p = (unsigned long *)start;
		k = 0;
		for(j=0; j<len; j++, p++)
		{
			if (k != offset)
				*p = p2;

			if (++k > MOD_SZ-1)
				k = 0;
			if((j % 0x1000) == 0){
				if (user_escape() == 1)
					return DRAM_BREAK;
			}

		} // end of j loop
//		printf("modtst iteration %x \n", l);
		serial_puts("modtst iteration ");
		serial_put_hex((unsigned int)l);
		serial_puts("\n");
	} // end of iteration loop

	/* Now check every nth location */
	p = (unsigned long *)start;
	p += offset;
	for (j = 0; j < len; j += MOD_SZ, p += MOD_SZ)
	{
		if ((bad=*p) != p1)
		{
			show_error((unsigned long)p, p1, bad);
			return DRAM_FAIL;
		}
		if((j % 0x1000) == 0){
			if (user_escape() == 1)
				return DRAM_BREAK;
		}
	} // end of j loop

	return DRAM_SUCCESS;
}

/* DRAM sequence access */
int  dram_sequential_access(int type, unsigned long start, unsigned long size, unsigned short pattern)
{
	unsigned long  len, j;

//	printf("<<<<< dram_sequential_access type is %x : >>>>> \n", type);
	serial_puts("<<<<< dram_sequential_access type is ");
	serial_put_hex((unsigned int)type);
	serial_puts(" : >>>>> \n");

	if(type == BYTE_SIZE) {
		unsigned char  b = (unsigned char)(pattern & 0x00ff);
		unsigned char *p, bad;
		len = size;
		p = (unsigned char *)start;
		for(j=0; j<len; j++, p++)
		{
			*p = b;
			if((j % 0x1000) == 0){
				if (user_escape() == 1)
					return DRAM_BREAK;
			}
		}
		p = (unsigned char *)start;
		for (j=0; j < len; j++,p++)
		{
			if ((bad=*p) != b)
			{
				show_error((unsigned long)p, (unsigned long)b, (unsigned long)bad);
				return DRAM_FAIL;
			}
			*p=0;
			if((j % 0x1000) == 0){
				if (user_escape() == 1)
					return DRAM_BREAK;
			}
		} // end of for j loop
	} else {
		unsigned short  w = (unsigned short)(pattern & 0xffff);
		unsigned short *p, bad;
		len = size/sizeof(short);
		p = (unsigned short *)start;
		for(j=0; j<len; j++, p++){
			*p = w;
			if((j % 0x1000) == 0){
				if (user_escape() == 1)
					return DRAM_BREAK;
			}
		}
		p = (unsigned short *)start;
		for (j=0; j < len; j++,p++)
		{
			if ((bad=*p) != w)
			{
				show_error((unsigned long)p, (unsigned long)w, (unsigned long)bad);
				return DRAM_FAIL;
			}
			*p=0;
			if((j % 0x1000) == 0){
				if (user_escape() == 1)
					return DRAM_BREAK;
			}
		} // end of for j loop
	}

	return DRAM_SUCCESS;
}

int ssn_inc_byte_test(unsigned long start, unsigned long size)
{
    unsigned char 	*p, bad;
	unsigned long	i;

    p = (unsigned char *)start;
	for (i=0; i<size; i++,p++) {
		*p = (i % 0xff);
	}

    p = (unsigned char *)start;
	for (i=0; i<size; i++,p++) {
		if ((bad=*p) != (i % 0xff)) {
	    	show_error((unsigned long)p, (unsigned long)(i % 0xff), (unsigned long)bad);
			return DRAM_FAIL;
		}
	}
    serial_puts("<<<<<SSN incremnet byte test");
    serial_puts(" pass : >>>>> \n");
    return DRAM_SUCCESS;
}

int ssn_inc_word_test(unsigned long start, unsigned long size)
{
    unsigned short  *p, bad;
	unsigned long	i;
	unsigned long	len;

    p = (unsigned short *)start;
    len = size/sizeof(short);
	for (i=0; i<len; i++,p++) {
		*p = (i % 0x00ff);
	}

    p = (unsigned short *)start;
	for (i=0; i<len; i++,p++) {
		if ((bad=*p) != (i % 0x00ff)) {
	    	show_error((unsigned long)p, (unsigned long)(i % 0x00ff), (unsigned long)bad);
			return DRAM_FAIL;
		}
	}
    serial_puts("<<<<<SSN incremnet word test");
    serial_puts(" pass : >>>>> \n");
    return DRAM_SUCCESS;
}

#endif /* CONFIG_CORTINA_DIAG */
