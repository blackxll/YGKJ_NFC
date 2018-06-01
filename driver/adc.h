/*******************************************************************************
*
* Copyright 2012-2016 Freescale Semiconductor, Inc.
* Copyright 2017~2018 NXP.
* All rights reserved.
*******************************************************************************/

#ifndef __ADC_H__
#define __ADC_H__

#define ADC_DMA_INDEX   1
typedef enum
{
	ADC_DMA_CHANNEL_DDM_INDEX     = 0,
	ADC_DMA_CHANNEL_QVOLT_INDEX
}ADC_DMA_CHANNEL_E;

#define ADC_MISC_INDEX  0
typedef enum
{
	ADC_MISC_CHANNEL_VRAIL_INDEX = 0,
	ADC_MISC_CHANNEL_INPUTCURRENT_INDEX,
	ADC_MISC_CHANNEL_VRAILA_INDEX,
	ADC_MISC_CHANNEL_VBAT_INDEX,
	ADC_MISC_CHANNEL_IPEAK_INDEX,
	ADC_MISC_CHANNEL_DUMMY,        //add a dummy channel, for facilitate SW operation
	ADC_MISC_CHANNEL_TEMP1_INDEX,
	ADC_MISC_CHANNEL_TEMP2_INDEX,
	ADC_MISC_CHANNEL_TEMP3_INDEX,
	ADC_MISC_CHANNEL_TEMP4_INDEX,
	ADC_MISC_CHANNEL_NUM
}ADC_MISC_CHANNEL_E;

#define ADC_REF_VOL         3300UL
#define ADC_BITS              12U

void ADC_Init(void);
void ADC_GetResult(uint8 byId, uint16 *pwData, uint8 byLen);
uint8 ADC_CheckConvertionFinished(uint8 byId);
uint8 ADC_CheckConvertionOngoing(uint8 byId);
void ADC_ClearConvertionFinishedFlag(uint8 byId);
void ADC_SetChannelIndex(uint8 byId, uint8 byChIndex);
void ADC_ISRAck(uint8 byId);
void ADC_StartConversion(uint8 byId);
uint32 ADC_GetResultRegAddr(uint8 byId);
void ADC_PowerDown(void);
void ADC_PowerUp(void);

//To optimize the speed performance
#define ADC_Read_D_DCDC_VolAdcCnt()    (ADC12_RSLT0>>3)

#endif
