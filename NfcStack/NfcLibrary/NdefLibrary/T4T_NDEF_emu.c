/* *************************
* Copyright 2018 NXP
* All rights reserved.
**************************** */

#ifdef CARDEMU_SUPPORT
#include <string.h>
#include "nfc_driver_config.h"
#include "Nfc.h"
#include "T4T_NDEF_EMU.h"

const unsigned char T4T_NDEF_EMU_APP_Select[] = {0x00,0xA4,0x04,0x00,0x07,0xD2,0x76,0x00,0x00,0x85,0x01,0x01};
const unsigned char T4T_NDEF_EMU_CC[] = {0x00, 0x0F, 0x20, 0x00, 0xFF, 0x00, 0xFF, 0x04, 0x06, 0xE1, 0x04, 0x00, 0xFF, 0x00, 0xFF};
const unsigned char T4T_NDEF_EMU_CC_Select[] = {0x00,0xA4,0x00,0x0C,0x02,0xE1,0x03};
const unsigned char T4T_NDEF_EMU_NDEF_Select[] = {0x00,0xA4,0x00,0x0C,0x02,0xE1,0x04};
const unsigned char T4T_NDEF_EMU_Read[] = {0x00,0xB0};

const unsigned char T4T_NDEF_EMU_OK[] = {0x90, 0x00};
const unsigned char T4T_NDEF_EMU_NOK[] = {0x6A, 0x82};

unsigned char *pT4T_NdefMessage;
unsigned short T4T_NdefMessage_size = 0;

typedef enum
{
	Ready,
    NDEF_Application_Selected,
    CC_Selected,
    NDEF_Selected
} T4T_NDEF_EMU_state_t;

typedef void T4T_NDEF_EMU_Callback_t (unsigned char*, unsigned short);

static T4T_NDEF_EMU_state_t eT4T_NDEF_EMU_State = Ready;

static T4T_NDEF_EMU_Callback_t *pT4T_NDEF_EMU_PushCb = NULL;

static void T4T_NDEF_EMU_FillRsp (unsigned char *pRsp, unsigned short offset, unsigned char length)
{
	if (offset == 0)
	{
		pRsp[0] = (T4T_NdefMessage_size & 0xFF00) >> 8;
		pRsp[1] = (T4T_NdefMessage_size & 0x00FF);
		memcpy(&pRsp[2], &pT4T_NdefMessage[0], length-2);
	}
	else if (offset == 1)
	{
		pRsp[0] = (T4T_NdefMessage_size & 0x00FF);
		memcpy(&pRsp[1], &pT4T_NdefMessage[0], length-1);
	}
	else
	{
		memcpy(pRsp, &pT4T_NdefMessage[offset-2], length);
	}

	/* Did we reached the end of NDEF message ?*/
	if ((offset + length) >= (T4T_NdefMessage_size + 2))
	{
		/* Notify application of the NDEF send */
		if(pT4T_NDEF_EMU_PushCb != NULL) pT4T_NDEF_EMU_PushCb(pT4T_NdefMessage, T4T_NdefMessage_size);
	}
}

boolean T4T_NDEF_EMU_SetMessage(unsigned char *pMessage, unsigned short Message_size, void *pCb)
{
	pT4T_NdefMessage = pMessage;
	T4T_NdefMessage_size = Message_size;
	pT4T_NDEF_EMU_PushCb = (T4T_NDEF_EMU_Callback_t*) pCb;

	return TRUE;
}

void T4T_NDEF_EMU_Reset(void)
{
	eT4T_NDEF_EMU_State = Ready;
}

void T4T_NDEF_EMU_Next(unsigned char *pCmd, unsigned short Cmd_size, unsigned char *pRsp, unsigned short *pRsp_size, uint8_t *is_ndef_selected)
{
    boolean eStatus = FALSE;
    unsigned short offset = 0;
    unsigned char length = 0;

    if (!memcmp(pCmd, T4T_NDEF_EMU_APP_Select, sizeof(T4T_NDEF_EMU_APP_Select)))
    {
        *pRsp_size = 0;
        eStatus = TRUE;
        eT4T_NDEF_EMU_State = NDEF_Application_Selected;
    }
    else if (!memcmp(pCmd, T4T_NDEF_EMU_CC_Select, sizeof(T4T_NDEF_EMU_CC_Select)))
    {
        if(eT4T_NDEF_EMU_State == NDEF_Application_Selected)
        {
            *pRsp_size = 0;
            eStatus = TRUE;
            eT4T_NDEF_EMU_State = CC_Selected;
        }
    }
    else if (!memcmp(pCmd, T4T_NDEF_EMU_NDEF_Select, sizeof(T4T_NDEF_EMU_NDEF_Select)))
    {
        *pRsp_size = 0;
        eStatus = TRUE;
        eT4T_NDEF_EMU_State = NDEF_Selected;
    }
    else if (!memcmp(pCmd, T4T_NDEF_EMU_Read, sizeof(T4T_NDEF_EMU_Read)))
    {
        if(eT4T_NDEF_EMU_State == CC_Selected)
        {
            memcpy(pRsp, T4T_NDEF_EMU_CC, sizeof(T4T_NDEF_EMU_CC));
            *pRsp_size = sizeof(T4T_NDEF_EMU_CC);
            eStatus = TRUE;
        }
        else if (eT4T_NDEF_EMU_State == NDEF_Selected)
        {
            offset = (pCmd[2] << 8) + pCmd[3];
            length = pCmd[4];

            if(length <= (T4T_NdefMessage_size + offset + 2))
            {
                T4T_NDEF_EMU_FillRsp(pRsp, offset, length);
                *pRsp_size = length;
                eStatus = TRUE;
                /* FMSTR adjustment to stop after one NDEF is read */
                *is_ndef_selected = (uint8_t) (*is_ndef_selected) + (uint8_t) 1;
            }
        }
    }

    if (eStatus == TRUE)
    {
		memcpy(&pRsp[*pRsp_size], T4T_NDEF_EMU_OK, sizeof(T4T_NDEF_EMU_OK));
		*pRsp_size += sizeof(T4T_NDEF_EMU_OK);
    } else
    {
		memcpy(pRsp, T4T_NDEF_EMU_NOK, sizeof(T4T_NDEF_EMU_NOK));
        *pRsp_size = sizeof(T4T_NDEF_EMU_NOK);
        T4T_NDEF_EMU_Reset();
    }
}
#endif
