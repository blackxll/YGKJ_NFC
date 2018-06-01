/*******************************************************************************
*
* Copyright 2012-2015 Freescale Semiconductor, Inc.
* Copyright 2017~2018 NXP.
* All rights reserved.
*******************************************************************************/

#ifndef __SYSTEM_COMMAND_H__
#define __SYSTEM_COMMAND_H__

typedef enum
{
	WCT_CMD_NONE           = 0,
	WCT_CMD_STOP_LIB       = 1,
	WCT_CMD_START_LIB      = 2,
	WCT_CMD_RE_CALIBRATION = 3
}CMD_TYPE_E;

void CMD_Handler(void);
void CMD_Notify(CMD_TYPE_E eCmd);

#endif
