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

#include "pwm.h"

void PWM_SetFreq(uint8 byId, uint32 dwPeriod)
{
	DBG_Assert(dwPeriod <= 32, 0, 0);
	
	dwPeriod -= 32;
	
	switch(byId)
	{
	case PWM_INVERTER_LEFT_INDEX:
		PWMA_MCTRL |= PWMA_MCTRL_CLDOK_1;
		PWMA_SM1INIT = 0;
		PWMA_SM1VAL1 = (dwPeriod >> 5);
		PWMA_SM1FRACVAL1 = (dwPeriod & 0x1f) << 11;
		PWMA_MCTRL |= PWMA_MCTRL_LDOK_1;
		break;
		
	case PWM_INVERTER_RIGHT_INDEX:
		PWMA_MCTRL |= PWMA_MCTRL_CLDOK_3;
		PWMA_SM3INIT = 0;
		PWMA_SM3VAL1 = (dwPeriod >> 5);
		PWMA_SM3FRACVAL1 = (dwPeriod & 0x1f) << 11;
		PWMA_MCTRL |= PWMA_MCTRL_LDOK_3;
		break;
		
	case PWM_Q_MEASURE_INDEX:  //shared PWMA3 with PWM_INVERTER_RIGHT_INDEX
		PWMA_MCTRL |= PWMA_MCTRL_CLDOK_3;
		PWMA_SM3INIT = 0;
		PWMA_SM3VAL1 = (dwPeriod >> 5);
		PWMA_SM3FRACVAL1 = (dwPeriod & 0x1f) << 11;
		PWMA_MCTRL |= PWMA_MCTRL_LDOK_3;
		break;
		
	default:
		DBG_Assert(1, 0, byId);
	}
}

void PWM_SetChannelPairs(uint8 byId, uint8 byChannel, uint32 dwStartPeriod, uint32 dwStopPeriod)
{
	switch(byId)
	{
	case PWM_INVERTER_LEFT_INDEX:  //independent mode
		PWMA_MCTRL |= PWMA_MCTRL_CLDOK_1;
		if(PWM_INVERTER_CHANNEL_LEFT_HIGH == byChannel)
		{
			PWMA_SM1VAL2 = (dwStartPeriod >> 5);
			PWMA_SM1FRACVAL2 = (dwStartPeriod & 0x1f) << 11;
			PWMA_SM1VAL3 = (dwStopPeriod >> 5);
			PWMA_SM1FRACVAL3 = (dwStopPeriod & 0x1f) << 11;
		}
		else if(PWM_INVERTER_CHANNEL_LEFT_LOW == byChannel)
		{
			PWMA_SM1VAL4 = (dwStartPeriod >> 5);
			PWMA_SM1FRACVAL4 = (dwStartPeriod & 0x1f) << 11;
			PWMA_SM1VAL5 = (dwStopPeriod >> 5);
			PWMA_SM1FRACVAL5 = (dwStopPeriod & 0x1f) << 11;
		}
		else
		{
			DBG_Assert(1, 0, byChannel);
		}
		PWMA_MCTRL |= PWMA_MCTRL_LDOK_1;
		break;
		
	case PWM_INVERTER_RIGHT_INDEX:  //independent mode
		PWMA_MCTRL |= PWMA_MCTRL_CLDOK_3;
		if(PWM_INVERTER_CHANNEL_RIGHT_HIGH == byChannel)
		{
			PWMA_SM3VAL2 = (dwStartPeriod >> 5);
			PWMA_SM3FRACVAL2 = (dwStartPeriod & 0x1f) << 11;
			PWMA_SM3VAL3 = (dwStopPeriod >> 5);
			PWMA_SM3FRACVAL3 = (dwStopPeriod & 0x1f) << 11;
		}
		else if(PWM_INVERTER_CHANNEL_RIGHT_LOW == byChannel)
		{
			PWMA_SM3VAL4 = (dwStartPeriod >> 5);
			PWMA_SM3FRACVAL4 = (dwStartPeriod & 0x1f) << 11;
			PWMA_SM3VAL5 = (dwStopPeriod >> 5);
			PWMA_SM3FRACVAL5 = (dwStopPeriod & 0x1f) << 11;
		}
		else
		{
			DBG_Assert(1, 0, byChannel);
		}
			
		PWMA_MCTRL |= PWMA_MCTRL_LDOK_3;
		break;

#if DIGITAL_DCDC
	case PWM_D_DCDC_BUCK_INDEX:  //complementary mode
		PWMA_MCTRL |= PWMA_MCTRL_CLDOK_0;
		if(PWM_D_DCDC_CHANNLE_BUCK_HIGH == byChannel)
		{
			PWMA_SM0VAL2 = 0;
			PWMA_SM0FRACVAL2 = 0;
			PWMA_SM0VAL3 = (dwStopPeriod >> 5);
			PWMA_SM0FRACVAL3 = (dwStopPeriod & 0x1f) << 11;
		}
		else
		{
			DBG_Assert(1, 0, byChannel);
		}
		PWMA_MCTRL |= PWMA_MCTRL_LDOK_0;
		break;
		
	case PWM_D_DCDC_BOOST_INDEX:  //complementary mode
		PWMA_MCTRL |= PWMA_MCTRL_CLDOK_2;
		if(PWM_D_DCDC_CHANNLE_BOOST_LOW == byChannel)
		{
			PWMA_SM2VAL2 = (dwStartPeriod >> 5);
			PWMA_SM2FRACVAL2 = (dwStartPeriod & 0x1f) << 11;
			PWMA_SM2VAL3 = 0;
			PWMA_SM2FRACVAL3 = 0;
		}
		else
		{
			DBG_Assert(1, 0, byChannel);
		}
		PWMA_MCTRL |= PWMA_MCTRL_LDOK_2;
		break;
#endif
		
	default:
		DBG_Assert(1, 0, byId);
	}
}

void PWM_SetChannelDuty(uint8 byId, uint8 byChannel, uint32 dwPeriod)
{
	switch(byId)
	{
	case PWM_INVERTER_LEFT_INDEX:
		PWMA_MCTRL |= PWMA_MCTRL_CLDOK_1;
		if(PWM_INVERTER_CHANNEL_TRIGGER == byChannel)
		{
			PWMA_SM1VAL0 = (dwPeriod>>5);
		}
		else
		{
			DBG_Assert(1, 0, byChannel);
		}
		PWMA_MCTRL |= PWMA_MCTRL_LDOK_1;
		break;
		
	case PWM_Q_MEASURE_INDEX:
		PWMA_MCTRL |= PWMA_MCTRL_CLDOK_3;
		if(PWM_Q_MEASURE_CHANNEL_TRIGGER == byChannel)
		{
			PWMA_SM3VAL0 = (dwPeriod>>5);
		}
		else
		{
			DBG_Assert(1, 0, byChannel);
		}
		PWMA_MCTRL |= PWMA_MCTRL_LDOK_3;
		break;
		
	default:
		DBG_Assert(1, 0, byId);
	}
}

void PWM_Start(uint8 byId)
{
	switch(byId)
	{
	case PWM_INVERTER_LEFT_INDEX:
		/*
		 * when VALx value is equal to INIT, VALx value may not be compared at first PWM cycles.
		 * Since VALx needs to updated from its buffer at first clock of PWM cycles.
		 * SW workaround is to force VALx update
		 */
		PWMA_MCTRL |= PWMA_MCTRL_CLDOK_1;
		PWMA_SM1CTRL  |= 0x04;
		PWMA_MCTRL |= PWMA_MCTRL_LDOK_1;
		PWMA_SM1CTRL  &= ~0x04;
		
		PWMA_MASK  &= ~(PWMA_MASK_MASKA_1 | PWMA_MASK_MASKB_1);
		PWMA_OUTEN |= (PWMA_OUTEN_PWMA_EN_1 | PWMA_OUTEN_PWMB_EN_1);
		PWMA_SM1CTRL2  |= 0x40; //Force event
		PWMA_MCTRL |= (PWMA_MCTRL_RUN_1 | PWMA_MCTRL_RUN_3);  //PWMA1 & PWMA3 must be synced
		break;
	case PWM_INVERTER_RIGHT_INDEX:
		PWMA_MCTRL |= PWMA_MCTRL_CLDOK_3;
		PWMA_SM3CTRL  |= 0x04;
		PWMA_MCTRL |= PWMA_MCTRL_LDOK_3;
		PWMA_SM3CTRL  &= ~0x04;
		
		PWMA_MASK  &= ~(PWMA_MASK_MASKA_3 | PWMA_MASK_MASKB_3);
		PWMA_OUTEN |= (PWMA_OUTEN_PWMA_EN_3 | PWMA_OUTEN_PWMB_EN_3);
		PWMA_SM3CTRL2  |= 0x40; //Force event
		PWMA_MCTRL |= (PWMA_MCTRL_RUN_3 | PWMA_MCTRL_RUN_1);  //PWMA1 & PWMA3 must be synced
		break;
	case PWM_Q_MEASURE_INDEX:
		PWMA_MCTRL |= PWMA_MCTRL_CLDOK_3;
		PWMA_SM3CTRL  |= 0x04;
		PWMA_MCTRL |= PWMA_MCTRL_LDOK_3;
		PWMA_SM3CTRL  &= ~0x04;
		PWMA_MCTRL |= PWMA_MCTRL_RUN_3;
		break;
#if DIGITAL_DCDC
	case PWM_D_DCDC_BUCK_INDEX:
	    PWMA_MCTRL |= PWMA_MCTRL_CLDOK_0;
        PWMA_SM0CTRL  |= 0x04;
        PWMA_MCTRL |= PWMA_MCTRL_LDOK_0;
        PWMA_SM0CTRL  &= ~0x04;
	    
		PWMA_MASK  &= ~(PWMA_MASK_MASKA_0 | PWMA_MASK_MASKB_0);		
		PWMA_OUTEN |= (PWMA_OUTEN_PWMA_EN_0 | PWMA_OUTEN_PWMB_EN_0);		
		PWMA_SM0CTRL2  |= 0x40;
		PWMA_MCTRL |= (PWMA_MCTRL_RUN_0 | PWMA_MCTRL_RUN_2);   //PWMA0 & PWMA2 must be synced
		break;
	case PWM_D_DCDC_BOOST_INDEX:
        PWMA_MCTRL |= PWMA_MCTRL_CLDOK_2;
        PWMA_SM2CTRL  |= 0x04;
        PWMA_MCTRL |= PWMA_MCTRL_LDOK_2;
        PWMA_SM2CTRL  &= ~0x04;
	    
		PWMA_MASK  &= ~(PWMA_MASK_MASKA_2 | PWMA_MASK_MASKB_2);		
		PWMA_OUTEN |= (PWMA_OUTEN_PWMA_EN_2 | PWMA_OUTEN_PWMB_EN_2);
		PWMA_SM2CTRL2  |= 0x40;
		PWMA_MCTRL |= (PWMA_MCTRL_RUN_2 | PWMA_MCTRL_RUN_0);   //PWMA0 & PWMA2 must be synced
		break;
#endif
	default:
		DBG_Assert(1, 0, byId);
	}
}

void PWM_Stop(uint8 byId)
{
	uint16 val2, val4;
	switch(byId)
	{
	case PWM_INVERTER_LEFT_INDEX:  //independent mode
		PWMA_MCTRL &= ~(PWMA_MCTRL_RUN_1 | PWMA_MCTRL_RUN_3);   //PWMA1 & PWMA3 must be synced
		PWMA_MASK  |= (PWMA_MASK_MASKA_1 | PWMA_MASK_MASKB_1);
		PWMA_OUTEN &= ~(PWMA_OUTEN_PWMA_EN_1 | PWMA_OUTEN_PWMB_EN_1);
		
		/*
		 * If val2 or val4 equals to INIT, the forceEn setting will setting PWMA or PWMB output high.
		 * SW workaround is to set val2 and val4 values not equals to INIT, then do forceEn setting.
		 * After that restore VAL2 & VAL4
		 */
		PWMA_SM1CTRL   |= 0x04;
		PWMA_MCTRL     |= PWMA_MCTRL_LDOK_1;
		val2            = PWMA_SM1VAL2;
		val4            = PWMA_SM1VAL4;
		PWMA_SM1VAL2    = PWMA_SM1INIT + 1;
		PWMA_SM1VAL4    = PWMA_SM1INIT + 1;
		PWMA_MCTRL     |= PWMA_MCTRL_LDOK_1;
		
		PWMA_SM1CTRL2  |= 0x80;
		PWMA_SM1CTRL2  |= 0x40;  //Force event
		PWMA_SM1CTRL2  &= ~0x80;
		
		PWMA_SM1CTRL   &= ~0x04;
		
		//Restore the VAL2 & VAL4 but not take effect immediately
		PWMA_SM1VAL2    = val2;
		PWMA_SM1VAL4    = val4;
		PWMA_MCTRL     |= PWMA_MCTRL_LDOK_1;
		break;
	case PWM_INVERTER_RIGHT_INDEX:  //independent mode
		PWMA_MCTRL &= ~(PWMA_MCTRL_RUN_3 | PWMA_MCTRL_RUN_1);
		PWMA_MASK  |= (PWMA_MASK_MASKA_3 | PWMA_MASK_MASKB_3);
		PWMA_OUTEN &= ~(PWMA_OUTEN_PWMA_EN_3 | PWMA_OUTEN_PWMB_EN_3);
		
		/*
		 * If val2 or val4 equals to INIT, the forceEn setting will setting PWMA or PWMB output high.
		 * SW workaround is to set val2 and val4 values not equals to INIT, then do forceEn setting.
		 */
		PWMA_SM3CTRL   |= 0x04;
		PWMA_MCTRL     |= PWMA_MCTRL_LDOK_3;
		val2            = PWMA_SM3VAL2;
		val4            = PWMA_SM3VAL4;
		PWMA_SM3VAL2    = PWMA_SM3INIT + 1;
		PWMA_SM3VAL4    = PWMA_SM3INIT + 1;
		PWMA_MCTRL     |= PWMA_MCTRL_LDOK_3;
		
		PWMA_SM3CTRL2  |= 0x80;
		PWMA_SM3CTRL2  |= 0x40;  //Force event
		PWMA_SM3CTRL2  &= ~0x80;
		
		PWMA_SM3CTRL   &= ~0x04;
		
		PWMA_SM3VAL2    = val2;
		PWMA_SM3VAL4    = val4;
		PWMA_MCTRL     |= PWMA_MCTRL_LDOK_3;
		break;
	case PWM_Q_MEASURE_INDEX:
		PWMA_MCTRL &= ~PWMA_MCTRL_RUN_3;
		break;
#if DIGITAL_DCDC
	case PWM_D_DCDC_BUCK_INDEX:  //complementary mode
		PWMA_MCTRL &= ~(PWMA_MCTRL_RUN_0 | PWMA_MCTRL_RUN_2);  //PWMA0 & PWMA2 must be synced
		PWMA_MASK  |= (PWMA_MASK_MASKA_0 | PWMA_MASK_MASKB_0);		
		PWMA_OUTEN &= ~(PWMA_OUTEN_PWMA_EN_0 | PWMA_OUTEN_PWMB_EN_0);		
		PWMA_SM0CTRL2  |= 0x80;
		PWMA_SM0CTRL2  |= 0x40;  //Force event
		PWMA_SM0CTRL2  &= ~0x80;
		break;
	case PWM_D_DCDC_BOOST_INDEX:  //complementary mode
		PWMA_MCTRL &= ~(PWMA_MCTRL_RUN_2 | PWMA_MCTRL_RUN_0);  //PWMA0 & PWMA2 must be synced
		PWMA_MASK  |= (PWMA_MASK_MASKA_2 | PWMA_MASK_MASKB_2);		
		PWMA_OUTEN &= ~(PWMA_OUTEN_PWMA_EN_2 | PWMA_OUTEN_PWMB_EN_2);		
		PWMA_SM2CTRL2  |= 0x80;
		PWMA_SM2CTRL2  |= 0x40;  //Force event
		PWMA_SM2CTRL2  &= ~0x80;
		break;
#endif
	default:
		DBG_Assert(1, 0, byId);
	}
}

void PWM_ClearChannelFlags(uint8 byId, uint8 byChannel)
{
	switch(byId)
	{
	case PWM_INVERTER_LEFT_INDEX:
		PWMA_SM1STS = (1U<<byChannel);
		break;
	case PWM_INVERTER_RIGHT_INDEX:
		PWMA_SM3STS = (1U<<byChannel);
		break;
	default:
		DBG_Assert(1, 0, byId);
	}
}

boolean PWM_CheckChannelFlags(uint8 byId, uint8 byChannel)
{
	boolean ret = 0;
	switch(byId)
	{
	case PWM_INVERTER_LEFT_INDEX:
		if(PWMA_SM1STS & (1U<<byChannel))
		{
			ret = 1;
		}
		break;
	case PWM_INVERTER_RIGHT_INDEX:
		if(PWMA_SM3STS & (1U<<byChannel))
		{
			ret = 1;
		}
		break;
	default:
		DBG_Assert(1, 0, byId);
	}
	return ret;
}

void PWM_EnableTrigger(uint8 byId, uint8 byChannel, uint8 byIsEn)
{
	switch(byId)
	{
	case PWM_Q_MEASURE_INDEX:
		if(byIsEn)
		{
			PWMA_SM3TCTRL |= (1U<<byChannel);
		}
		else
		{
			PWMA_SM3TCTRL &= ~(1U<<byChannel);
		}
		break;
		
	case PWM_INVERTER_LEFT_INDEX:
		if(byIsEn)
		{
			PWMA_SM1TCTRL |= (1U<<byChannel);
		}
		else
		{
			PWMA_SM1TCTRL &= ~(1U<<byChannel);
		}
		break;
		
	default:
		DBG_Assert(1, 0, byId);
	}
}

void PWM_ClearOverFlowFlag(uint8 byId)
{
	switch(byId)
	{
	case PWM_INVERTER_LEFT_INDEX:
		PWMA_SM1STS = 0x1000;   //Reload flag
		break;
	case PWM_Q_MEASURE_INDEX:
		PWMA_SM3STS = 0x02;   //VAL1 compare flag
		break;
	default:
		DBG_Assert(1, 0, byId);
	}
}

boolean PWM_CheckOverFlowFlag(uint8 byId)
{
	boolean ret = 0;
	switch(byId)
	{
	case PWM_INVERTER_LEFT_INDEX:
		if(PWMA_SM1STS & 0x1000)  //Reload flag
		{
			ret = 1;
		}
		break;
		
	case PWM_Q_MEASURE_INDEX:
		if(PWMA_SM3STS & 0x02)   //VAL1 compare flag
		{
			ret = 1;
		}
		break;
		
	default:
		DBG_Assert(1, 0, byId);
	}
	return ret;
}

boolean PWM_CheckRunning(uint8 byId)
{
	boolean ret = 0;
	DBG_Assert(byId>=4, 0, byId);
	
	switch(byId)
	{
	case PWM_INVERTER_LEFT_INDEX:
		if(PWMA_MCTRL & PWMA_MCTRL_RUN_1)
		{
			ret = 1;
		}
		break;
		
	default:
		DBG_Assert(1, 0, byId);
	}
	
	return ret;
}

void PWM_SetMode(uint8 byId, PWM_MODE_E mode)
{
	switch(byId)
	{
	case PWM_Q_MEASURE_INDEX:
	case PWM_INVERTER_RIGHT_INDEX:
	    //PWM3 reused for inverter and trigger generator for Q factor measurement
		if(PWM_MODE_NORMAL == mode)
		{
			PWMA_SM3CTRL2    = 0xA000U;
		}
		else if(PWM_MODE_COUNT == mode)
		{
			PWMA_SM3CTRL2   = 0xA300U;  //external SYNC causes initialization
		}
		break;
		
	default:
		DBG_Assert(1, 0, byId);
	}
}

void PWM_Init(uint16 wDeadTimeNs)
{
    //Note: PWM module works in independent mode, dead time setting doesn't take effect
	uint16 cpuClockNs = 1000000000UL/CPU_IPBUS_CLK;
	uint32 tmp;
	
	PWMA_MCTRL = 0;
	PWMA_OUTEN = 0x0000;
	PWMA_MASK  = 0xFFFF;
	
	PWMA_SM1CTRL2    = 0xA000U;  //independent mode, PWM behaves normal when CPU is in debug mode
	PWMA_SM1CTRL     = 0x0400U;  //PWM clock divider = 1
	PWMA_SM1DISMAP0  = 0xF000U;  //Disable fault input
	PWMA_SM1DISMAP1  = 0xF000U;
	PWMA_SM1DTCNT0   = wDeadTimeNs / cpuClockNs;
	PWMA_SM1DTCNT1   = wDeadTimeNs / cpuClockNs;
	PWMA_SM1FRCTRL  = 0x0116;
	PWMA_SM1CTRL2   |= 0x80;
	PWMA_SM1CTRL2   |= 0x40;   //Force output
	PWMA_SM1CTRL2  &= ~0x80;

	PWMA_SM3CTRL2    = 0xA000U;  //independent mode, PWM behaves normal when CPU is in debug mode
	PWMA_SM3CTRL     = 0x0400U;  //PWM clock divider = 1
	PWMA_SM3DISMAP0  = 0xF000U;  //Disable fault input
	PWMA_SM3DISMAP1  = 0xF000U;
	PWMA_SM3DTCNT0   = wDeadTimeNs / cpuClockNs;
	PWMA_SM3DTCNT1   = wDeadTimeNs / cpuClockNs;
	PWMA_SM3FRCTRL  = 0x0116;
	PWMA_SM3CTRL2   |= 0x80;
	PWMA_SM3CTRL2   |= 0x40;  //Force output
	PWMA_SM3CTRL2  &= ~0x80;
	
	/* SW workaround for errata e9432, see MWCT10x3A_2N27E.pdf */
	PWM_SetFreq(PWM_INVERTER_LEFT_INDEX, 0x1000);
	PWM_SetChannelPairs(PWM_INVERTER_LEFT_INDEX, PWM_INVERTER_CHANNEL_LEFT_HIGH, 0, 0x800);
	PWM_SetChannelPairs(PWM_INVERTER_LEFT_INDEX, PWM_INVERTER_CHANNEL_LEFT_LOW, 0x800, 0);
	
	PWM_SetFreq(PWM_INVERTER_RIGHT_INDEX, 0x1000);
	PWM_SetChannelPairs(PWM_INVERTER_RIGHT_INDEX, PWM_INVERTER_CHANNEL_RIGHT_HIGH, 0, 0x800);
	PWM_SetChannelPairs(PWM_INVERTER_RIGHT_INDEX, PWM_INVERTER_CHANNEL_RIGHT_LOW, 0x800, 0);
	
	//PWM run, but output masked
	PWMA_MCTRL |= (PWMA_MCTRL_RUN_1 | PWMA_MCTRL_RUN_3);
	
	PWM_ClearOverFlowFlag(PWM_INVERTER_LEFT_INDEX);
	
	//Let PWM run two cycles
	while( 0 == (PWM_CheckOverFlowFlag(PWM_INVERTER_LEFT_INDEX))) {}
	PWM_ClearOverFlowFlag(PWM_INVERTER_LEFT_INDEX);
	while( 0 == (PWM_CheckOverFlowFlag(PWM_INVERTER_LEFT_INDEX))) {}
	PWM_ClearOverFlowFlag(PWM_INVERTER_LEFT_INDEX);
	
	PWM_Stop(PWM_INVERTER_LEFT_INDEX);
	PWM_Stop(PWM_INVERTER_RIGHT_INDEX);
	
#if DIGITAL_DCDC
	//D_DCDC employ PWM0 & PWM2
	PWMA_SM0CTRL2   = 0xC000U;  //PWM behaves normal when CPU is in debug/wait mode, complementary mode
	PWMA_SM0CTRL    = 0x0400U;  //PWM clock divider = 1
	PWMA_SM0TCTRL   = 0xC000U;  //Route the PWMA/B output to the PWM_OUT_TRIG0/1 port.
	PWMA_SM0DISMAP0 = 0xF000U;  //Disable fault input
	PWMA_SM0DISMAP1 = 0xF000U;  //Disable fault input
	PWMA_SM0DTCNT0  = wDeadTimeNs / cpuClockNs;
	PWMA_SM0DTCNT1  = wDeadTimeNs / cpuClockNs;
	PWMA_SM0FRCTRL  = 0x0116;
	
	PWMA_SM2CTRL2   = 0xC20CU;  //initialization/force/reload from PWMA0
	PWMA_SM2CTRL    = 0x0400U;  //PWM clock divider = 1
	PWMA_SM2DISMAP0 = 0xF000U;  //Disable fault input
	PWMA_SM2DISMAP1 = 0xF000U;  //Disable fault input
	PWMA_SM2DTCNT0  = wDeadTimeNs / cpuClockNs;
	PWMA_SM2DTCNT1  = wDeadTimeNs / cpuClockNs;
	PWMA_SM2FRCTRL  = 0x0116;

	tmp = CPU_IPBUS_CLK/PWM_D_DCDC_CLK-1;
	PWMA_SM0INIT = 0;
	PWMA_SM0FRACVAL1 = 0x0;
	PWMA_SM0VAL1 = tmp;
	PWMA_SM2INIT = 0;
	PWMA_SM2FRACVAL1 = 0x0;
	PWMA_SM2VAL1 = tmp;

	PWMA_SM0CTRL2  |= 0x80;
	PWMA_SM2CTRL2  |= 0x80;
	PWMA_SM0CTRL2  |= 0x40;  //PWMA2 initialization/force/reload from PWMA0
	PWMA_SM0CTRL2  &= ~0x80;
	PWMA_SM2CTRL2  &= ~0x80;
	
	//Enable output for D_DCDC PWM module
	PWMA_MASK  &= ~0x0550U;
	PWMA_OUTEN |= 0x0550U;
#endif
}

//PWM fractional mode must be OFF before MCU enter low power mode
void PWM_FracOn(uint8 byId, uint8 byIsEn)
{
	if(byIsEn)
	{
		switch(byId)
		{
		case PWM_INVERTER_LEFT_INDEX:
			PWMA_SM1FRCTRL = 0x0116;
			break;
		case PWM_INVERTER_RIGHT_INDEX:
			PWMA_SM3FRCTRL = 0x0116;
			break;
#if DIGITAL_DCDC
		case PWM_D_DCDC_BUCK_INDEX:
			PWMA_SM0FRCTRL = 0x0116;
			break;
		case PWM_D_DCDC_BOOST_INDEX:
			PWMA_SM2FRCTRL = 0x0116;
			break;
#endif
		default:
			break;
		}
	}
	else
	{
		switch(byId)
		{
		case PWM_INVERTER_LEFT_INDEX:
			PWMA_SM1FRCTRL &= ~0x0116;
			break;
		case PWM_INVERTER_RIGHT_INDEX:
			PWMA_SM3FRCTRL &= ~0x0116;
			break;
#if DIGITAL_DCDC
		case PWM_D_DCDC_BUCK_INDEX:
			PWMA_SM0FRCTRL &= ~0x0116;
			break;
		case PWM_D_DCDC_BOOST_INDEX:
			PWMA_SM2FRCTRL &= ~0x0116;
			break;
#endif
		default:
			break;
		}
	}
}
