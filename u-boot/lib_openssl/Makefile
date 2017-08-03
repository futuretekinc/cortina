#
# (C) Copyright 2000-2006
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

#
# (C) Copyright 2000-2006
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

# CFLAGS += -DDEBUG
INCDIR		:= $(CURDIR)

LIB	= $(obj)libopenssl.a

COBJS-y += ecdsa/ecs_ossl.o
COBJS-y += ec/ec_lib.o
COBJS-y += ec/ec_mult.o
COBJS-y += ec/ec_key.o
COBJS-y += ec/ec_curve.o
COBJS-y += ec/ec_cvt.o
COBJS-y += ec/ecp_nist.o
COBJS-y += ec/ecp_smpl.o
COBJS-y += ec/ecp_mont.o
COBJS-y += ec/ec2_smpl.o
COBJS-y += ec/ec2_mult.o
COBJS-y += ec/ec_asn1.o
COBJS-y += bn/bn_lib.o
COBJS-y += bn/bn_add.o
COBJS-y += bn/bn_shift.o
COBJS-y += bn/bn_sqr.o
COBJS-y += bn/bn_rand.o
COBJS-y += bn/bn_mod.o
COBJS-y += bn/bn_nist.o
COBJS-y += bn/bn_gf2m.o
COBJS-y += bn/bn_gcd.o
COBJS-y += bn/bn_word.o
COBJS-y += bn/bn_sqrt.o
COBJS-y += bn/bn_mul.o
COBJS-y += bn/bn_mont.o
COBJS-y += bn/bn_kron.o
COBJS-y += bn/bn_div.o
COBJS-y += bn/bn_ctx.o
COBJS-y += bn/bn_exp.o
COBJS-y += bn/bn_recp.o
COBJS-y += bn/bn_asm.o
COBJS-y += bn/bn_print.o
COBJS-y += sha/sha256.o
COBJS-y += zlib/zlib.o
COBJS-y += lib.o
COBJS-y += libstart.o

COBJS	:= $(COBJS-y)
SRCS	:= $(COBJS:.o=.c)
OBJS	:= $(addprefix $(obj),$(COBJS))

$(LIB):	$(obj).depend $(OBJS)
	$(AR) $(ARFLAGS) $@ $(OBJS)

#########################################################################

# defines $(obj).depend target
include $(SRCTREE)/rules.mk 

sinclude $(obj).depend 

#########################################################################
