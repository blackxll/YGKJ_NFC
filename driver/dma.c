/*******************************************************************************
 *
 * Copyright 2012-2014 Freescale Semiconductor, Inc.
 * Copyright 2017~2018 NXP.
 * All rights reserved.
 *******************************************************************************/

#include "defines.h"
#include "cpu.h"

#include "systemDebug.h"

#include "dma.h"


//return the modulo value n, length = 2^(n+3)
static uint8 DMA_GetModValue(uint32 length)
{
	uint32 i, cnt, pos;
	uint8 ret = 0;
	
	cnt = 0;
	for(i=0; i<32; i++)
	{
		if(length & (1UL<<i))
		{
			cnt++;
			pos = i;
		}
	}
	
	//modulu value is not power of 2
	if(1 == cnt)
	{
		if( (pos >= 4) && (pos <= 18) )
		{
			ret = pos - 3;
		}
		else
		{
			DBG_Assert(1, 0, length);
		}
	}
	else if(cnt > 1)
	{
		DBG_Assert(1, 0, length);
	}
	return ret;
}

void DMA_SetConfig(uint8 byId, DMA_CFG_T* pCfg)
{
	uint32 dcr, sizeType;
	uint32 srcMod, dstMod;
	if(DMA_ADC_INDEX == byId)
	{
		DMA_DSR_BCR0 = BIT24;  //clear DONE flag
		dcr = DMA_DCR0 & 0xFF000000;
		DMA_DSR_BCR0 = pCfg->dwTransferLen & 0xFFFFFFUL;
		DMA_SAR0 = pCfg->dwSrcAddr;
		DMA_DAR0 = pCfg->dwDstAddr;
		
		if(0 == pCfg->bySrcAddrFixed)
		{
			dcr |= BIT22;
		}
		
		if(0 == pCfg->byDstAddrFixed)
		{
			dcr |= BIT19;
		}
		
		if(0x01 == pCfg->bySizePerRequest)
		{
			sizeType = 0x01;
		}
		else if(0x02 == pCfg->bySizePerRequest)
		{
			sizeType = 0x02;
		}
		else if(0x04 == pCfg->bySizePerRequest)
		{
			sizeType = 0x00;
		}
		else
		{
			DBG_Assert(1, 0, pCfg->bySizePerRequest);
		}
		
		dcr |= ((sizeType << 20) | (sizeType << 17));
		
		srcMod = DMA_GetModValue(pCfg->dwSrcMod);	
		dstMod = DMA_GetModValue(pCfg->dwDstMod);
		dcr |= ((dstMod << 8) | (srcMod << 12));
		
		DMA_DCR0 = dcr | BIT7;  //finished DMA setting
	}
	else
	{
		DBG_Assert(1, 0, byId);
	}
}

//enable/disable DMA request
void DMA_EnableRequest(uint8 byId, uint8 byIsEn)
{
	if(DMA_ADC_INDEX == byId)
	{
		if(byIsEn)
		{
			DMA_DCR0 |= BIT30;
		}
		else
		{
			DMA_DCR0 &= ~BIT30;
		}
	}
	else
	{
		DBG_Assert(1, 0, byId);
	}
}

void DMA_ISRAck(uint8 byId)
{
	if(DMA_ADC_INDEX == byId)
	{
		DMA_DSR_BCR0 |= DMA_DSR_BCR0_DONE; //Clear DMA0 interrupt flag
	}
	else
	{
		DBG_Assert(1, 0, byId);
	}
}

void DMA_Init(void)
{
	DMA_DCR0     = 0x00;
	DMA_DSR_BCR0 = BIT24;  //clear DMA done flag
	DMA_SAR0     = 0x00;
	DMA_DAR0     = 0x00;
	DMA_DCR0     = 0xA0000080;  //interrupt enabled, signal read/write per request. Clear ERQ bit when BCR exhausted
	DMA_REQC     = (BIT31 | (0x0DUL<<24));  //DMA CH0 request source, ADCB CC
}



