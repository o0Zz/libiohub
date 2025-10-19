#include "drv_cc1101.h"

//https://github.com/SpaceTeddy/CC1101
//https://www.ti.com/lit/ds/symlink/cc1101.pdf?ts=1598951166051&ref_url=https%253A%252F%252Fwww.google.com%252F
//Errata: https://www.ti.com/lit/er/swrz020e/swrz020e.pdf?ts=1599310539463&ref_url=https%253A%252F%252Fwww.ti.com%252Fproduct%252FCC1101

#define DEBUG
#ifdef DEBUG
#	define LOG_DEBUG_CC1101(...)				log_debug(__VA_ARGS__)
#	define LOG_ERROR_CC1101(...)				log_error(__VA_ARGS__)
#else
#	define LOG_DEBUG_CC1101(...)				do{}while(0)
#	define LOG_ERROR_CC1101(...)				do{}while(0)
#endif

/**
 * Type of transfers
 */
#define WRITE_BURST              0x40
#define READ_SINGLE              0x80
#define READ_BURST               0xC0

/**
 * Type of register
 */
 /* Note:
	According to documentation status/config register are differenciate by READ_SINGLE/READ_BURST 
	however, it doesn't means you will read burst, which is a bit confusing
*/
#define CC1101_CONFIG_REGISTER   READ_SINGLE
#define CC1101_STATUS_REGISTER   READ_BURST

/**
 * PATABLE & FIFO's
 */
#define CC1101_PATABLE           0x3E        // PATABLE address
#define CC1101_TXFIFO            0x3F        // TX FIFO address
#define CC1101_RXFIFO            0x3F        // RX FIFO address

/**
 * Command strobes
 */
#define CC1101_SRES              0x30        // Reset CC1101 chip
#define CC1101_SFSTXON           0x31        // Enable and calibrate frequency synthesizer (if MCSM0.FS_AUTOCAL=1). If in RX (with CCA):
                                             // Go to a wait state where only the synthesizer is running (for quick RX / TX turnaround).
#define CC1101_SXOFF             0x32        // Turn off crystal oscillator
#define CC1101_SCAL              0x33        // Calibrate frequency synthesizer and turn it off. SCAL can be strobed from IDLE mode without
                                             // setting manual calibration mode (MCSM0.FS_AUTOCAL=0)
#define CC1101_SRX               0x34        // Enable RX. Perform calibration first if coming from IDLE and MCSM0.FS_AUTOCAL=1
#define CC1101_STX               0x35        // In IDLE state: Enable TX. Perform calibration first if MCSM0.FS_AUTOCAL=1.
                                             // If in RX state and CCA is enabled: Only go to TX if channel is clear
#define CC1101_SIDLE             0x36        // Exit RX / TX, turn off frequency synthesizer and exit Wake-On-Radio mode if applicable
#define CC1101_SWOR              0x38        // Start automatic RX polling sequence (Wake-on-Radio) as described in Section 19.5 if
                                             // WORCTRL.RC_PD=0
#define CC1101_SPWD              0x39        // Enter power down mode when CSn goes high
#define CC1101_SFRX              0x3A        // Flush the RX FIFO buffer. Only issue SFRX in IDLE or RXFIFO_OVERFLOW states
#define CC1101_SFTX              0x3B        // Flush the TX FIFO buffer. Only issue SFTX in IDLE or TXFIFO_UNDERFLOW states
#define CC1101_SWORRST           0x3C        // Reset real time clock to Event1 value
#define CC1101_SNOP              0x3D        // No operation. May be used to get access to the chip status byte

/**
 * Status registers:    drv_cc1101_read_status_reg(aCtx, XXXXXX)
 */
#define CC1101_PARTNUM           0x30        // Chip ID
#define CC1101_VERSION           0x31        // Chip ID
#define CC1101_FREQEST           0x32        // Frequency Offset Estimate from Demodulator
#define CC1101_LQI               0x33        // Demodulator Estimate for Link Quality
#define CC1101_RSSI              0x34        // Received Signal Strength Indication
#define CC1101_MARCSTATE         0x35        // Main Radio Control State Machine State
#define CC1101_WORTIME1          0x36        // High Byte of WOR Time
#define CC1101_WORTIME0          0x37        // Low Byte of WOR Time
#define CC1101_PKTSTATUS         0x38        // Current GDOx Status and Packet Status
#define CC1101_VCO_VC_DAC        0x39        // Current Setting from PLL Calibration Module
#define CC1101_TXBYTES           0x3A        // Underflow and Number of Bytes
#define CC1101_RXBYTES           0x3B        // Overflow and Number of Bytes
#define CC1101_RCCTRL1_STATUS    0x3C        // Last RC Oscillator Calibration Result
#define CC1101_RCCTRL0_STATUS    0x3D        // Last RC Oscillator Calibration Result 

typedef enum
{
    STATE_SLEEP = 0x00,
    STATE_IDLE,
    STATE_XOFF,
    STATE_VCOON_MC,
    STATE_REGON_MC,
    STATE_MANCAL,
    STATE_VCOON,
    STATE_REGON,
    STATE_STARTCAL,
    STATE_SETTLING_BWBOOST,
    STATE_SETTLING_FS_LOCK, //0x0A = 10
    STATE_SETTLING_IFADCON,
    STATE_ENDCAL,
    STATE_RX,
    STATE_RX_END,
    STATE_RX_RST, //0x0F = 15
    STATE_TXRX_SWITCH, //0x10
    STATE_RXFIFO_OVERFLOW,
    STATE_FSTXON,
    STATE_TX,
    STATE_TX_END, //0x14 = 20
    STATE_RXTX_SWITCH, //0x15
    STATE_TXFIFO_UNDERFLOW,
	
		//For internal use only

	STATE_COUNT
}STATE;

/**
 * Macros
 */
#define getState(aCtx)                  (drv_cc1101_read_status_reg(aCtx, CC1101_MARCSTATE) & 0x1F)

#define setRxState(aCtx)                drv_cc1101_strobe(aCtx, CC1101_SRX)
#define setTxState(aCtx)                drv_cc1101_strobe(aCtx, CC1101_STX)
#define setIdleState(aCtx)              drv_cc1101_strobe(aCtx, CC1101_SIDLE)
#define setPowerDown(aCtx)              drv_cc1101_strobe(aCtx, CC1101_SPWD)

#define flushRxFifo(aCtx)               drv_cc1101_strobe(aCtx, CC1101_SFRX)
#define flushTxFifo(aCtx)               drv_cc1101_strobe(aCtx, CC1101_SFTX)

#define printState(aCtx)				drv_cc1101_print_state(getState(aCtx))

void drv_cc1101_reset(cc1101_ctx *aCtx);

/* -------------------------------------------------------------- */

/*
	Refer to page 31 of cc1101.pdf

       10.1 Chip Status Byte
        When the header byte, data byte, or command
        strobe is sent on the SPI interface, the chip
        status byte is sent by the CC1101 on the SO pin.
        The status byte contains key status signals,
        useful for the MCU. The first bit, s7, is the
        CHIP_RDYn signal and this signal must go low
        before the first positive edge of SCLK. The
        CHIP_RDYn signal indicates that the crystal is
        running.
		
	Bit 7    = CHIP_RDY
	Bit 6:4  = STATE[2:0]
	Bit 3:0  = FIFO_BYTES_AVAILABLE[3:0]
  */

ret_code_t drv_cc1101_check_status(const char *aFct, uint8_t aStatus)
{
	//LOG_DEBUG_CC1101("%s Status (n-1): 0x%x\n", aFct, aStatus);

	/*switch ((aStatus >> 4) & 0x07)
	{
		case 0x00: LOG_DEBUG_CC1101("IDLE"); break;
		case 0x01: LOG_DEBUG_CC1101("RX"); break;
		case 0x02: LOG_DEBUG_CC1101("TX"); break;
		case 0x03: LOG_DEBUG_CC1101("FSTXON"); break;
		case 0x04: LOG_DEBUG_CC1101("STARTCAL"); break;
		case 0x05: LOG_DEBUG_CC1101("SETTLING"); break;
		case 0x06: LOG_DEBUG_CC1101("RXFIFO_OVERFLOW"); break;
		case 0x07: LOG_DEBUG_CC1101("TXFIFO_UNDERFLOW"); break;
		default:   LOG_DEBUG_CC1101("UNKNOWN"); break;
	}
  
	LOG_DEBUG_CC1101("\n");
	
	if ( (aStatus & 0x80) != 0x00 )
	{
		log_error("Error: Chipset not ready, something goes wrong !\n");
		return E_INVALID_STATE;
	}
	*/
	return SUCCESS;
}

/* -------------------------------------------------------------- */

void drv_cc1101_print_state(uint8_t aState)
{
	static const char *sStateStr[STATE_COUNT] = 
	{
		"SLEEP",
		"IDLE",
		"XOFF",
		"VCOON_MC",
		"REGON_MC",
		"MANCAL",
		"VCOON",
		"REGON",
		"STARTCAL",
		"SETTLING_BWBOOST",
		"SETTLING_FS_LOCK",
		"SETTLING_IFADCON",
		"ENDCAL",
		"RX",
		"RX_END",
		"RX_RST",
		"TXRX_SWITCH",
		"RXFIFO_OVERFLOW",
		"FSTXON",
		"TX",
		"TX_END",
		"RXTX_SWITCH",
		"TXFIFO_UNDERFLOW"
	};
	
	if (aState < STATE_COUNT)
		LOG_DEBUG_CC1101("Chipset State %s\n", sStateStr[aState]);
	else
		LOG_DEBUG_CC1101("Chipset State UNKNOWN !\n");
}

/* -------------------------------------------------------------- */

u8 drv_cc1101_read_reg(cc1101_ctx *aCtx, u8 aRegAddr, u8 aType) 
{
	ASSERT(aCtx->mWakeupCount > 0);
	
	drv_spi_select(&aCtx->mSPICtx);

	u8 theReg = aRegAddr | aType;
	u8 theValue = 0x00;
	
	ret_code_t lErr = drv_cc1101_check_status( __FUNCTION__, drv_spi_transfer_byte(&aCtx->mSPICtx, theReg) );
	if (lErr == SUCCESS)
	{
		time_delay_us(10);  // Wait for chipset to be ready
		theValue = drv_spi_transfer_byte(&aCtx->mSPICtx, 0x00);
	}
	
	drv_spi_deselect(&aCtx->mSPICtx);
		
	//LOG_DEBUG_CC1101("Read register 0x%X = 0x%X\n", aRegAddr, theValue);
	
	return theValue;
}

/* -------------------------------------------------------------- */

u8 drv_cc1101_read_status_reg(cc1101_ctx *aCtx, u8 aRegAddr) 
{
	return drv_cc1101_read_reg(aCtx, aRegAddr, CC1101_STATUS_REGISTER);
}

/* -------------------------------------------------------------- */

u8 drv_cc1101_read_config_reg(cc1101_ctx *aCtx, u8 aRegAddr) 
{
	return drv_cc1101_read_reg(aCtx, aRegAddr, CC1101_CONFIG_REGISTER);
}

/* -------------------------------------------------------------- */

ret_code_t drv_cc1101_strobe(cc1101_ctx *aCtx, u8 aStrobe)
{
	LOG_DEBUG_CC1101("Write strobe %x\n", aStrobe);
	
	//ASSERT(aCtx->mWakeupCount > 0);
	
	drv_spi_select(&aCtx->mSPICtx);

	ret_code_t theRet = drv_cc1101_check_status( __FUNCTION__, drv_spi_transfer_byte(&aCtx->mSPICtx, aStrobe) );
	
	drv_spi_deselect(&aCtx->mSPICtx);
	
	return theRet;
}
 
/* -------------------------------------------------------------- */

ret_code_t drv_cc1101_read_burst_reg(cc1101_ctx *aCtx, u8 aRegAddr, u8 *aBuffer, u16 aLength) 
{
	ASSERT(aCtx->mWakeupCount > 0);
	
    drv_spi_select(&aCtx->mSPICtx);

    ret_code_t lErr = drv_cc1101_check_status( __FUNCTION__, drv_spi_transfer_byte(&aCtx->mSPICtx, aRegAddr | READ_BURST) );
	if (lErr == SUCCESS)
		lErr = drv_spi_transfer(&aCtx->mSPICtx, aBuffer, aLength);

    drv_spi_deselect(&aCtx->mSPICtx);
	
	return lErr;
}

/* -------------------------------------------------------------- */

ret_code_t drv_cc1101_write_reg(cc1101_ctx *aCtx, u8 aRegAddr, u8 aValue) 
{
	ASSERT(aCtx->mWakeupCount > 0);
	
	LOG_DEBUG_CC1101("Write reg 0x%X -> 0x%X\n", aRegAddr, aValue);
	
	drv_spi_select(&aCtx->mSPICtx);

	ret_code_t lErr = drv_cc1101_check_status( __FUNCTION__,  drv_spi_transfer_byte(&aCtx->mSPICtx, aRegAddr));
	if (lErr == SUCCESS)
		lErr = drv_spi_transfer(&aCtx->mSPICtx, &aValue, 1);
	
	drv_spi_deselect(&aCtx->mSPICtx);
	
	return lErr;
}

/* -------------------------------------------------------------- */

ret_code_t drv_cc1101_write_burst_reg(cc1101_ctx *aCtx, u8 aRegAddr, u8 *aBuffer, u16 aLength)
{
	ASSERT(aCtx->mWakeupCount > 0);
	
	LOG_DEBUG_CC1101("Write burst reg %x (len: %d)\n", aRegAddr | WRITE_BURST, aLength);
	
    drv_spi_select(&aCtx->mSPICtx);

    ret_code_t lErr = drv_cc1101_check_status( __FUNCTION__, drv_spi_transfer_byte(&aCtx->mSPICtx, aRegAddr | WRITE_BURST) );
	if (lErr == SUCCESS)
		lErr = drv_spi_transfer(&aCtx->mSPICtx, aBuffer, aLength);

    drv_spi_deselect(&aCtx->mSPICtx);
	
	return lErr;
}
	
/* -------------------------------------------------------------- */

BOOL drv_cc1101_wait_for_state(cc1101_ctx *aCtx, u8 aState, u32 aTimeoutMS)
{
	u32 theStartTimeMs = TIMER_START();
	
	while(TIMER_ELAPSED(theStartTimeMs) < aTimeoutMS)
	{
		if (getState(aCtx) == aState)
			return TRUE;
		
		time_delay_ms(1);
	}
	
	return FALSE;
}
	
/* -------------------------------------------------------------- */

static INLINE void drv_cc1101_interrupt_data_received(void) 
{
	LOG_DEBUG_CC1101("PIN TRIGGERED!!!\n");
    //packetReceived = true;
}

/* -------------------------------------------------------------- */

ret_code_t drv_cc1101_init(cc1101_ctx *aCtx, u32 aCSnPin, u32 aGDO0Pin, uint8_t aDefaultConfig[CC1101_REGISTERS_COUNT], uint8_t aFirstBytePaTable)
{
    ret_code_t   theRet;

    memset(aCtx, 0x00, sizeof(cc1101_ctx));

	aCtx->mGDO0Pin = aGDO0Pin;
	
    drv_digital_set_pin_mode(aCtx->mGDO0Pin, PinMode_Input);
    
    theRet = drv_spi_init(&aCtx->mSPICtx, aCSnPin, SPIMode_WaitForMisoLowAfterSelect);
    if (theRet != SUCCESS)
        return theRet;
    
	drv_cc1101_wakeup(aCtx);
	
	drv_cc1101_reset(aCtx);

    u8 theVersion = drv_cc1101_read_status_reg(aCtx, CC1101_VERSION);
    if ( theVersion != 0x14)
    {
        log_error("Invalid version: 0x%X\n", theVersion);
		
		drv_cc1101_standby(aCtx);

        return E_DEVICE_NOT_FOUND;
    }
	
        //Set default configuration
    for (uint8_t i=0; i<CC1101_REGISTERS_COUNT; i++) 
	{
		if (aDefaultConfig[i] != 0xFF)
			drv_cc1101_write_reg(aCtx, i,  aDefaultConfig[i]);
	}
		
	/* 
	If one byte is written to the PATABLE and this value is to be read out, CSn must be set high before the read access in order to set the index counter back to zero.
	Note that the content of the PATABLE is lost when entering the SLEEP state, except for the first byte (index 0).
	*/
		
	drv_cc1101_write_burst_reg(aCtx, CC1101_PATABLE, &aFirstBytePaTable, sizeof(aFirstBytePaTable));		//CC1101 PATABLE config
	
	setIdleState(aCtx);
	
	drv_cc1101_strobe(aCtx, CC1101_SCAL);
	
	flushRxFifo(aCtx);
	flushTxFifo(aCtx);
	
	return drv_cc1101_standby(aCtx);
}

/* -------------------------------------------------------------- */

void drv_cc1101_uninit(cc1101_ctx *aCtx)
{
    drv_spi_uninit(&aCtx->mSPICtx);
}

/* -------------------------------------------------------------- */

void drv_cc1101_reset(cc1101_ctx *aCtx)
{
	ASSERT(aCtx->mWakeupCount > 0);
		
    drv_cc1101_strobe(aCtx, CC1101_SRES);
	time_delay_ms(1);
}

/* -------------------------------------------------------------- */

ret_code_t drv_cc1101_wakeup(cc1101_ctx *aCtx)
{
	if (aCtx->mWakeupCount++ == 0)
	{
		LOG_DEBUG_CC1101("Wakeup CC1101 ...\n");
		
		drv_spi_select(&aCtx->mSPICtx); //Low
		time_delay_us(10);
		drv_spi_deselect(&aCtx->mSPICtx); //High
		time_delay_us(40);

		setIdleState(aCtx);
		flushRxFifo(aCtx);
		flushTxFifo(aCtx);

		attachInterrupt(digitalPinToInterrupt(aCtx->mGDO0Pin), drv_cc1101_interrupt_data_received, RISING); //Low to High
	}

	ASSERT(aCtx->mWakeupCount < 5);

	return SUCCESS;
}

/* -------------------------------------------------------------- */

ret_code_t drv_cc1101_standby(cc1101_ctx *aCtx)
{
	if (--aCtx->mWakeupCount == 0)
	{
		LOG_DEBUG_CC1101("Standby CC1101 ...\n");
		
		detachInterrupt(digitalPinToInterrupt(aCtx->mGDO0Pin));
	
		setIdleState(aCtx);	
		setPowerDown(aCtx);
	}
	
	ASSERT(aCtx->mWakeupCount >= 0);
	
	return SUCCESS;
}

/* -------------------------------------------------------------- */

ret_code_t drv_cc1101_send_data(cc1101_ctx *aCtx, cc1101_packet_ctx *aPacket)
{
	LOG_DEBUG_CC1101("drv_cc1101_send %d bytes...\n", aPacket->mLength);
	
	u8 theBuffer[32] = {0x00};

	drv_cc1101_wakeup(aCtx);

	theBuffer[0] = aPacket->mLength;
	theBuffer[1] = aPacket->mDst;
	memcpy(&theBuffer[2], aPacket->mData, aPacket->mLength);

    drv_cc1101_write_burst_reg(aCtx, CC1101_TXFIFO, theBuffer, aPacket->mLength + 2);

	setTxState(aCtx);
	
	(void)drv_cc1101_wait_for_state(aCtx, STATE_IDLE, 1000);

    return drv_cc1101_standby(aCtx);
}

/* -------------------------------------------------------------- */

BOOL drv_cc1101_is_data_available(cc1101_ctx *aCtx)
{
	ASSERT(aCtx->mWakeupCount > 0);

	u8 theValue = drv_cc1101_read_status_reg(aCtx, CC1101_RXBYTES);
	if (theValue > 0)
	{
		u8 theState = getState(aCtx);
		
		if (theState == STATE_IDLE || theState == STATE_RXFIFO_OVERFLOW) //Device will automatically enter in IDLE state when it will fully receive a message
			return TRUE;
	}
	
	return FALSE;
	
	/*
	if ( drv_digital_read(aCtx->mGDO0Pin) == PinLevel_High )
	{
		LOG_DEBUG_CC1101(".");
		//if(drv_cc1101_read_config_reg(aCtx, CC1101_IOCFG0) == 0x06)
        {
            while(drv_digital_read(aCtx->mGDO0Pin) == PinLevel_High)
                LOG_DEBUG_CC1101("!");
        }

		return TRUE;
	}
	
	return FALSE;
	*/
}


/* -------------------------------------------------------------- */

ret_code_t drv_cc1101_set_receive(cc1101_ctx *aCtx)
{
	LOG_DEBUG_CC1101("Enter in RX state !\n");
	
	ASSERT(aCtx->mWakeupCount > 0);
	
	setIdleState(aCtx);
	flushRxFifo(aCtx);
		
	setRxState(aCtx);
	
	(void)drv_cc1101_wait_for_state(aCtx, STATE_RX, 1000);
	
	return SUCCESS;
}

/* -------------------------------------------------------------- */

ret_code_t drv_cc1101_receive_data(cc1101_ctx *aCtx, cc1101_packet_ctx *aPacket)
{
	ret_code_t theErr = SUCCESS;
	
    aPacket->mLength = 0;
	
	drv_cc1101_wakeup(aCtx);
	
	u8 theValue = drv_cc1101_read_status_reg(aCtx, CC1101_RXBYTES);
	if (theValue & 0x80) //RX fifo overflow
	{
		log_error("RX FIFO Overflow, drop packets\n");
		
		drv_cc1101_set_receive(aCtx); //Re-enter in RX State
		
		theErr = E_READ_ERROR;
	}
	else if (theValue > 0)
	{
		aPacket->mLength = theValue & 0x7F;

		if (aPacket->mLength != 0) //RX bytes pending ?
		{
			LOG_DEBUG_CC1101("Reading data from FIFO (Len: %d)...\n", aPacket->mLength);
			
				// Read data packet
			drv_cc1101_read_burst_reg(aCtx, CC1101_RXFIFO, aPacket->mData, aPacket->mLength);

		    theValue = drv_cc1101_read_status_reg(aCtx, CC1101_LQI);
			aPacket->mLinkQualityIndex = (theValue & 0x7F);
			aPacket->mfCrcOk = (theValue & 0x80) ? TRUE : FALSE;
			aPacket->mRSSI = drv_cc1101_read_status_reg(aCtx, CC1101_RSSI);
			
			drv_cc1101_set_receive(aCtx); //Re-enter in RX State
		}
		else
		{
			LOG_DEBUG_CC1101("No data pending\n");
		}
	}
	
    (void)drv_cc1101_standby(aCtx);
	
	return theErr;
}
