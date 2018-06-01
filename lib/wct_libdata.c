/*******************************************************************************
*
* Copyright 2012-2016 Freescale Semiconductor, Inc.
* Copyright 2017~2018 NXP.
* All rights reserved.
*******************************************************************************/

#include "defines.h"

#include "appCfg.h"

#include "wct_lib.h"

//variables definition as library required

RUNTIME_PARAMS gRunTimeParams[NUM_DEVICES];
FSK_CTRL_T gFSK_Ctrl[NUM_DEVICES];

RF_DATA_PARAMS eRfDataParams[NUM_DEVICES][NUM_COMM_CHANNELS_PER_DEVICE];
RF_DATA_INTERRUPT_PARAMS eRfDataInterruptParams[NUM_DEVICES][NUM_COMM_CHANNELS_PER_DEVICE];

PACKET_RX_INFO_T gPKT_RxPktInfo[NUM_DEVICES];
WPC_DDM_CTRL_T gWpcDDMCtrlParams[NUM_DEVICES];

uint32 gDCD_PktChCnt[NUM_DEVICES][NUM_COMM_CHANNELS_PER_DEVICE];
