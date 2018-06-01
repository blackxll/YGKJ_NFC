/*******************************************************************************
*
* Copyright 2012-2014 Freescale Semiconductor, Inc.
* Copyright 2017~2018 NXP.
* All rights reserved.
*******************************************************************************/

#ifndef __I2C_H__
#define __I2C_H__ 

void I2C_Init(void);
void I2C_Enable(uint8 byIsEn);
boolean I2C_Read(uint8 add, uint8 *pRsp, uint16 Rsp_size);
boolean I2C_Write(uint8 add, uint8 *pCmd, uint16 Cmd_size);

#endif
