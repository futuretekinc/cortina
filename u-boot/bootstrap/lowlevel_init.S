/*
 * Copyright (C) 2008, Guennadi Liakhovetski <lg@denx.de>
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

/*
 * This porgrams initializes PLL configuration and DRAM configuration
 *
 */
 


.globl lowlevel_init
lowlevel_init:

	/* wait pll lock */
	mov    r4,#0x1000
1:    
    nop
	nop
	nop
	subs   	r4, r4, #1	 /* Decrement loop count */
	bge    	1b
        				
	/* mov	r0, offset dram_reg_val */
	adr    	r0,dram_reg_val 
	mov	r1,#0
	mov	r4,#0
								
loop_dram_init:									
	add	r4,r0,r1
	ldr     r2, [r4]
	cmp	r2,#0
	beq	dram_init_end
	ldr     r3,[r4,#4]
	str    	r3,[r2]
	add     r2,r2,#4
	ldr     r3,[r4,#8]
	str    	r3,[r2]
	add	r1, r1, #0xc	
	b       loop_dram_init
	
dram_init_end:			    
	b       func_end
    
dram_reg_val: .word 	0xf0500000, 0x00000400, 0x00000000, \
			0xf0500008, 0x50000050, 0x00000000, \
			0xf0500010, 0x000000c8, 0x04000a02, \
			0xf0500018, 0x16040202, 0x03050311, \
			0xf0500020, 0x00000502, 0x00030300, \
			0xf0500028, 0x06050101, 0x0000c80b, \
			0xf0500030, 0x00a01203, 0x01000006, \
			0xf0500038, 0x08000601, 0x00020002, \
			0xf0500040, 0x00c8000a, 0x01000037, \
			0xf0500048, 0x53000001, 0x0000440a, \
			0xf0500050, 0x00000000, 0x01000000, \
			0xf0500058, 0x00000001, 0x00000000, \
			0xf0500060, 0x00000000, 0x00000000, \
			0xf0500068, 0x01000002, 0x40004000, \
			0xf0500070, 0x01010002, 0x07070a01, \
			0xf0500078, 0x01010101, 0x0c000101, \
			0xf0500080, 0x00000000, 0x00000001, \
			0xf0500088, 0x00000000, 0x00000000, \
			0xf0500090, 0x00000000, 0x00000000, \
			0xf0500098, 0x00000000, 0x00000000, \
			0xf05000a0, 0x00000000, 0x00000000, \
			0xf05000a8, 0x00000000, 0x00000000, \
			0xf05000b0, 0x00000000, 0x00000000, \
			0xf05000b8, 0x00000000, 0x00000000, \
			0xf05000c0, 0x00000000, 0x04000000, \
			0xf05000c8, 0x00000000, 0x00000000, \
			0xf05000d0, 0x00000000, 0x00000000, \
			0xf05000d8, 0x00002819, 0x00000000, \
			0xf05000e0, 0x00000000, 0x00000000, \
			0xf05000e8, 0x00000000, 0x00000000, \
			0xf05000f0, 0x00000000, 0x00000000, \
			0xf05000f8, 0x001c1c00, 0x00000000, \
			0xf0500100, 0x00000000, 0x00000000, \
			0xf0500108, 0x00001c1c, 0x00000000, \
			0xf0500110, 0x00000000, 0x001c1c00, \
			0xf0500118, 0x00000000, 0x00000000, \
			0xf0500120, 0x1c000000, 0x0000001c, \
			0xf0500128, 0x0202ffff, 0x02ffff00, \
			0xf0500130, 0xffff0002, 0x00000202, \
			0xf0500138, 0x01000000, 0x01320300, \
			0xf0500140, 0x00013200, 0x32000132, \
			0xf0500148, 0x00000001, 0x00000000, \
			0xf0500150, 0x00000700, 0x0c2d0000, \
			0xf0500158, 0x02000200, 0x02000200, \
			0xf0500160, 0x03040c2d, 0x03038000, \
			0xf0500168, 0x000a0407, 0x00000c2d, \
			0xf0500170, 0x00000000, 0x0010ffff, \
			0xf0500178, 0x11070303, 0x0000000f, \
			0xf0500180, 0x00000c2d, 0x00000000, \
			0xf0500188, 0x00000000, 0x00000000, \
			0xf0500190, 0x00000c2d, 0x00000204, \
			0xf0500198, 0x00000000, 0x01000000, \
			0xf05001a0, 0x00006d60, 0x00000000, \
			0xf05001a8, 0x00000000, 0x00000000, \
			0xf05001b0, 0x00003ce1, 0x00000000, \
			0xf0500200, 0x00000000, 0x00011000, \
			0xf0500208, 0x00000e38, 0x00000000, \
			0xf0500210, 0x10108000, 0x00000020, \
			0xf0500218, 0x00000600, 0x00000000, \
			0xf0500220, 0x00000000, 0x00000000, \
			0xf0500228, 0x00000000, 0x00000000, \
			0xf0500230, 0x04252000, 0x00003000, \
			0xf0500238, 0x00000000, 0x00000000, \
			0xf0500240, 0x00000000, 0x00000000, \
			0xf0500000, 0x00000401, 0x00000000, \
			0xf5010c00, 0xe0000001, 0xe8000000, \
			0   	  , 0

func_end:
												
   	mov pc, lr 
