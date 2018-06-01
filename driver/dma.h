/*******************************************************************************
 *
 * Copyright 2012-2014 Freescale Semiconductor, Inc.
 * Copyright 2017~2018 NXP.
 * All rights reserved.
 *******************************************************************************/
#ifndef __DMA_H__
#define __DMA_H__


#define DMA_MAX_TRANFER_LENGTH  0xFFFFFCUL

#define DMA_ADC_INDEX    0

typedef struct
{
	uint8  bySizePerRequest;
	uint8  bySrcAddrFixed;
	uint8  byDstAddrFixed;
	uint8  reserved;
	uint32 dwSrcAddr;
	uint32 dwDstAddr;
	uint32 dwSrcMod;
	uint32 dwDstMod;
	uint32 dwTransferLen;
}DMA_CFG_T;

void DMA_SetConfig(uint8 byId, DMA_CFG_T* pCfg);
void DMA_EnableRequest(uint8 byId, uint8 byIsEn);
void DMA_ISRAck(uint8 byId);
void DMA_Init(void);

#endif
