#include "common.h"

#if (CONFIG_CORTINA_DIAG==1)

#define writel(v,a)	IO_WRITE(a,v)
#define readl(a)	IO_READ(a)

static void g2_pcie_read_err_counts(int port_number)
{
	unsigned int err_count;
	unsigned int reg_off;

	err_count = readl(PCIE_SATA_PCIE_GLBL_PHY_RX_10B_8B_DEC_ERR_CNT + port_number * 0x400);
	serial_puts("PCIE_SATA_PCIE_GLBL_PHY_RX_10B_8B_DEC_ERR_CNT=");
	serial_put_hex(err_count);
	serial_puts("\n");
	
	err_count = readl(PCIE_SATA_PCIE_GLBL_PHY_RX_DISPARITY_ERR_CNT  + port_number * 0x400);
	serial_puts("PCIE_SATA_PCIE_GLBL_PHY_RX_DISPARITY_ERR_CNT=");
	serial_put_hex(err_count);
        serial_puts("\n");

	for (reg_off = PCIE_SATA_PCIE_GLBL_RX_RCVD_TS1_ORDER_SET_CNT; 
		reg_off <= PCIE_SATA_PCIE_GLBL_RX_RCVD_REQ_ERR_CNT; reg_off += 4)
	{
		err_count = readl(PCIE_SATA_PCIE_GLBL_RX_RCVD_TS1_ORDER_SET_CNT  + port_number * 0x400);
		serial_puts("reg_off=");
		serial_put_hex(reg_off);
		serial_puts(", count=");
		serial_put_hex(err_count);
		serial_puts("\n");
	}
	
}

int g2_pcie_read_confs(int argc, char **argv)
{
	unsigned int rc_addr = 0xf00a0000;
	unsigned int base_addr = 0x80000000;
	unsigned int cfg_base_addr = 0x80100000;
        int port_number = 0;
	unsigned int value, i;
	unsigned int link_up = 0, dev_id;

	GLOBAL_BLOCK_RESET_t block_reset;

        if (argc < 2)
                return -1;

        port_number = iros_strtol(argv[1]);

	rc_addr = 0xf00a0000 + port_number * 0x400;
	base_addr = 0x80000000 + port_number * 0x20000000;
	cfg_base_addr = 0x80100000 + port_number * 0x20000000;


	/* reset PCIe */
	block_reset.wrd = 0x00600000;
	switch (port_number)
	{
		case 0:
			block_reset.bf.reset_pcie0 = 1;
			break;
		case 1:
			block_reset.bf.reset_pcie1 = 1;
			break;
		case 2:
			block_reset.bf.reset_pcie2 = 1;
			break;
	}
	writel(block_reset.wrd, GLOBAL_BLOCK_RESET);
	udelay(10000);

	switch (port_number)
        {
                case 0:
                        block_reset.bf.reset_pcie0 = 0;
                        break;
                case 1:
                        block_reset.bf.reset_pcie1 = 0;
                        break;
                case 2:
                        block_reset.bf.reset_pcie2 = 0;
                        break;
        }
        writel(block_reset.wrd, GLOBAL_BLOCK_RESET);
  udelay(10000);

	/* RC link up */
	value =	readl(rc_addr + 0x1c) | 0x21;
	writel(value, rc_addr + 0x1c);
	udelay(100000);

	for (i=0; i<10; i++){
		value =	readl(rc_addr + 0x20);
		if ((value & 0x3F0) == 0x110)
			break;
		serial_puts("Value of rc_addr + 0x20=");
		serial_put_hex(value);
		serial_puts("\n");
		udelay(100000);
	}
	value =	readl(rc_addr + 0x20);
	serial_puts("Value of rc_addr + 0x20=");
	serial_put_hex(value);
	serial_puts("\n");
	if ((value & 0x3F0) == 0x110)
		link_up = 1;

	serial_puts("PCIe ");
	serial_put_decimal(port_number);
	serial_puts("Link ");
	if (link_up == 1){
		serial_puts("UP\n");
	}
	else
		serial_puts("Fail!\n");

	/* RC configuration space mapping */
	/* PCIE_SATA_PCIE_GLBL_PCIE_CONTR_CFG_START_ADDR */
	writel(base_addr, rc_addr + 0x80);
	value = readl(rc_addr + 0x80);
	serial_puts("Value of rc_addr + 0x80=");
        serial_put_hex(value);
        serial_puts("\n");

	/* PCIE_SATA_PCIE_GLBL_PCIE_CONTR_CFG_END_ADDR */
	writel(base_addr + 0xfff, rc_addr + 0x84);
        value = readl(rc_addr + 0x84);
        serial_puts("Value of rc_addr + 0x84=");
        serial_put_hex(value);
        serial_puts("\n");


	/* RC configuration space */
	/* memory base and limit 0x20 :	 */
	writel(0x00100010 | (0x80008000 +  port_number * 0x20002000), base_addr + 0x20);
        value = readl(base_addr + 0x20);
        serial_puts("Value of base_addr + 0x20=");
        serial_put_hex(value);
        serial_puts("\n");

	/* prefetch memory base and limit 0x24 : */
	writel(0x0000fff0, base_addr + 0x24);
        value = readl(base_addr + 0x24);
        serial_puts("Value of base_addr + 0x24=");
        serial_put_hex(value);
        serial_puts("\n");

	/* command 0x4 : bus master enabled(optional) */
	writel(0x00100007, base_addr + 0x04);
        value = readl(base_addr + 0x04);
        serial_puts("Value of base_addr + 0x04=");
        serial_put_hex(value);
        serial_puts("\n");

	/* ATU       region mapping */
	/* atu viewport 0x200 : select region 0 */
	writel(0x00000000, base_addr + 0x900);
        value = readl(base_addr + 0x900);
        serial_puts("Value of base_addr + 0x900=");
        serial_put_hex(value);
        serial_puts("\n");

	/* atu lower base address 0x20c : */
	writel(cfg_base_addr, base_addr + 0x90c);
        value = readl(base_addr + 0x90c);
        serial_puts("Value of base_addr + 0x90c=");
        serial_put_hex(value);
        serial_puts("\n");

	/* atu upper base address 0x210 : */
        writel(0x00000000, base_addr + 0x910);
        value = readl(base_addr + 0x910);
        serial_puts("Value of base_addr + 0x910=");
        serial_put_hex(value);
        serial_puts("\n");

	/* atu lower limit address 0x214 : */
        writel(cfg_base_addr + 0xfff, base_addr + 0x914);
        value = readl(base_addr + 0x914);
        serial_puts("Value of base_addr + 0x914=");
        serial_put_hex(value);
        serial_puts("\n");

	/* atu lower target address 0x218 : */
        writel(0x01000000, base_addr + 0x918);
        value = readl(base_addr + 0x918);
        serial_puts("Value of base_addr + 0x918=");
        serial_put_hex(value);
        serial_puts("\n");

	/* atu upper target address 0x21c : */
	writel(0x00000000, base_addr + 0x91c);
        value = readl(base_addr + 0x91c);
        serial_puts("Value of base_addr + 0x91c=");
        serial_put_hex(value);
        serial_puts("\n");

	/* atu ctrl_1 0x204 : region attribute */
	writel(0x00000004, base_addr + 0x904);
        value = readl(base_addr + 0x904);
        serial_puts("Value of base_addr + 0x904=");
        serial_put_hex(value);
        serial_puts("\n");

	/* atu ctrl_2 0x208 : region enable */
	writel(0x80000000, base_addr + 0x908 );
        value = readl(base_addr + 0x908);
        serial_puts("Value of base_addr + 0x908=");
        serial_put_hex(value);
        serial_puts("\n");
	udelay(1000);

	g2_pcie_read_err_counts(port_number);

	if (link_up == 0)
		return 0;

	/* launch CfgRd */
	writel(0xffffffff, cfg_base_addr);
	value = readl(cfg_base_addr);
        serial_puts("Value of cfg_base_addr=");
        serial_put_hex(value);
        serial_puts("\n");

	writel(0xfffffffc, cfg_base_addr + 0x10);
        value = readl(cfg_base_addr);
        serial_puts("Value of cfg_base_addr+0x10=");
        serial_put_hex(value);
        serial_puts("\n");

	writel(0x8100000 + port_number * 0x20000000, cfg_base_addr + 0x10);
        value = readl(cfg_base_addr);
        serial_puts("Value of cfg_base_addr+0x10=");
        serial_put_hex(value);
        serial_puts("\n");

	serial_puts("PCIe ");
	serial_put_decimal(port_number);
	serial_puts("Link ");
	if (link_up == 1){
		serial_puts("UP\n");
	}
	else
		serial_puts("Fail!\n");

	dev_id = readl(cfg_base_addr);
	serial_puts("Device ID:");
	serial_put_hex(dev_id);
        return 0;
}

#endif /* CONFIG_CORTINA_DIAG */


