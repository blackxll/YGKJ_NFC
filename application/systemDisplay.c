/*******************************************************************************
 *
 * Copyright 2012-2014 Freescale Semiconductor, Inc.
 * Copyright 2017~2018 NXP.
 * All rights reserved.
 *******************************************************************************/
//------------------------------------------------------------------------------
//* Display operation:
//*
//* This module controls the display functionality based on settings retrieved
//* from a table stored in either ROM or NVM.  The module can be configured for
//* use in firmware builds supporting a supervisory micro, in which case the
//* function calls are coded in such a fashion that "local" control of the LEDs
//* is not conducted when the presence of a supervisor is detected.
//*
//* The programmer should configure the mapping file for appropriate operation,
//* including the specification of the LED control table location, and the number
//* of control options available.
//*
//------------------------------------------------------------------------------

//----------------------
// APPLICATION INCLUDES
//----------------------

#include "defines.h"

#include "appcfg.h"
#include "wct_lib.h"

#include "Hal.h"

#include "systemDebug.h"

#include "systemDisplay.h"


#define DISP_DIM_INTENSITY_MAX	HAL_DIM_MAX_INTENSITY
#define DISP_DIM_INTENSITY_MIN	(HAL_DIM_MAX_INTENSITY*5/100)

typedef enum
{
	DISP_DIM_UP = 0,
	DISP_DIM_DOWN
}DISP_DIM_STATUS_E;

typedef struct
{
	uint32				dwAccumTime;
	uint32				dwBlinkInterval;
	uint16				wDimUpStep;
	uint16				wDimDownStep;
	uint16				wDimIntensity;
	DISP_DIM_STATUS_E	DimDirection;
	DISP_MODE_E			mode;
}LED_CTRL;

LED_CTRL gLEDCtrl[NUM_DEVICES][HAL_LED_NUM];

void DISP_Init(void)
{
	uint8 byDeviceId, i;
	for(byDeviceId=0; byDeviceId<NUM_DEVICES; byDeviceId++)
	{
        for(i=0; i<HAL_LED_NUM; i++)
        {
            gLEDCtrl[byDeviceId][i].mode = DISP_MODE_OFF;
            DISP_SetMode(byDeviceId, (HAL_LED_INDEX_E)i, DISP_MODE_OFF, 0);
        }
	}
}

/*
 * For DIM mode, dwInterval is divided into two portion: low 16bit is dim up step each ms; high 16bit is dim down step each ms
 * For blink mode, dwInterval is blink period. blink is with 50% duty.
 * For other modes, dwInterval is useless.
 * 
 * Note: DIM mode needs HW support, normally needs PWM module engaged.
 */
void DISP_SetMode(uint8 byDeviceId, HAL_LED_INDEX_E byLedId, DISP_MODE_E eMode, uint32 dwInterval)
{
    LED_CTRL *pLed = &(gLEDCtrl[byDeviceId][byLedId]);
    
	DBG_Assert(NUM_DEVICES <= byDeviceId, 0, byDeviceId);
	DBG_Assert(byLedId >= HAL_LED_NUM, 0, byLedId);
	DBG_Assert(eMode >= DISP_MODE_MAX, 0, eMode);

	pLed->mode = eMode;
	switch(eMode)
	{
	case DISP_MODE_BLINK:
	    pLed->dwBlinkInterval = dwInterval;
	    pLed->dwAccumTime = (dwInterval >> 1);
		HAL_SetLED(byDeviceId, byLedId, HAL_LED_ON);
		break;
	case DISP_MODE_ON:
		HAL_SetLED(byDeviceId, byLedId, HAL_LED_ON);
		break;
	case DISP_MODE_OFF:
		HAL_SetLED(byDeviceId, byLedId, HAL_LED_OFF);
		break;
	case DISP_MODE_TOG:
		HAL_SetLED(byDeviceId, byLedId, HAL_LED_TOG);
		break;
	case DISP_MODE_DIM:
	    pLed->wDimUpStep = (uint16)(dwInterval & 0xFFFF);
	    pLed->wDimDownStep = (uint16)((dwInterval >> 16) & 0xFFFF);
	    pLed->DimDirection = DISP_DIM_UP;
	    pLed->wDimIntensity = DISP_DIM_INTENSITY_MIN;
		HAL_SetLEDDim(byDeviceId, byLedId, pLed->wDimIntensity);
		break;
	default:
		break;
	}
}

void DISP_Handler(uint16 wTimePassedMs)
{
	uint8 byDeviceId, i;
	for(byDeviceId=0; byDeviceId<NUM_DEVICES; byDeviceId++)
	{
        for(i=0; i<HAL_LED_NUM; i++)
        {
            LED_CTRL *pLed = &gLEDCtrl[byDeviceId][i];
            switch(pLed->mode)
            {
                case DISP_MODE_BLINK:
                    if(pLed->dwAccumTime >= wTimePassedMs)
                    {
                        pLed->dwAccumTime -= wTimePassedMs;
                    }
                    else
                    {
                        pLed->dwAccumTime = 0;
                    }
                    
                    if(pLed->dwAccumTime == 0)
                    {
                        pLed->dwAccumTime = (pLed->dwBlinkInterval >> 1);  //50% duty
                        HAL_SetLED(byDeviceId, (HAL_LED_INDEX_E)i, HAL_LED_TOG);
                    }
                    break;
                
                case DISP_MODE_DIM:
                    if(pLed->DimDirection == DISP_DIM_UP)
                    {
                        uint32 dwDimStep = pLed->wDimUpStep;
                        dwDimStep *= wTimePassedMs;
                        dwDimStep += pLed->wDimIntensity;
                        if(dwDimStep >= DISP_DIM_INTENSITY_MAX)
                        {
                            pLed->wDimIntensity = DISP_DIM_INTENSITY_MAX;
                            pLed->DimDirection = DISP_DIM_DOWN;
                        }
                        else
                        {
                            pLed->wDimIntensity = dwDimStep;
                        }
                    }
                    else if(pLed->DimDirection == DISP_DIM_DOWN)
                    {
                        uint32 dwDimStep = pLed->wDimDownStep;
                        dwDimStep *= wTimePassedMs;
                        
                        if( (DISP_DIM_INTENSITY_MIN + dwDimStep) >= pLed->wDimIntensity)
                        {
                            pLed->wDimIntensity = DISP_DIM_INTENSITY_MIN;
                            pLed->DimDirection = DISP_DIM_UP;
                        }
                        else
                        {
                            pLed->wDimIntensity -= dwDimStep;
                        }
                    }
                    HAL_SetLEDDim(byDeviceId, (HAL_LED_INDEX_E)i, pLed->wDimIntensity);
                    break;
                
                case DISP_MODE_ON:
                case DISP_MODE_OFF:
                case DISP_MODE_TOG:
                    break;
                
                default:
                    DBG_Assert(1, 0, pLed->mode);
                    break;
            }
        }
	}
}
