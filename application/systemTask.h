/*******************************************************************************
*
* Copyright 2012-2015 Freescale Semiconductor, Inc.
* Copyright 2017~2018 NXP.
* All rights reserved.
*******************************************************************************/

#ifndef __SYSTEMTASK_H__
#define __SYSTEMTASK_H__

typedef enum
{
    APP_SUCCESS = 0,
    APP_NVM_ERROR
} APP_ERROR_TYPE;

void App_SetErr(APP_ERROR_TYPE err);
APP_ERROR_TYPE App_GetErr(void);
void AppTask_Poll(void);


#endif
