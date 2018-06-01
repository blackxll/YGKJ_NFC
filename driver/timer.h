/*******************************************************************************
*
* Copyright 2012-2014 Freescale Semiconductor, Inc.
* Copyright 2017~2018 NXP.
* All rights reserved.
*******************************************************************************/

#ifndef __TIMER_H__
#define __TIMER_H__

#define TMR_PWM_MAX_DUTY  10000UL

typedef enum
{
	TMR_MODE_COUNTER = 0,
	TMR_MODE_CAPTURE
}TMR_MODE_E;


#define TMR_DDM_FRQ_INDEX    0
#define TMR_DAC_INDEX        1
#define TMR_TICK_INDEX       2

#define TMR_COMM_REF_INDEX   4

#define TMR_D_DCDC_INDEX	 5

#define TMR_LOW_POWER_INDEX  6

void TMR_Init(void);
void TMR_SetCompareValue(uint8 byId, uint16 wCompDuty);
void TMR_Enable(uint8 byId, uint8 byIsEn);
void TMR_ISRAck(uint8 byId);
uint32 TMR_GetCounterValue(uint8 byId);
void TMR_SetMode(uint8 byId, TMR_MODE_E mode);
boolean TMR_CheckCompare(uint8 byId);
boolean TMR_CheckCapture(uint8 byId);
boolean TMR_ClearCompareFlag(uint8 byId);
boolean TMR_ClearCaptureFlag(uint8 byId);

uint16 TMR_WaitUs(uint16 wWaitUs);
void TMR_EnableInStopMode(uint8 byId, uint8 byIsEn);

#endif
