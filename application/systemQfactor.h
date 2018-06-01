/*******************************************************************************
*
* Copyright 2012-2016 Freescale Semiconductor, Inc.
* Copyright 2017~2018 NXP.
* All rights reserved.
*******************************************************************************/
#ifndef __SYSTEM_Q_FACTOR_H__
#define __SYSTEM_Q_FACTOR_H__

typedef enum
{
    QF_MEASURE_FAILED = 0,
    QF_MEASURE_CONTINUE,
    QF_MEASURE_SUCCESS
}QF_MEASURE_RESULT_E;

//-----------
// FUNCTIONS
//-----------
QF_MEASURE_RESULT_E QF_QMeasurePrepare(uint8 deviceId, uint8 byCoilId);
QF_MEASURE_RESULT_E QF_QMeasure(uint8 deviceId, uint8 byCoilId);
QF_MEASURE_RESULT_E QF_GetQFactor(uint8 deviceId, uint8 byCoilId, uint32 *pdwFreq, uint32* pdwLCQ);

#endif
