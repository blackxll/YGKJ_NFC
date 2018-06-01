/*******************************************************************************
*
* Copyright 2012-2014 Freescale Semiconductor, Inc.
* Copyright 2017~2018 NXP.
* All rights reserved.
*******************************************************************************/

#ifndef __TRIGGER_H__
#define __TRIGGER_H__

//CrossbarA OUT index
#define TRG_DST_TA0IN_TRIG    49
#define TRG_DST_ADCB_TRIG     13

//CrossbarA IN index
#define TRG_SRC_NONE           0
#define TRG_SRC_PWMA1_TRG0    22
#define TRG_SRC_PWMA3_TRG0    26
#define TRG_SRC_CMPC_OUT      14

void TRG_Init(void);
void TRG_SetTriggerSource(uint8 byTrgDst, uint8 byTrgSrc);
boolean TRG_CheckADCBTriggerSource(void);

#endif
