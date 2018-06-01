/* *************************
* Copyright 2018 NXP
* All rights reserved.
**************************** */

#ifndef __T4T_NDEF_EMU_H__
#define __T4T_NDEF_EMU_H__

void T4T_NDEF_EMU_Reset(void);
void T4T_NDEF_EMU_Next(unsigned char *pCmd, unsigned short Cmd_size, unsigned char *Rsp, unsigned short *pRsp_size, uint8_t *is_ndef_selected);

#endif
