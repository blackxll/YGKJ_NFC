/*******************************************************************************
*
* Copyright 2012-2016 Freescale Semiconductor, Inc.
* Copyright 2017~2018 NXP.
* All rights reserved.
*******************************************************************************/

#include "defines.h"
#include "cpu.h"
#include "appcfg.h"

#include "systemDebug.h"

#include "cmp.h"

#define CMP_INTERNAL_DAC_REF_VOL   		3300
#define CMP_INTERNAL_DAC_BITS           6

void CMP_Init(void)
{
#if DIGITAL_DCDC
	CMPA_CR0 = 0x10;
	CMPA_CR1 = 0x14;
	CMPA_FPR = 0;
	CMPA_DACCR = 0x80;	//Enable DAC
	CMPA_MUXCR = 0x1F;   //plus:IN3,  minus:6bit DAC
	
	CMPD_CR0 = 0x10;
	CMPD_CR1 = 0x14;
	CMPD_FPR = 0;
	CMPD_DACCR = 0x80;	//Enable DAC
	CMPD_MUXCR = 0x38;   //plus:6bit DAC  minus:IN0
#endif
	
	//For resonance Q factor measurement
	CMPC_CR0   = 0x0030U;
	CMPC_CR1   = 0x0010U;
	CMPC_FPR   = 0x0006U;
}

void CMP_Enable(uint8 byId, uint8 byIsEn)
{
	switch(byId)
	{
	case CMP_FRQ_INDEX:
		if(byIsEn)
		{
			CMPC_CR1 |= 0x01;
		}
		else
		{
			CMPC_CR1 &= ~0x01;
		}
		break;
#if DIGITAL_DCDC
	case CMP_D_DCDC_CURRENT_HIGH:
		if(byIsEn)
		{
			CMPA_CR1 |= 0x01;
		}
		else
		{
			CMPA_CR1 &= ~0x01;
		}
		break;
	case CMP_D_DCDC_CURRENT_LOW:
		if(byIsEn)
		{
			CMPD_CR1 |= 0x01;
		}
		else
		{
			CMPD_CR1 &= ~0x01;
		}
		break;
#endif
	default:
		DBG_Assert(1, 0, byId);
	}
}

void CMP_SetChannel(uint8 byId, uint8 byPolarity, uint8 byChannel)
{
	DBG_Assert(byChannel > 7, 0, byChannel);
	switch(byId)
	{
	case CMP_FRQ_INDEX:
		if(CMP_POLARITY_POSITIVE == byPolarity)
		{
			CMPC_MUXCR &= ~0x38;
			CMPC_MUXCR |= byChannel<<3;
		}
		else
		{
			CMPC_MUXCR &= ~0x07;
			CMPC_MUXCR |= byChannel;
		}
		break;
	default:
		DBG_Assert(1, 0, byId);
	}
}

boolean CMP_CheckRaisingFlag(uint8 byId)
{
	boolean ret = 0;
	switch(byId)
	{
	case CMP_FRQ_INDEX:
		if(CMPC_SCR & CMPC_SCR_CFR)
		{
			ret = 1;
		}
		break;
#if DIGITAL_DCDC
	case CMP_D_DCDC_CURRENT_HIGH:
		if( CMPA_SCR & CMPA_SCR_CFR)
		{
			ret = 1;
		}
		break;
	case CMP_D_DCDC_CURRENT_LOW:
		if(CMPD_SCR & CMPD_SCR_CFR)
		{
			ret = 1;
		}
		break;
#endif
	default:
		DBG_Assert(1, 0, byId);
	}
	return ret;
}

void CMP_ClearRaisingFlag(uint8 byId)
{
	uint16 tmp;
	switch(byId)
	{
	case CMP_FRQ_INDEX:
		tmp = CMPC_SCR;
		tmp &= 0xFFF8;
		tmp |= CMPC_SCR_CFR;
		CMPC_SCR = tmp;
		break;
#if DIGITAL_DCDC
	case CMP_D_DCDC_CURRENT_HIGH:
		CMPA_SCR |= CMPA_SCR_CFR;
		break;
	case CMP_D_DCDC_CURRENT_LOW:
		CMPD_SCR |= CMPD_SCR_CFR;
		break;
#endif
	default:
		DBG_Assert(1, 0, byId);
	}
}

void CMP_SetInternalDAC(uint8 byId, uint16 wDACLevel)
{
	uint32 tmp;

	tmp = wDACLevel;
	tmp <<= CMP_INTERNAL_DAC_BITS;
	tmp /= CMP_INTERNAL_DAC_REF_VOL;
	
	switch(byId)
	{
	case CMP_D_DCDC_CURRENT_HIGH:
		CMPA_DACCR = (CMPA_DACCR&0xC0) | tmp;
		break;
	case CMP_D_DCDC_CURRENT_LOW:
		CMPD_DACCR = (CMPD_DACCR&0xC0) | tmp;
		break;
	default:
		DBG_Assert(1, 0, byId);
	}
}
