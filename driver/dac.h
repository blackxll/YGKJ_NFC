/*******************************************************************************
*
* Copyright 2012-2014 Freescale Semiconductor, Inc.
* Copyright 2017~2018 NXP.
* All rights reserved.
*******************************************************************************/

#ifndef __DAC_H__
#define __DAC_H__

#define DAC_Q_MVL_INDEX     0

void DAC_Init(void);
void DAC_SetVoltage(uint8 byId, uint16 wVoltage);
void DAC_PowerDown(void);
void DAC_PowerUp(void);

#endif // __DAC_H__
