/*******************************************************************************
*
* Copyright 2012-2014 Freescale Semiconductor, Inc.
* Copyright 2017~2018 NXP.
* All rights reserved.
*******************************************************************************/


#include "defines.h"
#include "cpu.h"
#include "systemDebug.h"
#include "qsci.h"

void QSCI0_Init(uint32 baudrate)
{
	uint32 dwTemp;
	uint16 wTemp;
	uint16 SBR, Frac_SBR;
	
	// Hold QSCI0 in standby state during configuration
	QSCI0_CTRL1 |= QSCI0_CTRL1_RWU;
	
	// Configure baud rate
	dwTemp = CPU_IPBUS_CLK / baudrate;
	SBR = (uint16) (dwTemp >> 4);
	dwTemp >>= 1;
	dwTemp -= SBR << 3;
	Frac_SBR = (uint16)dwTemp;
	wTemp = (SBR & 0x1FFF) << 3;
	wTemp |= (Frac_SBR & 0x0007);
	QSCI0_RATE = wTemp;
	
	//Set the SCI control register
	//8bit data, 1 stop bit, no parity bit
	QSCI0_CTRL1 = 0x000CU;
	QSCI0_CTRL2 = 0x1020U;
}

uint8 QSCI0_PutChar(uint8 byChar)
{
	uint8 ret = 0;
	if( !(QSCI0_STAT & QSCI0_STAT_TDRE) )
	{
		ret = 1; 
	}
	else
	{
		QSCI0_DATA = byChar;
	}
	
	return ret;
}

void QSCI1_Init(uint32 baudrate)
{
	uint32 dwTemp;
	uint16 wTemp;
	uint16 SBR, Frac_SBR;
	
	// Hold QSCI1 in standby state during configuration
	QSCI1_CTRL1 |= QSCI1_CTRL1_RWU;
	
	// Configure baud rate
	dwTemp = CPU_IPBUS_CLK / baudrate;
	SBR = (uint16) (dwTemp >> 4);
	dwTemp >>= 1;
	dwTemp -= SBR << 3;
	Frac_SBR = (uint16)dwTemp;
	wTemp = (SBR & 0x1FFF) << 3;
	wTemp |= (Frac_SBR & 0x0007);
	QSCI1_RATE = wTemp;
	
	//Set the SCI control register
	//8bit data, 1 stop bit, no parity bit
	QSCI1_CTRL1 = 0x000CU;
	QSCI1_CTRL2 = 0x1020U;
}

uint8 QSCI1_PutChar(uint8 byChar)
{
	uint8 ret = 0;
	if( !(QSCI1_STAT & QSCI1_STAT_TDRE) )
	{
		ret = 1; 
	}
	else
	{
		QSCI1_DATA = byChar;
	}
	
	return ret;
}

void QSCI_ISRAck(uint8 byId)
{
	uint16 st;
	switch(byId)
	{
	case 0:
		st = QSCI0_STAT;
		break;
	case 1:
		st = QSCI1_STAT;
		break;
	default:
		DBG_Assert(1, 0, byId);
	}
}

uint8 QSCI_CheckTxEmptyFlag(uint8 byId)
{
	uint8 ret = 0;
	switch(byId)
	{
	case 0:
		if(QSCI0_STAT & QSCI0_STAT_TDRE)
		{
			ret = 1;
		}
		break;
	case 1:
		if(QSCI1_STAT & QSCI1_STAT_TDRE)
		{
			ret = 1;
		}
		break;
	default:
		DBG_Assert(1, 0, byId);
	}
	return ret;	
}

void QSCI_PutChar(uint8 byId, uint8 byChar)
{
	switch(byId)
	{
	case 0:
		QSCI0_PutChar(byChar);
		break;
	case 1:
		QSCI1_PutChar(byChar);
		break;
	default:
		DBG_Assert(1, 0, byId);
	}
}

void QSCI_EnableTXEmptyInterrupt(uint8 byId, uint8 byIsEn)
{
	switch(byId)
	{
	case 0:
		if(byIsEn)
		{
			QSCI0_CTRL1 |= QSCI0_CTRL1_TEIE;
		}
		else
		{
			QSCI0_CTRL1 &= ~QSCI0_CTRL1_TEIE;
		}
		break;
	case 1:
		if(byIsEn)
		{
			QSCI1_CTRL1 |= QSCI1_CTRL1_TEIE;
		}
		else
		{
			QSCI1_CTRL1 &= ~QSCI1_CTRL1_TEIE;
		}
		break;
	default:
		DBG_Assert(1, 0, byId);
	}
}

