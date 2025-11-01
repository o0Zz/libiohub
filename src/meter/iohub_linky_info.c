#include "meter/iohub_linky_info.h"

//https://github.com/jaysee/teleInfo

#define LOG_LINKY
#ifdef LOG_LINKY
#	define LOG_LINKY_DEBUG(...)				IOHUB_LOG_DEBUG(__VA_ARGS__)
#else
#	define LOG_LINKY_DEBUG(...)				do{}while(0)
#endif

/* ----------------------------------------------------------------- */

const char sTeleInfoLabels[TELEINFO_COUNT][10] = {
	"ISOUSC",
	"IINST",
	"IINST1",
	"IINST2",
	"IINST3",
	"ADPS",
	"IMAX",
	"IMAX1",
	"IMAX2",
	"IMAX3",
	"PAPP",
	"PMAX",
	"BASE",
	"HCHC",
	"HCHP",
	"EJP_HN",
	"EJP_HPM",
	"PEJP",
	"BBR_HC_JB",
	"BBR_HP_JB",
	"BBR_HC_JW",
	"BBR_HP_JW",
	"BBR_HC_JR",
	"BBR_HP_JR",
	"HHPHC",
	"MOTDETAT",
	"ADCO",
	"OPTARIF",
	"PTEC",
	"DEMAIN"
};

/* ----------------------------------------------------------------- */

ret_code_t iohub_linky_info_init(linky_info *aCtx, u8 aRxPin)
{
    memset(aCtx, 0x00, sizeof(linky_info));

	return iohub_uart_init(&aCtx->mUart, UART_NOT_USED_PIN, aRxPin, 1200, UART_8N1);
}

/* ----------------------------------------------------------------- */

void iohub_linky_info_uninit(linky_info *aCtx)
{
	iohub_uart_close(&aCtx->mUart);
}

/* ----------------------------------------------------- */

	static const char *iohub_linky_info_read_line(linky_info *aCtx) 
	{
		/*
			Line format:  "KEY VALUE CRC\r\n"
		*/
		
		if (iohub_uart_data_available(&aCtx->mUart) > 0)
		{
			char theChar = iohub_uart_read_byte(&aCtx->mUart) & 0x7F;
			
			if (theChar == 0x02 /*STX*/ || theChar == 0x03 /*ETX*/)
				return NULL; //Nothing to do, skip STX and ETX and continue
		
			aCtx->mLine[aCtx->mLineIdx++] = theChar;

			if (theChar == '\n' || (aCtx->mLineIdx >= sizeof(aCtx->mLine) - 1))
			{
				if (aCtx->mLineIdx > 7)
				{
					//log_buffer(aCtx->mLine, aCtx->mLineIdx);
				
					aCtx->mLineIdx -= 3; // -3 = CRC + \r + \n
					
					u8 crc_computed = 0x20;
					for (u8 i=0; i<aCtx->mLineIdx; i++)
						crc_computed += aCtx->mLine[i];
					crc_computed = (crc_computed & 0x3F) + 0x20;
					
					u8 crc_read = aCtx->mLine[aCtx->mLineIdx];
					
					aCtx->mLine[aCtx->mLineIdx - 1] = '\0'; //Cut the string just before the CRC (Replace Space by \0)
					aCtx->mLineIdx = 0;
					
					if (crc_computed == crc_read)
						return aCtx->mLine;
				
					IOHUB_LOG_ERROR("TeleInfo: Invalid CRC: %s (Got: 0x%x, Computed: 0x%x)", aCtx->mLine, crc_read, crc_computed);
				}
				else
				{
					aCtx->mLineIdx = 0;
					IOHUB_LOG_ERROR("TeleInfo: Invalid string (Too small)");
				}
			}
		}

		return NULL;
	}
	
/* ----------------------------------------------------- */

BOOL iohub_linky_info_run(linky_info *aCtx) 
{
	const char *theLine = iohub_linky_info_read_line(aCtx);
	
	if (theLine != NULL)
	{
		//LOG_LINKY_DEBUG("%s\n", theLine);
		
		for (u8 i=0; i<TELEINFO_COUNT; i++)
		{
			if (strncmp(sTeleInfoLabels[i], theLine, strlen(sTeleInfoLabels[i])) == 0)
			{
				const char *theValue = &theLine[strlen(sTeleInfoLabels[i]) + 1];
				
				if (i == OPTARIF)
				{
					if(strncmp("BASE", theValue, 4) == 0)
						aCtx->mTeleInfo[i] = (u32)OPTARIF_BASE;
					else if(strncmp("HC..", theValue, 4) == 0)
						aCtx->mTeleInfo[i] = (u32)OPTARIF_HC;
					else if(strncmp("EJP", theValue, 3) == 0)
						aCtx->mTeleInfo[i] = (u32)OPTARIF_EJP;
					else if(strncmp("BBR", theValue, 3) == 0)
						aCtx->mTeleInfo[i] = (u32)OPTARIF_BBRx;
				}
				else if (i == PTEC)
				{
					if(strncmp("TH..", theValue, 4) == 0)
						aCtx->mTeleInfo[i] = (u32)PTEC_TH; 
					else if(strncmp("HC..", theValue, 4) == 0)
						aCtx->mTeleInfo[i] = (u32)PTEC_HC;  
					else if(strncmp("HP..", theValue, 4) == 0)
						aCtx->mTeleInfo[i] = (u32)PTEC_HP;  
					else if(strncmp("HN..", theValue, 4) == 0)
						aCtx->mTeleInfo[i] = (u32)PTEC_HN; 
					else if(strncmp("PM..", theValue, 4) == 0)
						aCtx->mTeleInfo[i] = (u32)PTEC_PM;  
					else if(strncmp("HCJB", theValue, 4) == 0)
						aCtx->mTeleInfo[i] = (u32)PTEC_HCJB;
					else if(strncmp("HCJW", theValue, 4) == 0)
						aCtx->mTeleInfo[i] = (u32)PTEC_HCJW;
					else if(strncmp("HCJR", theValue, 4) == 0)
						aCtx->mTeleInfo[i] = (u32)PTEC_HCJR;
					else if(strncmp("HPJB", theValue, 4) == 0)
						aCtx->mTeleInfo[i] = (u32)PTEC_HPJB;
					else if(strncmp("HPJW", theValue, 4) == 0)
						aCtx->mTeleInfo[i] = (u32)PTEC_HPJW;
					else if(strncmp("HPJR", theValue, 4) == 0)
						aCtx->mTeleInfo[i] = (u32)PTEC_HPJR;
				}
				else if (i == DEMAIN)
				{
					if(strncmp("BLEU", theValue, 4) == 0)
						aCtx->mTeleInfo[i] = (u32)DEMAIN_BLEU; 
					else if(strncmp("BLAN", theValue, 4) == 0)
						aCtx->mTeleInfo[i] = (u32)DEMAIN_BLANC;  
					else if(strncmp("ROUG", theValue, 4) == 0)
						aCtx->mTeleInfo[i] = (u32)DEMAIN_ROUGE;  
				}
				else
				{
					aCtx->mTeleInfo[i] = atol(theValue);
				}
				
				//LOG_LINKY_DEBUG("FOUND %d -> ValueStr: %s, ValueInt: %lu\n", i, theValue, aCtx->mTeleInfo[i]);
				
				if (i == MOTDETAT) //Last line
				{
					if (++aCtx->mNumberOfRefresh >= 2) //Before returning a valid state, be sure we have enough information
						return true;
				}
	
				return false;
			}
		}
		
		IOHUB_LOG_ERROR("TeleInfo: Line not handled: %s\n", theLine);
	}
	
	return false;
}

/* ----------------------------------------------------- */

u32 iohub_linky_info_get(linky_info *aCtx, teleinfo_t teleinfo_type)
{
	if (teleinfo_type >= TELEINFO_COUNT)
	{
		IOHUB_LOG_ERROR("TeleInfo: Get incorrect type: %d\n", teleinfo_type);
		return 0;
	}
	
	return aCtx->mTeleInfo[teleinfo_type];
}

/* ----------------------------------------------------- */

void iohub_linky_info_get_all(linky_info *aCtx, u32 aTeleInfo[TELEINFO_COUNT])
{
	memcpy(aTeleInfo, aCtx->mTeleInfo, sizeof(aCtx->mTeleInfo));
}

/* ----------------------------------------------------- */

const char *iohub_linky_info_type_to_str(teleinfo_t teleinfo_type)
{
	if (teleinfo_type >= TELEINFO_COUNT)
		return NULL;
	
	return sTeleInfoLabels[teleinfo_type];
}
