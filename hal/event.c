/*******************************************************************************
 *
 * Copyright 2012-2016 Freescale Semiconductor, Inc.
 * Copyright 2017~2018 NXP.
 * All rights reserved.
 *******************************************************************************/


#include "defines.h"
#include "cpu.h"

#include "appCfg.h"
#include "WCT_Lib.h"

#include "freemaster.h"
#include "freemaster_cfg.h"

#include "systemTimers.h"
#include "systemDebug.h"

#include "adc.h"
#include "timer.h"
#include "dma.h"
#include "qsci.h"
#include "FlexCAN.h"


#include "hal.h"

#include "event.h"

volatile uint8 gDBG_byIntNum;
volatile uint8 gDBG_byIntPri;

#pragma interrupt alignsp saveall
void MWCT10xx_ISR_UNDEFINED(void)
{
    //Save the ISR number and priority to global variables, for debug purpose
    gDBG_byIntNum = (INTC_CTRL >> 6) & 0x7F;
    gDBG_byIntPri = (INTC_CTRL >> 13) & 0x03;
    while(1) {}
}
#pragma interrupt off

#pragma interrupt alignsp saveall
void MWCT10xx_ISR_TMRA_0(void)
{
    //DDM ISR
	static uint8 fsk_cnt;
	
	TMR_ISRAck(TMR_DDM_FRQ_INDEX);  //clear timer interrupt flag
	
	//HAL_SetTP(0, HAL_TP_TYPE_SET);
	
	if (FSK_IsBusy(0))
	{
	    //FSK is sending data, ignore DDM
		fsk_cnt++;
		if (fsk_cnt >= 256/(RECORD_BUF_LEN/2/2))
		{
		    //FSK ISR should be called every 256 PWM cycles.
			fsk_cnt = 0;
			FSK_ISR(0);
		}
	}
	else
	{
		WCT_CommAnalyse(0); 
		fsk_cnt = 256/(RECORD_BUF_LEN/2/2) - 1;  //Make sure FSK_ISR could be run at next interrupt
	}
	
	//HAL_SetTP(0, HAL_TP_TYPE_CLR);
}
#pragma interrupt off

#pragma interrupt saveall
void MWCT10xx_ISR_DMA0(void)
{
	DMA_ISRAck(DMA_ADC_INDEX);	
	DBG_Assert(1, 0, 0);  //we should not reach here, must got error
}
#pragma interrupt off

#pragma interrupt saveall
void MWCT10xx_ISR_TMRA_2(void)
{
    //1ms tick ISR
	TMR_ISRAck(TMR_TICK_INDEX);
	ST_TimerUpdate();
#if DIGITAL_DCDC
	HAL_MiscADCRead();  // for D_DCDC, ADCA starts sample every D_DCDC PID control routine
#else
	ADC_StartConversion(ADC_MISC_INDEX);
#endif
}
#pragma interrupt off


#pragma interrupt alignsp saveall
void MWCT10xx_ISR_TX_REG(void)
{
#if FREEMASTER_SUPPORTED && FMSTR_USE_JTAG
	FMSTR_Isr();
#else
	DBG_Assert(1, 0, 0);
#endif
}
#pragma interrupt off

#pragma interrupt alignsp saveall
void MWCT10xx_ISR_RX_REG(void)
{
#if FREEMASTER_SUPPORTED && FMSTR_USE_JTAG
	FMSTR_Isr();
#else
	DBG_Assert(1, 0, 0);
#endif
}
#pragma interrupt off

#pragma interrupt alignsp saveall
void MWCT10xx_ISR_QSCI0_TDRE(void)
{
#if FREEMASTER_SUPPORTED && FMSTR_USE_SCI && (FMSTR_SCI_BASE == 0xE080)
	FMSTR_Isr();
#endif
	
#if DEBUG_CONSOLE_SUPPORTED && (QSCI_CONSOLE_INDEX == 0)
	QSCI_ISRAck(QSCI_CONSOLE_INDEX);
	HAL_PrintCharISRAck();
#endif
}
#pragma interrupt off

#pragma interrupt alignsp saveall
void MWCT10xx_ISR_QSCI0_RCV(void)
{
#if FREEMASTER_SUPPORTED && FMSTR_USE_SCI && (FMSTR_SCI_BASE == 0xE080)
	FMSTR_Isr();
#endif
}
#pragma interrupt off

#pragma interrupt alignsp saveall
void MWCT10xx_ISR_PIT1_ROLLOVER(void)
{
#if LOW_POWER_MODE_ENABLE
    TMR_ISRAck(TMR_LOW_POWER_INDEX);
    TMR_Enable(TMR_LOW_POWER_INDEX, 0);
#endif
}
#pragma interrupt off

#if DIGITAL_DCDC
#pragma interrupt saveall
void MWCT10xx_ISR_TMRA_1(void)
{	
	TMR_ISRAck(TMR_D_DCDC_INDEX);
	HAL_D_DCDC_Adjust(0);
	HAL_D_DCDC_ADCHandler(0);
}
#pragma interrupt off

#else

#pragma interrupt alignsp saveall
void MWCT10xx_ISR_QSCI1_TDRE(void)
{
#if FREEMASTER_SUPPORTED && FMSTR_USE_SCI && (FMSTR_SCI_BASE == 0xE090)
	FMSTR_Isr();
#endif
	
#if DEBUG_CONSOLE_SUPPORTED && (QSCI_CONSOLE_INDEX == 1)
	QSCI_ISRAck(QSCI_CONSOLE_INDEX);
	HAL_PrintCharISRAck();
#endif
}
#pragma interrupt off

#pragma interrupt alignsp saveall
void MWCT10xx_ISR_QSCI1_RCV(void)
{
#if FREEMASTER_SUPPORTED && FMSTR_USE_SCI && (FMSTR_SCI_BASE == 0xE090)
	FMSTR_Isr();
#endif
}
#pragma interrupt off

#pragma interrupt alignsp saveall
void MWCT10xx_ISR_ADC_CC0(void)
{
    ADC_ISRAck(ADC_MISC_INDEX);
    HAL_MiscADCISRAck(0);        //For not D_DCDC, ADCA starts to sample every 1ms
}
#pragma interrupt off

#endif

#if CAN_SUPPORTED
#pragma interrupt alignsp saveall
void MWCT10xx_ISR_CAN_MB(void)
{
	FlexCAN_MB_ISRAck();
}
#pragma interrupt off

#pragma interrupt alignsp saveall
void MWCT10xx_ISR_CAN_BUSOFF(void)
{
	FlexCAN_BusOff_ISRAck();
}
#pragma interrupt off

#pragma interrupt alignsp saveall
void MWCT10xx_ISR_CAN_ERROR(void)
{
	FlexCAN_Error_ISRAck();
}
#pragma interrupt off

#pragma interrupt alignsp saveall
void MWCT10xx_ISR_CAN_TX_WARN(void)
{
	FlexCAN_TxWarning_ISRAck();
}
#pragma interrupt off

#pragma interrupt alignsp saveall
void MWCT10xx_ISR_CAN_RX_WARN(void)
{
	FlexCAN_RxWarning_ISRAck();
}
#pragma interrupt off

#pragma interrupt alignsp saveall
void MWCT10xx_ISR_CAN_WAKEUP(void)
{
	FlexCAN_Wakeup_ISRAck();
}
#pragma interrupt off
#endif
