/*******************************************************************************
*
* Copyright 2012-2016 Freescale Semiconductor, Inc.
* Copyright 2017~2018 NXP.
* All rights reserved.
*******************************************************************************/
//----------------------
// APPLICATION INCLUDES
//----------------------

#include "defines.h"

#include "appCfg.h"
#include "wct_lib.h"

#include "hal.h"

#include "systemTimers.h"
#include "systemDisplay.h"
#include "systemCommand.h"
#include "systemProtection.h"
#include "systemCallback.h"

#include "wct_debug.h"
#include "systemTask.h"


#define NUM_1MS_COUNTS_FOR_10MS         10U
#define NUM_10MS_COUNTS_FOR_100MS       10U
#define NUM_100MS_COUNTS_FOR_1SEC       10U
#define NUM_1SEC_COUNTS_FOR_1MIN        60U

typedef struct
{
	uint16 w1msCount;
	uint16 w10msCount;
	uint16 w100msCount;
	uint16 w1secCount;
	uint16 w1minCount;
} TIMER_DATA;

APP_ERROR_TYPE gApp_error = APP_SUCCESS;

static void AppTask_1msHandle(uint16 wNumTicks)
{
	uint16 wLowPowerModeTimeMs;
	wLowPowerModeTimeMs = WCT_Run(wNumTicks);	
	CMD_Handler();
	if(LOW_POWER_MODE_TIME_INFINITE == wLowPowerModeTimeMs)	
	{
		//WCT library OFF, don't enter low power mode
	}
	else
	{
		HAL_EnterLowPowerMode(wLowPowerModeTimeMs);
	}
}

static void AppTask_10msHandle(uint16 wNumTicks)
{
#if FREEMASTER_SUPPORTED
	WBG_DebugProcess();
#endif
	PROT_CheckBoardParams(wNumTicks);
	DISP_Handler(wNumTicks);
}

static void AppTask_100msHandle(void)
{
	HAL_KickWatchDog();
}

static void AppTask_1secHandler(void)
{

}

static void AppTask_1minHandler(void)
{
	
}

void App_SetErr(APP_ERROR_TYPE err)
{
    gApp_error = err;
}

APP_ERROR_TYPE App_GetErr(void)
{
    return gApp_error;
}

void AppTask_Poll(void)
{
    static TIMER_DATA TimerData;
    static uint16 wAppLastTimerTicks;
    
	uint16 wSecondsToAdd;
	uint16 wNumTicks;
	
	wNumTicks = ST_GetElapasedTime(wAppLastTimerTicks);
	wAppLastTimerTicks += wNumTicks;
	wAppLastTimerTicks &= 0xffffu;
	
	if (wNumTicks)
	{
		// 1 ms Tick
		if (1U <= wNumTicks)
		{			
			TimerData.w1msCount += wNumTicks;
			
			AppTask_1msHandle(wNumTicks);
			
			// 10 MS Tick
			if (NUM_1MS_COUNTS_FOR_10MS <= TimerData.w1msCount)
			{
				wNumTicks = TimerData.w1msCount;
				TimerData.w10msCount += (TimerData.w1msCount / NUM_1MS_COUNTS_FOR_10MS);
				TimerData.w1msCount %= NUM_1MS_COUNTS_FOR_10MS;
				
				AppTask_10msHandle(wNumTicks);
				
				// 100 MS Tick
				if (NUM_10MS_COUNTS_FOR_100MS <= TimerData.w10msCount)
				{
					TimerData.w100msCount += (TimerData.w10msCount / NUM_10MS_COUNTS_FOR_100MS);
					TimerData.w10msCount %= NUM_10MS_COUNTS_FOR_100MS;
					
					AppTask_100msHandle();
					
					// 1 SEC Tick
					if (NUM_100MS_COUNTS_FOR_1SEC <= TimerData.w100msCount)
					{
						wSecondsToAdd = (TimerData.w100msCount / NUM_100MS_COUNTS_FOR_1SEC);
						TimerData.w100msCount %= NUM_100MS_COUNTS_FOR_1SEC;
						TimerData.w1secCount += wSecondsToAdd;
						
						AppTask_1secHandler();
						
						if (NUM_1SEC_COUNTS_FOR_1MIN <= TimerData.w1secCount)
						{
							TimerData.w1minCount += (TimerData.w1secCount / NUM_1SEC_COUNTS_FOR_1MIN);
							TimerData.w1secCount %= NUM_1SEC_COUNTS_FOR_1MIN;
							
							AppTask_1minHandler();
						}
					}
				}
			}
		}
	}
}
