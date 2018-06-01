/*******************************************************************************
*
* Copyright 2012-2014 Freescale Semiconductor, Inc.
* Copyright 2017~2018 NXP.
* All rights reserved.
*******************************************************************************/

#ifndef __APP_CFG_H__
#define __APP_CFG_H__

//---------------------------- Application Version Defines -----------------------------
#define WCT_SW_VERSION                     0x4000u //x.y.z: 4 bit x, 4 bit y, 8 bit z
#define WCT_HW_VERSION                     0x1000u //x.y.z: 4 bit x, 4 bit y, 8 bit z

#define DEVICE_ID_STRING                   "MPTX-MPA9-WCT1013A"
#define NUM_DEVICES                        1u
#define NUM_COILS_PER_DEVICE               3u

#define NVM_BASE_ADDR                      ( 0x01F000UL << 1 )

#define FREEMASTER_SUPPORTED	           (TRUE)
#define DEBUG_CONSOLE_SUPPORTED            (FALSE)
#define LOW_POWER_MODE_ENABLE			   (FALSE)

#define DIGITAL_DCDC					   (TRUE)

#define CAN_SUPPORTED                      (FALSE)

#define BOOTLOADER_USED					   (FALSE)
#define BOOTLOADER_PARAMS_ADDR			   ( 0x0001F3FCUL << 1 )


#endif
