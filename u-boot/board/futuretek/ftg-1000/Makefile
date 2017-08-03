#
# (C) Copyright 2000-2004
# Wolfgang Denk, DENX Software Engineering, wd@denx.de.
#
# See file CREDITS for list of people who contributed to this
# project.
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License as
# published by the Free Software Foundation; either version 2 of
# the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston,
# MA 02111-1307 USA
#

include $(TOPDIR)/config.mk

LIB	= $(obj)lib$(BOARD).a

CSMI_HEADERS = .
# or, if using under openwrt:
# CSMI_HEADERS = $(STAGING_DIR)/usr/include
 
# Needed for dependency generation
#
CPPFLAGS += -D_FROM_UBOOT_ -I$(CSMI_HEADERS)

# These are copied from cs-multi-image.  They have to be local because
# u-boot is compiled differently than standard libs.
#
CSMI_OBJS := csmi_user.o csmi_uboot_cmds.o cmd_ftg_1000.o

COBJS	:= ftg_1000.o pwr.o watchdog.o everify.o 
SOBJS	:= lowlevel_init.o

SRCS	:= $(SOBJS:.o=.S) $(COBJS:.o=.c)
OBJS	:= $(addprefix $(obj),$(COBJS))
SOBJS	:= $(addprefix $(obj),$(SOBJS))

$(LIB):	$(OBJS) $(SOBJS) $(CSMI_OBJS)
	$(AR) $(ARFLAGS) $@ $(OBJS) $(SOBJS) $(CSMI_OBJS)

# Need our own rules to sneek in the -D and -I needed
#
ftg_1000.o: ftg_1000.c
	$(CC) $(CFLAGS) -D_FROM_UBOOT_ -I$(CSMI_HEADERS) -c -o $@ $<
 
csmi_user.o: csmi_user.c
	$(CC) $(CFLAGS) -D_FROM_UBOOT_ -I$(CSMI_HEADERS) -c -o $@ $<
 
csmi_uboot_cmds.o: csmi_uboot_cmds.c
	$(CC) $(CFLAGS) -D_FROM_UBOOT_ -I$(CSMI_HEADERS) -c -o $@ $<

cmd_cortina.o: cmd_ftg_1000.c
	$(CC) $(CFLAGS) -D_FROM_UBOOT_ -I$(CSMI_HEADERS) -c -o $@ $<

clean:
	rm -f $(SOBJS) $(OBJS) $(CSMI_OBJS)

distclean:	clean
	rm -f $(LIB) core *.bak $(obj).depend

#########################################################################

# defines $(obj).depend target
include $(SRCTREE)/rules.mk

sinclude $(obj).depend

#########################################################################
