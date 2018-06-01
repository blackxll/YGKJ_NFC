/*******************************************************************************
*
* Copyright 2012-2014 Freescale Semiconductor, Inc.
* Copyright 2017~2018 NXP.
* All rights reserved.
*******************************************************************************/
#ifndef __SYSTEM_PROTECTION_H__
#define __SYSTEM_PROTECTION_H__

boolean PROT_CheckRunTimeParams(uint8 byDeviceId, uint8 byCoilId, TX_CHARGING_STATUS eState, uint16 wGuaranteedPower, uint16 wTimePassedMs);
boolean PROT_CheckBoardParams(uint16 wTimePassedMs);

uint16 PROT_GetRRQDFittingInputCurrent(uint8 byDeviceId, uint8 byCoilId, uint16 wRailVoltage, uint32 dwFreq);
uint16 PROT_GetRRQDFittingCoilCurrent(uint8 byDeviceId, uint8 byCoilId, uint16 wRailVoltage, uint32 dwFreq);
boolean PROT_SafeDigitalPingParamCheck(uint8 byDeviceId);

#endif
