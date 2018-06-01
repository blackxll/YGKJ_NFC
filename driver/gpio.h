/*******************************************************************************
*
* Copyright 2012-2014 Freescale Semiconductor, Inc.
* Copyright 2017~2018 NXP.
* All rights reserved.
*******************************************************************************/

#ifndef __GPIO_H__
#define __GPIO_H__

typedef enum
{
	GPIO_MODE_GP = 0,
	GPIO_MODE_PER
}GPIO_MODE_E;

//make below enum same as HAL_TP_TYPE_E
typedef enum
{
	GPIO_SET = 0,
	GPIO_CLR = 1,
	GPIO_TOG = 2
}GPIO_OUT_E;

typedef enum
{
	GPIO_IGN = 0,
	GPIO_NFC_WAKE,
	GPIO_NFC_nIRQ,
#if !DIGITAL_DCDC
	GPIO_DCDC_PG,
	GPIO_DCDC_EN,
#endif
	GPIO_RAIL_DIS,
	GPIO_COIL1_EN,
	GPIO_COIL2_EN,
	GPIO_LED1,
	GPIO_VBAT_SW,
	GPIO_ANALOG_PING_POWER_EN,
	GPIO_LED2,
	GPIO_COIL0_EN,
	GPIO_PWM_LEFT_LOW,
	GPIO_PWM_RIGHT_HIGH,
	GPIO_PWM_RIGHT_LOW,	
	GPIO_GAIN_SWITCH,
	GPIO_CAN_DEBUG_STB,
	GPIO_AUXP_CTRL,
	GPIO_CS_NFC,
	GPIO_FSKTOGGLE,
	GPIO_DDMTOGGLE,
	GPIO_MAX
}GPIO_ID_E;

void GPIO_Init(void);
void GPIO_SetMode(GPIO_ID_E byId, GPIO_MODE_E mode, boolean bOut);
void GPIO_SetOutput(GPIO_ID_E byId, GPIO_OUT_E output);
boolean GPIO_GetInput(GPIO_ID_E byId);

#endif
