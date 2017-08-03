STARTUP(libstart.o)

MEMORY
{
    ram : ORIGIN = 0xF6A30000, LENGTH = 64*1024
}

ENTRY(ossl_init)

SECTIONS
{
	.text		0xF6A30000 	:	{ *(.text*) *(.init) *(.glue_7) *(.glue_7t) } > ram
	.fini		ALIGN(4)	:	{ . = . ; *(.fini) } > ram
	.rodata		ALIGN(4)	:	{ . = . ; *(.rodata*) } > ram
	.fixup		ALIGN(4)	:	{ . = . ; *(.fixup) }    > ram
	.ARM.exidx    	ALIGN(4)  	: 	{ *(.ARM.exidx*) } > ram
	.data		ALIGN(4)	:	{
							*(.data*)  . = ALIGN(4);
						} > ram
		_edata = .;
		PROVIDE(edata = .);
	.bss		ALIGN(4)	:	{
							__bss_start = ABSOLUTE(.);
							*(.bss*) . = ALIGN(4);
							*(COMMON) __bss_end = ABSOLUTE(.);
						}  > ram
	.got		ALIGN(4)	:	{ . = . ; *(.got) } > ram
	.got.plt	ALIGN(4)	:	{ . = . ; *(.got.plt) } > ram
	. = ALIGN(4);
	_end = .;
	PROVIDE(end = .); 
}
