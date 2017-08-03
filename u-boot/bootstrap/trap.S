#include "config.h"
//		.section	".fixup", "ax"
		.code 32
	.global _exception_handlers
_exception_handlers:
	b	.reset_vector		     	 // 0x00
	b	.undefined_instruction       // 0x04
    b     	.software_interrupt          // 0x08 start && software int
	b	.abort_prefetch              // 0x0C
	b	.abort_data                  // 0x10
	.word   0                            // unused
	b	.IRQ                         // 0x18
	b	.FIQ                         // 0x1C

.reset_vector:
	bl	cpu_init_crit
	bl v7_invalidate_dcache_all

	LDR	r0, =CORTEX_SCU_BASE
	/* Read ACK */
	LDR	r7, [r0, #0x10C]

	LDR	r0, =CORTEX_SCU_BASE
	ORR	r0, r0, #0x1000
	/* Pending clear */
	LDR	r3, =0x00000001
	STR	r3, [r0,#0x280]

	/* Disable interrupt */
	MOV	r3, #0x000000FF
	ORR	r3, r3, #0xFF00
	STR	r3, [r0,#0x180]

	LDR	r0, =CORTEX_SCU_BASE
	/* EOI */
	STR	r7, [r0,#0x110]
	MOV	r7, #0


	LDR	r0, =CORTEX_SCU_BASE
	ORR	r0, r0, #0x1000
	/* Enable interrupt */
	MOV	r3, #0x000000FF
	ORR	r3, r3, #0xFF00
	STR	r3, [r0,#0x100]

	LDR	r0, =CORTEX_SCU_BASE
	/* Enable CPU interface */
	LDR	r3, =0x00000001
	STR	r3, [r0,#0x100]

	/* Set Interrupt Priority */
	MOV	r3, #0xF0
	STR	r3, [r0,#0x104]

	MOV	r0, #0xF0000000
	ORR	r0, r0, #0xB8
	MOV	r2, #0
	STR	r2, [r0]

loop_here:
	WFI
	nop
	LDR	r1, [r0]
	CMP	r1, #0
	BEQ     loop_here
	BX	r1
	B	loop_here

.undefined_instruction:
.software_interrupt:
.abort_prefetch:
.abort_data:
.IRQ:
.FIQ:
	b	.reset_vector
	nop


cpu_init_crit:
	/*
	 * Invalidate L1 I/D
	 */
	mov	r0, #0			@ set up for MCR
	mcr	p15, 0, r0, c8, c7, 0	@ invalidate TLBs
	mcr	p15, 0, r0, c7, c5, 0	@ invalidate icache

	/*
	 * disable MMU stuff and caches
	 */
	mrc	p15, 0, r0, c1, c0, 0
	bic	r0, r0, #0x00002000	@ clear bits 13 (--V-)
	bic	r0, r0, #0x00000007	@ clear bits 2:0 (-CAM)
	orr	r0, r0, #0x00000002	@ set bit 1 (--A-) Align
	orr	r0, r0, #0x00000800	@ set bit 12 (Z---) BTB
	mcr	p15, 0, r0, c1, c0, 0

	/*
	 * Jump to board specific initialization...
	 * The Mask ROM will have already initialized
	 * basic memory. Go here to bump up clock rate and handle
	 * wake up conditions.
	 */
	mov	ip, lr			@ persevere link reg across call
#ifdef CONFIG_CORTINA_FPGA
	bl	lowlevel_init		@ go setup pll,mux,memory
#endif
	mov	lr, ip			@ restore link
	mov	pc, lr			@ back to my caller

/*
*  v7_invalidate_dcache_all()
*
*  Invalidate the whole D-cache.
*
*  Corrupted registers: r0-r7, r9-r11 (r6 only in Thumb mode)
*
*  - mm    - mm_struct describing address space
*/
v7_invalidate_dcache_all:
  	dmb         				@ ensure ordering with previous memory accesses
  	mrc p15, 1, r0, c0, c0, 1   @ read clidr
  	ands  r3, r0, #0x7000000    @ extract loc from clidr
  	mov r3, r3, lsr #23     	@ left align loc bit field
  	beq finished2     			@ if loc is 0, then no need to clean
  	mov r10, #0       			@ start clean at cache level 0

loop11:
  	add r2, r10, r10, lsr #1    @ work out 3x current cache level
  	mov r1, r0, lsr r2      	@ extract cache type bits from clidr
  	and r1, r1, #7      		@ mask of the bits for current cache only
  	cmp r1, #2        			@ see what cache we have at this level
  	blt skip2       			@ skip if no cache, or just i-cache
  	mcr p15, 2, r10, c0, c0, 0  @ select current cache level in cssr
  	isb         				@ isb to sych the new cssr&csidr
  	mrc p15, 1, r1, c0, c0, 0   @ read the new csidr
  	and r2, r1, #7      		@ extract the length of the cache lines
  	add r2, r2, #4      		@ add 4 (line length offset)
  	ldr r4, =0x3ff
  	ands  r4, r4, r1, lsr #3    @ find maximum number on the way size
  	clz r5, r4        			@ find bit position of way size increment
  	ldr r7, =0x7fff
  	ands  r7, r7, r1, lsr #13   @ extract max number of the index size

loop22:
  	mov r9, r4        			@ create working copy of max way size

loop33:
  	orr r11, r10, r9, lsl r5  	@ factor way and cache number into r11
//	THUMB( lsl r6, r9, r5 )
//	THUMB( orr r11, r10, r6 ) 	@ factor way and cache number into r11
  	orr r11, r11, r7, lsl r2   	@ factor index number into r11
//	THUMB( lsl r6, r7, r2 )
//	THUMB( orr r11, r11, r6 ) 	@ factor index number into r11
  	mcr p15, 0, r11, c7, c6, 2  @ invalidate by set/way
  	subs  r9, r9, #1      		@ decrement the way
  	bge loop33
  	subs  r7, r7, #1      		@ decrement the index
  	bge loop22

skip2:
  	add r10, r10, #2      		@ increment cache number
  	cmp r3, r10
  	bgt loop11

finished2:
  	mov r10, #0       			@ swith back to cache level 0
  	mcr p15, 2, r10, c0, c0, 0  @ select current cache level in cssr
  	dsb
  	isb
  	mov pc, lr

	.global _exception_handlers_end
_exception_handlers_end:
	nop

