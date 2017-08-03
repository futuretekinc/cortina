LD := $(CROSS_COMPILE)ld
CC := $(CROSS_COMPILE)gcc
OBJCOPY := $(CROSS_COMPILE)objcopy


ifeq ($(PLATFORM_INCLUDE),)
  $(error Must pass in PLATFORM_INCLUDE to point to cs_types.h and registers.h)
endif

CFLAGS = -c -Wall -mlittle-endian -nostdlib -g -ggdb -O1 \
	-march=armv7-a -mtune=cortex-a9
INC    = -I$(shell pwd) -I$(PLATFORM_INCLUDE)

FIXADDR = -Tpost.ld

OBJECT = start.o         \
         lowlevel_init.o \
         post_cli.o      \
         post_io.o       \
         post_mtest.o    \
         post_common.o   \
         cache-cp15.o    \
         main.o          \
         serial_cs752x.o \
         time.o			 \
         sdram.o		 \
         trap.o		\
         sb_phy.o	\
         pcie.o		\
         usb_phy.o	

ifdef AHCI_DIAGNOSTIC
OBJECT += ahci.o 
CFLAGS += -D __AHCI_DIAGNOSTIC__
endif

all:  post.bin

clean:
	rm -f *.o code.* core .depend *~
	rm -f bsp.a post.elf post.bin post.map

post.bin: post.elf
	@$(OBJCOPY) -O binary post.elf post.bin

post.elf: bsp.a
	$(LD) -EL -static $(FIXADDR) -g -nostdlib -Map post.map \
		-o post.elf bsp.a

bsp.a:   $(OBJECT)
	$(LD) -EL -i $(OBJECT) -o $@

%.o: %.c
	$(CC) $(CFLAGS) $(INC) -c $<

%.o: %.S
	$(CC) $(CFLAGS) $(INC) -c $<
