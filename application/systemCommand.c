/*******************************************************************************
*
* Copyright 2012-2015 Freescale Semiconductor, Inc.
* Copyright 2017~2018 NXP.
* All rights reserved.
*******************************************************************************/

#include "defines.h"

#include "appCfg.h"
#include "wct_lib.h"

#include "NVM.h"

#include "systemCommand.h"

CMD_TYPE_E gCMD_eCommand;

void CMD_Handler(void)
{
	if(WCT_CMD_NONE != gCMD_eCommand)
	{
		switch(gCMD_eCommand)
		{
		case WCT_CMD_STOP_LIB:   //1
			WCT_Stop();
			break;
			
		case WCT_CMD_START_LIB:  //2
			WCT_Init();
			break;
			
		case WCT_CMD_RE_CALIBRATION:  //3
		    //For debug purpose
			//This command should be issued after WCT STOP
			NVM_ReCalibration();
			break;
		}
		gCMD_eCommand = WCT_CMD_NONE;
	}
}

//Should not be called in ISR, which leads critical status for gCMD_eCommand
void CMD_Notify(CMD_TYPE_E eCmd)
{
	gCMD_eCommand = eCmd;
}

