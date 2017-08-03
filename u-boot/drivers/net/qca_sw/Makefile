
include $(TOPDIR)/config.mk

LIB	:= $(obj)libqca_sw.a

COBJS-$(CONFIG_GOLDENGATE) += qca83xx_main.o isisc_reg_access.o sd.o
			 

# Platform related definitions
#ifdef CONFIG_GOLDENGATE
#CFLAGS += -D_LITTLE_ENDIAN -DMDC_MDIO_OPERATION -D$(CHIP)
#endif


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
