/*******************************************************************************
 *
 * Copyright 2012-2014 Freescale Semiconductor, Inc.
 * Copyright 2017~2018 NXP.
 * All rights reserved.
 *******************************************************************************/

#ifndef __DISPLAY_H__
#define __DISPLAY_H__

typedef enum
{
	DISP_MODE_ON = 0,
	DISP_MODE_OFF,
	DISP_MODE_BLINK,
	DISP_MODE_TOG,
	DISP_MODE_DIM,
	DISP_MODE_MAX
}DISP_MODE_E;

void DISP_Init(void);
void DISP_SetMode(uint8 byDeviceId, HAL_LED_INDEX_E byLedId, DISP_MODE_E eMode, uint32 dwInterval);
void DISP_Handler(uint16 wTimePassedMs);

#endif
