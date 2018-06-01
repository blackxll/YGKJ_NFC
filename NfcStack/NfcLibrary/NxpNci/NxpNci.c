/* *************************
* Copyright 2018 NXP
* All rights reserved.
**************************** */

#ifdef NFC_STACK_USED
//#include <stdint.h>
#include <string.h>
#include "nfc_driver_config.h"
#include "NxpNci.h"
#include "Nfc_settings.h"
#include "nfcTask.h"

/* MAK: TBD */
#include "freemaster.h"


#define MAX_NCI_FRAME_SIZE	256

/* NCI wait for discovery state machine */
uint8_t NCIRfDiscoverSelect[] = {0x21, 0x04, 0x03, 0x01, PROT_ISODEP, INTF_ISODEP};
uint8_t AnswerBuffer[MAX_NCI_FRAME_SIZE];
uint16_t AnswerSize;
//NciState_t nci_state = NCI_END;

uint8_t is_ndef_selected = 0;

static uint8_t NfcController_generation = 0;
static uint8_t gNextTag_Protocol = PROT_UNDETERMINED;

static boolean NxpNci_CheckDevPres(void)
{
    uint8_t NCICoreReset[] = {0x20, 0x00, 0x01, 0x01};
	uint8_t Answer[MAX_NCI_FRAME_SIZE];
	uint16_t NbBytes = 0;

	tml_Send(NCICoreReset, sizeof(NCICoreReset), &NbBytes);
	NCI_PRINT_BUF("NCI >> ", NCICoreReset, NbBytes);
	if (NbBytes != sizeof(NCICoreReset)) return NXPNCI_ERROR;
	tml_Receive(Answer, sizeof(Answer), &NbBytes, TIMEOUT_100MS);
	NCI_PRINT_BUF("NCI << ", Answer, NbBytes);
	if (NbBytes == 0) return NXPNCI_ERROR;

	return NXPNCI_SUCCESS;
}

static boolean NxpNci_WaitForReception(uint8_t *pRBuff, uint16_t RBuffSize, uint16_t *pBytesread, uint16_t timeout)
{
	tml_Receive(pRBuff, RBuffSize, pBytesread, timeout);
	if (*pBytesread == 0) return NXPNCI_ERROR;
	NCI_PRINT_BUF("NCI << ", pRBuff, *pBytesread);

	return NXPNCI_SUCCESS;
}

static boolean NxpNci_HostTransceive(uint8_t *pTBuff, uint16_t TbuffLen, uint8_t *pRBuff, uint16_t RBuffSize, uint16_t *pBytesread)
{
	tml_Send(pTBuff, TbuffLen, pBytesread);
	NCI_PRINT_BUF("NCI >> ", pTBuff, TbuffLen);
	if (*pBytesread != TbuffLen) return NXPNCI_ERROR;
	tml_Receive(pRBuff, RBuffSize, pBytesread, TIMEOUT_1S);
	NCI_PRINT_BUF("NCI << ", pRBuff, *pBytesread);
	if (*pBytesread == 0) return NXPNCI_ERROR;

	return NXPNCI_SUCCESS;
}

static void NxpNci_FillInterfaceInfo(NxpNci_RfIntf_t* pRfIntf, uint8_t* pBuf)
{
	switch(pRfIntf->ModeTech)
	{
		case (MODE_POLL | TECH_PASSIVE_NFCA):
			memcpy(pRfIntf->Info.NFC_APP.SensRes, &pBuf[0], 2);
			pRfIntf->Info.NFC_APP.NfcIdLen = pBuf[2];
			memcpy(pRfIntf->Info.NFC_APP.NfcId, &pBuf[3], pRfIntf->Info.NFC_APP.NfcIdLen);
			pRfIntf->Info.NFC_APP.SelResLen = pBuf[3+pBuf[2]];
			if(pRfIntf->Info.NFC_APP.SelResLen == 1) pRfIntf->Info.NFC_APP.SelRes[0] = pBuf[3+pBuf[2]+1];
		break;

		case (MODE_POLL | TECH_PASSIVE_NFCB):
			pRfIntf->Info.NFC_BPP.SensResLen = pBuf[0];
			memcpy(pRfIntf->Info.NFC_BPP.SensRes, &pBuf[1], pRfIntf->Info.NFC_BPP.SensResLen);
		break;

		case (MODE_POLL | TECH_PASSIVE_NFCF):
			pRfIntf->Info.NFC_FPP.BitRate = pBuf[0];
			pRfIntf->Info.NFC_FPP.SensResLen = pBuf[1];
			memcpy(pRfIntf->Info.NFC_FPP.SensRes, &pBuf[2], pRfIntf->Info.NFC_FPP.SensResLen);
		break;

		case (MODE_POLL | TECH_PASSIVE_15693):
			pRfIntf->Info.NFC_VPP.AFI = pBuf[0];
			pRfIntf->Info.NFC_VPP.DSFID = pBuf[1];
			memcpy(pRfIntf->Info.NFC_VPP.ID, &pBuf[2], sizeof(pRfIntf->Info.NFC_VPP.ID));
		break;

		default:
		break;
	}
}

#ifdef CARDEMU_SUPPORT
void NxpNci_ProcessCardMode(NxpNci_RfIntf_t RfIntf)
{
	uint8_t Answer[MAX_NCI_FRAME_SIZE];
	uint16_t AnswerSize = 0;
	
    /* DATA_PACKET */
    uint8_t Cmd[MAX_NCI_FRAME_SIZE];
    uint16_t CmdSize = 0;

	uint8_t index = 0;
	/* Reset Card emulation state */
	T4T_NDEF_EMU_Reset();

	while(NxpNci_WaitForReception(Answer, sizeof(Answer), &AnswerSize, TIMEOUT_1S) == NXPNCI_SUCCESS && index < 6)
    {		
		//index++;
		/* is RF_DEACTIVATE_NTF ? */
        if((Answer[0] == 0x61) && (Answer[1] == 0x06))
        {
            /* Come back to discovery state */
            break;
        }
        /* is DATA_PACKET ? */
        else if((Answer[0] == 0x00) && (Answer[1] == 0x00))
        {          
        	/* MAK: state machine has to be reviewed: software crashes with Samsung A5 */
            T4T_NDEF_EMU_Next(&Answer[3], Answer[2], &Cmd[3], (unsigned short *) &CmdSize,(uint8_t *) &is_ndef_selected);
            Cmd[0] = 0x00;
            Cmd[1] = (CmdSize & 0xFF00) >> 8;
            Cmd[2] = CmdSize & 0x00FF;
            
            NxpNci_HostTransceive(Cmd, CmdSize+3, Answer, sizeof(Answer), &AnswerSize);
            /* FMSTR adjustment to stop after one NDEF is read */
            if(is_ndef_selected == (uint8_t) 2)
            {
            	is_ndef_selected = 0;
            	FMSTR_AppState = ST_CE;
            	break;
            }
        }
    }
}
#endif

#ifdef P2P_SUPPORT
void NxpNci_ProcessP2pMode(NxpNci_RfIntf_t RfIntf)
{
	uint8_t Answer[MAX_NCI_FRAME_SIZE];
	uint16_t AnswerSize;

	uint8_t NCILlcpSymm[] = {0x00, 0x00, 0x02, 0x00, 0x00};
	uint8_t NCIRestartDiscovery[] = {0x21, 0x06, 0x01, 0x03};

	/* Reset P2P_NDEF state */
	P2P_NDEF_Reset();

    /* Is Initiator mode ? */
	if((RfIntf.ModeTech & MODE_LISTEN) != MODE_LISTEN)
	{
		/* Initiate communication (SYMM PDU) */
		NxpNci_HostTransceive(NCILlcpSymm, sizeof(NCILlcpSymm), Answer, sizeof(Answer), &AnswerSize);
	}

	/* Get frame from remote peer */
	while(NxpNci_WaitForReception(Answer, sizeof(Answer), &AnswerSize, TIMEOUT_1S) == NXPNCI_SUCCESS)
	{
        /* is DATA_PACKET ? */
        if((Answer[0] == 0x00) && (Answer[1] == 0x00))
        {
            uint8_t Cmd[MAX_NCI_FRAME_SIZE];
            uint16_t CmdSize;

            /* FreeMASTER adjustment: 
             * if NDEF transmitted via P2P get back to initial P2P state */
        	if(AnswerSize >= (uint16_t)5) 
			{
				if((Answer[AnswerSize - (uint16_t)5]) == 0x81)
				{
					FMSTR_AppState = ST_BT_P2P;
					break;
				}
			}             
            
			P2P_NDEF_Next(&Answer[3], Answer[2], &Cmd[3], (unsigned short *) &CmdSize);

			/* Compute DATA_PACKET to answer */
			Cmd[0] = 0x00;
			Cmd[1] = (CmdSize & 0xFF00) >> 8;
			Cmd[2] = CmdSize & 0x00FF;

			NxpNci_HostTransceive(Cmd, CmdSize+3, Answer, sizeof(Answer), &AnswerSize);
        }
		/* is CORE_INTERFACE_ERROR_NTF ?*/
		else if ((Answer[0] == 0x60) && (Answer[1] == 0x08))
		{
			break;
		}
		/* is RF_DEACTIVATE_NTF ? */
		else if((Answer[0] == 0x61) && (Answer[1] == 0x06))
        {
            /* Come back to discovery state */
			break;
        }
	}

    /* Is Initiator mode ? */
	if((RfIntf.ModeTech & MODE_LISTEN) != MODE_LISTEN)
	{
		/* Communication ended, restart discovery loop */
		NxpNci_HostTransceive(NCIRestartDiscovery, sizeof(NCIRestartDiscovery), Answer, sizeof(Answer), &AnswerSize);
		NxpNci_WaitForReception(Answer, sizeof(Answer), &AnswerSize, TIMEOUT_100MS);
	}
}
#endif

#ifdef RW_SUPPORT
boolean NxpNci_ReaderTagCmd (unsigned char *pCommand, unsigned char CommandSize, unsigned char *pAnswer, unsigned char *pAnswerSize)
{
	boolean status = NXPNCI_ERROR;
    uint8_t Cmd[MAX_NCI_FRAME_SIZE];
	uint8_t Ans[MAX_NCI_FRAME_SIZE];
	uint16_t AnsSize;

	/* Compute and send DATA_PACKET */
	Cmd[0] = 0x00;
	Cmd[1] = 0x00;
	Cmd[2] = CommandSize;
	memcpy(&Cmd[3], pCommand, CommandSize);
	NxpNci_HostTransceive(Cmd, CommandSize+3, Ans, sizeof(Ans), &AnsSize);

	/* Wait for Answer */
	NxpNci_WaitForReception(Ans, sizeof(Ans), &AnsSize, TIMEOUT_1S);

	if ((Ans[0] == 0x0) && (Ans[1] == 0x0))
	{
		status = NXPNCI_SUCCESS;
	}

	*pAnswerSize = Ans[2];
	memcpy(pAnswer, &Ans[3], *pAnswerSize);

	return status;
}

static boolean NxpNci_T3TretrieveIDm (void)
{
	uint8_t NCIPollingCmdT3T[] = {0x21, 0x08, 0x04, 0x12, 0xFC, 0x00, 0x01};
	uint8_t Answer[MAX_NCI_FRAME_SIZE];
	uint16_t AnswerSize;

	NxpNci_HostTransceive(NCIPollingCmdT3T, sizeof(NCIPollingCmdT3T), Answer, sizeof(Answer), &AnswerSize);
	NxpNci_WaitForReception(Answer, sizeof(Answer), &AnswerSize, TIMEOUT_100MS);
	if ((Answer[0] == 0x61) && (Answer[1] == 0x08) && (Answer[3] == 0x00))
	{
		RW_NDEF_T3T_SetIDm(&Answer[6]);
	}
	else
	{
		return NXPNCI_ERROR;
	}
	return NXPNCI_SUCCESS;
}

static void NxpNci_ReadNdef(NxpNci_RfIntf_t RfIntf)
{
	uint8_t Answer[MAX_NCI_FRAME_SIZE];
	uint16_t AnswerSize;
    uint8_t Cmd[MAX_NCI_FRAME_SIZE];
    uint16_t CmdSize = 0;

	RW_NDEF_Reset(RfIntf.Protocol);

	/* In case of T3T tag, retrieve card IDm for further operation */
	if (RfIntf.Protocol == PROT_T3T) NxpNci_T3TretrieveIDm();

	while(1)
	{
		RW_NDEF_Read_Next(&Answer[3], Answer[2], &Cmd[3], (unsigned short *) &CmdSize);
		if(CmdSize == 0)
		{
			/* End of the Read operation */
			break;
		}
		else
		{
			/* Compute and send DATA_PACKET */
			Cmd[0] = 0x00;
			Cmd[1] = (CmdSize & 0xFF00) >> 8;
			Cmd[2] = CmdSize & 0x00FF;

			NxpNci_HostTransceive(Cmd, CmdSize+3, Answer, sizeof(Answer), &AnswerSize);
	        NxpNci_WaitForReception(Answer, sizeof(Answer), &AnswerSize, TIMEOUT_100MS);
		}
	}
}

static void NxpNci_WriteNdef(NxpNci_RfIntf_t RfIntf)
{
	uint8_t Answer[MAX_NCI_FRAME_SIZE];
	uint16_t AnswerSize;
    uint8_t Cmd[MAX_NCI_FRAME_SIZE];
    uint16_t CmdSize = 0;

	RW_NDEF_Reset(RfIntf.Protocol);

	/* In case of T3T tag, retrieve card IDm for further operation */
	if (RfIntf.Protocol == PROT_T3T) NxpNci_T3TretrieveIDm();

	while(1)
	{
		RW_NDEF_Write_Next(&Answer[3], Answer[2], &Cmd[3], (unsigned short *) &CmdSize);
		if(CmdSize == 0)
		{
			/* End of the Write operation */
			break;
		}
		else
		{
			/* Compute and send DATA_PACKET */
			Cmd[0] = 0x00;
			Cmd[1] = (CmdSize & 0xFF00) >> 8;
			Cmd[2] = CmdSize & 0x00FF;

			NxpNci_HostTransceive(Cmd, CmdSize+3, Answer, sizeof(Answer), &AnswerSize);
	        NxpNci_WaitForReception(Answer, sizeof(Answer), &AnswerSize, TIMEOUT_2S);
		}
	}
}
 
static void NxpNci_PresenceCheck(NxpNci_RfIntf_t RfIntf)
{
    uint8_t Answer[MAX_NCI_FRAME_SIZE];
    uint16_t AnswerSize;

    uint8_t NCIPresCheckT1T[] = {0x00, 0x00, 0x07, 0x78, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    uint8_t NCIPresCheckT2T[] = {0x00, 0x00, 0x02, 0x30, 0x00};
    uint8_t NCIPresCheckT3T[] = {0x21, 0x08, 0x04, 0x12, 0xFC, 0x00, 0x01};
    uint8_t NCIPresCheckIsoDep[] = {0x2F, 0x11, 0x00};
    uint8_t NCIPresCheckIso15693[] = {0x00, 0x00, 0x03, 0x26, 0x01, 0x00};
    uint8_t NCIDeactivate[] = {0x21, 0x06, 0x01, 0x01};
    uint8_t NCISelectMIFARE[] = {0x21, 0x04, 0x03, 0x01, 0x80, 0x80};

    switch (RfIntf.Protocol) {
    case PROT_T1T:
        do
        {
            //SLEEP(500);
            NxpNci_HostTransceive(NCIPresCheckT1T, sizeof(NCIPresCheckT1T), Answer, sizeof(Answer), &AnswerSize);
            NxpNci_WaitForReception(Answer, sizeof(Answer), &AnswerSize, TIMEOUT_10MS);
        } while ((Answer[0] == 0x00) && (Answer[1] == 0x00));
        break;

    case PROT_T2T:
        do
        {
            //SLEEP(500);
            NxpNci_HostTransceive(NCIPresCheckT2T, sizeof(NCIPresCheckT2T), Answer, sizeof(Answer), &AnswerSize);
            NxpNci_WaitForReception(Answer, sizeof(Answer), &AnswerSize, TIMEOUT_10MS);
        } while ((Answer[0] == 0x00) && (Answer[1] == 0x00) && (Answer[2] == 0x11));
        break;

    case PROT_T3T:
        do
        {
            //SLEEP(500);
            NxpNci_HostTransceive(NCIPresCheckT3T, sizeof(NCIPresCheckT3T), Answer, sizeof(Answer), &AnswerSize);
            NxpNci_WaitForReception(Answer, sizeof(Answer), &AnswerSize, TIMEOUT_10MS);
        } while ((Answer[0] == 0x61) && (Answer[1] == 0x08) && (Answer[3] == 0x00));
        break;

    case PROT_ISODEP:
        do
        {
            //SLEEP(500);
            NxpNci_HostTransceive(NCIPresCheckIsoDep, sizeof(NCIPresCheckIsoDep), Answer, sizeof(Answer), &AnswerSize);
            NxpNci_WaitForReception(Answer, sizeof(Answer), &AnswerSize, TIMEOUT_10MS);
        } while ((Answer[0] == 0x6F) && (Answer[1] == 0x11) && (Answer[2] == 0x01) && (Answer[3] == 0x01));
        break;

    case PROT_ISO15693:
        do
        {
            //SLEEP(500);
            NxpNci_HostTransceive(NCIPresCheckIso15693, sizeof(NCIPresCheckIso15693), Answer, sizeof(Answer), &AnswerSize);
            NxpNci_WaitForReception(Answer, sizeof(Answer), &AnswerSize, TIMEOUT_10MS);
        } while ((Answer[0] == 0x00) && (Answer[1] == 0x00));
        break;

    case PROT_MIFARE:
        do
        {
            //SLEEP(500);
            /* Deactivate target */
            NxpNci_HostTransceive(NCIDeactivate, sizeof(NCIDeactivate), Answer, sizeof(Answer), &AnswerSize);
            NxpNci_WaitForReception(Answer, sizeof(Answer), &AnswerSize, TIMEOUT_10MS);
            /* Reactivate target */
            NxpNci_HostTransceive(NCISelectMIFARE, sizeof(NCISelectMIFARE), Answer, sizeof(Answer), &AnswerSize);
            NxpNci_WaitForReception(Answer, sizeof(Answer), &AnswerSize, TIMEOUT_10MS);
        } while ((Answer[0] == 0x61) && (Answer[1] == 0x05));
        break;

    default:
        /* Nothing to do */
        break;
    }
} 

void NxpNci_ProcessReaderMode(NxpNci_RfIntf_t RfIntf, NxpNci_RW_Operation_t Operation)
{
	switch (Operation)
	{
	case READ_NDEF:
		NxpNci_ReadNdef(RfIntf);
		break;

	case WRITE_NDEF:
		NxpNci_WriteNdef(RfIntf);
		break;

	case PRESENCE_CHECK:
		NxpNci_PresenceCheck(RfIntf);
		break;

	default:
		break;
	}
}
#endif

boolean NxpNci_Connect(void)
{
	uint8_t i = 10;
	uint8_t NCICoreInit[] = {0x20, 0x01, 0x00};
	uint8_t Answer[MAX_NCI_FRAME_SIZE];
	uint16_t AnswerSize;

	/* Open connection to NXPNCI */
	tml_Connect ();

	/* Loop until NXPNCI answers */
	while(NxpNci_CheckDevPres() != NXPNCI_SUCCESS)
	{
		if(i-- == 0) return NXPNCI_ERROR;
		SLEEP(500);
	}

	NxpNci_HostTransceive(NCICoreInit, sizeof(NCICoreInit), Answer, sizeof(Answer), &AnswerSize);
	if ((Answer[0] != 0x40) || (Answer[1] != 0x01) || (Answer[3] != 0x00)) return NXPNCI_ERROR;

	/* Retrieve NXP-NCI NFC Controller generation */
	if (Answer[17+Answer[8]] == 0x08) NfcController_generation = 1;
	else if (Answer[17+Answer[8]] == 0x10) NfcController_generation = 2;

	return NXPNCI_SUCCESS;
}

boolean NxpNci_ConfigureSettings(void)
{
    uint8_t NCICoreReset[] = {0x20, 0x00, 0x01, 0x01};
	uint8_t NCICoreInit[] = {0x20, 0x01, 0x00};
	uint8_t Answer[MAX_NCI_FRAME_SIZE];
	uint16_t AnswerSize;
	uint8_t *NxpNci_RF_CONF;
	uint16_t NxpNci_RF_CONF_size = 0;

	/* Apply settings according definition of Nfc_settings.h header file */

#if NXP_CORE_CONF
	if (sizeof(NxpNci_CORE_CONF) != 0)
	{
		NxpNci_HostTransceive(NxpNci_CORE_CONF, sizeof(NxpNci_CORE_CONF), Answer, sizeof(Answer), &AnswerSize);
		if ((Answer[0] != 0x40) || (Answer[1] != 0x02) || (Answer[3] != 0x00) || (Answer[4] != 0x00)) return NXPNCI_ERROR;
	}
#endif

#if NXP_CORE_CONF_EXTN
	if (sizeof(NxpNci_CORE_CONF_EXTN) != 0)
	{
		NxpNci_HostTransceive(NxpNci_CORE_CONF_EXTN, sizeof(NxpNci_CORE_CONF_EXTN), Answer, sizeof(Answer), &AnswerSize);
		if ((Answer[0] != 0x40) || (Answer[1] != 0x02) || (Answer[3] != 0x00) || (Answer[4] != 0x00)) return NXPNCI_ERROR;
	}
#endif

#if NXP_CORE_STANDBY
	if (sizeof(NxpNci_CORE_STANDBY) != 0)
	{
		NxpNci_HostTransceive(NxpNci_CORE_STANDBY, sizeof(NxpNci_CORE_STANDBY), Answer, sizeof(Answer), &AnswerSize);
		if ((Answer[0] != 0x4F) || (Answer[1] != 0x00) || (Answer[3] != 0x00)) return NXPNCI_ERROR;
	}

#endif

#if NXP_RF_CONF
	if(NfcController_generation == 1)
	{
		NxpNci_RF_CONF = NxpNci_RF_CONF_1stGen;
		NxpNci_RF_CONF_size = sizeof(NxpNci_RF_CONF_1stGen);
	}
	else if(NfcController_generation == 2)
	{
		NxpNci_RF_CONF = NxpNci_RF_CONF_2ndGen;
		NxpNci_RF_CONF_size = sizeof(NxpNci_RF_CONF_2ndGen);
	}
	if (NxpNci_RF_CONF_size != 0)
	{
		NxpNci_HostTransceive(NxpNci_RF_CONF, NxpNci_RF_CONF_size, Answer, sizeof(Answer), &AnswerSize);
		if ((Answer[0] != 0x40) || (Answer[1] != 0x02) || (Answer[3] != 0x00) || (Answer[4] != 0x00)) return NXPNCI_ERROR;

		/* Reset the NFC Controller to insure RF settings apply */
		NxpNci_HostTransceive(NCICoreReset, sizeof(NCICoreReset), Answer, sizeof(Answer), &AnswerSize);
		if ((Answer[0] != 0x40) || (Answer[1] != 0x00) || (Answer[3] != 0x00)) return NXPNCI_ERROR;
		NxpNci_HostTransceive(NCICoreInit, sizeof(NCICoreInit), Answer, sizeof(Answer), &AnswerSize);
		if ((Answer[0] != 0x40) || (Answer[1] != 0x01) || (Answer[3] != 0x00)) return NXPNCI_ERROR;

	}
#endif
	return NXPNCI_SUCCESS;
}

boolean NxpNci_ConfigureMode(unsigned char mode)
{
	uint8_t Command[MAX_NCI_FRAME_SIZE];
	uint8_t Answer[MAX_NCI_FRAME_SIZE];
	uint16_t AnswerSize;
	uint8_t Item = 0;

	uint8_t NCIDiscoverMap[] = {0x21, 0x00};
#ifdef CARDEMU_SUPPORT
	    const uint8_t DM_CARDEMU[] = {0x4, 0x2, 0x2};
	    const uint8_t R_CARDEMU[] = {0x1, 0x3, 0x0, 0x1, 0x4};
#endif
#ifdef P2P_SUPPORT
	    const uint8_t DM_P2P[] = {0x5, 0x3, 0x3};
	    const uint8_t R_P2P[] = {0x1, 0x3, 0x0, 0x1, 0x5};
	    uint8_t NCISetConfig_NFC[] = {0x20, 0x02, 0x1F, 0x02, 0x29, 0x0D, 0x46, 0x66, 0x6D, 0x01, 0x01, 0x11, 0x03, 0x02, 0x00, 0x01, 0x04, 0x01, 0xFA, 0x61, 0x0D, 0x46, 0x66, 0x6D, 0x01, 0x01, 0x11, 0x03, 0x02, 0x00, 0x01, 0x04, 0x01, 0xFA};
#endif
#ifdef RW_SUPPORT
	    const uint8_t DM_RW[] = {0x1, 0x1, 0x1,	 0x2, 0x1, 0x1,  0x3, 0x1, 0x1,  0x4, 0x1, 0x2,  0x80, 0x01, 0x80};
	    uint8_t NCIPropAct[] = {0x2F, 0x02, 0x00};
#endif
#if defined P2P_SUPPORT || defined CARDEMU_SUPPORT
	    uint8_t NCIRouting[] = {0x21, 0x01, 0x07, 0x00, 0x01};
	    uint8_t NCISetConfig_NFCA_SELRSP[] = {0x20, 0x02, 0x04, 0x01, 0x32, 0x01, 0x00};
#endif

	if(mode == 0) return NXPNCI_SUCCESS;

	/* Enable Proprietary interface for T4T card presence check procedure */
//#ifdef RW_SUPPORT
    //if((FMSTR_AppState == 1)||(FMSTR_AppState == 2))
    //{
	    if (mode == NXPNCI_MODE_RW)
	    {
		    NxpNci_HostTransceive(NCIPropAct, sizeof(NCIPropAct), Answer, sizeof(Answer), &AnswerSize);
		    if ((Answer[0] != 0x4F) || (Answer[1] != 0x02) || (Answer[3] != 0x00)) return NXPNCI_ERROR;
	    }
    //}
//#endif

	/* Building Discovery Map command */
	Item = 0;
//#ifdef CARDEMU_SUPPORT
    //if(FMSTR_AppState == 4)
    //{
	    if (mode & NXPNCI_MODE_CARDEMU)
	    {
		    memcpy(&Command[4+(3*Item)], DM_CARDEMU, sizeof(DM_CARDEMU));
		    Item++;
	    }
    //}
//#endif
//#ifdef P2P_SUPPORT
    //if(FMSTR_AppState == 3)
    //{
	    if (mode & NXPNCI_MODE_P2P)
	    {
		    memcpy(&Command[4+(3*Item)], DM_P2P, sizeof(DM_P2P));
		    Item++;
	    }
    //}
//#endif
//#ifdef RW_SUPPORT
    //if((FMSTR_AppState == 1)||(FMSTR_AppState == 2))
    //{
	    if (mode & NXPNCI_MODE_RW)
	    {
		    memcpy(&Command[4+(3*Item)], DM_RW, sizeof(DM_RW));
		    Item+=sizeof(DM_RW)/3;
	    }
    //}
//#endif
#if defined P2P_SUPPORT || defined CARDEMU_SUPPORT || defined RW_SUPPORT
	if (Item != 0)
	{
		memcpy(Command, NCIDiscoverMap, sizeof(NCIDiscoverMap));
		Command[2] = 1 + (Item*3);
		Command[3] = Item;
	    NxpNci_HostTransceive(Command, 3 + Command[2], Answer, sizeof(Answer), &AnswerSize);
		if ((Answer[0] != 0x41) || (Answer[1] != 0x00) || (Answer[3] != 0x00)) return NXPNCI_ERROR;
	}
#endif

	/* Configuring routing */
	Item = 0;
//#ifdef CARDEMU_SUPPORT
    //if(FMSTR_AppState == 4)
    //{
	    if (mode & NXPNCI_MODE_CARDEMU)
	    {
		    memcpy(&Command[5+(5*Item)], R_CARDEMU, sizeof(R_CARDEMU));
		    Item++;
	    }
    //}
//#endif
//#ifdef P2P_SUPPORT
    //if(FMSTR_AppState == 3)
    //{
	    if (mode & NXPNCI_MODE_P2P)
	    {
		    memcpy(&Command[5+(5*Item)], R_P2P, sizeof(R_P2P));
		    Item++;
	    }
    //}
//#endif
//#if defined P2P_SUPPORT || defined CARDEMU_SUPPORT
    //if((FMSTR_AppState == 3)||(FMSTR_AppState == 4))
    //{
	    if (Item != 0)
	    {
		    memcpy(Command, NCIRouting, sizeof(NCIRouting));
		    Command[2] = 2 + (Item*5);
		    Command[4] = Item;
		    NxpNci_HostTransceive(Command, 3 + Command[2] , Answer, sizeof(Answer), &AnswerSize);
		    if ((Answer[0] != 0x41) || (Answer[1] != 0x01) || (Answer[3] != 0x00)) return NXPNCI_ERROR;
	    }
    //}
//#endif

	/* Setting NFCA SEL_RSP */
//#ifdef CARDEMU_SUPPORT
    //if(FMSTR_AppState == 4)
    //{
	    if (mode & NXPNCI_MODE_CARDEMU)
	    {
		    NCISetConfig_NFCA_SELRSP[6] += 0x20;
	    }
    //}
//#endif
//#ifdef P2P_SUPPORT
    //if(FMSTR_AppState == 3)
    //{
    	if (mode & NXPNCI_MODE_P2P)
		{
		    NCISetConfig_NFCA_SELRSP[6] += 0x40;
	    }
    //}
//#endif
//#if defined P2P_SUPPORT || defined CARDEMU_SUPPORT
    //if((FMSTR_AppState == 3)||(FMSTR_AppState == 4))
    //{
    	if (NCISetConfig_NFCA_SELRSP[6] != 0x00)
		{
			NxpNci_HostTransceive(NCISetConfig_NFCA_SELRSP, sizeof(NCISetConfig_NFCA_SELRSP), Answer, sizeof(Answer), &AnswerSize);
			if ((Answer[0] != 0x40) || (Answer[1] != 0x02) || (Answer[3] != 0x00)) return NXPNCI_ERROR;
		}
    //}
//#endif

	/* Setting LLCP support */
//#ifdef P2P_SUPPORT
    //if(FMSTR_AppState == 3)
    //{
    	if (mode & NXPNCI_MODE_P2P)
		{
			NxpNci_HostTransceive(NCISetConfig_NFC, sizeof(NCISetConfig_NFC), Answer, sizeof(Answer), &AnswerSize);
			if ((Answer[0] != 0x40) || (Answer[1] != 0x02) || (Answer[3] != 0x00)) return NXPNCI_ERROR;
		}
    //}
//#endif

	return NXPNCI_SUCCESS;
}

boolean NxpNci_StartDiscovery(unsigned char *pTechTab, unsigned char TechTabSize)
{
	uint8_t Buffer[MAX_NCI_FRAME_SIZE];
	uint8_t Answer[MAX_NCI_FRAME_SIZE];
	uint16_t AnswerSize;
	uint8_t i;

	Buffer[0] = 0x21;
	Buffer[1] = 0x03;
	Buffer[2] = (TechTabSize * 2) + 1;
	Buffer[3] = TechTabSize;
	for (i=0; i<TechTabSize; i++)
	{
		Buffer[(i*2)+4] = pTechTab[i];
		Buffer[(i*2)+5] = 0x01;
	}

	NxpNci_HostTransceive(Buffer, (TechTabSize * 2) + 4, Answer, sizeof(Answer), &AnswerSize);
	if ((Answer[0] != 0x41) || (Answer[1] != 0x03) || (Answer[3] != 0x00)) return NXPNCI_ERROR;

	return NXPNCI_SUCCESS;
}

boolean NxpNci_RestartDiscovery(void)
{
	uint8_t NCIRestartDiscovery[] = {0x21, 0x06, 0x01, 0x03};
	uint8_t Answer[MAX_NCI_FRAME_SIZE];
	uint16_t AnswerSize;

	NxpNci_HostTransceive(NCIRestartDiscovery, sizeof(NCIRestartDiscovery), Answer, sizeof(Answer), &AnswerSize);
	NxpNci_WaitForReception(Answer, sizeof(Answer), &AnswerSize, TIMEOUT_1S);

	return NXPNCI_SUCCESS;
}

boolean NxpNci_StopDiscovery(void)
{
	uint8_t NCIRestartDiscovery[] = {0x21, 0x06, 0x01, 0x00};
	uint8_t Answer[MAX_NCI_FRAME_SIZE];
	uint16_t AnswerSize;

	NxpNci_HostTransceive(NCIRestartDiscovery, sizeof(NCIRestartDiscovery), Answer, sizeof(Answer), &AnswerSize);
	NxpNci_WaitForReception(Answer, sizeof(Answer), &AnswerSize, TIMEOUT_1S);

	return NXPNCI_SUCCESS;
}

#ifdef WAIT_NOTIF_STM
boolean NxpNci_WaitForDiscNotification_stm(NxpNci_RfIntf_t *pRfIntf)
{
	//uint8_t NCIRfDiscoverSelect[] = {0x21, 0x04, 0x03, 0x01, PROT_ISODEP, INTF_ISODEP};
	//uint8_t AnswerBuffer[MAX_NCI_FRAME_SIZE];
	//uint16_t AnswerSize;
	uint8_t polling_timeout_cntr = 0;
	
    uint8_t irq_state = 0;
	
    switch(nci_state)
    {
        case NCI_WAIT_IRQ:
        {
        	irq_state = GET_IRQ_VALUE;
        	
        	if (GET_IRQ_VALUE != 0u)
        	{
        		nci_state = NCI_START_DISCOVERY;
        	} 
        	else 
        	{
        		/* do nothing, wait for irq */
        	}
        	
            break;	
        }
        
        case NCI_START_DISCOVERY:
        {
    		if (NXPNCI_ERROR == NxpNci_WaitForReception(AnswerBuffer, sizeof(AnswerBuffer), &AnswerSize, TIMEOUT_500MS)) { };
    				//return NXPNCI_ERROR;
    		if(polling_timeout_cntr++ > 10)
    				return NXPNCI_ERROR; 
    		
    		if((AnswerBuffer[0] != 0x61) || ((AnswerBuffer[1] != 0x05) && (AnswerBuffer[1] != 0x03)))
    		{
    			nci_state = NCI_WAIT_IRQ;
    		}
    		else
    		{
    			nci_state = NCI_INTERFACE_INFO;
    		}
    		
    		break;
        }
        
        case NCI_INTERFACE_INFO:
        {
        	/* Is RF_INTF_ACTIVATED_NTF ? */
        	if (AnswerBuffer[1] == 0x05)
        	{
        		pRfIntf->Interface = AnswerBuffer[4];
        		pRfIntf->Protocol = AnswerBuffer[5];
        		pRfIntf->ModeTech = AnswerBuffer[6];
        		NxpNci_FillInterfaceInfo(pRfIntf, &AnswerBuffer[10]);
        		
        		nci_state = NCI_END;
        	}
        	else
        	{
            	/* RF_DISCOVER_NTF */
        		pRfIntf->Interface = INTF_UNDETERMINED;
        		pRfIntf->Protocol = AnswerBuffer[4];
        		pRfIntf->ModeTech = AnswerBuffer[5];

        		nci_state = NCI_WAIT_SECOND_IRQ;
        	}
        	
        	break;
        }
        
        case NCI_WAIT_SECOND_IRQ:
        {
        	irq_state = GET_IRQ_VALUE;
        	
        	if (GET_IRQ_VALUE != 0u)
        	{
        		nci_state = NCI_RF_DISCOVER_NTF;
        	} 
        	else 
        	{
        		/* do nothing, wait for irq */
        	}
        	
            break;	
        }
        
        case NCI_RF_DISCOVER_NTF:
        {
    		/* Remaining NTF ? */
    		if(AnswerBuffer[AnswerSize-1] == 0x02)
    		{
    			NxpNci_WaitForReception(AnswerBuffer, sizeof(AnswerBuffer), &AnswerSize, TIMEOUT_100MS);
    			
    			nci_state = NCI_WAIT_SECOND_IRQ;
    		}
    		else
    		{
        		/* In case of multi-protocol card, select the ISODEP protocol */
        		if (pRfIntf->Protocol == PROT_ISODEP)
        		{
        			NxpNci_HostTransceive(NCIRfDiscoverSelect, sizeof(NCIRfDiscoverSelect), AnswerBuffer, sizeof(AnswerBuffer), &AnswerSize);
        			if ((AnswerBuffer[0] == 0x41) || (AnswerBuffer[1] == 0x04) || (AnswerBuffer[3] == 0x00))
        			{
        				nci_state = NCI_WAIT_THIRD_IRQ;
        			}
        			else
        			{
        				nci_state = NCI_END;
        			}
        		}
        		else
        		{
        			nci_state = NCI_END;
        		}
    		}
    		
    		break;
        }
        
        case NCI_WAIT_THIRD_IRQ:
        {
        	irq_state = GET_IRQ_VALUE;
        	
        	if (GET_IRQ_VALUE != 0u)
        	{
        		nci_state = NCI_MULTI_PROTOCOL;
        	} 
        	else 
        	{
        		/* do nothing, wait for irq */
        	}
        	
            break;	
        }
        
        case NCI_MULTI_PROTOCOL:
        {

    		NxpNci_WaitForReception(AnswerBuffer, sizeof(AnswerBuffer), &AnswerSize, TIMEOUT_100MS);
    		if ((AnswerBuffer[0] == 0x61) || (AnswerBuffer[1] == 0x05))
    		{
    			pRfIntf->Interface = AnswerBuffer[4];
    			pRfIntf->Protocol = AnswerBuffer[5];
    			pRfIntf->ModeTech = AnswerBuffer[6];
    			NxpNci_FillInterfaceInfo(pRfIntf, &AnswerBuffer[10]);
    			
    			nci_state = NCI_END;
    		}
    		else
    		{
    		    nci_state = NCI_INVALID;
    		}
    			
        	break;
        }
        
        case NCI_END:
        {
        	/* Do nothing */
        	break;
        }
        
        case NCI_INVALID:
        {
        	/* unexpected behavior */
        	break;
        }

        default:
        	break;
    }

	return NXPNCI_SUCCESS;
}
#endif

boolean NxpNci_WaitForDiscoveryNotification(NxpNci_RfIntf_t *pRfIntf)
{
	uint8_t NCIRfDiscoverSelect[] = {0x21, 0x04, 0x03, 0x01, PROT_ISODEP, INTF_ISODEP};
	uint8_t AnswerBuffer[MAX_NCI_FRAME_SIZE];
	uint16_t AnswerSize;
	uint8_t polling_timeout_cntr = 0;
	do
	{
		if (NXPNCI_ERROR == NxpNci_WaitForReception(AnswerBuffer, sizeof(AnswerBuffer), &AnswerSize, TIMEOUT_100MS)) { };
				//return NXPNCI_ERROR;
		if(polling_timeout_cntr++ > 10)
				return NXPNCI_ERROR; 
	}while ((AnswerBuffer[0] != 0x61) || ((AnswerBuffer[1] != 0x05) && (AnswerBuffer[1] != 0x03)));

    gNextTag_Protocol = PROT_UNDETERMINED;

	/* Is RF_INTF_ACTIVATED_NTF ? */
	if (AnswerBuffer[1] == 0x05)
	{
		pRfIntf->Interface = AnswerBuffer[4];
		pRfIntf->Protocol = AnswerBuffer[5];
		pRfIntf->ModeTech = AnswerBuffer[6];
		NxpNci_FillInterfaceInfo(pRfIntf, &AnswerBuffer[10]);
	}
	else /* RF_DISCOVER_NTF */
	{
		pRfIntf->Interface = INTF_UNDETERMINED;
		pRfIntf->Protocol = AnswerBuffer[4];
		pRfIntf->ModeTech = AnswerBuffer[5];
        pRfIntf->MoreTags = TRUE;

        /* Get next NTF for further activation */
        do {
            if(NxpNci_WaitForReception(AnswerBuffer, sizeof(AnswerBuffer), &AnswerSize, TIMEOUT_100MS) == NXPNCI_ERROR)    return NXPNCI_ERROR;
        } while ((AnswerBuffer[0] != 0x61) || (AnswerBuffer[1] != 0x03));
        gNextTag_Protocol = AnswerBuffer[4];

		/* Remaining NTF ? */
		while(AnswerBuffer[AnswerSize-1] == 0x02) NxpNci_WaitForReception(AnswerBuffer, sizeof(AnswerBuffer), &AnswerSize, TIMEOUT_100MS);

        /* In case of multiple cards, select the first one */
        NCIRfDiscoverSelect[4] = pRfIntf->Protocol;
        if (pRfIntf->Protocol == PROT_ISODEP) NCIRfDiscoverSelect[5] = INTF_ISODEP;
        else if (pRfIntf->Protocol == PROT_NFCDEP) NCIRfDiscoverSelect[5] = INTF_NFCDEP;
        else if (pRfIntf->Protocol == PROT_MIFARE) NCIRfDiscoverSelect[5] = INTF_TAGCMD;
        else NCIRfDiscoverSelect[5] = INTF_FRAME;
			NxpNci_HostTransceive(NCIRfDiscoverSelect, sizeof(NCIRfDiscoverSelect), AnswerBuffer, sizeof(AnswerBuffer), &AnswerSize);
			if ((AnswerBuffer[0] == 0x41) || (AnswerBuffer[1] == 0x04) || (AnswerBuffer[3] == 0x00))
			{
            NxpNci_WaitForReception(AnswerBuffer, sizeof(AnswerBuffer), &AnswerSize, TIMEOUT_100MS);
				if ((AnswerBuffer[0] == 0x61) || (AnswerBuffer[1] == 0x05))
				{
					pRfIntf->Interface = AnswerBuffer[4];
					pRfIntf->Protocol = AnswerBuffer[5];
					pRfIntf->ModeTech = AnswerBuffer[6];
					NxpNci_FillInterfaceInfo(pRfIntf, &AnswerBuffer[10]);
				}
			}
		}

    /* In case of unknown target align protocol information */
    if (pRfIntf->Interface == INTF_UNDETERMINED) pRfIntf->Protocol = PROT_UNDETERMINED;

    return NXPNCI_SUCCESS;
	}

boolean NxpNci_LpcdWakeup(uint16_t *agc_val, uint8_t delta_agc, uint16_t *agc_nfcld_val, uint8_t delta_agc_nfcld)
{
    uint8_t NCICoreReset[] = {0x20, 0x00, 0x01, 0x01};
	uint8_t NCICoreInit[] = {0x20, 0x01, 0x00};
	uint8_t NCIAntennaTestCmdAgc[] = {0x2F, 0x3D, 0x04, 0x02, 0xC8, 0x60, 0x03};
	uint8_t NCIAntennaTestCmdAgcNfcLd[] = {0x2F, 0x3D, 0x04, 0x04, 0x20, 0x08, 0x20};
	uint8_t Answer[MAX_NCI_FRAME_SIZE];
	uint8_t AnswerTestCmdAgc[MAX_NCI_FRAME_SIZE];
	uint8_t AnswerTestCmdAgcNfcLd[MAX_NCI_FRAME_SIZE];
	uint16_t AnswerSize;
	uint16_t agc_tmp = 0x00;
	/* antenna test cmd with agc only */
	NxpNci_HostTransceive(NCICoreReset, sizeof(NCICoreReset), Answer, sizeof(Answer), &AnswerSize);
	if ((Answer[0] != 0x40) || (Answer[1] != 0x00) || (Answer[3] != 0x00)) return NXPNCI_ERROR;
	NxpNci_HostTransceive(NCICoreInit, sizeof(NCICoreInit), Answer, sizeof(Answer), &AnswerSize);
	if ((Answer[0] != 0x40) || (Answer[1] != 0x01) || (Answer[3] != 0x00)) return NXPNCI_ERROR;
	NxpNci_HostTransceive(NCIAntennaTestCmdAgc, sizeof(NCIAntennaTestCmdAgc), Answer, sizeof(Answer), &AnswerSize);	
	if ((Answer[0] != 0x4F) || (Answer[1] != 0x3D) || (Answer[3] != 0x00)) return NXPNCI_ERROR;
	if(AnswerSize == 8)
	{
		agc_tmp = (Answer[5] & 0x03) << 8;
		*agc_val = agc_tmp | Answer[4];
	}
	/* antenna test cmd with agc nfc ld 
	 * Currently unused */
	/*NxpNci_HostTransceive(NCICoreReset, sizeof(NCICoreReset), Answer, sizeof(Answer), &AnswerSize);
	if ((Answer[0] != 0x40) || (Answer[1] != 0x00) || (Answer[3] != 0x00)) return NXPNCI_ERROR;
	NxpNci_HostTransceive(NCICoreInit, sizeof(NCICoreInit), Answer, sizeof(Answer), &AnswerSize);
	if ((Answer[0] != 0x40) || (Answer[1] != 0x01) || (Answer[3] != 0x00)) return NXPNCI_ERROR;	
	NxpNci_HostTransceive(NCIAntennaTestCmdAgcNfcLd, sizeof(NCIAntennaTestCmdAgcNfcLd), Answer, sizeof(Answer), &AnswerSize);
	if ((Answer[0] != 0x4F) || (Answer[1] != 0x3D) || (Answer[3] != 0x00)) return NXPNCI_ERROR;*/
	
}
#endif
