/*******************************************************************************
*
* Copyright 2012-2014 Freescale Semiconductor, Inc.
* Copyright 2017~2018 NXP.
* All rights reserved.
*******************************************************************************/

#include "defines.h"
#include "cpu.h"

#include "systemDebug.h"

#include "dac.h"

#define DAC_REF_VOL  3300U
#define DAC_BITS       12U

void DAC_Init(void)
{
	DAC_CTRL1 = 0x000CU;
    DAC_CTRL0 = 0x1000U;  //DAC enabled
}

void DAC_SetVoltage(uint8 byId, uint16 wVoltage)
{
	uint32 tmp = wVoltage;
	
	DBG_Assert(wVoltage > DAC_REF_VOL, 0, wVoltage);
	
	tmp <<= DAC_BITS;
	tmp  /= DAC_REF_VOL;
	
	if(DAC_Q_MVL_INDEX == byId)
	{
		DAC_DATAREG_FMT0 = tmp;
	}
	else
	{
		DBG_Assert(1, 0, byId);
	}
}

void DAC_PowerDown(void)
{
	DAC_CTRL0 |= DAC_CTRL0_PDN;
}

void DAC_PowerUp(void)
{
	DAC_CTRL0 &= ~DAC_CTRL0_PDN;
}
