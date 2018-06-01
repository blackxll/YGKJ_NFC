/* *************************
* Copyright 2018 NXP
* All rights reserved.
**************************** */

#ifndef __NFC_DRIVER_CONFIG_H__
#define __NFC_DRIVER_CONFIG_H__

#include "cpu.h"
#include "defines.h"
#include "systemTimers.h"

/***********************************************************************************************
 **	Global macros and definitions
 ***********************************************************************************************/
#define NXPNCI_I2C_ADDR		  					0x28U

#define HIGH              	  					1
#define LOW               	  					0

#define ERROR									0
#define SUCCESS									1

#define SET_VEN_HIGH							(GPIOA_DR |= (1<<2))
#define SET_VEN_LOW								(GPIOA_DR &= ~(1<<2))
#define GET_IRQ_VALUE							(GPIOB_DR & (1<<4))

#define SLEEP( ms )								ST_WaitMs( ms )

//For test
#define LED_RED_ON								(GPIOE_DR &= ~0x01)
#define LED_RED_OFF								(GPIOE_DR |= 0x01)
#define LED_RED_TOGGLE		    				(GPIOE_DR ^= 0x01)
/***********************************************************************************************
 **	Global variables
 ***********************************************************************************************/

/***********************************************************************************************
 **	Global function prototypes
 ***********************************************************************************************/

#endif // __NFC_DRIVER_CONFIG_H__

/***********************************************************************************************
 **                            End Of File
 ***********************************************************************************************/
