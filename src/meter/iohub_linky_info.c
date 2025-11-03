#include "meter/iohub_linky_info.h"
#include "utils/iohub_logs.h"

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

ret_code_t iohub_linky_info_init(linky_info *ctx, uart_ctx *uartCtx)
{
    memset(ctx, 0x00, sizeof(linky_info));

	ctx->mUart = uartCtx;

	return iohub_uart_open(ctx->mUart, 1200, IOHubUartParity_None, 1);
}

/* ----------------------------------------------------------------- */

void iohub_linky_info_uninit(linky_info *ctx)
{
	iohub_uart_close(ctx->mUart);
}

/* ----------------------------------------------------- */

	static const char *iohub_linky_info_read_line(linky_info *ctx) 
	{
		/*
			Line format:  "KEY VALUE CRC\r\n"
		*/

		if (iohub_uart_data_available(ctx->mUart) > 0)
		{
			char theChar = iohub_uart_read_byte(ctx->mUart) & 0x7F;

			if (theChar == 0x02 /*STX*/ || theChar == 0x03 /*ETX*/)
				return NULL; //Nothing to do, skip STX and ETX and continue

			ctx->mLine[ctx->mLineIdx++] = theChar;

			if (theChar == '\n' || (ctx->mLineIdx >= sizeof(ctx->mLine) - 1))
			{
				if (ctx->mLineIdx > 7)
				{
					//log_buffer(ctx->mLine, ctx->mLineIdx);

					ctx->mLineIdx -= 3; // -3 = CRC + \r + \n

					u8 crc_computed = 0x20;
					for (u8 i=0; i<ctx->mLineIdx; i++)
						crc_computed += ctx->mLine[i];
					crc_computed = (crc_computed & 0x3F) + 0x20;

					u8 crc_read = ctx->mLine[ctx->mLineIdx];

					ctx->mLine[ctx->mLineIdx - 1] = '\0'; //Cut the string just before the CRC (Replace Space by \0)
					ctx->mLineIdx = 0;

					if (crc_computed == crc_read)
						return ctx->mLine;

					IOHUB_LOG_ERROR("TeleInfo: Invalid CRC: %s (Got: 0x%x, Computed: 0x%x)", ctx->mLine, crc_read, crc_computed);
				}
				else
				{
					ctx->mLineIdx = 0;
					IOHUB_LOG_ERROR("TeleInfo: Invalid string (Too small)");
				}
			}
		}

		return NULL;
	}
	
/* ----------------------------------------------------- */

BOOL iohub_linky_info_run(linky_info *ctx) 
{
	const char *theLine = iohub_linky_info_read_line(ctx);

	if (theLine != NULL)
	{
		//LOG_LINKY_DEBUG("%s", theLine);
		
		for (u8 i=0; i<TELEINFO_COUNT; i++)
		{
			if (strncmp(sTeleInfoLabels[i], theLine, strlen(sTeleInfoLabels[i])) == 0)
			{
				const char *theValue = &theLine[strlen(sTeleInfoLabels[i]) + 1];
				
				if (i == OPTARIF)
				{
					if(strncmp("BASE", theValue, 4) == 0)
						ctx->mTeleInfo[i] = (u32)OPTARIF_BASE;
					else if(strncmp("HC..", theValue, 4) == 0)
						ctx->mTeleInfo[i] = (u32)OPTARIF_HC;
					else if(strncmp("EJP", theValue, 3) == 0)
						ctx->mTeleInfo[i] = (u32)OPTARIF_EJP;
					else if(strncmp("BBR", theValue, 3) == 0)
						ctx->mTeleInfo[i] = (u32)OPTARIF_BBRx;
				}
				else if (i == PTEC)
				{
					if(strncmp("TH..", theValue, 4) == 0)
						ctx->mTeleInfo[i] = (u32)PTEC_TH; 
					else if(strncmp("HC..", theValue, 4) == 0)
						ctx->mTeleInfo[i] = (u32)PTEC_HC;  
					else if(strncmp("HP..", theValue, 4) == 0)
						ctx->mTeleInfo[i] = (u32)PTEC_HP;  
					else if(strncmp("HN..", theValue, 4) == 0)
						ctx->mTeleInfo[i] = (u32)PTEC_HN; 
					else if(strncmp("PM..", theValue, 4) == 0)
						ctx->mTeleInfo[i] = (u32)PTEC_PM;  
					else if(strncmp("HCJB", theValue, 4) == 0)
						ctx->mTeleInfo[i] = (u32)PTEC_HCJB;
					else if(strncmp("HCJW", theValue, 4) == 0)
						ctx->mTeleInfo[i] = (u32)PTEC_HCJW;
					else if(strncmp("HCJR", theValue, 4) == 0)
						ctx->mTeleInfo[i] = (u32)PTEC_HCJR;
					else if(strncmp("HPJB", theValue, 4) == 0)
						ctx->mTeleInfo[i] = (u32)PTEC_HPJB;
					else if(strncmp("HPJW", theValue, 4) == 0)
						ctx->mTeleInfo[i] = (u32)PTEC_HPJW;
					else if(strncmp("HPJR", theValue, 4) == 0)
						ctx->mTeleInfo[i] = (u32)PTEC_HPJR;
				}
				else if (i == DEMAIN)
				{
					if(strncmp("BLEU", theValue, 4) == 0)
						ctx->mTeleInfo[i] = (u32)DEMAIN_BLEU; 
					else if(strncmp("BLAN", theValue, 4) == 0)
						ctx->mTeleInfo[i] = (u32)DEMAIN_BLANC;  
					else if(strncmp("ROUG", theValue, 4) == 0)
						ctx->mTeleInfo[i] = (u32)DEMAIN_ROUGE;  
				}
				else
				{
					ctx->mTeleInfo[i] = atol(theValue);
				}

				//LOG_LINKY_DEBUG("FOUND %d -> ValueStr: %s, ValueInt: %lu", i, theValue, ctx->mTeleInfo[i]);

				if (i == MOTDETAT) //Last line
				{
					if (++ctx->mNumberOfRefresh >= 2) //Before returning a valid state, be sure we have enough information
						return true;
				}
	
				return false;
			}
		}
		
		IOHUB_LOG_ERROR("TeleInfo: Line not handled: %s", theLine);
	}
	
	return false;
}

/* ----------------------------------------------------- */

u32 iohub_linky_info_get(linky_info *ctx, teleinfo_t teleinfo_type)
{
	if (teleinfo_type >= TELEINFO_COUNT)
	{
		IOHUB_LOG_ERROR("TeleInfo: Get incorrect type: %d", teleinfo_type);
		return 0;
	}
	
	return ctx->mTeleInfo[teleinfo_type];
}

/* ----------------------------------------------------- */

void iohub_linky_info_get_all(linky_info *ctx, u32 aTeleInfo[TELEINFO_COUNT])
{
	memcpy(aTeleInfo, ctx->mTeleInfo, sizeof(ctx->mTeleInfo));
}

/* ----------------------------------------------------- */

const char *iohub_linky_info_type_to_str(teleinfo_t teleinfo_type)
{
	if (teleinfo_type >= TELEINFO_COUNT)
		return NULL;
	
	return sTeleInfoLabels[teleinfo_type];
}
