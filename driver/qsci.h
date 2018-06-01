/*******************************************************************************
 *
 * Copyright 2012-2014 Freescale Semiconductor, Inc.
 * Copyright 2017~2018 NXP.
 * All rights reserved.
 *******************************************************************************/

#ifndef __QSCI_H__
#define __QSCI_H__

#define QSCI_CONSOLE_INDEX     1	//QSCI1
#define QSCI_FREEMASTER_INDEX  0	//QSCI0

void QSCI0_Init(uint32 baudrate);
uint8 QSCI0_PutChar(uint8 byChar);
void QSCI1_Init(uint32 baudrate);
uint8 QSCI1_PutChar(uint8 byChar);

void QSCI_ISRAck(uint8 byId);
uint8 QSCI_CheckTxEmptyFlag(uint8 byId);
void QSCI_PutChar(uint8 byId, uint8 byChar);
void QSCI_EnableTXEmptyInterrupt(uint8 byId, uint8 byIsEn);
#endif
