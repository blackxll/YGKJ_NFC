/*******************************************************************************
*
* Copyright 2012-2014 Freescale Semiconductor, Inc.
* Copyright 2017~2018 NXP.
* All rights reserved.
*******************************************************************************/

#ifndef __WDG_H__
#define __WDG_H__


void WDG_Init(void);
void WDG_Kick(void);
void WDG_Enable(uint8 byIsEn);

#endif // __WDG_H__
