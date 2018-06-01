/*******************************************************************************
*
* Copyright 2012-2014 Freescale Semiconductor, Inc.
* Copyright 2017~2018 NXP.
* All rights reserved.
*******************************************************************************/

#include "defines.h"
#include "cpu.h"
#include "appcfg.h"

#include "systemDebug.h"

#include "timer.h"

#define TMR_DAC_FREQ      50000UL
#define TMR_D_DCDC_PID_FREQ  50000UL

void TMR_Init(void)
{
	//For DDM and FRQ usage
	TMRA_0_SCTRL  = 0U;
	TMRA_0_COMP1  = 0x00FFU;
	TMRA_0_CMPLD1 = 0x00FFU;
	TMRA_0_COMP2  = 0U;
	TMRA_0_CMPLD2 = 0U;
	TMRA_0_LOAD   = 0U;
	TMRA_0_CSCTRL = 0x0041U;  //COMP1 interrupt enable, load upon COMP1 compared.
	TMRA_0_CTRL   = 0x2020U;  //counter rising edge, clock source=TA0 IN, count until compare.
	
	//For tick timer usage
    TMRA_2_COMP1     = CPU_TMR_CLK/32/1000 - 1;  //1ms
    TMRA_2_LOAD      = 0U;
    TMRA_2_SCTRL    |= 0x4000u;
    TMRA_2_CTRL     |= 0x3A20U;  //input clock divided 32
    TMRA_0_ENBL     |= TMRA_0_ENBL_ENBL_2;

#if DIGITAL_DCDC
	TMRA_0_ENBL &= ~TMRA_0_ENBL_ENBL_1;
	TMRA_1_COMP1 = CPU_TMR_CLK/TMR_D_DCDC_PID_FREQ-1;
	TMRA_1_LOAD = 0;
	TMRA_1_CNTR = 0;
	TMRA_1_CSCTRL = 0x0040U;  //COMP1 interrupt enable
	TMRA_1_CTRL = 0x3020U;    //counter rising edge, clock source=IP bus clock(divider = 1), count until compare.
	TMRA_0_ENBL |= TMRA_0_ENBL_ENBL_1;
#else
	//FOR simulated DAC usage
	TMRA_1_LOAD   = 0x00;
	TMRA_1_CNTR   = 0x00;
	TMRA_1_SCTRL  = 0x05;      //output enable
	TMRA_1_CSCTRL = 0x86;      
	TMRA_1_CTRL   = 0x3024;    //clock source=IP bus clock(divider = 1), count until compare. Toggle Output when compare
#endif
	
	//For WCT communication reference counter
	PIT0_CTRL  = 0U;
	PIT0_MOD   = 0xFFFFU;
	PIT0_CTRL  = 0x0041U;   //select IP Bus clock as clock source, divider=256, PIT0 enabled
	
	//For low power mode wake up source
	PIT1_CTRL = 0x0302U;  //select ROSC32K as clock source, roll-over interrupt enable
}

void TMR_Enable(uint8 byId, uint8 byIsEn)
{
	switch (byId)
	{
		case TMR_DDM_FRQ_INDEX:
			TMRA_0_CNTR  = 0U;
			if(byIsEn)
			{
				TMRA_0_ENBL |= TMRA_0_ENBL_ENBL_0;
			}
			else
			{
				TMRA_0_ENBL &= ~TMRA_0_ENBL_ENBL_0;
			}
			break;
			
		case TMR_DAC_INDEX:
			TMRA_1_CNTR  = 0U;
			if(byIsEn)
			{
				TMRA_0_ENBL |= TMRA_0_ENBL_ENBL_1;
			}
			else
			{
				TMRA_0_ENBL &= ~TMRA_0_ENBL_ENBL_1;
			}
			break;
			
#if DIGITAL_DCDC
		case TMR_D_DCDC_INDEX:
			TMRA_1_CNTR  = 0U;
			if(byIsEn)
			{
				TMRA_0_ENBL |= TMRA_0_ENBL_ENBL_1;
			}
			else
			{
				TMRA_0_ENBL &= ~TMRA_0_ENBL_ENBL_1;
			}
			break;
#endif

		case TMR_LOW_POWER_INDEX:
		    if(byIsEn)
		    {
		        PIT1_CTRL |= PIT1_CTRL_CNT_EN;
		    }
		    else
		    {
		        PIT1_CTRL &= ~(PIT1_CTRL_CNT_EN); 
		    }
		    break;

		default:
			break;
	}
}


//For TMR_DDM_FRQ_INDEX, wCompDuty is a compare value;
//For TMR_DAC_INDEX, wCompDuty is a duty cycle (max duty cycle is TMR_PWM_MAX_DUTY)
//For TMR_LOW_POWER_INDEX, wCompDuty is a time value in unit of ms
void TMR_SetCompareValue(uint8 byId, uint16 wCompDuty)
{
	uint32 tmp;
	switch(byId)
	{
	case TMR_DDM_FRQ_INDEX:
		TMRA_0_COMP1  = wCompDuty;
		TMRA_0_CMPLD1 = wCompDuty;
		break;
	case TMR_DAC_INDEX:
		tmp = CPU_TMR_CLK/TMR_DAC_FREQ * wCompDuty;
		tmp /= TMR_PWM_MAX_DUTY;
		TMRA_1_CMPLD2 = tmp;
		TMRA_1_CMPLD1 = CPU_TMR_CLK/TMR_DAC_FREQ - tmp;
		break;
	case TMR_LOW_POWER_INDEX:
	    {
            uint16 tmp;
            tmp = (PIT1_CTRL & PIT1_CTRL_PRESCALER) >> 3;
            tmp <<= 1;
            tmp = CPU_LP_CLK / tmp;
            tmp /= 1000;
            tmp *= wCompDuty;
            
            PIT1_MOD = tmp;
	    }
	    break;
	default:
		DBG_Assert(1, 0, byId);
	}
}

void TMR_SetMode(uint8 byId, TMR_MODE_E mode)
{
	if(TMR_DDM_FRQ_INDEX == byId)
	{
	    //TMRA1 multiplex for DDM and Q factor measurement
		DBG_Assert(TMRA_0_ENBL & 0x01, 0, TMRA_0_ENBL);
		TMRA_0_LOAD   = 0U;
		TMRA_0_CNTR   = 0X00U;
		
		if(TMR_MODE_COUNTER == mode)
		{
			//For DDM, simple conuter mode
			TMRA_0_SCTRL  = 0U;
			TMRA_0_CSCTRL = 0x0041U;
			TMRA_0_CTRL   = 0x2020U;
		}
		else if(TMR_MODE_CAPTURE == mode)
		{
			//For FR Q mesurement, works in capture mode
			TMRA_0_CTRL   = 0XD000U; // CM mode 110, T0 input pin as secondary count source
			TMRA_0_SCTRL  = 0X0040U;
			TMRA_0_CSCTRL = 0X0400U; // Reload counter
		}
		else
		{
			DBG_Assert(1, 0, mode);
		}
	}
	else
	{
		DBG_Assert(1, 0, byId);
	}
}

uint32 TMR_GetCounterValue(uint8 byId)
{
	uint16 ret = 0;
	switch(byId)
	{
	case TMR_COMM_REF_INDEX:
		ret = PIT0_CNTR;
		break;
		
	case TMR_TICK_INDEX:
		ret =  TMRA_2_CNTR;
		break;
		
	case TMR_DDM_FRQ_INDEX:
		ret = TMRA_0_CAPT;   //return the captured value
		break;
		
	default:
		DBG_Assert(1, 0, byId);
	}
	return ret;
}

boolean TMR_CheckCompare(uint8 byId)
{
	uint16 ret = 0;
	switch(byId)
	{
	case TMR_DDM_FRQ_INDEX:
		if(TMRA_0_CSCTRL & TMRA_0_CSCTRL_TCF1)
		{
		    //compared
			ret = 1;
		}
		break;
		
	default:
		DBG_Assert(1, 0, byId);
	}
	return ret;
}

boolean TMR_CheckCapture(uint8 byId)
{
	uint16 ret = 0;
	switch(byId)
	{
	case TMR_DDM_FRQ_INDEX:
		if(TMRA_0_SCTRL & TMRA_0_SCTRL_IEF)
		{
		    //captured
			ret = 1;
		}
		break;
		
	default:
		DBG_Assert(1, 0, byId);
	}
	return ret;
}

boolean TMR_ClearCompareFlag(uint8 byId)
{
	uint16 ret = 0;
	switch(byId)
	{
	case TMR_DDM_FRQ_INDEX:
		TMRA_0_CSCTRL &= ~TMRA_0_CSCTRL_TCF1;
		break;
		
	default:
		DBG_Assert(1, 0, byId);
	}
	return ret;
}

boolean TMR_ClearCaptureFlag(uint8 byId)
{
	uint16 ret = 0;
	switch(byId)
	{
	case TMR_DDM_FRQ_INDEX:
		TMRA_0_SCTRL &= ~TMRA_0_SCTRL_IEF;
		break;
		
	default:
		DBG_Assert(1, 0, byId);
	}
	return ret;
}

//Wait time should be less than 1ms
uint16 TMR_WaitUs(uint16 wWaitUs)
{
	//According to TMR_TICK_INDEX setting
	//1 count = CPU_TMR_CLK / 32 = 1000000/(CPU_TMR_CLK / 32)us
	//WaitCnt = wWaitUs / (1000000/(CPU_TMR_CLK / 32)) 
	//        = wWaitUs / (32*1000000 / CPU_TMR_CLK)
	//        = wWaitUs * CPU_TMR_CLK / (32*1000000)
	//        = wWaitUs * (CPU_TMR_CLK/1000000) / 32
	uint16 currCnt, previousCnt = TMR_GetCounterValue(TMR_TICK_INDEX);
	uint16 waitCnt, diffCnt;
	uint32 tmp = wWaitUs;
	tmp *= (CPU_TMR_CLK/1000000);
	tmp /= 32;
	
	waitCnt = tmp;

	while(1)
	{
		currCnt = TMR_GetCounterValue(TMR_TICK_INDEX);
		if(currCnt >= previousCnt)
		{
			diffCnt = currCnt - previousCnt;
		}
		else
		{
			diffCnt = currCnt + (CPU_TMR_CLK / 32 / 1000 - previousCnt);
		}
		
		if(diffCnt >= waitCnt)
		{
			break;
		}
	}
}

void TMR_ISRAck(uint8 byId)
{
    //clear interrupt flag
	switch(byId)
	{
	case TMR_TICK_INDEX:
		TMRA_2_SCTRL &= ~TMRA_2_SCTRL_TCF;
		break;
		
	case TMR_DDM_FRQ_INDEX:
		TMRA_0_CSCTRL &= ~TMRA_0_CSCTRL_TCF1;
		break;
		
#if DIGITAL_DCDC
	case TMR_D_DCDC_INDEX:
		TMRA_1_CSCTRL &= ~TMRA_1_CSCTRL_TCF1;
		break;
#endif
		
	case TMR_LOW_POWER_INDEX:
	    PIT1_CTRL &= ~(PIT1_CTRL_PRF);
	    break;

	default:
		DBG_Assert(1, 0, byId);
	}
}

void TMR_EnableInStopMode(uint8 byId, uint8 byIsEn)
{
    switch(byId)
    {
    case TMR_LOW_POWER_INDEX:
        if(byIsEn)
        {
            SIM_SD2 |= SIM_SD2_PIT1;
        }
        else
        {
            SIM_SD2 &= ~SIM_SD2_PIT1;
        }
        break;
    }
}

