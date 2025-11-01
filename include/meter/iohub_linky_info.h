#pragma once

#include "utils/iohub_types.h"
#include "platform/iohub_uart.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum 
{
	OPTARIF_UNKNOWN = 0,
	OPTARIF_BASE,
	OPTARIF_HC,
	OPTARIF_EJP,
	OPTARIF_BBRx
} OPTARIF_t;

typedef enum
{
	PTEC_UNKNOWN = 0,
	PTEC_TH,        //Toutes les Heures.
	PTEC_HC,        //Heures Creuses.
	PTEC_HP,        //Heures Pleines.
	PTEC_HN,        //Heures Normales.
	PTEC_PM,        //Heures de Pointe Mobile.
	PTEC_HCJB,      //Heures Creuses Jours Bleus.
	PTEC_HCJW,      //Heures Creuses Jours Blancs (White).
	PTEC_HCJR,      //Heures Creuses Jours Rouges.
	PTEC_HPJB,      //Heures Pleines Jours Bleus.
	PTEC_HPJW,      //Heures Pleines Jours Blancs (White).
	PTEC_HPJR       //Heures Pleines Jours Rouges
} PTEC_t;

typedef enum
{
	DEMAIN_UNKNOWN = 0,
	DEMAIN_BLEU,    
	DEMAIN_BLANC,    
	DEMAIN_ROUGE,    
} DEMAIN_t;

	// http://www.magdiblog.fr/wp-content/uploads/2014/09/ERDF-NOI-CPT_02E.pdf
typedef enum 
{
	ISOUSC = 0, // Intensité souscrite (en Ampères)
	IINST,  	// Intensité instantanée (en ampères)
	IINST1,
	IINST2,
	IINST3,
	ADPS,  		// Avertissement de dépassement de puissance souscrite (en ampères)
	IMAX,  		// Intensité maximale (en ampères)
	IMAX1,
	IMAX2,
	IMAX3,
	PAPP,		// Puissance apparente (en Volt.ampères)
	PMAX,		// Puissance apparente max (en Volt.ampères)
	BASE,		// Index si option = base (en Wh)
	HCHC, 		// Index heures creuses si option = heures creuses (en Wh)
	HCHP,		// Index heures pleines si option = heures creuses (en Wh)
	EJP_HN,		// Index heures normales si option = EJP (en Wh)
	EJP_HPM,	// Index heures de pointe mobile si option = EJP (en Wh)
	PEJP,		// Préavis EJP si option = EJP 30mn avant période EJP, en minutes
	BBR_HC_JB,  // Index heures creuses jours bleus si option = tempo (en Wh)
	BBR_HP_JB,  // Index heures pleines jours bleus si option = tempo (en Wh)
	BBR_HC_JW,  // Index heures creuses jours blancs si option = tempo (en Wh)
	BBR_HP_JW,  // Index heures pleines jours blancs si option = tempo (en Wh)
	BBR_HC_JR,  // Index heures creuses jours rouges si option = tempo (en Wh)
	BBR_HP_JR,  // Index heures pleines jours rouges si option = tempo (en Wh)
	HHPHC, 		// Groupe horaire si option = heures creuses ou tempo
	MOTDETAT,
	ADCO, 		// Identifiant compteur (uint64)
	OPTARIF, 	// (OPTARIF_t)
	PTEC, 		// Période tarifaire en cours (PTEC_t)
	DEMAIN,		// Couleur du lendemain si option = tempo (DEMAIN_t)
	
	TELEINFO_COUNT,
} teleinfo_t;

/* -------------------------------------------------------------- */

typedef struct linky_info_s
{	
	uart_ctx 		mUart;
	char 			mLine[32];
	u8 				mLineIdx;
	u32				mNumberOfRefresh;
	uint32_t		mTeleInfo[TELEINFO_COUNT];
}linky_info;

/* -------------------------------------------------------------- */

ret_code_t		iohub_linky_info_init(linky_info *aCtx, u8 aRxPin);

void    		iohub_linky_info_uninit(linky_info *aCtx);

BOOL    		iohub_linky_info_run(linky_info *aCtx);

uint32_t 		iohub_linky_info_get(linky_info *aCtx, teleinfo_t teleinfo_type);

void	 		iohub_linky_info_get_all(linky_info *aCtx, uint32_t aTeleInfo[TELEINFO_COUNT]);

const char  	*iohub_linky_info_type_to_str(teleinfo_t teleinfo_type);

#ifdef __cplusplus
}
#endif
