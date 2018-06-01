/*******************************************************************************
*
* Copyright 2012-2015 Freescale Semiconductor, Inc.
* Copyright 2017~2018 NXP.
* All rights reserved.
*******************************************************************************/

#ifndef __SYSTEM_CALLBACK_H__
#define __SYSTEM_CALLBACK_H__

void WCT_UpdateDevUsrIndication(uint8 byDeviceId);
FOD_PARAMS_T* WCT_GetFODParams(PACKET_RX_INFO_T *pRxInfo, POWER_CONTRACT *pPowerContractInfo);
uint32 WCT_SetReChargeTimeOnAbnormal(E_RECHARGETIME_SETTYPE eAbnormalType);
void WCT_OnWPCPacketRecv(uint8 byDeviceId, uint8 bySize, uint8 *pbyData);

#endif
