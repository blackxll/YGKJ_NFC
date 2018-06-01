/*******************************************************************************
*
* Copyright 2012-2016 Freescale Semiconductor, Inc.
* Copyright 2017~2018 NXP.
* All rights reserved.
*******************************************************************************/

#ifndef __PWM_H__
#define __PWM_H__
    
#define PWM_INVERTER_LEFT_INDEX   1
#define PWM_INVERTER_CHANNEL_TRIGGER    0
#define PWM_INVERTER_CHANNEL_LEFT_HIGH  2
#define PWM_INVERTER_CHANNEL_LEFT_LOW   4

#define PWM_INVERTER_RIGHT_INDEX  3
#define PWM_INVERTER_CHANNEL_RIGHT_HIGH  2
#define PWM_INVERTER_CHANNEL_RIGHT_LOW   4

#define PWM_Q_MEASURE_INDEX       13   //actually using PWM3, reused with PWM_INVERTER_RIGHT_INDEX
#define PWM_Q_MEASURE_CHANNEL_TRIGGER    0

#define PWM_D_DCDC_BUCK_INDEX      0
#define PWM_D_DCDC_CHANNLE_BUCK_HIGH    0

#define PWM_D_DCDC_BOOST_INDEX     2
#define PWM_D_DCDC_CHANNLE_BOOST_LOW    0

#define PWM_D_DCDC_CLK      200000UL

typedef enum
{
	PWM_MODE_NORMAL = 0,
	PWM_MODE_COUNT
}PWM_MODE_E;

void PWM_SetFreq(uint8 byId, uint32 dwPeriod);
void PWM_SetChannelPairs(uint8 byId, uint8 byChannel, uint32 dwStartPeriod, uint32 dwStopPeriod);
void PWM_SetChannelDuty(uint8 byId, uint8 byChannel, uint32 dwPeriod);
void PWM_Start(uint8 byId);
void PWM_Stop(uint8 byId);
void PWM_ClearChannelFlags(uint8 byId, uint8 byChannel);
boolean PWM_CheckChannelFlags(uint8 byId, uint8 byChannel);
void PWM_EnableTrigger(uint8 byId, uint8 byChannel, uint8 byIsEn);
boolean PWM_CheckOverFlowFlag(uint8 byId);
void PWM_ClearOverFlowFlag(uint8 byId);
boolean PWM_CheckRunning(uint8 byId);
void PWM_Init(uint16 wDeadTimeNs);
void PWM_SetMode(uint8 byId, PWM_MODE_E mode);
void PWM_FracOn(uint8 byId, uint8 byIsEn);


//Optimization for speed performance
#define PWM_Update_D_DCDC_Channels(buckDuty, boostDuty) \
		PWMA_SM0FRACVAL3 = buckDuty<<11; \
		PWMA_SM0VAL3 = buckDuty>>5; \
		PWMA_SM2FRACVAL2 = boostDuty<<11; \
		PWMA_SM2VAL2 = boostDuty>>5; \
		PWMA_MCTRL |= (PWMA_MCTRL_LDOK_0|PWMA_MCTRL_LDOK_2);

#define PWM_UpdateFreq(pwmIndex, period) \
		if(PWM_INVERTER_LEFT_INDEX == pwmIndex) \
		{\
			PWMA_SM1INIT = 0;\
			PWMA_SM1VAL1 = (period >> 5) - 1;\
			PWMA_SM1FRACVAL1 = (period & 0x1f) << 11;\
		}\
        else if(PWM_INVERTER_RIGHT_INDEX == pwmIndex) \
        {\
			PWMA_SM3INIT = 0;\
			PWMA_SM3VAL1 = (period >> 5) - 1;\
			PWMA_SM3FRACVAL1 = (period & 0x1f) << 11;\
		}

#define PWM_UpdateChannelPairs(pwmIndex, pwmChIndex, pwmHH, pwmHL) \
		if( (PWM_INVERTER_LEFT_INDEX == pwmIndex) && (PWM_INVERTER_CHANNEL_LEFT_HIGH == pwmChIndex) )\
		{\
			PWMA_SM1VAL2 = (pwmLHH >> 5);\
			PWMA_SM1FRACVAL2 = (pwmLHH & 0x1f) << 11;\
			PWMA_SM1VAL3 = (pwmLHL >> 5);\
			PWMA_SM1FRACVAL3 = (pwmLHL & 0x1f) << 11;\
		}\
		if( (PWM_INVERTER_LEFT_INDEX == pwmIndex) && (PWM_INVERTER_CHANNEL_LEFT_LOW == pwmChIndex) )\
		{\
			PWMA_SM1VAL4 = (pwmLLH >> 5);\
			PWMA_SM1FRACVAL4 = (pwmLLH & 0x1f) << 11;\
			PWMA_SM1VAL5 = (pwmLLL >> 5);\
			PWMA_SM1FRACVAL5 = (pwmLLL & 0x1f) << 11;\
		}\
		if( (PWM_INVERTER_RIGHT_INDEX == pwmIndex) && (PWM_INVERTER_CHANNEL_RIGHT_HIGH == pwmChIndex) )\
		{\
			PWMA_SM3VAL2 = (pwmRHH >> 5);\
			PWMA_SM3FRACVAL2 = (pwmRHH & 0x1f) << 11;\
			PWMA_SM3VAL3 = (pwmRHL >> 5);\
			PWMA_SM3FRACVAL3 = (pwmRHL & 0x1f) << 11;\
		}\
		if( (PWM_INVERTER_RIGHT_INDEX == pwmIndex) && (PWM_INVERTER_CHANNEL_RIGHT_LOW == pwmChIndex) )\
		{\
			PWMA_SM3VAL4 = (pwmRLH >> 5);\
			PWMA_SM3FRACVAL4 = (pwmRLH & 0x1f) << 11;\
			PWMA_SM3VAL5 = (pwmRLL >> 5);\
			PWMA_SM3FRACVAL5 = (pwmRLL & 0x1f) << 11;\
		}

#define PWM_InvalidUpdate(pwmIndex1, pwmIndex2) \
		if( (PWM_INVERTER_LEFT_INDEX == pwmIndex1) && (PWM_INVERTER_RIGHT_INDEX == pwmIndex2) ) \
		{\
			PWMA_MCTRL |= (PWMA_MCTRL_CLDOK_1|PWMA_MCTRL_CLDOK_3);\
		}

#define PWM_ValidUpdate(pwmIndex1, pwmIndex2) \
		if( (PWM_INVERTER_LEFT_INDEX == pwmIndex1) && (PWM_INVERTER_RIGHT_INDEX == pwmIndex2) ) \
		{\
			PWMA_MCTRL |= (PWMA_MCTRL_LDOK_1|PWMA_MCTRL_LDOK_3);\
		}

#endif
