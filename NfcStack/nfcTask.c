/* *************************
* Copyright 2018 NXP
* All rights reserved.
**************************** */

#ifdef NFC_STACK_USED

#include <string.h>
#include "nfc_driver_config.h"
#include "Nfc.h"
#include "defines.h"
#include "nfcTask.h"

#include "aes.h"
#include <stdlib.h>
#include "systemTimers.h"

#ifdef   DEBUG
    #include <stdio.h>
    #define  DEBUG_PRINTF(...) printf(__VA_ARGS__)
#else /* DEBUG */
    #define  DEBUG_PRINTF(...)
#endif /* DEBUG */

#define print_buf(x,y,z)  {int loop; DEBUG_PRINTF(x); for(loop=0;loop<z;loop++) DEBUG_PRINTF("0x%.2x ", y[loop]); DEBUG_PRINTF("\n");}

#if defined P2P_SUPPORT || defined RW_SUPPORT
#define NDEF_RECORD_MB_MASK		0x80
#define NDEF_RECORD_ME_MASK		0x40
#define NDEF_RECORD_CF_MASK		0x20
#define NDEF_RECORD_SR_MASK		0x10
#define NDEF_RECORD_IL_MASK		0x08
#define NDEF_RECORD_TNF_MASK	0x07

#define NDEF_EMPTY			    0x00
#define NDEF_WELL_KNOWN		    0x01
#define NDEF_MEDIA			    0x02
#define NDEF_ABSOLUTE_URI	    0x03
#define NDEF_EXTERNAL		    0x04
#define NDEF_UNKNOWN		    0x05
#define NDEF_UNCHANGED		    0x06
#define NDEF_RESERVED		    0x07

typedef enum
{
    MODE_RD = 1,
    MODE_WR,
    MODE_P2P,
    MODE_CE
} mode_type;

typedef enum
{
	WELL_KNOWN_SIMPLE_TEXT,
	WELL_KNOWN_SIMPLE_URI,
	WELL_KNOWN_SMART_POSTER,
	WELL_KNOWN_HANDOVER_SELECT,
	MEDIA_VCARD,
	MEDIA_HANDOVER_WIFI,
	MEDIA_HANDOVER_BT,
	ABSOLUTE_URI,
	UNSUPPORTED_NDEF_RECORD = 0xFF
}NdefRecordType_e;

/* NFC task FSM */
NfcState_t           state = NFC_IDLE;
NxpNci_RfIntf_t      RfInterface;
uint8_t PreviousMode = 0xFF;

/* FreeMASTER variables : Tag information */
uint8_t FMSTR_Uid[18] = {0};
uint8_t FMSTR_UidLength;

uint8_t FMSTR_APDU_Exchange_Return_Code[2] = {0};
uint8_t FMSTR_APDU_ExchangeLength = 0x00;
uint8_t FMSTR_APDU_ExchangeStatus = APDU_ST_INIT;
uint8_t FMSTR_AES_128_BIT_KEY[16] = { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
									  0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F }; 

uint8_t FMSTR_Atqa[2] = {0};
uint8_t FMSTR_Sak[2] = {0};
uint8_t FMSTR_NfcTechnology;
uint8_t FMSTR_TagDetected;
boolean prev_tag_detected = FALSE;
uint8_t FMSTR_TagType = 0;

/* NDEF related variables */
uint8_t FMSTR_Url[20] = {'n', 'x', 'p', '.', 'c', 'o', 'm'};
uint8_t FMSTR_MacAddress[10] = {0x00, 0x12, 0x6F, 0x7D, 0x87, 0x5F}; /* JBL Clip device */
uint8_t FMSTR_NdefLength = 7;

uint16_t lpcd_calib_agc_val = 0;
#define LPCD_CALIB_CYCLES 16
#define LPCD_DELTA 4

uint8_t p2p_processed = 0;

static unsigned char  DiscoveryTechnologies[] = { MODE_POLL | TECH_PASSIVE_NFCA
												 ,MODE_POLL | TECH_PASSIVE_NFCB
												 ,MODE_POLL | TECH_PASSIVE_NFCF
												 ,MODE_POLL | TECH_PASSIVE_15693
};

static unsigned char  DiscoveryTechnologies_P2P[] = {  MODE_POLL | TECH_PASSIVE_NFCA
                                          ,MODE_POLL | TECH_PASSIVE_NFCB
                                          ,MODE_POLL | TECH_PASSIVE_NFCF
                                          ,MODE_POLL | TECH_ACTIVE_NFCF
                                          ,MODE_LISTEN | TECH_PASSIVE_NFCA
                                          ,MODE_LISTEN | TECH_PASSIVE_NFCF
                                          ,MODE_LISTEN | TECH_ACTIVE_NFCA
                                          ,MODE_LISTEN | TECH_ACTIVE_NFCF
};

static unsigned char  DiscoveryTechnologies_CARDEMU[] = { MODE_LISTEN | TECH_PASSIVE_NFCA
												         ,MODE_LISTEN | TECH_PASSIVE_NFCB
};

static char * auth(unsigned char x)
{
	switch (x)
	{
	case 0x01: return "Open";
	case 0x02: return "WPA-Personal";
	case 0x04: return "Shared";
	case 0x08: return "WPA-Entreprise";
	case 0x10: return "WPA2-Entreprise";
	case 0x20: return "WPA2-Personal";
	default: return "unknown";
	}
}

static char * encrypt(unsigned char x)
{
	switch (x)
	{
	case 0x01: return "None";
	case 0x02: return "WEP";
	case 0x04: return "TKIP";
	case 0x08: return "AES";
	case 0x10: return "AES/TKIP";
	default: return "unknown";
	}
}

/* Set NCI in all modes */
unsigned char mode = 0
#ifdef CARDEMU_SUPPORT
| NXPNCI_MODE_CARDEMU
#endif

#ifdef P2P_SUPPORT
| NXPNCI_MODE_P2P
#endif

#ifdef RW_SUPPORT
| NXPNCI_MODE_RW
#endif
;

mode_type user_mode = MODE_RD;

extern int __debugger_read(int handle, 
                           unsigned char *buffer,  
                           size_t *count);

#define NDEF_PRINT_URI_CODE(x) {    switch(x)									\
									{                                           \
									case 1: DEBUG_PRINTF("http://www."); break;       \
									case 2: DEBUG_PRINTF("https://www."); break;      \
									case 3: DEBUG_PRINTF("http://"); break;           \
									case 4: DEBUG_PRINTF("https://"); break;          \
									case 5: DEBUG_PRINTF("tel:"); break;              \
									case 6: DEBUG_PRINTF("mailto:"); break;           \
									default: break;                             \
									}                                           \
							   }

static NdefRecordType_e DetectNdefRecordType(unsigned char *pNdefRecord, unsigned short NdefRecordSize)
{
	uint8_t typeField;

	/* Short or normal record */
	if (pNdefRecord[0] & NDEF_RECORD_SR_MASK) typeField = 3;
	else typeField = 6;

	/* Well known Record Type? */
	if((pNdefRecord[0] & NDEF_RECORD_TNF_MASK) == NDEF_WELL_KNOWN)
	{
		if(pNdefRecord[1] == 0x1)
		{
			switch(pNdefRecord[typeField])
			{
			case 'T':
				return WELL_KNOWN_SIMPLE_TEXT;
				break;
			case 'U':
				return WELL_KNOWN_SIMPLE_URI;
				break;
			}
		}
		else if(pNdefRecord[1] == 0x2)
		{
			if(memcmp(&pNdefRecord[typeField], "Sp", pNdefRecord[1]) == 0x0)
			{
				return WELL_KNOWN_SMART_POSTER;
			}
			if(memcmp(&pNdefRecord[typeField], "Hs", pNdefRecord[1]) == 0x0)
			{
				return WELL_KNOWN_HANDOVER_SELECT;
			}
		}
	}
	/* Media Record Type? */
	else if((pNdefRecord[0] & NDEF_RECORD_TNF_MASK) == NDEF_MEDIA)
	{
		if((memcmp(&pNdefRecord[typeField], "text/x-vCard", pNdefRecord[1]) == 0x0) || (memcmp(&pNdefRecord[typeField], "text/vcard", pNdefRecord[1]) == 0x0))
		{
			return MEDIA_VCARD;
		}
		else if (memcmp(&pNdefRecord[typeField], "application/vnd.wfa.wsc", pNdefRecord[1]) == 0x0)
		{
			return MEDIA_HANDOVER_WIFI;
		}
		else if (memcmp(&pNdefRecord[typeField], "application/vnd.bluetooth.ep.oob", pNdefRecord[1]) == 0x0)
		{
			return MEDIA_HANDOVER_WIFI;
		}
	}
	/* Absolute URI Record Type? */
	else if((pNdefRecord[0] & NDEF_RECORD_TNF_MASK) == NDEF_ABSOLUTE_URI)
	{
		return ABSOLUTE_URI;
	}

	return UNSUPPORTED_NDEF_RECORD;
}

static void NdefPull_Cb(unsigned char *pNdefRecord, unsigned short NdefRecordSize) 
{
	NdefRecordType_e NdefRecordType;

	if (pNdefRecord == NULL)
	{
		DEBUG_PRINTF("--- Issue during NDEF message reception (check provisioned buffer size) \n");
		return;
	}

	DEBUG_PRINTF("--- NDEF message received:\n");

	NdefRecordType = DetectNdefRecordType(pNdefRecord, NdefRecordSize);

	switch(NdefRecordType)
	{
	case MEDIA_VCARD:
		{
			char *pString;
			DEBUG_PRINTF("   vCard found:\n");
			/* Short record or normal record */
			if (pNdefRecord[0] & NDEF_RECORD_SR_MASK) pString = strstr((char*)&pNdefRecord[3], "BEGIN");
			else pString = strstr((char*)&pNdefRecord[6], "BEGIN");
			DEBUG_PRINTF("%s", pString);
		}
		break;

	case WELL_KNOWN_SIMPLE_TEXT:
		{
			pNdefRecord[7 + pNdefRecord[2]] = '\0';
			DEBUG_PRINTF("   Text record (language = %c%c): %s\n", pNdefRecord[5], pNdefRecord[6], &pNdefRecord[7]);
		}
		break;

	case WELL_KNOWN_SIMPLE_URI:
		{
			DEBUG_PRINTF("   URI record: ");
			NDEF_PRINT_URI_CODE(pNdefRecord[4])
				pNdefRecord[4 + pNdefRecord[2]] = '\0';
			DEBUG_PRINTF("%s\n", &pNdefRecord[5]);
		}
		break;

	case MEDIA_HANDOVER_WIFI:
		{
			unsigned char index = 26, i;

			DEBUG_PRINTF ("--- Received WIFI credentials:\n");
			if ((pNdefRecord[index] == 0x10) && (pNdefRecord[index+1] == 0x0e)) index+= 4;
			while(index < NdefRecordSize)
			{
				if (pNdefRecord[index] == 0x10)
				{
					if (pNdefRecord[index+1] == 0x45) {DEBUG_PRINTF ("- SSID = "); for(i=0;i<pNdefRecord[index+3];i++) DEBUG_PRINTF("%c", pNdefRecord[index+4+i]); DEBUG_PRINTF ("\n");}
					else if (pNdefRecord[index+1] == 0x03) DEBUG_PRINTF ("- Authenticate Type = %s\n", auth(pNdefRecord[index+5]));
					else if (pNdefRecord[index+1] == 0x0f) DEBUG_PRINTF ("- Encryption Type = %s\n", encrypt(pNdefRecord[index+5]));
					else if (pNdefRecord[index+1] == 0x27) {DEBUG_PRINTF ("- Network key = "); for(i=0;i<pNdefRecord[index+3];i++) DEBUG_PRINTF("#"); DEBUG_PRINTF ("\n");}
					index += 4 + pNdefRecord[index+3];
				}
				else continue;
			}
		}
		break;

	default:
		DEBUG_PRINTF("   Unsupported NDEF record, cannot parse\n");
		break;
	}
	DEBUG_PRINTF("\n");
}
#endif

#if defined P2P_SUPPORT || defined CARDEMU_SUPPORT

char NDEF_RECORD_CE[64] = { 0xD1,   // MB/ME/CF/1/IL/TNF
		0x01,   // TYPE LENGTH
		0x0E,   // PAYLOAD LENTGH
		'U',    // TYPE
		0x02};   // Status

/* Initialize NDEF without MAC address and with wrong payload size */
char NDEF_RECORD_BT[64] = { 0xDA,   /* MB/ME/CF/1/IL/TNF */
         0x20,   /* TYPE LENGTH */
         0x08,   /* PAYLOAD LENTGH */
         0x01,
         'a','p','p','l','i','c','a','t','i','o','n','/','v','n','d',0x2e,
         'b','l','u','e','t','o','o','t','h',0x2e,
         'e','p',0x2e,
         'o','o','b','0',
         0x08, 0x00};
         
static void NdefPush_Cb(unsigned char *pNdefRecord, unsigned short NdefRecordSize) 
{
	DEBUG_PRINTF("--- NDEF Record sent\n\n");
}
#endif

static void APDUChallengeResponseScenario (void)
{
	boolean status;
    uint8_t index = 0;
    unsigned char Resp[256] = {0};
	unsigned char RespSize;
    unsigned char SelectEngineApplet[] = {0x00, 0xA4, 0x04, 0x00, 0x05, 0xA0, 0x61, 0x75, 0x74, 0x6F};
    unsigned char GetVersion[] = {0x00, 0x00, 0x00, 0x00};
    unsigned char AESRandomBytes[] = {0x00, 0x02, 0x00, 0x00, 0x10, 0x00, 0x01, 0x02,
    								  0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A,
    								  0x0B, 0x0C, 0x0D, 0x0E, 0x0F};
    unsigned char BytesToBeDerypted[] = {0x0A, 0x94, 0x0B, 0xB5, 0x41, 0x6E, 0xF0, 0x45, 0xF1, 0xC3, 0x94, 0x58, 0xC6, 0x53, 0xEA, 0x5A};
    uint8_t AESComparisonBytes[16] = {0};

    status = NxpNci_ReaderTagCmd(SelectEngineApplet, sizeof(SelectEngineApplet), Resp, &RespSize);
    FMSTR_APDU_ExchangeLength = RespSize;
    if(RespSize >= 2) {
		FMSTR_APDU_Exchange_Return_Code[0] = Resp[RespSize-2];
		FMSTR_APDU_Exchange_Return_Code[1] = Resp[RespSize-1];
		if((Resp[RespSize-2] != 0x90) || (Resp[RespSize-1] != 0x00)) {
			FMSTR_APDU_ExchangeStatus = APDU_ST_AID_SELECT_ERR;
		return;
		} else {
			FMSTR_APDU_ExchangeStatus = APDU_ST_AID_SELECT_SUCC;
			// create random bytes
			for(index = 0; index < 16; index++) {
				AESRandomBytes[5+index] = rand();
	}
			status = NxpNci_ReaderTagCmd(AESRandomBytes, sizeof(AESRandomBytes), Resp, &RespSize);
		    if(RespSize >= 2) {
		    	//FMSTR_APDU_ExchangeLength = RespSize;
				FMSTR_APDU_Exchange_Return_Code[0] = Resp[RespSize-2];
				FMSTR_APDU_Exchange_Return_Code[1] = Resp[RespSize-1];
				if((Resp[RespSize-2] != 0x90) || (Resp[RespSize-1] != 0x00)) {
					FMSTR_APDU_ExchangeStatus = APDU_ST_RDM_BYTES_ERR;
		return;
				} else {    	  
					if(RespSize != 18) {
						FMSTR_APDU_ExchangeStatus = APDU_ST_RDM_BYTES_ERR;
					} else {
						FMSTR_APDU_ExchangeStatus = APDU_ST_RDM_BYTES_SUCC;
						AES_ECB_decrypt(Resp, FMSTR_AES_128_BIT_KEY, AESComparisonBytes, 0x10);
						for(index = 0; index < 16; index++) {
							if(AESComparisonBytes[index] != AESRandomBytes[5+index]) {
								FMSTR_APDU_ExchangeStatus = APDU_ST_RDM_BYTES_ERR;
    		return;
    	}
    	}
						FMSTR_APDU_ExchangeStatus = APDU_ST_RDM_BYTES_SUCC;
    	}
    }
	}
}
	}
	return;
}

static void displayCardInfo(NxpNci_RfIntf_t RfIntf)
{
	uint8_t index = 0;
	
	switch(RfIntf.ModeTech)
	{
		case (MODE_POLL | TECH_PASSIVE_NFCA):
			DEBUG_PRINTF("\tSENS_RES = 0x%.2x 0x%.2x\n", RfIntf.Info.NFC_APP.SensRes[0], RfIntf.Info.NFC_APP.SensRes[1]);
			print_buf("\tNFCID = ", RfIntf.Info.NFC_APP.NfcId, RfIntf.Info.NFC_APP.NfcIdLen);
			if(RfIntf.Info.NFC_APP.SelResLen != 0) DEBUG_PRINTF("\tSEL_RES = 0x%.2x\n", RfIntf.Info.NFC_APP.SelRes[0]);
			
			/* update FreeMASTER information */
            for (index = 0; index < RfIntf.Info.NFC_APP.NfcIdLen; index++)
            {
            	FMSTR_Uid[index]       = RfIntf.Info.NFC_APP.NfcId[index];
            }

            FMSTR_UidLength = RfIntf.Info.NFC_APP.NfcIdLen;
			FMSTR_Atqa[0]      = RfIntf.Info.NFC_APP.SensRes[0];
			FMSTR_Atqa[1]      = RfIntf.Info.NFC_APP.SensRes[1];
			FMSTR_Sak[0]    = RfIntf.Info.NFC_APP.SelRes[0];
			FMSTR_NfcTechnology = 1;
		break;

		case (MODE_POLL | TECH_PASSIVE_NFCB):
			if(RfIntf.Info.NFC_BPP.SensResLen != 0) print_buf("\tSENS_RES = ", RfIntf.Info.NFC_BPP.SensRes, RfIntf.Info.NFC_BPP.SensResLen);
		
		    /* update FreeMASTER information */
            for (index = 0; index < RfIntf.Info.NFC_BPP.SensResLen; index++)
            {
            	FMSTR_Uid[index]       = RfIntf.Info.NFC_BPP.SensRes[index];
            }
            
            FMSTR_UidLength = RfIntf.Info.NFC_BPP.SensResLen;
			FMSTR_NfcTechnology = 2;
            
		break;

		case (MODE_POLL | TECH_PASSIVE_NFCF):
			DEBUG_PRINTF("\tBitrate = %s\n", (RfIntf.Info.NFC_FPP.BitRate==1)?"212":"424");
			if(RfIntf.Info.NFC_FPP.SensResLen != 0) print_buf("\tSENS_RES = ", RfIntf.Info.NFC_FPP.SensRes, RfIntf.Info.NFC_FPP.SensResLen);
		
		    /* update FreeMASTER information */
            for (index = 0; index < RfIntf.Info.NFC_FPP.SensResLen; index++)
            {
            	FMSTR_Uid[index]       = RfIntf.Info.NFC_FPP.SensRes[index];
            }
            
            FMSTR_UidLength = RfIntf.Info.NFC_FPP.SensResLen;
			FMSTR_NfcTechnology = 3;
            
		break;

		case (MODE_POLL | TECH_PASSIVE_15693):
			print_buf("\tID = ", RfIntf.Info.NFC_VPP.ID, sizeof(RfIntf.Info.NFC_VPP.ID));
			DEBUG_PRINTF("\tAFI = 0x%.2x\n", RfIntf.Info.NFC_VPP.AFI);
			DEBUG_PRINTF("\tDSFID = 0x%.2x\n", RfIntf.Info.NFC_VPP.DSFID);
		
		    /* update FreeMASTER information */
            for (index = 0; index < sizeof(RfIntf.Info.NFC_VPP.ID); index++)
            {
            	FMSTR_Uid[index]       = RfIntf.Info.NFC_VPP.ID[index];
            }
            
            FMSTR_UidLength = sizeof(RfIntf.Info.NFC_VPP.ID);
		    /* further details to be displayed for type V? AFI and DSFID ?*/
			FMSTR_NfcTechnology = 5;
            
		break;

		default:
		break;
	}
}

void init_nfc(unsigned char usermode)
{
			
	// init seed for AES random byte generation once here
	srand(ST_GetTimerTick());
	
	/* Open connection to NXPNCI device */
	if (NxpNci_Connect() == NFC_ERROR) {
		DEBUG_PRINTF("Error: cannot connect to NXPNCI device\n");
        while (1) 
        {
            /* code */
        };
	}

	if (NxpNci_ConfigureSettings() == NFC_ERROR) {
		DEBUG_PRINTF("Error: cannot configure NXPNCI settings\n");
        while (1) 
        {
            /* code */
        };
	}

	/* Set NXPNCI mode */
	if (NxpNci_ConfigureMode(usermode) == NFC_ERROR)
	{
		DEBUG_PRINTF("Error: cannot configure NXPNCI\n");
        while (1) 
        {
            /* code */
        };
	}
	if((FMSTR_AppState == ST_BT_P2P_TRANSMIT) || (FMSTR_AppState == ST_CE_TRANSMIT))
	{
	    /* Start Discovery */
        if (usermode == NXPNCI_MODE_P2P)
        {
    	    if (NxpNci_StartDiscovery(DiscoveryTechnologies_P2P,sizeof(DiscoveryTechnologies_P2P)) != NFC_SUCCESS)
    		    {
    			    DEBUG_PRINTF("Error: cannot start discovery\n");
    	            while (1) 
    	            {
    	                /* code */
    	            };
    		    }
        }
        else if (usermode == NXPNCI_MODE_CARDEMU)
        {
    	    if (NxpNci_StartDiscovery(DiscoveryTechnologies_CARDEMU,sizeof(DiscoveryTechnologies_CARDEMU)) != NFC_SUCCESS)
    		    {
    			    DEBUG_PRINTF("Error: cannot start discovery\n");
    	            while (1) 
    	            {
    	                /* code */
    	            };
    		    }
        }
        else if (NxpNci_StartDiscovery(DiscoveryTechnologies,sizeof(DiscoveryTechnologies)) != NFC_SUCCESS)
	    {
		    DEBUG_PRINTF("Error: cannot start discovery\n");
            while (1) 
            {
                /* code */
            };
	    }
	}
}

boolean lpcd_calib(void)
{
	uint8_t index = 0;
	uint32_t calib_sum = 0;
	lpcd_calib_agc_val = 0;

	for(index = 0; index < LPCD_CALIB_CYCLES; index++)
	{
		NxpNci_LpcdWakeup(&lpcd_calib_agc_val, 0, &lpcd_calib_agc_val, 0);
		calib_sum += lpcd_calib_agc_val;
	}
	lpcd_calib_agc_val = calib_sum / LPCD_CALIB_CYCLES;
	return 0;

}

boolean lpcd_wakeup(void)
{
	/* get calibraton value */
	uint16_t agc_val = 0;
	NxpNci_LpcdWakeup(&agc_val, 0, &agc_val, 0);
	/* check for wake-up */
	if(agc_val >= lpcd_calib_agc_val)
	{
		if(agc_val - lpcd_calib_agc_val >= LPCD_DELTA)
		{
			return 1;
		}
	}
	if(agc_val < lpcd_calib_agc_val)
	{
		if(lpcd_calib_agc_val - agc_val >= LPCD_DELTA)
		{
			return 1;
		}
	}
	return 0;
}

void task_nfc(void)
{
    unsigned char m = 0;
    size_t count = 1;
    
    if( (FMSTR_AppState == ST_CARD_DETECTION) || (FMSTR_AppState == ST_APDU_EXCHANGE) ||
	    (FMSTR_AppState == ST_NFC_WCH) 	      || (FMSTR_AppState == ST_NFC_WCH_TAG) )
    {
    	/* update previous mode */
    	PreviousMode = FMSTR_AppState;
    	
        if (NxpNci_StartDiscovery(DiscoveryTechnologies,sizeof(DiscoveryTechnologies)) != NFC_SUCCESS)
        {
            DEBUG_PRINTF("Error: cannot start discovery\n");
            init_nfc(0);
        }
    
        DEBUG_PRINTF("Starting Polling for tags..\n");
        if(NFC_ERROR != NxpNci_WaitForDiscoveryNotification(&RfInterface))
        {
            FMSTR_APDU_ExchangeStatus = APDU_ST_INIT;
            FMSTR_TagType = RfInterface.Protocol;
        	if( (RfInterface.Protocol == PROT_ISO15693) ||
				(RfInterface.Protocol == PROT_T1T) 		||
				(RfInterface.Protocol == PROT_T2T) 		||
				(RfInterface.Protocol == PROT_UNDETERMINED) )
        	{
        		/* passive tag detected */
            FMSTR_TagDetected = 1u;
        	}
        	else
        	{
        		/* could be a phone */
        		FMSTR_TagDetected = 2u;
        	}
            if(FMSTR_AppState == ST_APDU_EXCHANGE && prev_tag_detected == FALSE) {
            	APDUChallengeResponseScenario();
            
            } else if(prev_tag_detected == FALSE) {
            displayCardInfo(RfInterface);
            }
            /* Process card Presence check */
            NxpNci_ProcessReaderMode(RfInterface, PRESENCE_CHECK);
            DEBUG_PRINTF("Communication stopped / card removed\n");
            LED_RED_OFF;
            prev_tag_detected = TRUE;
        }
        else
        {
            FMSTR_TagDetected = 0u;
            prev_tag_detected = FALSE;
        }
        DEBUG_PRINTF("No tag detected or tag removed..\n");
        NxpNci_StopDiscovery();
    }
    else if( (FMSTR_AppState == ST_BT_P2P_TRANSMIT) || (FMSTR_AppState == ST_CE_TRANSMIT) )
    	{
        	/* update previous mode */
    		PreviousMode = FMSTR_AppState;
    		
            switch (FMSTR_AppState)
            {
                case NFC_RD:
                //DEBUG_PRINTF("1 User Mode - Read Tag\n>",c);
                    m = NXPNCI_MODE_RW;
                    user_mode = MODE_RD;
                    break;
            
                case NFC_WR:
                //DEBUG_PRINTF("2 User Mode - Write Tag\n>",c);
                    m = NXPNCI_MODE_RW;
                    user_mode = MODE_WR;
                    break;
                
            case ST_BT_P2P_TRANSMIT:
                //DEBUG_PRINTF("3 User Mode - Peer2Peer\n>",c);
                    m = NXPNCI_MODE_P2P;
                    user_mode = MODE_P2P;
                    break;
                        
            case ST_CE_TRANSMIT:
                //DEBUG_PRINTF("4 User Mode - Card emulation\n>",c);
                    m = NXPNCI_MODE_CARDEMU;// | NXPNCI_MODE_RW;
                    user_mode = MODE_CE;
                    break;
                        
                default:
                    break;
            }
            
            if (m != 0)
            {
                mode = m;
                init_nfc(m);
            }

        /* Wait until a peer is discovered */
        if(NFC_ERROR != NxpNci_WaitForDiscoveryNotification(&RfInterface))
        {
        
#ifdef CARDEMU_SUPPORT
            /* Is activated from remote T4T ? */
            if ((RfInterface.Interface == INTF_ISODEP) && ((RfInterface.ModeTech & MODE_MASK) == MODE_LISTEN)) 
            {
                DEBUG_PRINTF(" - LISTEN MODE: Activated from remote Reader\n");
                NxpNci_ProcessCardMode(RfInterface);
                DEBUG_PRINTF("READER DISCONNECTED\n");
            }
            else
#endif
#ifdef P2P_SUPPORT
            /* Is activated from remote T4T ? */
            if ( (RfInterface.Interface == INTF_NFCDEP) )//&& (FMSTR_AppState == ST_BT_P2P_TRANSMIT) ) 
            {
                if ((RfInterface.ModeTech & MODE_LISTEN) == MODE_LISTEN) DEBUG_PRINTF(" - P2P TARGET MODE: Activated from remote Initiator\n");
                else DEBUG_PRINTF(" - P2P INITIATOR MODE: Remote Target activated\n");
        
                /* Process with SNEP for NDEF exchange */
				if(FMSTR_AppState == ST_BT_P2P_TRANSMIT) NxpNci_ProcessP2pMode(RfInterface);
                DEBUG_PRINTF("PEER LOST\n");
				//return;
            }
            else
#endif
#ifdef RW_SUPPORT
            if ((RfInterface.ModeTech & MODE_MASK) == MODE_POLL)
            {
                /* Is known target ?*/
                if ((RfInterface.Protocol != PROT_NFCDEP) && (RfInterface.Interface != INTF_UNDETERMINED))
                {
                    DEBUG_PRINTF(" - POLL MODE: Remote T%dT activated\n", RfInterface.Protocol);
                    displayCardInfo(RfInterface);
        
                    /* Process NDEF message read */
                    NxpNci_ProcessReaderMode(RfInterface, READ_NDEF);
        
#ifdef NDEF_WRITING
                    RW_NDEF_SetMessage ((unsigned char *) NDEF_RECORD, sizeof(NDEF_RECORD), *NdefPush_Cb);
                    /* Process NDEF message read */
                    NxpNci_ProcessReaderMode(RfInterface, WRITE_NDEF);
#endif
                    /* Process card Presence check */
                    NxpNci_ProcessReaderMode(RfInterface, PRESENCE_CHECK);
        
                    DEBUG_PRINTF("CARD REMOVED\n");
        
                    /* Restart the discovery loop */
                    NxpNci_RestartDiscovery();
                }
                else
                {
                    DEBUG_PRINTF(" - POLL MODE: Undetermined target\n");
                    /* Restart discovery loop */
                    NxpNci_StopDiscovery();
                    if (mode == NXPNCI_MODE_P2P)
                    {
                        if (NxpNci_StartDiscovery(DiscoveryTechnologies_P2P,sizeof(DiscoveryTechnologies_P2P)) != NFC_SUCCESS)
                            {
                                DEBUG_PRINTF("Error: cannot start discovery\n");
                                while (1) {
                                    /* code */
                                };
                            }
                    }
                    else if (NxpNci_StartDiscovery(DiscoveryTechnologies,sizeof(DiscoveryTechnologies)) != NFC_SUCCESS)
                    {
                        DEBUG_PRINTF("Error: cannot start discovery\n");
                        while (1) {
                            /* code */
                        };
                    }
                }
            }
            else
#endif
            {
                DEBUG_PRINTF("WRONG DISCOVERY\n");
                
            }
            }
        else 
        {
        	DEBUG_PRINTF("DEVICE DISCONNECTED\n");
        }
    }
}

void NDEFMessage_Init(void)
{	
	uint8_t index = 0;
	
	/* format BT NDEF based on the data from FreeMASTER */
	//memcpy(&NDEF_RECORD_BT[39], FMSTR_MacAddress, FMSTR_NdefLength);
	/* The MAC address is stored LSByte first in the NDEF */
	for (index = (FMSTR_NdefLength); index > 0; index--)
	{
		NDEF_RECORD_BT[38 + index] = FMSTR_MacAddress[FMSTR_NdefLength - index];
	}
	
	NDEF_RECORD_BT[2] = FMSTR_NdefLength + 2;
	NDEF_RECORD_BT[37] = FMSTR_NdefLength;
	
	for (index = 0; index < FMSTR_NdefLength; index++)
	{
		NDEF_RECORD_CE[5+index] = FMSTR_Url[index];
	}	
	/* format CE NDEF based on the data from FreeMASTER */
	NDEF_RECORD_CE[2] = FMSTR_NdefLength + 1;
	
#ifdef CARDEMU_SUPPORT
	    /* Register NDEF message to be sent to remote reader */
	    //T4T_NDEF_EMU_SetMessage((unsigned char *) NDEF_RECORD_CE, sizeof(NDEF_RECORD_CE), *NdefPush_Cb);
	    T4T_NDEF_EMU_SetMessage((unsigned char *) NDEF_RECORD_CE, (uint8_t)(FMSTR_NdefLength + 5), *NdefPush_Cb);
#endif

#ifdef P2P_SUPPORT
	    /* Register NDEF message to be sent to remote peer */
	    //P2P_NDEF_SetMessage((unsigned char *) NDEF_RECORD_P2P, sizeof(NDEF_RECORD_P2P), *NdefPush_Cb);
	    P2P_NDEF_SetMessage((unsigned char *) NDEF_RECORD_BT, (uint8_t)(FMSTR_NdefLength + 39), *NdefPush_Cb);
	    /* Register callback for reception of NDEF message from remote peer */
	    P2P_NDEF_RegisterPullCallback(*NdefPull_Cb);
#endif

#ifdef RW_SUPPORT
	    /* Register callback for reception of NDEF message from remote cards */
	    RW_NDEF_RegisterPullCallback(*NdefPull_Cb);
#endif
}
#endif
