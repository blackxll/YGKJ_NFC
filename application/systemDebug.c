/*******************************************************************************
*
* Copyright 2012-2015 Freescale Semiconductor, Inc.
* Copyright 2017~2018 NXP.
* All rights reserved.
*******************************************************************************/

#include "defines.h"

#include "appCfg.h"
#include "wct_lib.h"
#include "freemaster.h"

#include "hal.h"

#include "systemDebug.h"


//DEBUG MODULE

volatile uint32 gDBG_dwAssertCode, gDBG_dwAssertParam;
volatile uint32 gDBG_dwWarningCode, gDBG_dwWarningParam;

#ifdef _DEBUG

void DBG_Assert(uint8 byAssert, uint32 dwAssertCode, uint32 dwParameter)
{
	if(byAssert)
	{
		uint8 i;
		for(i=0; i<NUM_DEVICES; i++)
		{
			HAL_EnableWCT(i, 0);
		}
		gDBG_dwAssertCode  = dwAssertCode;
		gDBG_dwAssertParam = dwParameter;
		while(1) 
        {
#if FREEMASTER_SUPPORTED
            FMSTR_Poll();
#endif
        }
	}
}

void DBG_Warning(uint8 byWarning, uint32 dwWarningCode, uint32 dwParameter)
{
	if(byWarning)
	{
		gDBG_dwWarningCode  = dwWarningCode;
		gDBG_dwWarningParam = dwParameter;
	}
}

#endif

