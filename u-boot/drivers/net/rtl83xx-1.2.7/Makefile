#
# (C) Copyright 2006
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
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston,
# MA 02111-1307 USA
#

include $(TOPDIR)/config.mk

LIB	:= $(obj)librtl83xx.a

COBJS-$(CONFIG_GOLDENGATE) += rtl83xx_main.o rtk_api.o rtl8367b_asicdrv.o rtl8367b_asicdrv_port.o smi.o
			 
CHIP	:= CHIP_RTL8367RB

ifdef CONFIG_MK_ENGINEERING
CHIP	:= CHIP_RTL8367M_VB
endif

ifdef CONFIG_MK_ENGINEERINGS
CHIP	:= CHIP_RTL8367RB
endif

ifdef CONFIG_MK_REFERENCE
CHIP	:= CHIP_RTL8367M_VB
endif

ifdef CONFIG_MK_REFERENCEB
CHIP	:= CHIP_RTL8367M_VB
endif

ifdef CONFIG_MK_REFERENCES
CHIP	:= CHIP_RTL8367RB
endif

ifdef CONFIG_MK_PON
CHIP	:= CHIP_RTL8367RB
endif

ifdef CONFIG_MK_WAN
CHIP	:= CHIP_RTL8367RB
endif

ifdef CONFIG_MK_BHR
CHIP	:= CHIP_RTL8367RB
endif

# Platform related definitions
ifdef CONFIG_GOLDENGATE
CFLAGS += -D_LITTLE_ENDIAN -DMDC_MDIO_OPERATION -D$(CHIP)
endif


COBJS	:= $(COBJS-y)
SRCS	:= $(COBJS:.o=.c)
OBJS	:= $(addprefix $(obj),$(COBJS))

all:	$(LIB)

$(LIB):	$(obj).depend $(OBJS)
	$(AR) $(ARFLAGS) $@ $(OBJS)

#########################################################################

# defines $(obj).depend target
include $(SRCTREE)/rules.mk

sinclude $(obj).depend

#########################################################################
