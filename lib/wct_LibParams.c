/*******************************************************************************
*
* Copyright 2012-2016 Freescale Semiconductor, Inc.
* Copyright 2017~2018 NXP.
* All rights reserved.
*******************************************************************************/
#include "defines.h"
#include "cpu.h"

#include "appCfg.h"
#include "wct_lib.h"
#include "NVM.h"

#include "wct_libParams.h"

//Library only use parameter located in gWCT_Params.
//NvmParams is an application variable, library doesn't touch it.
//Runtime gWCT_Params member change takes effect immediately for library
//Runtime NvmParams member change doesn't take effect immediately for library

WCT_PARAM_T gWCT_Params;
void PARAM_WctParamInit(NVM_PARAMS *pNvmParam);

void PARAM_WctParamInit(NVM_PARAMS *pNvmParam)
{
	uint8 i;
	//General Configure
    gWCT_Params.byDeviceNum                     = NUM_DEVICES;
    gWCT_Params.byCoilNumPerDevice              = NUM_COILS_PER_DEVICE;
	gWCT_Params.wManufacturerCode               = 0x28;
	
	gWCT_Params.byTxPowerClass                  = 0;
	gWCT_Params.wTxMaxPowerHalfWatts            = 30;
	gWCT_Params.wDeviceEnableFlag               = 1;
	gWCT_Params.uCtrlBit.bAnalogPingDisable     = 0;
	gWCT_Params.uCtrlBit.bFODEnable             = 1;
	gWCT_Params.uCtrlBit.bRRQDEnable            = 1;
	gWCT_Params.uCtrlBit.bSafeDPEnable          = 1;
	gWCT_Params.uCtrlBit.bMVLEnable             = 1;
	gWCT_Params.uCtrlBit.bFSKAppendHalfBit      = 0;
	gWCT_Params.uCtrlBit.bFastChargingEnable    = 1;
	gWCT_Params.uCtrlBit.bQfactorRetry          = 1;
	gWCT_Params.uCtrlBit.bMPLEnable             = 1;
	gWCT_Params.uCtrlBit.bActivePowerProtectionEnable = 1;
	gWCT_Params.uCtrlBit.bRechargeRetryLowPowerEnable = 0;
	gWCT_Params.uCtrlBit.byKeyfobAvoidanceDisableCoil = 0;
	gWCT_Params.uCtrlBit.bPowerDownForMaxVrail = 1;
    gWCT_Params.byChargingTryNumOnOneCoilThreshold  = 5;
    gWCT_Params.byRxRemovedConfirmDPNum         = 2;
	
    //Analog Ping Configuration
    gWCT_Params.byAnalogPingDetectAbsoluteValue     = 10;
    gWCT_Params.byAnalogPingDetectThresholdPercent  = pNvmParam->OpParams.OpStateParams.wAnalogPingCoilCurrentThreshold;
    
	//Timing Configuration
    gWCT_Params.wPingInterval                     = pNvmParam->OpParams.OpStateParams.wPingIntervalMs;
    gWCT_Params.wAnalogPingInterval               = 2;
    gWCT_Params.wDigitalPingInterval              = 30;
    gWCT_Params.wDigitalPingDuration              = pNvmParam->OpParams.OpStateParams.wPingPulseDurationTimeMs;
    gWCT_Params.wNextPacketTimeOut                = 24;
    gWCT_Params.wFirstPacketDuration              = 22;
    gWCT_Params.wMaxPacketDuration                = 160;
	gWCT_Params.wRPPTimeOut                       = 23000;
	gWCT_Params.wCEPTimeOut                       = 3000;
	gWCT_Params.wMsgHeaderTimeOut                 = 1500;
	gWCT_Params.wTimeForWaitNextNegotiationPacket = 220;
    gWCT_Params.wQPrepareInterval                 = gWCT_Params.wDigitalPingInterval;
    gWCT_Params.wQMeasureInterval                 = 1;
    gWCT_Params.wRailSetupTime                    = 10;
	gWCT_Params.wAnalogPingPowerSetupTime         = 20;
	gWCT_Params.wRailDischargeTime                = 10;
    gWCT_Params.wSendFSKDelay                     = 5;
    gWCT_Params.wCalibrationTimeout               = 10000;
    gWCT_Params.byDigitalPingRetryInterval        = 10*pNvmParam->OpParams.OpStateParams.byDigitalPingRetryIntervalSeconds;
    gWCT_Params.wDDMStartDelayTimeAfterCharging   = 3;
	gWCT_Params.wDDMRetryTimeout                  = 300;
    
    //Inverter Params
    for(i=0; i<NUM_COILS_PER_DEVICE; i++)
    {
    	gWCT_Params.wDefaultRailVoltageMv[i]     = pNvmParam->OpParams.OpStateParams.dwDefaultRailVoltageMv[i];
    }

    gWCT_Params.dwMaxFreq                        = pNvmParam->OpParams.OpStateParams.dwMaxFreq;
    gWCT_Params.dwMinFreq                        = pNvmParam->OpParams.OpStateParams.dwMinFreq;
    gWCT_Params.wMaxDuty                         = 50 * (LIB_MAX_DUTY_CYCLE/50);
    gWCT_Params.wMinDuty                         = 10 * (LIB_MAX_DUTY_CYCLE/50);
    gWCT_Params.wMaxPhase                        = 100 * (LIB_MAX_PHASE_SHIFT/100);
    gWCT_Params.wMinPhase                        = 26 * (LIB_MAX_PHASE_SHIFT/100);
    gWCT_Params.wMinRailVoltageMv                = pNvmParam->OpParams.OpStateParams.wMinRailVoltageMv;
    gWCT_Params.wMaxRailVoltageMv                = pNvmParam->OpParams.OpStateParams.wMaxRailVoltageMv;
    
    gWCT_Params.dwDigitalPingFreq                = pNvmParam->OpParams.OpStateParams.dwPingFrequency;
    gWCT_Params.wDigitalPingDuty                 = gWCT_Params.wMaxDuty;
    gWCT_Params.wDigitalPingPhase                = gWCT_Params.wMaxPhase;
    gWCT_Params.byDigitalPinglBridgeType         = 1;
    
    gWCT_Params.dwFobAvoidFreqency               = gWCT_Params.dwDigitalPingFreq + 5000;
    
    //Safe digital ping
	gWCT_Params.wSafeDigitalPingCheckTime		= 3;
	gWCT_Params.dwSafeDigitalPingFreq           = gWCT_Params.dwDigitalPingFreq;
	gWCT_Params.wSafeDigitalPingDuty            = gWCT_Params.wDigitalPingDuty;
	gWCT_Params.wSafeDigitalPingPhase           = gWCT_Params.wDigitalPingPhase/2;
	gWCT_Params.bySafeDigitalPinglBridgeType    = gWCT_Params.byDigitalPinglBridgeType;
    
    //FOD
    gWCT_Params.wPowerLossIndicationToPwrCessationMs  = pNvmParam->OpParams.PowerLossParams.dwPowerLossIndicationToPwrCessationMs;
    gWCT_Params.wLPPowerLossThresholdInOperationMode  = pNvmParam->OpParams.PowerLossParams.wPowerLossLPThreshold;
    gWCT_Params.wMPPowerLossThresholdInOperationMode  = pNvmParam->OpParams.PowerLossParams.wPowerLossMPThreshold;
    gWCT_Params.wPowerLossThresholdInCalibLightMode   = 1000;
    gWCT_Params.wPowerLossThresholdInCalibConnectMode = 2000;
    gWCT_Params.byNumFodTripsToIndication             = pNvmParam->OpParams.PowerLossParams.byNumFodTripsToIndication;
    gWCT_Params.byDefaultWindowSize                   = 1;
    gWCT_Params.byQfactorThresholdPercent             = 70;
    gWCT_Params.byQfactorAdjsutPercent                = 85;
    gWCT_Params.byEffiThresholdPercentForLegacyRx     = 40;
    gWCT_Params.wPowerLossThresholdForLegacyRx        = 2000;
    gWCT_Params.pFodExternalCheck                     = NULL;
    
    //Calibration
	gWCT_Params.sbyMaxErrorForLightMode        = 15;
	gWCT_Params.sbyMinErrorForLightMode        = -15;
	gWCT_Params.sbyMaxErrorForConnectMode      = 20;
	gWCT_Params.sbyMinErrorForConnectMode      = -20;
	gWCT_Params.wLightModeMaxRecvPwrThreshPercent   = 50;  //%
	gWCT_Params.wConnectModeMaxRecvPwrThreshPercent = 200;  //%
	gWCT_Params.wPowerDiffThresholdBetweenCalibrationLightAndConnect        = 2000;
    
    //PID
    gWCT_Params.byNumPidAdjustmentsPerActiveWindow    = 5;
    gWCT_Params.byIntervalBetweenPidAdjust            = 2;
    gWCT_Params.wRailStepMv                           = 10;
    gWCT_Params.wRailPidScaleFactor                   = 100;
    gWCT_Params.byRailKp                              = 5;
    gWCT_Params.byRailKi                              = 1;
    gWCT_Params.byRailKd                              = 0;
    gWCT_Params.wIntegralUpdateInterval               = 5;
    gWCT_Params.wDerivativeUpdateInterval             = 5;
    gWCT_Params.swIntegralUpperLimit                  = 3000;
    gWCT_Params.swIntegralLowerLimit                  = -3000;
    gWCT_Params.swPidUpperLimit                       = 20000;
    gWCT_Params.swPidLowerLimit                       = -20000;
    gWCT_Params.byActiveTimeMs                        = 20;
    gWCT_Params.bySettleTimeMs                        = 3;
    gWCT_Params.byDefaultPidHoldTimeMs                = 5;
    gWCT_Params.byMaxPidHoldTimeMs                    = 205;
    gWCT_Params.wOverCurrentLimitMa                   = pNvmParam->OpParams.OpStateParams.wOverCoilCurrentThreshold;  //mA
    
    //DDM
    gWCT_Params.byMaxPreambleDuty                 = 57;
    gWCT_Params.byMinPreambleDuty                 = 43;
    gWCT_Params.byMaxPreambleCount                = 25;
    gWCT_Params.byMinPreambleCount                = 4;
    gWCT_Params.wCommunicationRate                = 2000;
    gWCT_Params.wCommunicationRateTolerance       = 20;
    gWCT_Params.wCommunicationFailISRCount        = 3;
    gWCT_Params.byDDMThreshold                    = 2 * 8;
    
    //Rx remove quick detection
    gWCT_Params.wRRQDInputCurrentAbsoluteThreshold = 15;
    gWCT_Params.wRRQDInputCurrentPercentThreshold  = 5;
    gWCT_Params.wRRQDCoilCurrentAbsoluteThreshold  = 50;
    gWCT_Params.wRRQDCoilCurrentPercentThreshold   = 5;
    
    //reference timer
#define COMM_REFERENCE_TIME_FREQ   390625UL
    gWCT_Params.dwCommReferenceTimerFreq          = COMM_REFERENCE_TIME_FREQ;
    gWCT_Params.dwCommReferenceTimerMaxCount      = 0x10000UL;
    gWCT_Params.wMaxDigitalPingTimeRefCounts      = (uint16)(COMM_REFERENCE_TIME_FREQ*0.067);
    gWCT_Params.wFirstPacketTimeoutRefCounts      = (uint16)(COMM_REFERENCE_TIME_FREQ*0.022);
    gWCT_Params.wNextPacketTimeoutRefCounts       = (uint16)(COMM_REFERENCE_TIME_FREQ*0.024);
    
    gWCT_Params.tDebugConfig.bGeneralDbg          = 0;
    gWCT_Params.tDebugConfig.bStateMachineDbg     = 0;
    gWCT_Params.tDebugConfig.bKeepLastAbnormDbg   = 0;
    
    //MVL
    gWCT_Params.wMaxVolForLpPowerRx               = 15000;
    gWCT_Params.wMaxVolForMpPowerRx               = 23000;
    gWCT_Params.wMaxVolForLowLoading              = 11500;
    gWCT_Params.wLowLoadingThreshold              = 1400;
    gWCT_Params.wHeavyLoadingThreshold            = 14000;
    gWCT_Params.wMinPowerFactorForHeavyLoading    = 40;
    
    gWCT_Params.wQfactorChangeThreshold           = 20;
    
    //MPL
    gWCT_Params.wMPLRPPThreshold                  = 9000;
	gWCT_Params.wMPLHysteresis                    = 500;
	
	//active power protection
	gWCT_Params.wAPPDumpPowerAbsoluteThreshold    = 4000;  //mW
	gWCT_Params.byAPPRollBackWin                  = 2;
	gWCT_Params.byAPPDumpPowerPercentageThreshold = 60;
	gWCT_Params.byAPPVolDumpScale                 = 100;   //percentage
	
	gWCT_Params.byMaxRxReportedQFactor            = 150;
	gWCT_Params.byMinTxMeasuredQfToStopRetry      = 155;
	gWCT_Params.byQfAveNumForRetry                = 10;
}

QF_CALIB_PARAMS* WCT_GetQFParams(uint8 byDeviceId, uint8 byCoilId)
{
	return NVM_GetQfCalibParams(byDeviceId, byCoilId);
}

FOD_CHARACTERIZATION_PARAMS* WCT_GetCharacterizatioinParams(uint8 byDeviceId, uint8 byCoilId, uint8 byControlType)
{
	return NVM_GetCharactParams(byDeviceId, byCoilId, byControlType);
}

FOD_NORMALIZATION_PARAMS* WCT_GetNormalizationParams(uint8 byDeviceId, uint8 byCoilId, uint8 byControlType)
{
	return NVM_GetNormalizationParams(byDeviceId, byCoilId, byControlType);
}
