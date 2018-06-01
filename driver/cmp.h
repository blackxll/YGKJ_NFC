/*******************************************************************************
*
* Copyright 2012-2016 Freescale Semiconductor, Inc.
* Copyright 2017~2018 NXP.
* All rights reserved.
*******************************************************************************/

#ifndef __CMP_H__
#define __CMP_H__

#define CMP_GENERAL_CHANNEL_12BIT_DAC_INDEX   4

#define CMP_FRQ_INDEX            2
#define CMP_FRQ_CHANNEL_RESONANCE_INDEX  0

#define CMP_D_DCDC_CURRENT_HIGH     0
#define CMP_D_DCDC_CURRENT_LOW      3

#define CMP_POLARITY_NEGATIVE        0
#define CMP_POLARITY_POSITIVE        1


void CMP_Enable(uint8 byId, uint8 byIsEn);
void CMP_SetChannel(uint8 byId, uint8 byPolarity, uint8 byChannel);
void CMP_Init(void);
boolean CMP_CheckRaisingFlag(uint8 byId);
void CMP_ClearRaisingFlag(uint8 byId);
void CMP_SetInternalDAC(uint8 byId, uint16 wDACLevel);

//To optimize the speed performance
#define CMP_Check_D_DCDC_Status()     ( (CMPA_SCR | CMPD_SCR)&0x04 ? 1U : 0U)
#define CMP_Clean_D_DCDC_Flag()       {CMPA_SCR |= 0x04;  CMPD_SCR |= 0x04;}

#endif // __CMP_H__
