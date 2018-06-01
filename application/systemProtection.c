/*******************************************************************************
*
* Copyright 2012-2014 Freescale Semiconductor, Inc.
* Copyright 2017~2018 NXP.
* All rights reserved.
*******************************************************************************/

#include "defines.h"

#include "appCfg.h"
#include "wct_lib.h"
#include "NVM.h"

#include "systemCommand.h"

#include "wct_LibParams.h"

#include "hal.h"

#include "systemProtection.h"

typedef enum
{
	PROT_ERROR_INPUT_CURRENT = 0,
	PROT_ERROR_VRAIL,
	PROT_ERROR_COIL_CURRENT,
	PROT_ERROR_INPUT_POWER,
	PROT_ERROR_VBAT_VOL_HIGH,
	PROT_ERROR_VBAT_VOL_LOW,
	PROT_ERROR_TEMP,
	PROT_ERROR_SAFE_DIGITAL_PING
}PROT_ERROR_E;

typedef struct
{
    uint16 wMaxRailVol;
    uint16 wMaxBatteryVol;
    uint16 wMinBatteryVol;
    uint16 wBatteryHystVol;
    uint16 wMaxInputCurrent;
    uint16 wMaxCoilCurrent;
    uint16 wMaxInputPowerDuringNonXfer;
    uint16 wMaxInternalLoss;
    uint16 wMinInternalLoss;
    
    sint16 swTemperatureThreshold;
    uint16 wTemperatureHyst;
    uint16 wSafeDigitalPingInputCurrentThreshold;
    uint16 wDurationToTriggerInputPowerNonXferFault;
    uint16 wDurationToTriggerRailFault;
    uint16 wDurationToTriggerBatteryVolFault;
    uint16 wDurationToTriggerInputCurrentFault;
    uint16 wDurationToTriggerTemperatureFault;
    uint32 dwMaxPower;
}PROT_PARAMS_T;

typedef struct
{
	uint16 wLastErrStatus;
	uint16 wInputCurrentFaultAccumMs;
	uint16 wVrailFaultAccumMs;
	uint16 wInputPowerAccumMs;
	uint16 wInputPower;
	uint16 wInputCurrent;
	uint16 wRailVoltage;
	uint16 wCoilCurrent;
}PROT_RUNTIME_CTRL_T;

typedef struct
{
	uint16 wLastErrStatus;
	uint16 wBatteryVolFaultAccumMs;
	uint16 wTemperatureAccumMs;
	uint16 wBatteryVol;
	sint16 swTopTemperature;
}PROT_BOARD_CTRL_T;


#define PROT_ERROR(x)  (1U<<x)

PROT_PARAMS_T gPROT_Params;

PROT_RUNTIME_CTRL_T gPROT_RunTimeCtrl[NUM_DEVICES];

PROT_BOARD_CTRL_T gPROT_BoardCtrl;

//Protection parameter setting, some parameters from NVM, support runtime change
//Note: battery voltage means Tx board input voltage, to distinguish with rail voltage
void PROT_Init(NVM_PARAMS *pNvmParam);

void PROT_Init(NVM_PARAMS *pNvmParam)
{
	uint8 powerClass = gWCT_Params.byTxPowerClass;
	gPROT_Params.wMaxRailVol       = pNvmParam->OpParams.OpStateParams.wOverRailVoltageThreshold;  //mV
	gPROT_Params.wMaxBatteryVol    = pNvmParam->OpParams.OpStateParams.wOverInputVoltageThreshold;  //mV
	gPROT_Params.wMinBatteryVol    =  pNvmParam->OpParams.OpStateParams.wUnderInputVoltageThreshold;  //mV
	gPROT_Params.wBatteryHystVol   =  1000U;  //mV
	gPROT_Params.wMaxInputCurrent  =  pNvmParam->OpParams.OpStateParams.wOverInputCurrentThreshold;  //mA
	gPROT_Params.wMaxCoilCurrent   =  pNvmParam->OpParams.OpStateParams.wOverCoilCurrentThreshold;  //mA, RMS
	gPROT_Params.wMaxInputPowerDuringNonXfer = 3000U;  //mW
	gPROT_Params.wMinInternalLoss  = 5000;	
	//Qi 1.2.4, MP.TX.SYSCTRL.POWXFER.LDSTP.TC1 case may absorb maximum 25W, 
	//so for Qi compliance test, set wMaxInternalLoss = 20W, about 1.4 times of Tx maximum power(15W).
	//For real case, recommend set wMaxInternalLoss = 11W, for the case that Tx maimum power=15W.
	gPROT_Params.wMaxInternalLoss  = 20000;
    
	gPROT_Params.swTemperatureThreshold                     = pNvmParam->OpParams.OpStateParams.swOverTemperatureThreshold;  //Celsius
	gPROT_Params.wTemperatureHyst                           = 10;  //Celsius
	gPROT_Params.wSafeDigitalPingInputCurrentThreshold		= 200; //mA
	gPROT_Params.wDurationToTriggerInputPowerNonXferFault   = 10;  //ms
	gPROT_Params.wDurationToTriggerRailFault                = 5;   //ms
	gPROT_Params.wDurationToTriggerBatteryVolFault          = 100; //ms
	gPROT_Params.wDurationToTriggerInputCurrentFault        = 5;   //ms
	gPROT_Params.wDurationToTriggerTemperatureFault         = 2000;  //ms
	gPROT_Params.dwMaxPower = gWCT_Params.wTxMaxPowerHalfWatts * 500;
	while(powerClass--)
	{
		gPROT_Params.dwMaxPower *= 10;
	}
}

//WCT state machine call this function to check if runtime parameters are under safe level.
boolean PROT_CheckRunTimeParams(uint8 byDeviceId, uint8 byCoilId, TX_CHARGING_STATUS eState, uint16 wGuaranteedPower, uint16 wTimePassedMs)
{
	boolean ret = FALSE;
	PROT_RUNTIME_CTRL_T *pProCtrl = &gPROT_RunTimeCtrl[byDeviceId];
	
	uint16 wStatus = 0;
	
    uint16 wCoilCurrent  = HAL_GetCoilCurrent(byDeviceId, byCoilId);
    uint16 wInputCurrent = HAL_GetInputCurrent(byDeviceId);
    uint16 wVrail        = HAL_GetRailVoltage(byDeviceId);
    uint32 dwInputPower;
    
    dwInputPower  = wInputCurrent;
    dwInputPower *= wVrail;
    dwInputPower /= 1000;
    
    //Save the runtime parameters to gloable variables
    pProCtrl->wInputPower    = dwInputPower;
    pProCtrl->wInputCurrent  = wInputCurrent;
    pProCtrl->wRailVoltage   = wVrail;
    pProCtrl->wCoilCurrent   = wCoilCurrent;
    
    if(wInputCurrent > gPROT_Params.wMaxInputCurrent)
    {
    	pProCtrl->wInputCurrentFaultAccumMs += wTimePassedMs;
    	if(pProCtrl->wInputCurrentFaultAccumMs > gPROT_Params.wDurationToTriggerInputCurrentFault)
    	{
    		wStatus |= PROT_ERROR(PROT_ERROR_INPUT_CURRENT);
    		pProCtrl->wInputCurrentFaultAccumMs = 0;
    		ret = TRUE;
    	}
    }
    else
    {
    	pProCtrl->wInputCurrentFaultAccumMs = 0;
    }
    
    if(wCoilCurrent > gPROT_Params.wMaxCoilCurrent)
	{
    	ret = TRUE;
    	wStatus |= PROT_ERROR(PROT_ERROR_COIL_CURRENT);
	}
    
    if(wVrail > gPROT_Params.wMaxRailVol)
   	{
    	pProCtrl->wVrailFaultAccumMs += wTimePassedMs;
		if(pProCtrl->wVrailFaultAccumMs > gPROT_Params.wDurationToTriggerRailFault)
		{
			wStatus |= PROT_ERROR(PROT_ERROR_VRAIL);
			pProCtrl->wVrailFaultAccumMs = 0;
			ret = TRUE;
		}
   	}
    else
    {
    	pProCtrl->wVrailFaultAccumMs = 0;
    }

    if((TX_CALIBRATION == eState) || (TX_POWER_TRANSFER == eState) || (TX_RENEGOTIATION == eState))
    {
        //For these phases, receiver connects its loading, check the maximum input power
    	uint32 dwInputPowerLimit;
    	
    	dwInputPowerLimit  = gPROT_Params.wMaxInternalLoss - gPROT_Params.wMinInternalLoss;
    	dwInputPowerLimit *= wGuaranteedPower;
    	dwInputPowerLimit /= gPROT_Params.dwMaxPower;
    	
    	dwInputPowerLimit += wGuaranteedPower;
    	dwInputPowerLimit += gPROT_Params.wMinInternalLoss;

    	if(dwInputPower >  dwInputPowerLimit)
    	{
    		wStatus |= PROT_ERROR(PROT_ERROR_INPUT_POWER);	
    		ret = TRUE;
    	}
    	pProCtrl->wInputPowerAccumMs = 0;
    }
    else if((dwInputPower > gPROT_Params.wMaxInputPowerDuringNonXfer) && (eState != TX_DIGITAL_PING))
	{
        /*
         * (eState != TX_DIGITAL_PING) is a workaround, since Qi compliance test equipment may absorb 
         * a little more power sometimes at the start of digital ping.
         */
        
        // Rx doesn't connect its loading yet, limit the Tx inverter input power for safe
		pProCtrl->wInputPowerAccumMs += wTimePassedMs;
		if(pProCtrl->wInputPowerAccumMs > gPROT_Params.wDurationToTriggerInputPowerNonXferFault)
		{
			wStatus |= PROT_ERROR(PROT_ERROR_INPUT_POWER);
			pProCtrl->wInputPowerAccumMs = 0;
			ret = TRUE;
		}
	}
    else
    {
    	pProCtrl->wInputPowerAccumMs = 0;
    }
    
    if(HAL_CheckDCDCError(byDeviceId))
    {
        //this error will be automatically recovered, not record the error status
        ret = TRUE;
    }
    
    if(wStatus)
    {
        //record error status for better observation, only for debug purpose.
    	pProCtrl->wLastErrStatus |= wStatus;
    }

    return ret;
}

uint16 PROT_GetRRQDFittingInputCurrent(uint8 byDeviceId, uint8 byCoilId, uint16 wRailVoltage, uint32 dwFreq)
{
	CURRENT_FITTING_PARAMS *pInCurFrmRailVol   = NVM_GetRRQDInputCurrParams(byDeviceId, byCoilId);
	sint32 sdwTemp;
	uint32  dwTemp;
	
	//calibration algorithm
	//   InputCurrent = (slope*(Vrail/10)+Offset)>>norm
	// =>InputCurrent = ((slope*Vrail+Offset*10)>>norm)/10
	sdwTemp = wRailVoltage;
	sdwTemp *= pInCurFrmRailVol->sdwSlope;
	sdwTemp += (pInCurFrmRailVol->sdwOffset*10);
	if(sdwTemp < 0)
	{
	    //in case of error
		sdwTemp = 0;
	}
	dwTemp = sdwTemp;
	dwTemp >>= pInCurFrmRailVol->wNorm;
	return (uint16)(dwTemp/10);
}

uint16 PROT_GetRRQDFittingCoilCurrent(uint8 byDeviceId, uint8 byCoilId, uint16 wRailVoltage, uint32 dwFreq)
{
	CURRENT_FITTING_PARAMS *pCoilCurFrmRailVol = NVM_GetRRQDCoilCurrParams(byDeviceId, byCoilId);
	sint32 sdwTemp;
	uint32  dwTemp;
	
    //calibration algorithm
    //   CoilCurrent = (slope*(Vrail/10)+Offset)>>norm
    // =>CoilCurrent = ((slope*Vrail+Offset*10)>>norm)/10
	
	sdwTemp = wRailVoltage;
	sdwTemp *= pCoilCurFrmRailVol->sdwSlope;
	sdwTemp += (pCoilCurFrmRailVol->sdwOffset*10);
	if(sdwTemp < 0)
	{
	    //in case of error
		sdwTemp = 0;
	}
	dwTemp = sdwTemp;
	dwTemp >>= pCoilCurFrmRailVol->wNorm;
	return (uint16)(dwTemp/10);
}

/*
 * Check the temperature and input voltage
 * stops WCT state machine if parameters exceed threshold
 * Restart WCT state machine when parameter back to normal
 */
boolean PROT_CheckBoardParams(uint16 wTimePassedMs)
{
	uint8 byDeviceId;
	sint16 swTopTemp = 0, swTemp;
	uint16 wBatteryVol = HAL_GetBatteryVoltage();
	uint16 wPreStatus;

	//Get the board top temperature
	for(byDeviceId=0; byDeviceId<NUM_DEVICES; byDeviceId++)
	{
		swTemp = HAL_GetTopTemperature(byDeviceId);
		if(swTemp > swTopTemp)
		{
			swTopTemp = swTemp;
		}
	}
	
	//save the parameter
	gPROT_BoardCtrl.wBatteryVol = wBatteryVol;
	gPROT_BoardCtrl.swTopTemperature = swTopTemp;
	
	wPreStatus = gPROT_BoardCtrl.wLastErrStatus;
	
	if(gPROT_BoardCtrl.wLastErrStatus & PROT_ERROR(PROT_ERROR_TEMP))
	{
		if(swTopTemp < (gPROT_Params.swTemperatureThreshold-gPROT_Params.wTemperatureHyst))
		{
			gPROT_BoardCtrl.wTemperatureAccumMs += wTimePassedMs;
			if(gPROT_BoardCtrl.wTemperatureAccumMs > gPROT_Params.wDurationToTriggerTemperatureFault)
			{
			    //temperature got normal
				gPROT_BoardCtrl.wLastErrStatus &= ~PROT_ERROR(PROT_ERROR_TEMP);
				gPROT_BoardCtrl.wTemperatureAccumMs = 0;
			}
		}
		else
		{
			gPROT_BoardCtrl.wTemperatureAccumMs = 0;
		}
	}
	else if(swTopTemp > gPROT_Params.swTemperatureThreshold)
	{
		gPROT_BoardCtrl.wTemperatureAccumMs += wTimePassedMs;
		if(gPROT_BoardCtrl.wTemperatureAccumMs > gPROT_Params.wDurationToTriggerTemperatureFault)
		{
		    //Temperature is high, stop WCT state machine
			CMD_Notify(WCT_CMD_STOP_LIB);
			gPROT_BoardCtrl.wLastErrStatus |= PROT_ERROR(PROT_ERROR_TEMP);
			gPROT_BoardCtrl.wTemperatureAccumMs = 0;
		}
	}
	else
	{
		gPROT_BoardCtrl.wTemperatureAccumMs = 0;
	}
	
	
	if(gPROT_BoardCtrl.wLastErrStatus & PROT_ERROR(PROT_ERROR_VBAT_VOL_HIGH))
	{
		if(wBatteryVol < (gPROT_Params.wMaxBatteryVol-gPROT_Params.wBatteryHystVol))
		{
			gPROT_BoardCtrl.wBatteryVolFaultAccumMs += wTimePassedMs;
			if(gPROT_BoardCtrl.wBatteryVolFaultAccumMs > gPROT_Params.wDurationToTriggerBatteryVolFault)
			{
			    //board input voltage get normal
				gPROT_BoardCtrl.wLastErrStatus &= ~PROT_ERROR(PROT_ERROR_VBAT_VOL_HIGH);
				gPROT_BoardCtrl.wBatteryVolFaultAccumMs = 0;
			}
		}
		else
		{
			gPROT_BoardCtrl.wBatteryVolFaultAccumMs = 0;
		}
	}
	else if(gPROT_BoardCtrl.wLastErrStatus & PROT_ERROR(PROT_ERROR_VBAT_VOL_LOW))
	{
		if(wBatteryVol > (gPROT_Params.wMinBatteryVol+gPROT_Params.wBatteryHystVol))
		{
			gPROT_BoardCtrl.wBatteryVolFaultAccumMs += wTimePassedMs;
			if(gPROT_BoardCtrl.wBatteryVolFaultAccumMs > gPROT_Params.wDurationToTriggerBatteryVolFault)
			{
			    //board input voltage get normal
				gPROT_BoardCtrl.wLastErrStatus &= ~PROT_ERROR(PROT_ERROR_VBAT_VOL_LOW);
				gPROT_BoardCtrl.wBatteryVolFaultAccumMs = 0;
			}
		}
		else
		{
			gPROT_BoardCtrl.wBatteryVolFaultAccumMs = 0;
		}
	}
	else if(wBatteryVol > gPROT_Params.wMaxBatteryVol)
	{
		gPROT_BoardCtrl.wBatteryVolFaultAccumMs += wTimePassedMs;
		if(gPROT_BoardCtrl.wBatteryVolFaultAccumMs > gPROT_Params.wDurationToTriggerBatteryVolFault)
		{
		    //high input voltage, stop WCT state machine
			CMD_Notify(WCT_CMD_STOP_LIB);
			gPROT_BoardCtrl.wLastErrStatus |= PROT_ERROR(PROT_ERROR_VBAT_VOL_HIGH);
			gPROT_BoardCtrl.wBatteryVolFaultAccumMs = 0;
		}
	}
	else if(wBatteryVol < gPROT_Params.wMinBatteryVol)
	{
		gPROT_BoardCtrl.wBatteryVolFaultAccumMs += wTimePassedMs;
		if(gPROT_BoardCtrl.wBatteryVolFaultAccumMs > gPROT_Params.wDurationToTriggerBatteryVolFault)
		{
		    //low input voltage, stop WCT state machine
			CMD_Notify(WCT_CMD_STOP_LIB);
			gPROT_BoardCtrl.wLastErrStatus |= PROT_ERROR(PROT_ERROR_VBAT_VOL_LOW);
			gPROT_BoardCtrl.wBatteryVolFaultAccumMs = 0;
		}
	}
	else
	{
		gPROT_BoardCtrl.wBatteryVolFaultAccumMs = 0;
	}
	
	if( (0 != (wPreStatus & (PROT_ERROR(PROT_ERROR_TEMP)|   //previous status got error
	                         PROT_ERROR(PROT_ERROR_VBAT_VOL_LOW)|
	                         PROT_ERROR(PROT_ERROR_VBAT_VOL_HIGH)))) &&	    
	    (0 == (gPROT_BoardCtrl.wLastErrStatus & (PROT_ERROR(PROT_ERROR_TEMP)|  //latest status is OK
                                                 PROT_ERROR(PROT_ERROR_VBAT_VOL_LOW)|
                                                 PROT_ERROR(PROT_ERROR_VBAT_VOL_HIGH))))
      )
	{
	    //seems no blocking, restart WCT state machine
		CMD_Notify(WCT_CMD_START_LIB);
	}
	
    return FALSE;
}

boolean PROT_SafeDigitalPingParamCheck(uint8 byDeviceId)
{
	boolean ret = TRUE;
	uint16 wInputCurrent;
	
	//HAL layer get the ADC sample each ms, force the ADC sample the latest analog inputs.
	HAL_MiscADCForceSample(byDeviceId);
	wInputCurrent = HAL_GetInputCurrent(byDeviceId);

	if(wInputCurrent > gPROT_Params.wSafeDigitalPingInputCurrentThreshold)
	{
	    //Input current exceed the threshold, abnormal case, return FALSE
	    //record the errors for better observation, only for debug purpose
	    gPROT_RunTimeCtrl[byDeviceId].wLastErrStatus |= PROT_ERROR(PROT_ERROR_SAFE_DIGITAL_PING);
		ret = FALSE;
	}

	return ret;
}
