/*******************************************************************************
*
* Copyright 2012-2016 Freescale Semiconductor, Inc.
* Copyright 2017~2018 NXP.
* All rights reserved.
*******************************************************************************/

#include "defines.h"
#include "cpu.h"

#include "systemDebug.h"

#include "adc.h"

void ADC_Init( void )
{
	ADC12_CTRL1   = 0x0805U;  //ADCA interrupt enable; triggered parallel mode
	ADC12_CTRL2   = 0x9009U;  //ADCB allows external sync trigger; ADCA clock = coreClock/10
	ADC12_PWR2    = 0x0905U;  //ADCB clock = coreClock/10
	ADC12_CLIST1  = 0x6735U;
	ADC12_CLIST2  = 0x1240U;
	ADC12_CLIST3  = 0xDA98U;
	ADC12_CLIST4  = 0xECBFU;
	ADC12_SDIS    = 0xE0E0U;

	ADC12_STAT    = 0x1800u;   // clear interrupt flags by writting a 1 to them
	ADC12_CTRL1  |= 0x0800u;   // Enable ADCA end of scan interrupts
	
	ADC12_PWR = 0x01A0U;    //power the ADC module
	
	// Wait for power-up to complete
	while (( ADC12_PWR & ( BIT10 | BIT11) )!= 0u)
	{ }
}


void ADC_GetResult(uint8 byId, uint16 *pwData, uint8 byLen)
{
	uint16 *pAdcResultReg;
	if(ADC_MISC_INDEX == byId)
	{
		pAdcResultReg = (uint16*)&ADC12_RSLT0;
	}
	else if(ADC_DMA_INDEX == byId)
	{
		pAdcResultReg = (uint16*)&ADC12_RSLT8;
	}
	else
	{
		DBG_Assert(1, 0, byId);
	}
	
	if(pwData)
	{
		while(byLen--)
		{
			uint16 adcData = *pAdcResultReg++;
			*pwData++ = (adcData>>3);
		}
	}
}

uint8 ADC_CheckConvertionFinished(uint8 byId)
{
    uint8 ret = 0;
    if(ADC_DMA_INDEX == byId)
    {        
        if(ADC12_STAT & BIT12)
        {
            ret = 1;
        }
    }
    else if(ADC_MISC_INDEX == byId)
    {
    	if(ADC12_STAT & BIT11)
		{
			ret = 1;
		}
    }
	return ret;
}

uint8 ADC_CheckConvertionOngoing(uint8 byId)
{
    uint8 ret = 0;
    if(ADC_DMA_INDEX == byId)
    {        
        if(ADC12_STAT & BIT14)
        {
            ret = 1;
        }
    }
    else if(ADC_MISC_INDEX == byId)
    {
    	if(ADC12_STAT & BIT15)
		{
			ret = 1;
		}
    }
	return ret;
}

void ADC_ClearConvertionFinishedFlag(uint8 byId)
{
    if(ADC_DMA_INDEX == byId)
    {
    	ADC12_STAT = BIT12;
    }
    else if(ADC_MISC_INDEX == byId)
    {
    	ADC12_STAT = BIT11;
    }
}

void ADC_SetChannelIndex(uint8 byId, uint8 byChIndex)
{
	if(ADC_DMA_INDEX == byId)
	{
		ADC12_CLIST3 &= ~0x000F;
		if(ADC_DMA_CHANNEL_DDM_INDEX == byChIndex)
		{	
			ADC12_CLIST3 |= 0x08;
		}
		else if(ADC_DMA_CHANNEL_QVOLT_INDEX == byChIndex)
		{
			ADC12_CLIST3 |= 0x0B;
		}
		else
		{
			DBG_Assert(1, 0, byChIndex);
		}
	}
	else
	{
		DBG_Assert(1, 0, byId);
	}
}

void ADC_ISRAck(uint8 byId)
{
	if(ADC_MISC_INDEX == byId)
	{
		ADC_ClearConvertionFinishedFlag(byId);
	}
}

//Manual start ADC to sample
void ADC_StartConversion(uint8 byId)
{
	if(ADC_DMA_INDEX == byId)
	{        
		ADC12_CTRL2 |= BIT13;
	}
	else if(byId == ADC_MISC_INDEX)
	{
		ADC12_CTRL1 |= BIT13;
	}
}

//Return result register byte address
uint32 ADC_GetResultRegAddr(uint8 byId)
{
	uint32 addr = 0;
	if(ADC_DMA_INDEX == byId)
	{
		addr = (uint32)&ADC12_RSLT8;
		addr *= 2;
	}
	else
	{
		DBG_Assert(1, 0, byId);
	}
	return addr;
}

void ADC_PowerDown(void)
{
	ADC12_PWR |= (ADC12_PWR_PD1 | ADC12_PWR_PD0);
}

void ADC_PowerUp(void)
{
	ADC12_PWR &= ~(ADC12_PWR_PD1 | ADC12_PWR_PD0);
	// Wait for power-up to complete
	while (( ADC12_PWR & ( BIT10 | BIT11)) != 0u) {}
}
