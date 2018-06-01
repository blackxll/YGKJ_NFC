/*******************************************************************************
 *
 * Copyright 2012-2016 Freescale Semiconductor, Inc.
 * Copyright 2017~2018 NXP.
 * All rights reserved.
 *******************************************************************************/

#include "defines.h"

#include "appCfg.h"
#include "wct_lib.h"
#include "NVM.h"

#include "freemaster.h"

#include "hal.h"

#include "systemTask.h"
#include "systemPrint.h"
#include "systemDisplay.h"
#include "systemTimers.h"
#include "systemCallback.h"
#include "systemDebug.h"
#include "systemProtection.h"

#ifdef NFC_STACK_USED
#include "nfcTask.h"
#include "nfc_driver_config.h"
#define LPCD_POWER_DOWN_TIME_MS 200
#define QI_POLLING_TIME_MS 5000

/* FreeMASTER variables - information */
uint32_t FMSTR_PcbVersion;
uint32_t FMSTR_FwVersion;
uint32_t FMSTR_QiLibVersion;
uint8_t FMSTR_AppState;
uint8_t FMSTR_WchState;

uint8_t cntr = 0;
uint16_t start_time;
/* WCT init status flag: 0 - WCT stop; 1 - WCT run */
uint8_t wct_init = 0;
RX_CHARGING_STATUS rx_status;

extern unsigned char mode;
#endif

LIB_Version glib_version;
static void VERSION_Print(void)
{
    //Device Name
    APP_PrintString("\r\n");
    APP_PrintString(DEVICE_ID_STRING);
    
    //Application SW version
    APP_PrintString("\n\rWCT MP App SW Version: ");
    APP_PrintDoubleWordValue((WCT_SW_VERSION & 0xf000u)>>12);
    APP_PrintString(".");
    APP_PrintDoubleWordValue((WCT_SW_VERSION & 0x0f00u)>>8);
    APP_PrintString(".");
    APP_PrintDoubleWordValue(WCT_SW_VERSION & 0x00ffu);

    //Application HW version
    APP_PrintString("\n\rWCT MP HW Version: ");
    APP_PrintDoubleWordValue((WCT_HW_VERSION & 0xf000u)>>12);
    APP_PrintString(".");
    APP_PrintDoubleWordValue((WCT_HW_VERSION & 0x0f00u)>>8);
    APP_PrintString(".");
    APP_PrintDoubleWordValue(WCT_HW_VERSION & 0x00ffu);

    //SW Library version
    WCT_GetLibVer(&glib_version);
    APP_PrintString("\n\rWTC MP Lib Version: ");
    APP_PrintDoubleWordValue(glib_version.bMajorVersion);
    APP_PrintChar('.');
    APP_PrintDoubleWordValue(glib_version.bMinorVersion);
    APP_PrintChar('.');
    APP_PrintDoubleWordValue(glib_version.bSubVersion);
    
    APP_PrintString("\r\n");
}




#ifndef NFC_STACK_USED

void main(void) 
{
    HAL_Init();
    
    //Wait to build the battery/board input voltage
    ST_WaitMs(100);

    VERSION_Print();

#if FREEMASTER_SUPPORTED
    FMSTR_Init();
#endif
    DISP_Init();
    
    if(FALSE == NVM_ReadParams(1u))
    {
        uint8 i;
        for(i=0; i<NUM_DEVICES; i++)
        {
            App_SetErr(APP_NVM_ERROR);
            // For APP_NVM_ERROR, doesn't support LED blink display,
            // since no chance to call DISP_Handler
            WCT_UpdateDevUsrIndication(i);
        }
        WCT_Stop();
        DBG_Assert(1, 0, 0);
    }
    else
    {
        WCT_Init();
    }

    ST_WaitMs(50);
    
    while (1)
    {
        AppTask_Poll();
        
#if CAN_SUPPORTED
        HAL_CANTest();
#endif
        
#if FREEMASTER_SUPPORTED
        FMSTR_Poll();
#endif
    }
}

#else

void main(void)
{
    HAL_Init();
    
    //Wait to build the battery voltage 
    ST_WaitMs(100);

    VERSION_Print();

#if FREEMASTER_SUPPORTED
    FMSTR_Init();
#endif
    DISP_Init();

    if(FALSE == NVM_ReadParams(1u))
    {
        uint8 i;
        for(i=0; i<NUM_DEVICES; i++)
        {
            App_SetErr(APP_NVM_ERROR);
            // For APP_NVM_ERROR, doesn't support LED blink display,
            // since no chance to call DISP_Handler
            WCT_UpdateDevUsrIndication(i);
        }
        WCT_Stop();
        wct_init = 0;
        DBG_Assert(1, 0, 0);
    }

    ST_WaitMs(50);
    // Initialise FreeMASTER variables for NFC
    FMSTR_FwVersion     = 0x00010001;
    WCT_GetLibVer(&glib_version);
    FMSTR_QiLibVersion  = ((uint32_t) glib_version.bMajorVersion << 16) | ((uint32_t) glib_version.bMinorVersion << 8) | ((uint32_t) glib_version.bSubVersion << 0);//( (lib_version.bMajorVersion << 16) | (lib_version.bMinorVersion << 8) | (lib_version.bSubVersion) ) ;
    FMSTR_PcbVersion    = (uint32_t) WCT_HW_VERSION;    
    /* Default app state (no FMSTR connected) is WCH only */
    FMSTR_AppState      = ST_WCH;
    FMSTR_WchState      = 0;
    FMSTR_TagDetected   = 0;
    FMSTR_NfcTechnology = 0;
    
    NDEFMessage_Init();
    init_nfc(mode);
    
    start_time = ST_GetTimerTick();
    lpcd_calib();
    
    while (1)
    {
        FMSTR_WchState = WCT_GetRxStatus(0);
        AppTask_Poll();
        /* Start WCh without NFC after power-on.
         * In all other modes turn off charging */ 
        if(FMSTR_AppState == ST_WCH)
        {
            if(wct_init == 0) 
            {
                WCT_Init();
                wct_init = 1;                
            }
        } else if(FMSTR_AppState != ST_WCH_NO_NFC)
        {
            if(wct_init == 1)
            {
                WCT_Stop();
                wct_init = 0;
            }
        }
        if(FMSTR_AppState == ST_NFC_WCH)
        {
            /* LPCD polling only in specific period */
            if( ((ST_GetTimerTick() - start_time) > LPCD_POWER_DOWN_TIME_MS) )
            {                    
                start_time = ST_GetTimerTick();
                if(lpcd_wakeup() == TRUE)
                {
                    FMSTR_AppState = ST_NFC_WCH_TAG;
                }
            }
        }
        if(FMSTR_AppState == ST_NFC_WCH_TAG) 
        {
            init_nfc(mode);
            task_nfc();    
            if( (FMSTR_TagDetected == 0) )
            {
                FMSTR_AppState = ST_WCH_NO_NFC;
            } 
            start_time = ST_GetTimerTick();
        }
        if(FMSTR_AppState == ST_WCH_NO_NFC)
        {
            if(wct_init == 0)
            {
                WCT_Init();
                wct_init = 1;
            }
            /* Qi polling only in specific period */
            if( (wct_init == 1) && (FMSTR_WchState != RX_CHARGING) &&
                     ( (ST_GetTimerTick() - start_time) > QI_POLLING_TIME_MS) )
            {
                WCT_Stop();
                wct_init = 0;
                FMSTR_AppState = ST_NFC_WCH;
                lpcd_calib();
            }
        }
        /* handle all other NFC modes to evaluate capabilities of NCx3340 */
        if( (FMSTR_AppState == ST_CARD_DETECTION) || (FMSTR_AppState == ST_BT_P2P) || (FMSTR_AppState == ST_BT_P2P_TRANSMIT) ||
            (FMSTR_AppState == ST_CE) || (FMSTR_AppState == ST_CE_TRANSMIT) || (FMSTR_AppState == ST_APDU_EXCHANGE) )
        {
            if(wct_init == 1)
            {
                WCT_Stop();
                wct_init = 0;                
            }
            NDEFMessage_Init();        
            init_nfc(mode);
            task_nfc();    
        }

        
#if FREEMASTER_SUPPORTED
        FMSTR_Poll();
#endif
    }
}

#endif



