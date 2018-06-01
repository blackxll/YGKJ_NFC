/* *************************
* Copyright 2018 NXP
* All rights reserved.
**************************** */

#ifdef RW_SUPPORT
#include "nfc_driver_config.h"
#include "Nfc.h"
#include "RW_NDEF.h"
#include "RW_NDEF_T1T.h"
#include "RW_NDEF_T2T.h"
#include "RW_NDEF_T3T.h"
#include "RW_NDEF_T4T.h"

/* Allocate buffer for NDEF operations */
unsigned char NdefBuffer[RW_MAX_NDEF_FILE_SIZE];

typedef void RW_NDEF_Fct_t (unsigned char *pCmd, unsigned short Cmd_size, unsigned char *Rsp, unsigned short *pRsp_size);

unsigned char *pNdefMessage;
unsigned short NdefMessage_size;

RW_NDEF_Callback_t *pRW_NDEF_PullCb;
RW_NDEF_Callback_t *pRW_NDEF_PushCb;

static RW_NDEF_Fct_t *pReadFct = NULL;
static RW_NDEF_Fct_t *pWriteFct = NULL;

boolean RW_NDEF_SetMessage(unsigned char *pMessage, unsigned short Message_size, void *pCb)
{
    if (Message_size <= RW_MAX_NDEF_FILE_SIZE)
    {
        pNdefMessage = pMessage;
        NdefMessage_size = Message_size;
        pRW_NDEF_PushCb = (RW_NDEF_Callback_t*) pCb;
		return TRUE;
    }
    else
    {
        NdefMessage_size = 0;
		pRW_NDEF_PushCb = NULL;
		return FALSE;
    }
}

void RW_NDEF_RegisterPullCallback(void *pCb)
{
	pRW_NDEF_PullCb = (RW_NDEF_Callback_t *) pCb;
}

void RW_NDEF_Reset(unsigned char type)
{
	pReadFct = NULL;
	pWriteFct = NULL;

	switch (type)
	{
	case RW_NDEF_TYPE_T1T:
		RW_NDEF_T1T_Reset();
		pReadFct = RW_NDEF_T1T_Read_Next;
		break;
	case RW_NDEF_TYPE_T2T:
		RW_NDEF_T2T_Reset();
		pReadFct = RW_NDEF_T2T_Read_Next;
		pWriteFct = RW_NDEF_T2T_Write_Next;
		break;
	case RW_NDEF_TYPE_T3T:
		RW_NDEF_T3T_Reset();
		pReadFct = RW_NDEF_T3T_Read_Next;
		break;
	case RW_NDEF_TYPE_T4T:
		RW_NDEF_T4T_Reset();
		pReadFct = RW_NDEF_T4T_Read_Next;
		break;
	default:
		break;
	}
}

void RW_NDEF_Read_Next(unsigned char *pCmd, unsigned short Cmd_size, unsigned char *Rsp, unsigned short *pRsp_size)
{
	if (pReadFct != NULL) pReadFct(pCmd, Cmd_size, Rsp, pRsp_size);
}

void RW_NDEF_Write_Next(unsigned char *pCmd, unsigned short Cmd_size, unsigned char *Rsp, unsigned short *pRsp_size)
{
	if (pWriteFct != NULL) pWriteFct(pCmd, Cmd_size, Rsp, pRsp_size);
}
#endif
