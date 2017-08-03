# These are the locations of the CS eth driver headers
# in the "g2" tree.
EXTRA_FLAGS += -I$(srctree)/include/linux
EXTRA_FLAGS += -I$(srctree)/drivers/net/ethernet/realtek/rtl8370
EXTRA_FLAGS += -I$(srctree)/drivers/net/ethernet/cs752x/src/ni
EXTRA_FLAGS += -I$(srctree)/drivers/net/ethernet/cs752x/src/qm
EXTRA_FLAGS += -I$(srctree)/drivers/net/ethernet/cs752x/src/fe/include
EXTRA_FLAGS += -I$(srctree)/drivers/net/ethernet/cs752x/src/include


EXTRA_FLAGS += -D_LITTLE_ENDIAN -DMDC_MDIO_OPERATION -DCHIP_RTL8370_VB -DRTL83XX_DBG

EXTRA_CFLAGS += $(EXTRA_FLAGS)

obj-$(CONFIG_RTL8370) += rtl83xx.o
rtl83xx-objs := rtl8370_main.o \
		rtk_api.o \
		rtl8370_asicdrv_acl.o \
		rtl8370_asicdrv.o \
		rtl8370_asicdrv_cputag.o \
		rtl8370_asicdrv_dot1x.o \
		rtl8370_asicdrv_eee.o \
		rtl8370_asicdrv_eeelldp.o \
		rtl8370_asicdrv_fc.o \
		rtl8370_asicdrv_green.o \
		rtl8370_asicdrv_igmp.o \
		rtl8370_asicdrv_inbwctrl.o \
		rtl8370_asicdrv_interrupt.o \
		rtl8370_asicdrv_led.o \
		rtl8370_asicdrv_lut.o \
		rtl8370_asicdrv_meter.o \
		rtl8370_asicdrv_mib.o \
		rtl8370_asicdrv_mirror.o \
		rtl8370_asicdrv_misc.o \
		rtl8370_asicdrv_oam.o \
		rtl8370_asicdrv_phy.o \
		rtl8370_asicdrv_port.o \
		rtl8370_asicdrv_portIsolation.o \
		rtl8370_asicdrv_qos.o \
		rtl8370_asicdrv_rldp.o \
		rtl8370_asicdrv_rma.o \
		rtl8370_asicdrv_rrcp.o \
		rtl8370_asicdrv_scheduling.o \
		rtl8370_asicdrv_specialCongest.o \
		rtl8370_asicdrv_storm.o \
		rtl8370_asicdrv_svlan.o \
		rtl8370_asicdrv_trunking.o \
		rtl8370_asicdrv_unknownMulticast.o \
		rtl8370_asicdrv_vlan.o\
		smi.o

