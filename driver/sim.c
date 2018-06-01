/*******************************************************************************
*
* Copyright 2012-2016 Freescale Semiconductor, Inc.
* Copyright 2017~2018 NXP.
* All rights reserved.
*******************************************************************************/

#include "defines.h"
#include "cpu.h"
#include "appcfg.h"

#include "systemDebug.h"

#include "sim.h"

void SIM_Init(void)
{
	SIM_CLKOUT = 0x1020U;	//Disable clock output
	SIM_GPSCH  = 0x0000U;   
#if DIGITAL_DCDC
	SIM_GPSFL  = 0x0500U;   //GPIOF4/F5 = XB_OUT8/XB_OUT9
#else
	SIM_GPSFL  = 0x0000U;   //GPIOF4/F5 = TXD1/RXD1
#endif
	SIM_PCE0   = 0xF07EU;
	SIM_PCE1   = 0x3A41U;
	SIM_PCE2   = 0x1E8CU;
	SIM_PCE3   = 0x00F0U;
	SIM_SD2    = 0x0004U;   //PIT1 works in STOP mode
	SIM_IPS0   = 0x0100U;   //TA0 from XB_OUT49
}
