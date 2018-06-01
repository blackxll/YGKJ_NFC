/*******************************************************************************
*
* Copyright 2012-2014 Freescale Semiconductor, Inc.
* Copyright 2017~2018 NXP.
* All rights reserved.
*******************************************************************************/
#ifndef	__FLEXCAN_H__
#define __FLEXCAN_H__

// Error code
typedef enum
{
	FLEXCAN_ERROR_SUCCESS            = 0,
	FLEXCAN_ERROR_CODE_BASE          = 0x100,
	FLEXCAN_ERROR_INVALID_BAUD       = (FLEXCAN_ERROR_CODE_BASE),
	FLEXCAN_ERROR_PENDING_TX         = (FLEXCAN_ERROR_CODE_BASE+1),
	FLEXCAN_ERROR_TX_DONE            = (FLEXCAN_ERROR_CODE_BASE+2),
	FLEXCAN_ERROR_TX_ERR             = (FLEXCAN_ERROR_CODE_BASE+3),
	FLEXCAN_ERROR_PARAM_OUT_OF_RANGE = (FLEXCAN_ERROR_CODE_BASE+4)
}FLEXCAN_ERROR_E;

// Definition of Receive MBs index
#define	FLEXCAN_RX_MB_START	8 /* start # of Rx MB */                              
#define	FLEXCAN_RX_MB_END	8 /* end # of Rx MB */
                              
// Definition of Transmit MBs index
#define FLEXCAN_TX_MB_START	9u //(FLEXCAN_RX_MB_END+1)
#define	FLEXCAN_TX_MB_END	(NUMBER_OF_MB-1)

void FlexCAN_Init(void);
FLEXCAN_ERROR_E FlexCAN_Open(uint32  baudrateKHz);
void FlexCAN_Start(void);
void FlexCAN_Stop(void);

uint16 FlexCAN_ConfigureRxFIFO_IDTable(uint32 idList[], uint16 idListNo);
uint16 FlexCAN_ConfigureRxMBMask(uint16 iMB, uint32 mask);
uint16 FlexCAN_ConfigureRxFIFOFilterTabElementMask(uint16 iTabElement, uint32 mask);
uint16 FlexCAN_ConfigureRxFIFOGlobalMask(uint32 mask);

void FlexCAN_Write(uint32 msgID, uint8 prio,uint8 bBytes[], uint8 iNoBytes);
void FlexCAN_PrepareRxCANMB(uint16 iMB, uint32 id);
uint16 FlexCAN_Read(uint16 iMailBoxQue, uint32 *id, uint8 data[]);
uint16 FlexCAN_Read_RxFIFOQueue(uint32 *id, uint8 data[]);

void FlexCAN_EnableMB_Interrupt(uint16 iMB);
void FlexCAN_DisableMB_Interrupt(uint16 iMB);
void FlexCAN_EnableBusOff_Interrupt(void);
void FlexCAN_DisableBusOff_Interrupt(void);
void FlexCAN_EnableError_Interrupt(void);
void FlexCAN_DisableError_Interrupt(void);
void FlexCAN_EnableTxWarning_Interrupt(void);
void FlexCAN_DisableTxWarning_Interrupt(void);
void FlexCAN_EnableRxWarning_Interrupt(void);
void FlexCAN_DisableRxWarning_Interrupt(void);
void FlexCAN_EnableWakeup_Interrupt(void);
void FlexCAN_DisableWakeup_Interrupt(void);

void FlexCAN_MB_ISRAck(void);
void FlexCAN_BusOff_ISRAck(void);
void FlexCAN_Error_ISRAck(void);
void FlexCAN_TxWarning_ISRAck(void);
void FlexCAN_RxWarning_ISRAck(void);
void FlexCAN_Wakeup_ISRAck(void);

#endif
