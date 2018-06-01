/* *************************
* Copyright 2018 NXP
* All rights reserved.
**************************** */

#ifndef __RW_NDEF_T2T_H__
#define __RW_NDEF_T2T_H__

void RW_NDEF_T2T_Reset(void);
void RW_NDEF_T2T_Read_Next(unsigned char *pCmd, unsigned short Cmd_size, unsigned char *Rsp, unsigned short *pRsp_size);
void RW_NDEF_T2T_Write_Next(unsigned char *pCmd, unsigned short Cmd_size, unsigned char *Rsp, unsigned short *pRsp_size);

#endif
