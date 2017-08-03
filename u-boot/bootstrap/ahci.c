#if 0
#include <inc/typedef.h>
#include <inc/api.h>
#include <inc/sata.h>
#include <inc/g2.h>
#endif

#include "common.h"


#define UPDATE_GPT1_LDVAL               	(1 << 2)
#define GPT_CTRL_EN                             (1 << 7)
#define GPT_CTRL_RLMODE                 	(1 << 6)

#define CLOCK_RATE				50000000


#define CS_SETF_CMD_XFER	0x03

#define IDEN_BUF_SIZE		512


#define SET_FEATURE_TIMEOUT 	10000
#define IDENTIFY_TIMEOUT 	10000
#define READ_TIMEOUT		10000
#define AHCI_CTL_RESET_TIMEOUT	3000



#define AHCI_RX_FIS_SZ		256
#define AHCI_CMD_TBL_HDR	0x80
/*
 *  1. cmd queue slot : 32 bytes. 256 for alignment
 *  2. rx_fis size: 256 + AHCI_RX_FIS_SZ = 512
 *  3. cmd table size: 512 + 0x80 = 640
 *  4. cmd sg one slot : 16 + 640 = 656
 */
#define AHCI_PORT_PRIV_SZ	1024

#define AHCI_CAP		0x00 
#define AHCI_CTL		0x04
#define AHCI_PI			0x0C
#define AHCI_TESTER 		0xf4
#define AHCI_OOBR   		0xBC


#define AHCI_CTL_HR		(1 << 0) 
#define AHCI_CTL_AE		(1 << 31)

#define AHCI_PxCLB		0x00 
#define AHCI_PxFB		0x08 
#define AHCI_PxCMD		0x18 
#define AHCI_PxSSTS		0x28 
#define AHCI_PxSCTL		0x2C
#define AHCI_PxSERR		0x30 
#define AHCI_PxCI		0x38 

#define AHCI_PxCMD_ICC		(1 << 28)
#define AHCI_PxCMD_CR		(1 << 15)
#define AHCI_PxCMD_FR		(1 << 14)
#define AHCI_PxCMD_FRE		(1 << 4)
#define AHCI_PxCMD_POD		(1 << 2)
#define AHCI_PxCMD_SUD		(1 << 1)
#define AHCI_PxCMD_ST		(1 << 0)


#define MEM_ADDRESS 		0xF6400000

struct ahci_cmd_hdr {
	unsigned int	descInfo;
	unsigned int	cmdStatus;
	unsigned int	tblBaseAddr;
	unsigned int	reserved[8];
};

struct ahci_sg {
	unsigned int	addr;
	unsigned int	addr_hi;
	unsigned int	reserved;
	unsigned int	data_byte_count;
};

struct ahci_private {
	unsigned int	port_mmio;
	struct ahci_cmd_hdr	*cmd_slot;
	struct ahci_sg		*cmd_sg;
	unsigned int	cmd_tbl;
};




static void mdelay( unsigned int t)
{
	udelay( t*1000);
}

static struct ahci_private priv;
static int waiting_register (unsigned int addr, unsigned int mask, unsigned int val, unsigned int timeout)
{
	unsigned int i;
	volatile unsigned int status;

	for( i = 0; i < timeout; ++i) {
		status= IO_READ( addr);
		if((status & mask) == val ){
			return 1;
		}
		mdelay( 1 );
	}

	return 0;
}


#define RESET_PHY 	29

#define POR_N_I 	19
#define REFCLKSEL0 	0
#define REFCLKSEL1 	1
#define PD		8

#define CMU_RESETN_I 	16
#define LN0_RESETN_I 	17

#define CMU_OK		18
#define LANE_OK		16

#define	GLOBAL_BLOCK_RESET 0xf0000004
#define	PCIE_SATA_SATA2_PHY_SATA_CORE_PHY_STAT 0xf00a0e00
#define	GLOBAL_PHY_CONTROL  0xf0000014 
#define	GOLDENGATE_AHCI_BASE  0xf4100000
#define PCIE_SATA_SATA2_CTRL_SATA_CORE_CTRL1 0xf00a0c00


struct PhyConfig
{
	unsigned short offset;
	unsigned int value;
};

static const unsigned short step2_delay= 1;
static const unsigned short step3_delay= 100;
static const unsigned short step4_delay= 1;
static const unsigned short step7_delay= 1;
static const unsigned short step8_delay= 1;

static int sata_sb_phy_initial (int port, int pole_inverse, int external_clock)
{
	static const struct PhyConfig conf_24MHZ[]= 
	{
		{0  ,   0x00890006 }, 
		{4  ,   0x0003c97b },
		{32 ,   0x54a00000 },
		{44 ,   0x50040000 },
		{48 ,   0x40250270 },
		{52 ,   0x00004001 },
		{96 ,   0x5e002e00 },
		{100 ,  0xf0d14200 },
		{104 ,  0xb92c7828 },
		{108 ,  0x0000035e },
		{120 ,  0x00000002 },
		{124 ,  0x04841000 },
		{128 ,  0x000000e0 },
		{132 ,  0x07400023 },
		{136 ,  0x680017d0 },
		{140 ,  0x0d181ef2 },
		{144 ,  0x0000000c },
		{196 ,  0x0f600000 },
		{200 ,  0x40002000 },
		{204 ,  0x4919ae24 },
		{208 ,  0xc54b8304 },
		{212 ,  0x98280301 },
		{216 ,  0x80000019 },
		{220 ,  0x0000d0f0 },
		{232 ,  0xa0a0a000 },
		{236 ,  0xa0a0a0a0 },
		{240 ,  0x58800054 },
		{244 ,  0x865c4400 },
		{248 ,  0x9009d08d },
		{252 ,  0x00004007 },
		{256 ,  0x00322000 },
		{300 ,  0xd8000000 },
		{304 ,  0x0011ff1a },
		{308 ,  0xf0000000 },
		{312 ,  0xffffffff },
		{316 ,  0x3fc3c21c },
		{320 ,  0x0000000a },
		{324 ,  0x00f80000 },
		{0   ,  0x00890007 },
	};

	static const struct PhyConfig conf_60MHZ[]= 
	{
		{ 0   ,0x00810006},
		{ 32  ,0x64a00000},
		{ 44  ,0x50040000},
		{ 48  ,0x40250270},
		{ 52  ,0x00004001},
		{ 96  ,0x5e002e00},
		{ 100 ,0xA0d14200},
		{ 104 ,0xce447828},
		{ 108 ,0x0000030b},
		{ 120 ,0x00000002},
		{ 124 ,0x04841000},
		{ 128 ,0x000000e0},
		{ 132 ,0x05000023},
		{ 136 ,0x68000438},
		{ 140 ,0x0d181ef2},
		{ 144 ,0x0000000d},
		{ 196 ,0x0f600000},
		{ 200 ,0x40002000},
		{ 204 ,0x4919ae24},
		{ 208 ,0xc6482304},
		{ 212 ,0x98280301},
		{ 216 ,0x80000019},
		{ 220 ,0x0000d0f0},
		{ 232 ,0xa0a0a000},
		{ 236 ,0xa0a0a0a0},
		{ 240 ,0x58800064},
		{ 244 ,0x865c4400},
		{ 248 ,0x9009d08d},
		{ 252 ,0x00004007},
		{ 256 ,0x00322000},
		{ 300 ,0xd8000000},
		{ 304 ,0x0011ff1a},
		{ 308 ,0xf0000000},
		{ 312 ,0xffffffff},
		{ 316 ,0x3fc3c21c},
		{ 320 ,0x0000000a},
		{ 324 ,0x00f80000},
		{ 0,   0x00810007},
	};

	
	static const unsigned int lane_addr =  0xf00a1000;

	GLOBAL_STRAP_t strap;

	unsigned int tmp;
	const struct PhyConfig *p;
	const struct PhyConfig *endp;



	// step1 :Release the power on reset by programming por_n_i to high
	tmp =  IO_READ( GLOBAL_PHY_CONTROL );

	tmp |= 1 << ( POR_N_I + port*4); // por_n_i
	tmp &= ~(1 << ( PD + port ));

	if( external_clock == 1) { // =1 
		tmp &= ~( 1 << ( REFCLKSEL1 + port *2 ));
		tmp |= 1 << (  REFCLKSEL0 + port*2 );
	} else { // = 2, internal
		tmp |= 1 << (  REFCLKSEL1 + port*2 );
		tmp &= ~( 1 << ( REFCLKSEL0 + port *2 ));
	}

	IO_WRITE( GLOBAL_PHY_CONTROL, tmp );

	mdelay( step2_delay);

	// step 2: Program the Snowbush PHY registers
	if ( external_clock ) {
		p= conf_60MHZ;
		endp= &conf_60MHZ[ sizeof( conf_60MHZ)/sizeof( conf_60MHZ[0]) ];
	} else {
		p= conf_24MHZ;
		endp= &conf_24MHZ[ sizeof( conf_24MHZ)/sizeof( conf_24MHZ[0]) ];
	}

	for( ; p != endp ; ++p) {
		IO_WRITE( lane_addr + port*0x4000 + p->offset, p->value ); 
	}

	// Step3 :Apply block level reset 
	
	tmp= IO_READ( GLOBAL_BLOCK_RESET );
	tmp |= 0x01 << RESET_PHY;
	IO_WRITE( GLOBAL_BLOCK_RESET, tmp);

	// Step 4: Delay
	mdelay( step3_delay );

	// Step 5: Release  block level reset 
	tmp= IO_READ( GLOBAL_BLOCK_RESET );
	tmp &= ~( 1 << RESET_PHY );
	IO_WRITE( GLOBAL_BLOCK_RESET, tmp);
	
	mdelay( step4_delay);

	// Step 6: Selection of Interface
	if( port < 3) {
		tmp= IO_READ( 0xf0000010 );
		tmp &= ~(1 << (port*2 + 10));
		tmp |= (1 << (port*2 + 11) );
		IO_WRITE( 0xf0000010,  tmp) ;
	}

	// step6: release the cmu reset
	tmp =  IO_READ( GLOBAL_PHY_CONTROL );
	tmp |= 1 << ( CMU_RESETN_I + port *4) ;
	IO_WRITE( GLOBAL_PHY_CONTROL,  tmp);

	// step7 : Wait for CMU OK
	if ( waiting_register( PCIE_SATA_SATA2_PHY_SATA_CORE_PHY_STAT + port* 0x80, (1<< CMU_OK), ( 1<<CMU_OK ), 1000 ) == 0 ) {
		serial_puts( "Wait CMU OK failed\n");
		return -1;
	}

	// step8: Release the lane reset
	IO_WRITE( lane_addr + port* 0x4000 +200, 0x40002002);

	tmp =  IO_READ( GLOBAL_PHY_CONTROL );
	tmp |= 1 << ( LN0_RESETN_I + port *4) ;
	IO_WRITE( GLOBAL_PHY_CONTROL, tmp);

	// step9 : Wait for lane OK
	if ( waiting_register( PCIE_SATA_SATA2_PHY_SATA_CORE_PHY_STAT + port* 0x80, (1<< LANE_OK), (1<< LANE_OK), 1000 ) == 0) {
		serial_puts( "Wait lan OK failed\n");
		return -1;
	}

	mdelay( step8_delay);

	// PCIE_SATA_SATA2_CTRL_SATA_CORE_CTRL1
	tmp = IO_READ(PCIE_SATA_SATA2_CTRL_SATA_CORE_CTRL1 + port * 0x80);
	if( pole_inverse ) {
		tmp |= 0x80000106;
	} else {
		tmp |= 0x80000006;
	}
	IO_WRITE(PCIE_SATA_SATA2_CTRL_SATA_CORE_CTRL1 + port * 0x80, tmp);

	return 0;
}

static int host_controller_init( int port) 
{
	unsigned int tmp;

	IO_WRITE(GOLDENGATE_AHCI_BASE, 0x6B26ff83);
	IO_WRITE(GOLDENGATE_AHCI_BASE + AHCI_PI, 0xF);

	tmp = IO_READ(GLOBAL_STRAP);
	tmp = (tmp >> 1) & 0x07;
	switch ( tmp) {
	case 0:		// 133 MHZ
		tmp = 0x0A132039;
		break;
	case 2:		// 140 MHZ
		tmp = 0x0A14223C;
		break;
	case 1:
	case 4:
	case 5:		// 150 MHZ
		tmp = 0x0B152540;
		break;
	case 3:		// 160 MHZ
		tmp = 0x0B172744;
		break;
	case 6:		// 141.67 MHZ
		tmp = 0x0A14223C;
		break;
	default:
		return -1;
	}

	IO_WRITE(GOLDENGATE_AHCI_BASE + AHCI_TESTER, port << 16);
	IO_WRITE(GOLDENGATE_AHCI_BASE + AHCI_OOBR, 0x80000000);
	IO_WRITE(GOLDENGATE_AHCI_BASE + AHCI_OOBR, tmp);
	IO_WRITE(GOLDENGATE_AHCI_BASE + AHCI_TESTER, 0x00000000);

	tmp = IO_READ(GOLDENGATE_AHCI_BASE + AHCI_CTL);
	tmp |= AHCI_CTL_HR;
	IO_WRITE(GOLDENGATE_AHCI_BASE + AHCI_CTL, tmp);
	if (!waiting_register
	    (GOLDENGATE_AHCI_BASE + AHCI_CTL, AHCI_CTL_HR, 0,
	     AHCI_CTL_RESET_TIMEOUT)) {
		return -1;
	}

	IO_WRITE(GOLDENGATE_AHCI_BASE + 0xa4, 0x00003700);

	return 0;
}

static int issueAHCICmd(unsigned int port_mmio, const unsigned char *fis,
			int fis_len, unsigned char *buf, int buf_len,
			unsigned int timeout)
{

	memcpy((unsigned char *)priv.cmd_tbl, fis, fis_len);

	priv.cmd_sg->addr = (unsigned int)buf;

	if (buf_len > 0) {
		priv.cmd_sg->data_byte_count = buf_len - 1;
		priv.cmd_slot->descInfo = (fis_len >> 2) | (1 << 16);
	} else {
		priv.cmd_slot->descInfo = fis_len >> 2;
		priv.cmd_sg->data_byte_count = 0;
	}

	IO_WRITE(port_mmio + AHCI_PxCI, 1);

	if (waiting_register(port_mmio + AHCI_PxCI, 0x01, 0x0, timeout)) {
		return 1;
	}

	return 0;
}

static int ahci_set_feature(unsigned int port_mmio)
{
	static const unsigned char fis[20] = {
		[0] = 0x27,
		[1] = 1 << 7,
		[2] = 0xEF,
		[3] = CS_SETF_CMD_XFER,
		[12] = 0x46,
	};

	return issueAHCICmd(port_mmio, fis, sizeof(fis), NULL, 0,
			    SET_FEATURE_TIMEOUT);
}

static int ahci_identify(unsigned int port_mmio)
{
	unsigned char *tmpid;

	tmpid = MEM_ADDRESS + 0x400;

	static const unsigned char fis[20] = {
		[0] = 0x27,
		[1] = 1 << 7,
		[2] = 0xEC,
	};

	if (issueAHCICmd
	    (port_mmio, &fis[0], sizeof(fis), tmpid, IDEN_BUF_SIZE,
	     IDENTIFY_TIMEOUT)) {
		return 1;
	}
	return 0;
}

static int host_port_init(int port)
{

	unsigned int i, tmp, port_mmio;
	static const unsigned int value[] = { 0x304, 0x301, 0x300 };
	unsigned int mem;

	port_mmio = GOLDENGATE_AHCI_BASE + 0x100 + port * 0x80;

	priv.port_mmio = port_mmio;

	/* Phy reset  */
	for (i = 0; i < sizeof(value) / sizeof(value[0]); ++i) {
		tmp = IO_READ(port_mmio + AHCI_PxSCTL);
		tmp = (tmp & 0x0f0) | value[i];
		IO_WRITE(port_mmio + AHCI_PxSCTL, tmp);
		mdelay(1);
	}

	/* make sure port is not active */
	tmp = IO_READ(port_mmio + AHCI_PxCMD);
	i = AHCI_PxCMD_CR | AHCI_PxCMD_FR | AHCI_PxCMD_FRE | AHCI_PxCMD_ST;
	if (tmp & i) {
		IO_WRITE(port_mmio + AHCI_PxCMD, tmp & ~i);
		waiting_register(port_mmio + AHCI_PxCMD, AHCI_PxCMD_CR, 0,
				 1000);
	}

	IO_WRITE(port_mmio + AHCI_PxCMD, AHCI_PxCMD_SUD);

	tmp = IO_READ(port_mmio + AHCI_PxSERR);
	IO_WRITE(port_mmio + AHCI_PxSERR, tmp);

	if (!waiting_register(port_mmio + AHCI_PxSSTS, 0x0f, 0x03, 5000)) {
		serial_puts("link failed\n");
		return -1;
	}

	mem = MEM_ADDRESS;

	IO_WRITE(port_mmio + AHCI_PxCLB, mem);
	priv.cmd_slot = mem;

	mem += 256;
	IO_WRITE(port_mmio + AHCI_PxFB, mem);

	mem += AHCI_RX_FIS_SZ;
	priv.cmd_slot->cmdStatus = 0;
	priv.cmd_slot->tblBaseAddr = mem;
	priv.cmd_tbl = mem;

	mem += AHCI_CMD_TBL_HDR;
	priv.cmd_sg = (struct ahci_sg *)mem;
	priv.cmd_sg->addr_hi = 0;

	tmp =
	    AHCI_PxCMD_ICC | AHCI_PxCMD_FRE | AHCI_PxCMD_POD | AHCI_PxCMD_SUD |
	    AHCI_PxCMD_ST;
	IO_WRITE(port_mmio + AHCI_PxCMD, tmp);

	if (ahci_set_feature(port_mmio) == 0) {
		serial_puts("set feature failed\n");
		return -1;
	}

	if (ahci_identify(port_mmio) == 0) {
		serial_puts("identify failed\n");
		return -1;
	}

	return 0;
}

int g2_sata_initial(int argc, char **argv)
{
	int port;
	int external_clock;
	int pole_inverse;

	unsigned int tmp;

	if (argc > 4)
		return -1;

	port = iros_strtol(argv[1]);
	if (port < 0 || port > 4)
		return -1;
	
	pole_inverse = argc >= 3 && iros_strtol( argv[2]);

	if( argc >= 4 ) {
		external_clock = iros_strtol( argv[3]);
	} else {
		external_clock = 0;
	}

	if ( sata_sb_phy_initial(port, pole_inverse, external_clock ) != 0) {
		serial_puts("g2_phy_initial failed\n");
		goto SB_PHY_POWER_DOWN;
	}

	if (host_controller_init(port) != 0) {
		serial_puts("g2_host_control failed\n");
		goto SB_PHY_POWER_DOWN;
	}

	if (host_port_init(port ) != 0) {
		serial_puts("g2_host_port failed\n");
		goto SB_PHY_POWER_DOWN;
	}

	serial_puts("g2_sata_initial ok\n");

SB_PHY_POWER_DOWN:

	tmp =  IO_READ( GLOBAL_PHY_CONTROL );

	tmp &= ~(1 << ( POR_N_I + port*4)); // por_n_i
	tmp |= (1 << ( PD + port ));
	tmp &= ~(1 << ( CMU_RESETN_I + port *4)) ;
	tmp &= ~(1 << ( LN0_RESETN_I + port *4)) ;

	IO_WRITE( GLOBAL_PHY_CONTROL, tmp );

	return 0;
}
