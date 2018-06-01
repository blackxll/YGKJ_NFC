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

#include "gpio.h"

typedef struct
{
	uint16* pwPort;
	uint8  bit;
}GPIO_CTRL_T;

GPIO_CTRL_T gGPIO_Ctrl[] =
{
	{(uint16*)FGPIOA_BASE, 1},  //GPIO_IGN
	{(uint16*)FGPIOA_BASE, 2},  //GPIO_NFC_WAKE	
	{(uint16*)FGPIOB_BASE, 4},  //GPIO_NFC_nIRQ
#if !DIGITAL_DCDC
	{(uint16*)FGPIOE_BASE, 5},  //GPIO_DCDC_PG
	{(uint16*)FGPIOB_BASE, 6},  //GPIO_DCDC_EN
#endif
	{(uint16*)FGPIOF_BASE, 1},  //GPIO_RAIL_DIS
	{(uint16*)FGPIOF_BASE, 2},  //GPIO_COIL1_EN
	{(uint16*)FGPIOF_BASE, 3},  //GPIO_COIL2_EN
	{(uint16*)FGPIOE_BASE, 0},  //GPIO_LED1
	{(uint16*)FGPIOF_BASE, 7},  //GPIO_VBAT_SW
	{(uint16*)FGPIOF_BASE, 6},  //GPIO_ANALOG_PING_POWER_EN
	{(uint16*)FGPIOE_BASE, 1},  //GPIO_LED2
	{(uint16*)FGPIOC_BASE, 13}, //GPIO_COIL0_EN
	{(uint16*)FGPIOE_BASE, 2},  //GPIO_PWM_LEFT_LOW
	{(uint16*)FGPIOE_BASE, 7},  //GPIO_PWM_RIGHT_HIGH
	{(uint16*)FGPIOE_BASE, 6},  //GPIO_PWM_RIGHT_LOW
	{(uint16*)FGPIOF_BASE, 0},  //GPIO_GAIN_SWITCH
	{(uint16*)FGPIOC_BASE, 3},  //GPIO_CAN_DEBUG_STB
	{(uint16*)FGPIOC_BASE, 6},  //GPIO_AUXP_CTRL
	{(uint16*)FGPIOC_BASE, 7},  //GPIO_CS_NFC
	{(uint16*)FGPIOC_BASE, 8},  //GPIO_FSKTOGGLE
	{(uint16*)FGPIOC_BASE, 9}   //GPIO_DDMTOGGLE
};

void GPIO_Init(void)
{
	GPIOA_PUR    = 0x0004U;
	GPIOA_DDR    = 0x0004U;
	GPIOA_PER    = 0x00F9U;
	GPIOA_PPMODE = 0x00FFU;               
	
	GPIOB_PUR    = 0x0034U;
	GPIOB_DDR    = 0x00CCU;
	GPIOB_PER    = 0x00AFU;
	GPIOB_PPMODE = 0x00FFU;
	GPIOB_PUS    = 0x0030U;
	
	GPIOC_PUR    = 0x0000U;
	GPIOC_DR     = 0x0000U;
	GPIOC_DDR    = 0xE7E8U;
	GPIOC_PER    = 0xD837U;
	
	GPIOD_PUR    = 0x0000U;
	GPIOD_PER    = 0x001FU;
	GPIOD_PPMODE = 0x00FFU;
	GPIOD_DRIVE  = 0x0000U;
	
	GPIOE_PUR    = 0x0030U;
	GPIOE_DDR    = 0x000FU;
#if DIGITAL_DCDC
	GPIOE_PER    = 0x00FCU;
#else
	GPIOE_PER    = 0x00CCU;
#endif
	GPIOE_PPMODE = 0x00FFU;

	GPIOF_PUR    = 0x00E0U;
	GPIOF_DDR    = 0x00EFU;
	GPIOF_PER    = 0x0130U;
	GPIOF_PPMODE = 0x00FFU;
}

void GPIO_SetMode(GPIO_ID_E byId, GPIO_MODE_E mode, boolean bOut)
{
	uint16* ptr;
	
	DBG_Assert(byId >= GPIO_MAX, 0, byId);
	
	ptr = gGPIO_Ctrl[byId].pwPort;

	if(GPIO_MODE_PER == mode)
	{
		//GPIOx_PER
		ptr[3] |= (1U<<gGPIO_Ctrl[byId].bit);
	}
	else
	{
		//GPIOx_PER
		ptr[3] &= ~(1U<<gGPIO_Ctrl[byId].bit);
		if(bOut)
		{
			//GPIOx_DDR
			ptr[2] |= (1U<<gGPIO_Ctrl[byId].bit);
		}
		else
		{
			//GPIOx_DDR
			ptr[2] &= ~(1U<<gGPIO_Ctrl[byId].bit);
		}
	}
}

void GPIO_SetOutput(GPIO_ID_E byId, GPIO_OUT_E output)
{
	uint16* ptr;
		
	DBG_Assert(byId >= GPIO_MAX, 0, byId);
	
	ptr = gGPIO_Ctrl[byId].pwPort;

	if(GPIO_SET == output)
	{
		//GPIOx_DR
		ptr[1] |= (1U<<gGPIO_Ctrl[byId].bit);
	}
	else if(GPIO_CLR == output)
	{
		//GPIOx_DR
		ptr[1] &= ~(1U<<gGPIO_Ctrl[byId].bit);
	}
	else if(GPIO_TOG == output)
	{
		//GPIOx_DR
		ptr[1] ^= (1U<<gGPIO_Ctrl[byId].bit);
	}
}

boolean GPIO_GetInput(GPIO_ID_E byId)
{
	boolean ret = 0;
	uint16* ptr;
		
	DBG_Assert(byId >= GPIO_MAX, 0, byId);
	
	ptr = gGPIO_Ctrl[byId].pwPort;
	
	//GPIOx_DR
	if(ptr[1] & (1U<<gGPIO_Ctrl[byId].bit))
	{
		ret = 1;
	}
	
	return ret;
}
