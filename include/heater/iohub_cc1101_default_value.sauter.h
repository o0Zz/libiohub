#include "iohub_cc1101.h"

//https://www.ti.com/lit/ds/symlink/cc1101.pdf

#define CC1101_DEFVAL_IOCFG2           0x29
#define CC1101_DEFVAL_IOCFG1           0x2E
#define CC1101_DEFVAL_IOCFG0           0x06 //06 = Asserts when sync word has been sent / received and de-asserts at the end of the packet. In RX the pin will also deassert when a packet is discarded due to address or maximum length filtering or when the radio enters RXFIFO_OVERFLOW state. In TX the pin will de-assert if the TX FIFO underflows
#define CC1101_DEFVAL_FIFOTHR          0x0F
#define CC1101_DEFVAL_SYNC1            0x50
#define CC1101_DEFVAL_SYNC0            0x56
#define CC1101_DEFVAL_PKTLEN           0x28
#define CC1101_DEFVAL_PKTCTRL1         0x40
#define CC1101_DEFVAL_PKTCTRL0         0x01
#define CC1101_DEFVAL_ADDR             0x00 //Device Addr
#define CC1101_DEFVAL_CHANNR           0x00
#define CC1101_DEFVAL_FSCTRL1          0x06
#define CC1101_DEFVAL_FSCTRL0          0x00
#define CC1101_DEFVAL_FREQ2            0x21
#define CC1101_DEFVAL_FREQ1            0x64
#define CC1101_DEFVAL_FREQ0            0xF2
#define CC1101_DEFVAL_MDMCFG4          0x88
#define CC1101_DEFVAL_MDMCFG3          0x83
#define CC1101_DEFVAL_MDMCFG2          0x09
#define CC1101_DEFVAL_MDMCFG1          0x72
#define CC1101_DEFVAL_MDMCFG0          0xF8
#define CC1101_DEFVAL_DEVIATN          0x41
#define CC1101_DEFVAL_MCSM2            0x07
#define CC1101_DEFVAL_MCSM1            0x00 
#define CC1101_DEFVAL_MCSM0            0x18 
#define CC1101_DEFVAL_FOCCFG           0x36
#define CC1101_DEFVAL_BSCFG            0x6C
#define CC1101_DEFVAL_AGCCTRL2         0x03
#define CC1101_DEFVAL_AGCCTRL1         0x40
#define CC1101_DEFVAL_AGCCTRL0         0x91
#define CC1101_DEFVAL_WOREVT1          0x00
#define CC1101_DEFVAL_WOREVT0          0x00
#define CC1101_DEFVAL_WORCTRL          0x00
#define CC1101_DEFVAL_FREND1           0x56
#define CC1101_DEFVAL_FREND0           0x10
#define CC1101_DEFVAL_FSCAL3           0xE9
#define CC1101_DEFVAL_FSCAL2           0x2A
#define CC1101_DEFVAL_FSCAL1           0x00
#define CC1101_DEFVAL_FSCAL0           0x1F
#define CC1101_DEFVAL_RCCTRL1          0x00
#define CC1101_DEFVAL_RCCTRL0          0x00
         
#define CC1101_PATABLE_FIRSTBYTE	   0x26

static u8 sC1101SauterConfig[CC1101_REGISTERS_COUNT] = 
{
	CC1101_DEFVAL_IOCFG2,
	CC1101_DEFVAL_IOCFG1,
	CC1101_DEFVAL_IOCFG0,
	CC1101_DEFVAL_FIFOTHR,
	CC1101_DEFVAL_SYNC1,
	CC1101_DEFVAL_SYNC0,
	CC1101_DEFVAL_PKTLEN,
	CC1101_DEFVAL_PKTCTRL1,
	CC1101_DEFVAL_PKTCTRL0,
	CC1101_DEFVAL_ADDR,
	CC1101_DEFVAL_CHANNR,
	CC1101_DEFVAL_FSCTRL1,
	CC1101_DEFVAL_FSCTRL0,
	CC1101_DEFVAL_FREQ2,
	CC1101_DEFVAL_FREQ1,
	CC1101_DEFVAL_FREQ0,
	CC1101_DEFVAL_MDMCFG4,
	CC1101_DEFVAL_MDMCFG3,
	CC1101_DEFVAL_MDMCFG2,
	CC1101_DEFVAL_MDMCFG1,
	CC1101_DEFVAL_MDMCFG0,
	CC1101_DEFVAL_DEVIATN,
	CC1101_DEFVAL_MCSM2,
	CC1101_DEFVAL_MCSM1,
	CC1101_DEFVAL_MCSM0,
	CC1101_DEFVAL_FOCCFG,
	CC1101_DEFVAL_BSCFG,
	CC1101_DEFVAL_AGCCTRL2,
	CC1101_DEFVAL_AGCCTRL1,
	CC1101_DEFVAL_AGCCTRL0,
	CC1101_DEFVAL_WOREVT1,
	CC1101_DEFVAL_WOREVT0,
	CC1101_DEFVAL_WORCTRL,
	CC1101_DEFVAL_FREND1,
	CC1101_DEFVAL_FREND0,
	CC1101_DEFVAL_FSCAL3,
	CC1101_DEFVAL_FSCAL2,
	CC1101_DEFVAL_FSCAL1,
	CC1101_DEFVAL_FSCAL0,
	CC1101_DEFVAL_RCCTRL1,
	CC1101_DEFVAL_RCCTRL0
};


/*
#Introduction
	Below comment explain sauter remote controller initialization and command set.
	
# Chipset initialization
	0x00 0x29
	0x01 0x2E
	0x02 0x06
	0x03 0x0F
	0x04 0x50
	0x05 0x56
	0x06 0x28
	0x07 0x40
	0x08 0x01
	0x09 0x00
	0x0A 0x00
	0x0B 0x06
	0x0C 0x00
	0x0D 0x21
	0x0E 0x64
	0x0F 0xF2
	0x10 0x88
	0x11 0x83
	0x12 0x09
	0x13 0x72
	0x14 0xF8
	0x15 0x41
	0x16 0x07
	0x17 0x00
	0x18 0x18
	0x19 0x36
	0x1A 0x6C
	0x1B 0x03
	0x1C 0x40
	0x1D 0x91
	0x1E 0x00
	0x1F 0x00
	0x20 0x00
	0x21 0x56
	0x22 0x10
	0x23 0xE9 
	0x24 0x2A
	0x25 0x00
	0x26 0x1F
	0x27 0x00
	0x28 0x00

	0x3E 0x26 (PATABLE) //If one byte is written to the PATABLE and this value is to be read out, CSn must be set high before the read access in order to set the index counter back to zero.
						//Note that the content of the PATABLE is lost when entering the SLEEP state, except for the first byte (index 0).

	0x36 (SIDLE)
	0x33 (SCAL)
	0x3A (SFRX)
	0x3B (SFTX)
	0x39 (SPWD)


# Sending a packet (Power On)

	 //After a while
	 
	0x36 (SIDLE)
	0x39 (SPWD) - Power down when CSn goes high

	 //After a while

	0x36 (SIDLE)
	0x3B (SFTX) - Flush TX fifo
	0x3A (SFRX) - Flush RX

	 // After 4ms

	0x7F 0x19 0x00 0x20 0x5A 0xB5 0x07 0x52 0x00 0x00 0x00 0x54 0x00 0x00 0x00 0x62 0x01 0x04 0x00 0x70 0x00 0x00 0x00 0x08 0x0B 0xDA 0x77  
	0x35 (STX) Enable TX

	 // 0x7F=Burst tx fifo - Need to set R/W bit  to 0

*/