/*******************************************************************************
*
* Copyright 2012-2014 Freescale Semiconductor, Inc.
* Copyright 2017~2018 NXP.
* All rights reserved.
*******************************************************************************/

#ifndef __FLASH_H__
#define __FLASH_H__

boolean FLASH_SectorDataErase(uint32 dwAddr);
boolean FLASH_WriteData(uint32 dwAddr, uint32 dwData);
void FLASH_ReadData(uint8 * pbyBuf, NVM_BYTE_POINTER pbyAddr, uint16 wLen);
void FLASH_Init(void);

#endif
