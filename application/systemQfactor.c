/*******************************************************************************
*
* Copyright 2012-2016 Freescale Semiconductor, Inc.
* Copyright 2017~2018 NXP.
* All rights reserved.
*******************************************************************************/
//----------------------
// APPLICATION INCLUDES
//----------------------
#include <math.h>

#include "defines.h"
#include "appCfg.h"
#include "wct_lib.h"

#include "systemTimers.h"
#include "systemDebug.h"
#include "hal.h"

#include "systemQfactor.h"

#define MEASURE_METHOD_FR   0
#define MEASURE_METHOD_ED   1   //Not implemented yet, could be used for debug purpose
#define Q_MEASURE_METHOD    MEASURE_METHOD_FR

typedef enum
{
	QF_FR_ERROR_NONE                = 0,  // 0
	QF_FR_ERROR_TIMEOUT,				  // 1
	QF_FR_ERROR_RATE_OUT_OF_RANGE,		  // 2
	QF_FR_ERROR_OVER_VOLTAGE,		  	  // 3
	QF_FR_ERROR_NO_OSCILLATION		  	  // 4
}QF_ERROR_E;

QF_ERROR_E gQF_byDBGErrorCode;  //For debug purpose


#if(Q_MEASURE_METHOD == MEASURE_METHOD_FR)

/*
 * Set the parameter, according to HW configuration.
 * Recommend setting example: 
 *    when no object on Tx surface, setting parameter better to let the 
 *    first three oscillation cycle valley of sense circuit saturation.
 */
#define QF_FR_MEASURE_FREQ               		125000UL
#define QF_FR_MEASURE_MINIMUM_RESONANCE_FREQ    (QF_FR_MEASURE_FREQ/5)
#define QF_FR_MEASURE_DUTY               		(LIB_MAX_DUTY_CYCLE*30/100)
#define QF_FR_MEASURE_PERIOD_CYCLE_NUM   		10
#define QF_FR_MEASURE_VALLEY_CYCLE_NUM   		(QF_FR_MEASURE_PERIOD_CYCLE_NUM*2)
#define QF_FR_MEASURE_SCAN_START_POS     		22
#define QF_FR_MEASURE_SCAN_STOP_POS      		23

uint32 gQF_dwFRResonanceFreq[NUM_DEVICES];
uint8  gQF_byFRTriggerPosForValleySearch[NUM_DEVICES];
uint16  gQF_wFREnvolopeData[NUM_DEVICES][QF_FR_MEASURE_VALLEY_CYCLE_NUM];
uint8  gQF_byFRResonanceCycleForPeriodMeasure = QF_FR_MEASURE_PERIOD_CYCLE_NUM;
uint32 gQF_dwFRChargeCapacitorFreq            = QF_FR_MEASURE_FREQ;
uint32 gQF_dwFRChargeCapacitorDuty            = QF_FR_MEASURE_DUTY;
uint32 gQF_dwFRMinimumResonanceFreq           = QF_FR_MEASURE_MINIMUM_RESONANCE_FREQ;

//before call this function
//  1.The coil selector MOSFET should be connect for a while(say 1ms) already
//  2.The power should be stable already
//  3.The coil discharge should be enable already for a while(say 1ms)
QF_MEASURE_RESULT_E QF_QMeasurePrepare(uint8 byDeviceId, uint8 byCoilId)
{
    uint32 sts;
    uint32 resonancePeriod = 0;
    uint32 resonanceCycleCnt = 0;
    QF_MEASURE_RESULT_E  ret = QF_MEASURE_FAILED;
    
    DBG_Assert(NUM_DEVICES <= byDeviceId, 0, byDeviceId);
    
    gQF_byDBGErrorCode = QF_FR_ERROR_NONE;

    //measure free resonance period    
    HAL_ResonanceCircuitSenseOffsetInit(byDeviceId, byCoilId, HAL_DDM_MODE_FR_Q);
    HAL_EnableCoilDischarge(byDeviceId, byCoilId, 0);
    HAL_PrepareChargingResonanceCapacitor(byDeviceId, byCoilId, gQF_dwFRChargeCapacitorFreq);
    HAL_PreapareMeasureResonancePeriod(byDeviceId, byCoilId, gQF_dwFRMinimumResonanceFreq);
    
    sts = HAL_DisableIRQ();
    //the operation below is time critical    
    HAL_ChargeResonanceCapacitor(byDeviceId, byCoilId, gQF_dwFRChargeCapacitorFreq, gQF_dwFRChargeCapacitorDuty);
    HAL_StartMesaureResonancePeriod(byDeviceId, byCoilId);
    
    while(1)
    {
        uint16 tmpPeriod = HAL_CheckResonancePeriod(byDeviceId, byCoilId);
        if(0xFFFF == tmpPeriod)  //means timeout, exit
        {
            gQF_byDBGErrorCode = QF_FR_ERROR_TIMEOUT;
            break;
        }
        else if(0 != tmpPeriod)
        {
            if(resonanceCycleCnt != 0)  //skip the first cycle
            {
                //Accumulate the period for each cycle
                resonancePeriod += tmpPeriod;
            }
            
            resonanceCycleCnt++;
            if(resonanceCycleCnt > gQF_byFRResonanceCycleForPeriodMeasure)
            {
                //Got enough cycles, exit
                break;
            }
        }
    }
    HAL_RestoreIRQ(sts);

    HAL_StopChargingResonanceCapacitor(byDeviceId, byCoilId);
    HAL_StopMeasureResonancePeriod(byDeviceId, byCoilId);
    HAL_EnableCoilDischarge(byDeviceId, byCoilId, 1);
    
    if(resonanceCycleCnt > gQF_byFRResonanceCycleForPeriodMeasure)
    {
        //Calculate the resonance frequency
        uint8 i;
        resonancePeriod /= gQF_byFRResonanceCycleForPeriodMeasure;
        gQF_dwFRResonanceFreq[byDeviceId] = HAL_ConvertPeriodToFreq(byDeviceId, byCoilId, resonancePeriod);
        ret = QF_MEASURE_SUCCESS;
        
        //prepare for the next step of Qfactor measurement
        gQF_byFRTriggerPosForValleySearch[byDeviceId] = QF_FR_MEASURE_SCAN_START_POS;
        for(i=0; i<QF_FR_MEASURE_VALLEY_CYCLE_NUM; i++)
        {
        	gQF_wFREnvolopeData[byDeviceId][i] = 0;
        }
    }
    else
    {
        gQF_byDBGErrorCode = QF_FR_ERROR_NO_OSCILLATION;
        gQF_dwFRResonanceFreq[byDeviceId] = 0;
    }

    return ret;
}

//before call function
//  1.The coil selector MOSFET should be connect for a while(say 1ms) already
//  2.The power should be stable already
//  3.The coil discharge should be enable already for a while(say 1ms)
QF_MEASURE_RESULT_E QF_QMeasure(uint8 byDeviceId, uint8 byCoilId)
{
    uint32 sts;
    uint16 *pData;
    QF_MEASURE_RESULT_E ret = QF_MEASURE_FAILED;
    uint8 result;
    
    DBG_Assert(NUM_DEVICES <= byDeviceId, 0, byDeviceId);

    HAL_EnableCoilDischarge(byDeviceId, byCoilId, 0);
    HAL_PrepareChargingResonanceCapacitor(byDeviceId, byCoilId, gQF_dwFRChargeCapacitorFreq);
    HAL_PreQMeasure(byDeviceId, byCoilId, gQF_dwFRResonanceFreq[byDeviceId], gQF_byFRTriggerPosForValleySearch[byDeviceId]);

    sts = HAL_DisableIRQ();
    //the operation below is time critical    
    HAL_ChargeResonanceCapacitor(byDeviceId, byCoilId, gQF_dwFRChargeCapacitorFreq, gQF_dwFRChargeCapacitorDuty);
    result = HAL_StartQMeasure(byDeviceId, byCoilId);
    HAL_RestoreIRQ(sts);

    if(result)
    {
        while(1)
        {
            uint16 tmp = HAL_CheckQMeasureFinished(byDeviceId, byCoilId, QF_FR_MEASURE_VALLEY_CYCLE_NUM);
            if(0xFFFF == tmp)  //means timeout
            {
                gQF_byDBGErrorCode = QF_FR_ERROR_TIMEOUT;
                break;
            }
            else if(0 != tmp)
            {
                //wish to next round measurement
                ret = QF_MEASURE_CONTINUE;
                break;
            }
        }
    }

    HAL_StopChargingResonanceCapacitor(byDeviceId, byCoilId);
    HAL_StopQMeasure(byDeviceId, byCoilId);
    HAL_EnableCoilDischarge(byDeviceId, byCoilId, 1);
           
    if(QF_MEASURE_FAILED != ret)
    {
        uint8 i;
        pData = HAL_GetQMeasureData(byDeviceId, byCoilId, QF_FR_MEASURE_VALLEY_CYCLE_NUM);
        for(i=0; i<QF_FR_MEASURE_VALLEY_CYCLE_NUM; i++)
        {
            //save the measured data array(maximum value for each array position) to local buffer
            if(gQF_wFREnvolopeData[byDeviceId][i] < pData[i])
            {
                gQF_wFREnvolopeData[byDeviceId][i] = pData[i];
            }
        }
        
        gQF_byFRTriggerPosForValleySearch[byDeviceId]++;
        if(gQF_byFRTriggerPosForValleySearch[byDeviceId] >= QF_FR_MEASURE_SCAN_STOP_POS)
        {
            //finished all measurement, exit
            ret = QF_MEASURE_SUCCESS;
        }
    }
    
    return ret;
}

QF_MEASURE_RESULT_E QF_GetQFactor(uint8 byDeviceId, uint8 byCoilId, uint32 *pFreq, uint32* plcQ)
{
    uint8 validDataStartIndex, validDataEndIndex;
    uint16 offset = HAL_GetQFacotrSenseCircuitADCCntOffset(byDeviceId, byCoilId);
    QF_MEASURE_RESULT_E ret = QF_MEASURE_FAILED;
    uint8 i;
    
    DBG_Assert(NUM_DEVICES <= byDeviceId, 0, byDeviceId);
    
    validDataStartIndex = validDataEndIndex = QF_FR_MEASURE_VALLEY_CYCLE_NUM-1;
    
    for(i=0; i<QF_FR_MEASURE_VALLEY_CYCLE_NUM; i++)
    {
        if( offset >= gQF_wFREnvolopeData[byDeviceId][i] )
        {
            //check the measurement data array. header members may saturate, skip those members
            if( ( offset - gQF_wFREnvolopeData[byDeviceId][i]) > 100)
            {
                validDataStartIndex = i;
                break;
            }
        }
        else
        {
            //measured data error, exit
            validDataStartIndex = QF_FR_MEASURE_VALLEY_CYCLE_NUM;
        	break;
        }
    }
    
    if(validDataStartIndex < 3)
    {
    	//skip the first three samples, due to distortion of sense circuit
        validDataStartIndex = 3;
    }
    
    for(i=validDataStartIndex; i<(QF_FR_MEASURE_VALLEY_CYCLE_NUM-1); i++)
    {
        if( (gQF_wFREnvolopeData[byDeviceId][i] <= gQF_wFREnvolopeData[byDeviceId][i+1]) || 
            (gQF_wFREnvolopeData[byDeviceId][i+1] < 20) )
        {
            validDataEndIndex = i;
            break;
        }
    }
    
    if( (validDataEndIndex-validDataStartIndex) > (QF_FR_MEASURE_VALLEY_CYCLE_NUM/3) )
    {
        //Valid data, calculate the Q factor LC resonance tank
        uint32 rate = 0;
        uint32 tmpRate;
        float32 fRate;
        for(i=validDataStartIndex; i<validDataEndIndex; i++)
        {
            tmpRate = gQF_wFREnvolopeData[byDeviceId][i+1];
            tmpRate *= 10000UL;
            tmpRate /= gQF_wFREnvolopeData[byDeviceId][i];
            rate += tmpRate;
        }
        rate /= (validDataEndIndex-validDataStartIndex);
        
        fRate = rate;
        fRate /= 10000;
        
        //Q factor = -Pi/log(rate)
        //multiply 100 for better resolution.
        if( (fRate < 1) && (fRate > 0) )
        {
            uint32 qlc = (uint32)(-100*PI/log(fRate));
            if(plcQ)
            {
                *plcQ = qlc;
            }
            if(pFreq)
            {
                *pFreq = gQF_dwFRResonanceFreq[byDeviceId];
            }
            ret = QF_MEASURE_SUCCESS;
        }
        else
        {
            gQF_byDBGErrorCode = QF_FR_ERROR_RATE_OUT_OF_RANGE;
        }
    }
    else
    {
        gQF_byDBGErrorCode = QF_FR_ERROR_NO_OSCILLATION;
    }
    return ret;
}

#elif(Q_MEASURE_METHOD == MEASURE_METHOD_ED)
QF_MEASURE_RESULT_E QF_QMeasurePrepare(uint8 deviceId, uint8 byCoilId)
{
	return QF_MEASURE_SUCCESS;
}

QF_MEASURE_RESULT_E QF_QMeasure(uint8 deviceId, uint8 byCoilId)
{
	return QF_MEASURE_SUCCESS;
}

QF_MEASURE_RESULT_E QF_GetQFactor(uint8 deviceId, uint8 byCoilId, uint32 *pdwFreq, uint32* pdwLCQ)
{
    //return fixed value, a fake Qfactor measurement implementation.
	if(pdwFreq)
	{
		*pdwFreq = 80000UL;
	}
	if(pdwLCQ)
	{
		*pdwLCQ  = 3500;
	}
	return QF_MEASURE_SUCCESS;
}

#else

#error None valid Q factor measurement method is selected

#endif
