/*******************************************************************************
*
* Copyright 2012-2016 Freescale Semiconductor, Inc.
* Copyright 2017~2018 NXP.
* All rights reserved.
*******************************************************************************/

#ifndef __SYSTEMTIMERS_H__
#define __SYSTEMTIMERS_H__

void ST_TimerUpdate(void);
uint16 ST_GetTimerTick(void);
void ST_AddTickTimer(uint16 wTicks);
uint16 ST_GetElapasedTime(uint16 wLastTick);
void ST_WaitMs(uint16 wNumMs);

#endif

