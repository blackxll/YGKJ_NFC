/* *************************
* Copyright 2018 NXP
* All rights reserved.
**************************** */

#ifndef __RW_NDEF_T4T_H__
#define __RW_NDEF_T4T_H__

void RW_NDEF_T4T_Reset(void);
void RW_NDEF_T4T_Read_Next(unsigned char *pCmd, unsigned short Cmd_size, unsigned char *Rsp, unsigned short *pRsp_size);

#endif
