OUTPUT_FORMAT("elf32-littlearm", "elf32-littlearm", "elf32-littlearm")
OUTPUT_ARCH(arm)
ENTRY(_start)
SECTIONS
{
 . = 0x04000000;
 . = ALIGN(4);
 .text :
 {
    cpu/arm_cortexa9/start.o (.text)
  *(.text)
 }
 .rodata : { *(.rodata) }
 . = ALIGN(4);
 .data : { *(.data) }
 . = ALIGN(4);
 .got : { *(.got) }
 . = .;
 __u_boot_cmd_start = .;
 .u_boot_cmd : { *(.u_boot_cmd) }
 __u_boot_cmd_end = .;
 . = ALIGN(4);
 __bss_start = .;
 .bss : { *(.bss) }
 _end = .;
}
