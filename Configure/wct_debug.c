/*******************************************************************************
 *
 * Copyright 2012-2016 Freescale Semiconductor, Inc.
 * Copyright 2017~2018 NXP.
 * All rights reserved.
 *******************************************************************************/

#include "defines.h"

#include "appCfg.h"
#include "wct_lib.h"
#include "wct_libParams.h"
#include "NVM.h"

#include "systemTimers.h"
#include "systemQfactor.h"
#include "systemDebug.h"

#include "hal.h"

#include "wct_debug.h"


#define WCTTX_DEBUG_BUFFER_SIZE                          20U

//wcttxDebugStatus low bytes
#define WCTTX_DEBUG_CMD_STATE_IDLE                       0U // Sets by PC after command is executed
#define WCTTX_DEBUG_CMD_STATE_RECEIVED                   1U // Sets by PC when new command is send to board
#define WCTTX_DEBUG_CMD_STATE_DONE                       2U // Sets by Board, when command is executed

//wcttxDebugStatus high bytes, board use below values to return the command execution result to PC
#define WCTTX_DEBUG_CMD_STATUS_OK                        0x0000U
#define WCTTX_DEBUG_CMD_STATUS_FAILED                    0x8000U
#define WCTTX_DEBUG_CMD_STATUS_FAILED_FLASHWRITE         0x8100U
#define WCTTX_DEBUG_CMD_STATUS_FAILED_UNCALIBRATED       0x8200U
#define WCTTX_DEBUG_CMD_STATUS_FAILED_INV_PARAM          0x8300U
#define WCTTX_DEBUG_CMD_STATUS_FAILED_INV_COMMAND        0x8400U
#define WCTTX_DEBUG_CMD_STATUS_FAILED_NON_DEBUG_MODE     0x8500U

//wcttxDebugBuffer[0], command type
#define WCTTX_DEBUG_CMD_NVM_FLASH                        0x10U
#define WCTTX_DEBUG_CMD_DEBUG_MODE                       0x12U
#define WCTTX_DEBUG_CMD_GET_INPUT_VOLTAGE                0x13U
#define WCTTX_DEBUG_CMD_RUN_RAILVOLTAGE_CALIBRATION      0x14U
#define WCTTX_DEBUG_CMD_READ_RAIL_VOLTAGE_CONTROL        0x15U
#define WCTTX_DEBUG_CMD_COIL_CONTROL                     0x16U
#define WCTTX_DEBUG_CMD_COIL_DISCHARGE                   0x17U
#define WCTTX_DEBUG_CMD_COIL_DUTY_CYCLE                  0x18U
#define WCTTX_DEBUG_CMD_DAC_COUNTS                       0x1AU
#define WCTTX_DEBUG_CMD_NOISE_ANAL						 0x1CU

//-------------------
// PRIVATE VARIABLES
//-------------------
#pragma define_section wct_dbg_buf ".wct_debug_buffer" RW
#pragma section wct_dbg_buf begin
uint8 wcttxDebugBuffer[WCTTX_DEBUG_BUFFER_SIZE];
#pragma section wct_dbg_buf end

static uint16 wcttxDebugStatus = 0U;

boolean gWDB_bWctDebugMode;
uint8   gWDB_byDeviceId = 0;
uint32  gWDB_dwFreq, gWDB_dwDuty = LIB_MAX_DUTY_CYCLE, gWDB_dwPhase = LIB_MAX_PHASE_SHIFT;

static uint8 WBG_GetMultiParameters(uint8 byDeviceId, uint8* pBuff)
{
	uint8 i;
	uint8 byCoilId = pBuff[3];  //pBuff[3] is coilId

	uint32 dwRail;
	uint32 dwInputCurrent;
	uint32 dwCoilCurrent;
	uint32 dwInputPower;
	
	sint16 swInternalLoss = 0;
	
	uint8  ret = WCTTX_DEBUG_CMD_STATUS_OK;
	
	if(byCoilId < NUM_COILS_PER_DEVICE)
	{
		if( (TX_POWER_TRANSFER == WCT_GetTxStatus(byDeviceId)) && (0 == gWDB_bWctDebugMode) )
		{
		    //Normally wireless charging
			RUNTIME_PARAMS* pRunTime = &gRunTimeParams[byDeviceId];
			dwRail          = pRunTime->wCalculatedInputVoltage;
			dwInputCurrent  = pRunTime->wCalculatedInputCurrent;
			dwCoilCurrent   = pRunTime->wCalculatedCoilCurrent;
			dwInputPower    = pRunTime->dwCalculatedInputPower;
			swInternalLoss  = pRunTime->swTxInternalLoss;
		}
		else
		{
		    //Debug mode
			dwRail         = 0;
			dwInputCurrent = 0;
			dwCoilCurrent  = 0;
			for(i=0; i<4; i++)
			{
				dwRail           += HAL_GetRailVoltage(byDeviceId);
				dwInputCurrent   += HAL_GetInputCurrent(byDeviceId);
				
				if(gWDB_bWctDebugMode && HAL_CheckInterverRunning(byDeviceId))
				{
				    //Must be in debug mode, and coil is ON (PWM running)
				    //gWDB_dwFreq is assigned a value when set coil ON
					HAL_FindAdcTriggerPos(byDeviceId, byCoilId, 0, gWDB_dwFreq, gWDB_dwDuty, gWDB_dwPhase);
				}
				dwCoilCurrent    += HAL_GetCoilCurrent(byDeviceId, byCoilId);
				
				ST_WaitMs(4);
			}
			dwRail          /= 4;
			dwInputCurrent  /= 4;
			dwCoilCurrent   /= 4;
			
			dwInputPower   = (dwRail * dwInputCurrent) / 1000;
		}
	
		pBuff[2]  = (dwRail>>0) & 0xFF;
		pBuff[3]  = (dwRail>>8) & 0xFF;
		
		pBuff[4]  = (dwInputCurrent>>0) & 0xFF;
		pBuff[5]  = (dwInputCurrent>>8) & 0xFF;
		
		pBuff[6]  = (dwCoilCurrent>>0) & 0xFF;
		pBuff[7]  = (dwCoilCurrent>>8) & 0xFF;
		
		pBuff[8]  = (dwInputPower>>0) & 0xFF;
		pBuff[9]  = (dwInputPower>>8) & 0xFF;
		
		pBuff[10]  = (swInternalLoss>>0) & 0xFF;
		pBuff[11]  = (swInternalLoss>>8) & 0xFF;
	
		pBuff[12]  = (dwRail>>0) & 0xFF;
		pBuff[13]  = (dwRail>>8) & 0xFF;
		
		pBuff[14]  = 0;
		pBuff[15]  = 0;
	}
	else
	{
	    ret = WCTTX_DEBUG_CMD_STATUS_FAILED_INV_PARAM;
	}
	
	return ret;
}

static void WBG_SetDiffVoltage(uint8 byDeviceId, uint16 wOriVol, uint16 wTargetVol)
{
    //Converge the vrail to target voltage, with 1V step, 
    //to prevent surge current(due to big vrail change)
	do
	{
		if(wOriVol < wTargetVol)
		{
			wOriVol += 1000;
			if(wOriVol > wTargetVol)
			{
				wOriVol = wTargetVol;
			}
		}
		else
		{
			wOriVol -= 1000;
			if(wOriVol < wTargetVol)
			{
				wOriVol = wTargetVol;
			}
		}
		HAL_SetVrailVoltage(byDeviceId, wOriVol);
		ST_WaitMs(gWCT_Params.wRailSetupTime);
	}while(wOriVol != wTargetVol);
}

static void WBG_CalibrateRailVoltage(uint8 byDeviceId)
{
	uint16 wPreVol = gWCT_Params.wDefaultRailVoltageMv[0];
	
#if DIGITAL_DCDC
	//CtrlVol is the setting vRail
	uint16 wMinCtrlVol = 3000U;
	uint16 wMaxCtrlVol = 18000U;
#else
	//CtrlVol is the DAC output voltage, to control buck/boost circuit to generate the Vrail
	//make sure the HW is working with linear scale when Vrail control voltage is in 1~2V
	uint16 wMinCtrlVol = 1000U;
	uint16 wMaxCtrlVol = 2000U;
#endif
	uint16 wCtrlVol, wCtrlVolStep;
	uint8 byCalibNum = 5;
	
	uint16 wReadRailVol, wReadInputCurrent;
	
	uint16 wSetVol, wSetVolMargin = 500;
	
	uint8 i;
	uint32 x, y;
	uint32 dwSumX_IC, dwSumXX_IC;
	uint32 dwSumX_RV, dwSumXX_RV;
	uint32 dwSumY_IC, dwSumXY_IC;
	uint32 dwSumY_RV, dwSumXY_RV;
	sint32 sdwTemp, sdwSlope, sdwOffset;
	uint16 wNorm;
	
	ANALOG_CAL_PARAMS* pAnalogCalParams = NVM_GetAnalogCalibrationParam(byDeviceId);
	
	//Init the analog calibration parameter(that means actually no calibration)
	pAnalogCalParams->sdwInputCurrentSlope  = 0;
	pAnalogCalParams->sdwInputCurrentOffset = 0;
	pAnalogCalParams->wInputCurrentNorm     = 0;
	
	pAnalogCalParams->sdwRailVoltageSlope  = 1;
	pAnalogCalParams->sdwRailVoltageOffset = 0;
	pAnalogCalParams->wRailVoltageNorm     = 0;
	
	// Disable all coils
	for(i=0; i<gWCT_Params.byCoilNumPerDevice; i++)
	{
		HAL_EnableChargingOnCoil(byDeviceId, i, 0);
		HAL_EnableCoilDischarge(byDeviceId, i, 1);
	}
	
	// initialize the least squares intermediate values
	dwSumX_IC = 0u;
	dwSumXX_IC = 0u;
	dwSumY_IC = 0u;
	dwSumXY_IC = 0u;
	dwSumX_RV = 0u;
	dwSumXX_RV = 0u;
	dwSumY_RV = 0u;
	dwSumXY_RV = 0u;
	
	// NOTE: The input current sense reference value is dependent upon the operating rail voltage value.
	//  This calibration routine will also perform a calibration of the input current sense channel to
	//  provide linearization coefficients for use at run-time.
	wCtrlVolStep = (wMaxCtrlVol - wMinCtrlVol) / byCalibNum;
	for(i=0; i<byCalibNum; i++)
	{
		
#if DIGITAL_DCDC
		wCtrlVol = wMinCtrlVol + i*wCtrlVolStep;
		WBG_SetDiffVoltage(byDeviceId, wPreVol, wCtrlVol);
		wPreVol = wCtrlVol;
#else
		wCtrlVol = wMaxCtrlVol - i*wCtrlVolStep;
		//The HAL layer handler the converge to target voltage to prevent surge current
		HAL_SetVrailCtrlVoltage(gWDB_byDeviceId, wCtrlVol);
#endif
		ST_WaitMs(gWCT_Params.wRailSetupTime * 2);  //Wait voltage get stable
		wReadRailVol      = HAL_GetRailVoltage(byDeviceId);
		wReadInputCurrent = HAL_GetInputCurrent(byDeviceId);
		
		x = wCtrlVol;
		dwSumX_RV  += x;
		dwSumXX_RV += (x * x);
		
		y = wReadRailVol;
		dwSumY_RV  += y;
		dwSumXY_RV += (x * y);
		
		/*************************************************************************************
		 * read_vol to read_input current                                                   *
		 * Some current sensor output is dependent of Vrail, it also compensate the sensor  *
		 * zero offset                                                                      *
		 * wReadRailVol divide 10, otherwise the result overflow                            *
		**************************************************************************************/
		x = wReadRailVol/10;
		dwSumX_IC  += x;
		dwSumXX_IC += (x*x);
		
		y = wReadInputCurrent;
		dwSumY_IC += y;
		dwSumXY_IC	+= (x*y);
	}
	
	// Calculate the normalized slope and offset using the following equations
	//      y = ( norm_slope * x + norm_offset ) >> norm
	//      norm_slope = ( ( sumX * sumY - n * sumXY ) << norm ) / ( sumX * sumX - n * sumXX )
	//      norm_offset = ( ( sumY << norm ) - norm_slope * sumX ) / n
	
	// Calculate the denominator of the input current slope
	sdwTemp = ((sint32) dwSumX_IC * (sint32) dwSumX_IC - (sint32) byCalibNum * (sint32) dwSumXX_IC);
	
	// Calculate the numerator of the input current slope 
	sdwSlope = ((sint32) dwSumX_IC * (sint32) dwSumY_IC - (sint32) byCalibNum * (sint32) dwSumXY_IC);

	// Increment the input current normalization value until the slope is about to overrun
	// Note: 1073741824 = 0x40000000
	wNorm = 0;
	while ((sdwSlope != 0) && (sdwSlope < 1073741824) && (sdwSlope > -1073741824))
	{
		sdwSlope = (sdwSlope * 2);
		wNorm++;
	}
	
	// Finish calculating the input current slope
	sdwSlope /= sdwTemp;
	
	// Calculate the input current offset
	sdwOffset = (((sint32) dwSumY_IC * (sint32) (1UL<<wNorm)) - sdwSlope * (sint32) dwSumX_IC) / (sint32) byCalibNum;
	
	// Save the slope and offset values to the calibration structure
	pAnalogCalParams->sdwInputCurrentSlope = sdwSlope;
	pAnalogCalParams->sdwInputCurrentOffset = sdwOffset;
	pAnalogCalParams->wInputCurrentNorm = wNorm;
	
	// Calculate the denominator of the rail voltage slope
	sdwTemp = ((sint32) dwSumX_RV * (sint32) dwSumX_RV - (sint32) byCalibNum * (sint32) dwSumXX_RV);
	
	// Calculate the numerator of the rail voltage slope 
	sdwSlope = ((sint32) dwSumX_RV * (sint32) dwSumY_RV - (sint32) byCalibNum * (sint32) dwSumXY_RV);
	
	//Shift 3 bits to achieve enough resolution for later calculation
	sdwTemp  >>= 3;
	sdwSlope >>= 3;
	// Increment the rail voltage normalization value until the slope is about to overrun
	// Note: 1073741824 = 0x40000000
	wNorm = 0u;
	while ((sdwSlope != 0) && (sdwSlope < 1073741824) && (sdwSlope > -1073741824))
	{
		sdwSlope = (sdwSlope * 2);
		wNorm++;
	}
	
	// Finish calculating the rail voltage slope
	sdwSlope /= sdwTemp;
	
	// Calculate the rail voltage offset
	sdwOffset = (((sint32) dwSumY_RV * (sint32) (1UL<<wNorm)) - sdwSlope * (sint32) dwSumX_RV) / (sint32) byCalibNum;
	
	// Save the slope and offset values to the calibration structure
	pAnalogCalParams->sdwRailVoltageSlope = sdwSlope;
	pAnalogCalParams->sdwRailVoltageOffset = sdwOffset;
	pAnalogCalParams->wRailVoltageNorm = wNorm;
	
	
#if !DIGITAL_DCDC
	//Set voltage in HAL layer doesn't keep track with the real voltage since real voltage is set by HAL_SetVrailCtrlVoltage
	//Keep them accordance.
	wSetVol = wReadRailVol;
	HAL_SetVrailVoltage(byDeviceId, wSetVol);
	do
	{
		uint16 diff;
		wReadRailVol = HAL_GetRailVoltage(byDeviceId);
		if(wSetVol >= wReadRailVol)
		{
			diff = wSetVol - wReadRailVol;
		}
		else
		{
			diff = wReadRailVol - wSetVol;
		}
		if(diff < 500)
		{
			break;
		}
	}while(1);
#endif
	
	//Search to get maximum voltage HW could generate
	for(wSetVol = wReadRailVol; wSetVol<HAL_VRAIL_MAX_VOL; wSetVol += wSetVolMargin)
	{
		uint16 diff;
		HAL_SetVrailVoltage(byDeviceId, wSetVol);
		ST_WaitMs(gWCT_Params.wRailSetupTime);
		wReadRailVol = HAL_GetRailVoltage(byDeviceId);
		
		if(wSetVol >= wReadRailVol)
		{
			diff = wSetVol - wReadRailVol;
		}
		else
		{
			diff = wReadRailVol - wSetVol;
		}
		
		if(diff >= wSetVolMargin)
		{
			break;
		}
	}
	pAnalogCalParams->wMaxRailVoltageMv = wSetVol - 2*wSetVolMargin;
	
	//Search to get minimum voltage HW could generate
	for(wSetVol=pAnalogCalParams->wMaxRailVoltageMv; wSetVol>HAL_VRAIL_MIN_VOL; wSetVol -= wSetVolMargin)
	{
		uint16 diff;
		HAL_SetVrailVoltage(byDeviceId, wSetVol);
		ST_WaitMs(gWCT_Params.wRailSetupTime);
		wReadRailVol = HAL_GetRailVoltage(byDeviceId);
		
		if(wSetVol >= wReadRailVol)
		{
			diff = wSetVol - wReadRailVol;
		}
		else
		{
			diff = wReadRailVol - wSetVol;
		}
		
		if(diff >= wSetVolMargin)
		{
			break;
		}
	}
	pAnalogCalParams->wMinRailVoltageMv = wSetVol + 2*wSetVolMargin;
	
	
	DBG_Assert(pAnalogCalParams->wMinRailVoltageMv > 5000, 0, pAnalogCalParams->wMinRailVoltageMv);
	DBG_Assert(pAnalogCalParams->wMaxRailVoltageMv < 20000, 0, pAnalogCalParams->wMaxRailVoltageMv);
	
	//Set a default voltage for next calibration phase
	WBG_SetDiffVoltage(byDeviceId, wSetVol, gWCT_Params.wDefaultRailVoltageMv[0]);
}

void WBG_DebugProcess(void)
{
	uint16 wStatus = WCTTX_DEBUG_CMD_STATUS_OK;
	if (wcttxDebugStatus == WCTTX_DEBUG_CMD_STATE_RECEIVED)
	{
	    //wcttxDebugBuffer[0] store the command type, set by PC
		switch (wcttxDebugBuffer[0])
		{
			case WCTTX_DEBUG_CMD_NVM_FLASH:
			{
			    //Read/write NVM from/to flash
				if (wcttxDebugBuffer[2] == 0U)
				{
					NVM_ReadParams(1u);
					WCT_Init();
				}
				else
				{
					NVM_WriteParams();
				}
				break;
			}
			case WCTTX_DEBUG_CMD_DEBUG_MODE:
			{
				uint8 debugMode = (wcttxDebugBuffer[2]) ? 0U : 1U;
				gWDB_bWctDebugMode = debugMode; //keep the debug mode value for later usage
				if(debugMode)
				{
					WCT_Stop();
					HAL_SetVrailVoltage(gWDB_byDeviceId, gWCT_Params.wDefaultRailVoltageMv[0]);			
					HAL_EnableWCT(gWDB_byDeviceId, 1);
				}
				else
				{
					HAL_EnableWCT(gWDB_byDeviceId, 0);
					NVM_ReadParams(0u);
					WCT_Init();
				}
				break;
			}
			
			case WCTTX_DEBUG_CMD_COIL_DISCHARGE:
			{
				if(gWDB_bWctDebugMode)
				{
				    //In debug mode, permit coil discharge operation
					uint8 byCoilId = wcttxDebugBuffer[3];
					uint8 byCoilDischarge = wcttxDebugBuffer[2] ? 0U : 1U;
					HAL_EnableChargingOnCoil(gWDB_byDeviceId, byCoilId, 0);
					HAL_EnableCoilDischarge(gWDB_byDeviceId, byCoilId, byCoilDischarge);
				}
				else
				{
					wStatus = WCTTX_DEBUG_CMD_STATUS_FAILED_NON_DEBUG_MODE;
				}
				break;
			}
			
			case WCTTX_DEBUG_CMD_COIL_CONTROL:
			{
				uint8 byCoilId = wcttxDebugBuffer[3];
				uint32 temp = 0u;
				uint32 dwFreq = 0u;
				
				temp = wcttxDebugBuffer[7];
				temp = temp << 8;
				temp |= wcttxDebugBuffer[6];
				temp = temp << 8;
				temp |= wcttxDebugBuffer[5];
				temp = temp << 8;
				temp |= wcttxDebugBuffer[4];
				
				dwFreq = temp;
				
				if(gWDB_bWctDebugMode)
				{
				    //In debug mode, permit coil operation
					if (wcttxDebugBuffer[2] == 0u)
					{
					    //charging
						HAL_EnableChargingOnCoil(gWDB_byDeviceId, byCoilId, 0);
						HAL_EnableWCT(gWDB_byDeviceId, 0);
						HAL_EnableWCT(gWDB_byDeviceId, 1);
						if (dwFreq < 999999u)
						{
							gWDB_dwFreq = dwFreq;
							HAL_SetChargingFreqDutyPhase(gWDB_byDeviceId, byCoilId, gWDB_dwFreq, gWDB_dwDuty, gWDB_dwPhase);
							HAL_EnableCoilDischarge(gWDB_byDeviceId, byCoilId, 0);
							HAL_EnableChargingOnCoil(gWDB_byDeviceId, byCoilId, 1);

							ST_WaitMs(10u);
							//Sample coil current
							HAL_FindAdcTriggerPos(gWDB_byDeviceId, byCoilId, 0u, gWDB_dwFreq, gWDB_dwDuty, gWDB_dwPhase);
						}
						else
						{
							wStatus = WCTTX_DEBUG_CMD_STATUS_FAILED_INV_PARAM;
						}
					}
					else
					{
					    //Stop charging
						HAL_EnableChargingOnCoil(gWDB_byDeviceId, byCoilId, 0);
						HAL_EnableWCT(gWDB_byDeviceId, 0);
					}
				}
				else
				{
					wStatus = WCTTX_DEBUG_CMD_STATUS_FAILED_NON_DEBUG_MODE;
				}
				break;
			}
			
			case WCTTX_DEBUG_CMD_READ_RAIL_VOLTAGE_CONTROL:
			{	
				if(gWDB_bWctDebugMode)
				{
					if (wcttxDebugBuffer[2] != 0u)
					{
						// Sets the vRail voltage
						uint32 dwVoltage;
						dwVoltage = wcttxDebugBuffer[7];
						dwVoltage <<= 8;
						dwVoltage |= wcttxDebugBuffer[6];
						dwVoltage <<= 8;
						dwVoltage |= wcttxDebugBuffer[5];
						dwVoltage <<= 8;
						dwVoltage |= wcttxDebugBuffer[4];
						HAL_SetVrailVoltage(gWDB_byDeviceId, (uint16)dwVoltage);
					}
					else
					{
						//Get vRail
						uint16 wRailVoltage = HAL_GetRailVoltage(0);
						wcttxDebugBuffer[2] = wRailVoltage & 0xFF;
						wcttxDebugBuffer[3] = (wRailVoltage >> 8) & 0xFF;
					}
				}
				else
				{
					wStatus = WCTTX_DEBUG_CMD_STATUS_FAILED_NON_DEBUG_MODE;
				}
				break;
			}
			
			case WCTTX_DEBUG_CMD_GET_INPUT_VOLTAGE:
			{
			    //The command name is not exactly, only to keep accordance with PC setting
				wStatus = WBG_GetMultiParameters(gWDB_byDeviceId, wcttxDebugBuffer);
				break;
			}
			
			case WCTTX_DEBUG_CMD_RUN_RAILVOLTAGE_CALIBRATION:
			{	
				if(gWDB_bWctDebugMode)
				{
					WBG_CalibrateRailVoltage(gWDB_byDeviceId);
				}
				else
				{
					wStatus = WCTTX_DEBUG_CMD_STATUS_FAILED;
				}
				break;
			}
			
			case WCTTX_DEBUG_CMD_NOISE_ANAL:
			{
				volatile sint16 *pDDMBufAddress = HAL_GetDDMBuffer(gWDB_byDeviceId);
				
				uint8 coilId = 0; //Only for coil 0

                //wcttxDebugBuffer[4] returns the error type
				wcttxDebugBuffer[4] = 0u;  //no error
				
				if(gWDB_bWctDebugMode && HAL_CheckInterverRunning(gWDB_byDeviceId))
				{
				    //In debug mode, DDM could run
					wcttxDebugBuffer[2] = (uint8)(((uint16)pDDMBufAddress) & 0xff);
					wcttxDebugBuffer[3] = (uint8)(((uint16)pDDMBufAddress) >> 8);
					
					pDDMBufAddress[RECORD_BUF_LEN / 2 - 1] = 0xFFFF;
					HAL_FindAdcTriggerPos(gWDB_byDeviceId, coilId, 1, gWCT_Params.dwDigitalPingFreq, gWDB_dwDuty, gWDB_dwPhase);
					HAL_EnableDDM(gWDB_byDeviceId, coilId, 1);
					//Wait for one frame DDM data sampled
					while(0xFFFF == pDDMBufAddress[RECORD_BUF_LEN / 2 - 1]) {}
					HAL_EnableDDM(gWDB_byDeviceId, coilId, 0);
				}
				else
				{        	  
				  if(!gWDB_bWctDebugMode)
				  {
					  wcttxDebugBuffer[4] |= 0x01u;  //not in debug mode
				  }
				  if(!(HAL_CheckInterverRunning(gWDB_byDeviceId)))
				  {
					  wcttxDebugBuffer[4] |= 0x02u;  //PWM is not running
				  }
				}
				break;
			}
			
			default:
			{
				wStatus = WCTTX_DEBUG_CMD_STATUS_FAILED_INV_COMMAND; //invalid command
				break;
			}
		}
		
		//Return command execution result
		wcttxDebugStatus  = WCTTX_DEBUG_CMD_STATE_DONE;
		wcttxDebugStatus |= wStatus;
	}
}

//The function target to get below coefficient(a1,a2,b1,b2)
//inputCurrent = a1*vRail+b1
//coilCurrent  = a2*vRail+b2
static void WDB_RRQDInit(uint8 byDeviceId, uint8 byCoilId)
{
	uint32 x, y, i;
	uint32 dwSumX_Ii, dwSumXX_Ii;
	uint32 dwSumX_Ic, dwSumXX_Ic;
	uint32 dwSumY_Ii, dwSumXY_Ii; 
	uint32 dwSumY_Ic, dwSumXY_Ic; 
	sint32 sdwTemp, sdwSlope, sdwOffset;
	uint16 wNorm;
	
	uint16 wPreVol = gWCT_Params.wDefaultRailVoltageMv[0];
	
	uint16 wInputCurrent, wCoilCurrent;
	ANALOG_CAL_PARAMS* pAnalogCalParams = NVM_GetAnalogCalibrationParam(byDeviceId);	
	uint16 wStartVol, wStopVol, wStepVol, wSetVol;
	uint8 byCalibNum = 5;
	
	CURRENT_FITTING_PARAMS *pInCurFrmRailVol = NVM_GetRRQDInputCurrParams(byDeviceId, byCoilId);
	CURRENT_FITTING_PARAMS *pCoilCurFrmRailVol = NVM_GetRRQDCoilCurrParams(byDeviceId, byCoilId);
	
	//Set initial voltage for safety
	HAL_SetVrailVoltage(byDeviceId, gWCT_Params.wDefaultRailVoltageMv[byCoilId]);
	ST_WaitMs(gWCT_Params.wRailSetupTime);
	HAL_SetChargingBridge(byDeviceId, byCoilId, gWCT_Params.byDigitalPinglBridgeType);
	HAL_SetChargingFreqDutyPhase(byDeviceId, byCoilId, gWCT_Params.dwDigitalPingFreq, 
			gWCT_Params.wDigitalPingDuty, gWCT_Params.wDigitalPingPhase);
	HAL_EnableCoilDischarge(byDeviceId, byCoilId, 0);
	HAL_EnableChargingOnCoil(byDeviceId, byCoilId, 1);
	
	wStartVol = pAnalogCalParams->wMinRailVoltageMv;
	wStopVol  = pAnalogCalParams->wMaxRailVoltageMv;
	
	//Limit the stop voltage to 2/3 maximum voltage
	x = wStopVol;
	x *= 2;
	x /= 3;
	wStopVol = x;
	
	//Make the start/step voltage 0.1V alignment, to facilitate below process
	wStartVol += 100;
	wStartVol -= wStartVol%100;
	
	wStepVol = (wStopVol-wStartVol)/byCalibNum;	
	wStepVol -= 100;
	wStepVol += (100-wStepVol%100);

	dwSumX_Ii = 0u;
	dwSumXX_Ii = 0u;
	dwSumY_Ii = 0u;
	dwSumXY_Ii = 0u;
	dwSumX_Ic = 0u;
	dwSumXX_Ic = 0u;
	dwSumY_Ic = 0u;
	dwSumXY_Ic = 0u;

	for(i=0; i<byCalibNum; i++)
	{
		wSetVol = wStartVol+i*wStepVol;		
		WBG_SetDiffVoltage(byDeviceId, wPreVol, wSetVol);
		wPreVol = wSetVol;
		
		//Sample coil current.
		//Note: Coil current is not sampled automatically by HAY layer.
		//      Needs manually call HAL_FindAdcTriggerPos to sample coil current
		HAL_FindAdcTriggerPos(byDeviceId, byCoilId, 0, gWCT_Params.dwDigitalPingFreq, 
				gWCT_Params.wDigitalPingDuty, gWCT_Params.wDigitalPingPhase);
		wInputCurrent = HAL_GetInputCurrent(byDeviceId);
		wCoilCurrent  = HAL_GetCoilCurrent(byDeviceId, byCoilId);
		
		//setVol divide 10, otherwise the result overflow
		x = wSetVol/10;
		dwSumX_Ii += x;
		dwSumXX_Ii += (x * x);
		
		dwSumX_Ic += x;
		dwSumXX_Ic += (x * x);
		
		y = wInputCurrent;
		dwSumY_Ii += y;
		dwSumXY_Ii += (x * y);
		
		y = wCoilCurrent;
		dwSumY_Ic += y;
		dwSumXY_Ic += (x * y);
	}
	
	//Set rail voltage to default rail voltage slowly
	WBG_SetDiffVoltage(byDeviceId, wSetVol, gWCT_Params.wDefaultRailVoltageMv[0]);
	
	HAL_EnableChargingOnCoil(byDeviceId, byCoilId, 0);
	HAL_EnableCoilDischarge(byDeviceId, byCoilId, 1);
	
	// Calculate the normalized slope and offset using the following equations
	//      y = ( norm_slope * x + norm_offset ) >> norm
	//      norm_slope = ( ( sumX * sumY - n * sumXY ) << norm ) / ( sumX * sumX - n * sumXX )
	//      norm_offset = ( ( sumY << norm ) - norm_slope * sumX ) / n
	
	// Calculate the denominator of the coil current slope
	sdwTemp = ((sint32) dwSumX_Ic * (sint32) dwSumX_Ic - (sint32) byCalibNum * (sint32) dwSumXX_Ic);
	
	// Calculate the numerator of the coil current slope 
	sdwSlope = ((sint32) dwSumX_Ic * (sint32) dwSumY_Ic - (sint32) byCalibNum * (sint32) dwSumXY_Ic);

	// Increment the coil current normalization value until the slope is about to overrun
	// Note: 1073741824 = 0x40000000
	wNorm = 0;
	while ((sdwSlope != 0) && (sdwSlope < 1073741824) && (sdwSlope > -1073741824))
	{
		sdwSlope = (sdwSlope * 2);
		wNorm++;
	}
	
	// Finish calculating the coil current slope
	sdwSlope /= sdwTemp;
	
	// Calculate the coil current offset
	sdwOffset = (((sint32) dwSumY_Ic * (sint32) (1UL<<wNorm)) - sdwSlope * (sint32) dwSumX_Ic) / (sint32) byCalibNum;
	
	// Save the slope and offset values to the calibration structure
	pCoilCurFrmRailVol->sdwSlope = sdwSlope;
	pCoilCurFrmRailVol->sdwOffset = sdwOffset;
	pCoilCurFrmRailVol->wNorm = wNorm;
	
	// Calculate the denominator of the input current slope
	sdwTemp = ((sint32) dwSumX_Ii * (sint32) dwSumX_Ii - (sint32) byCalibNum * (sint32) dwSumXX_Ii);
	
	// Calculate the numerator of the input current slope 
	sdwSlope = ((sint32) dwSumX_Ii * (sint32) dwSumY_Ii - (sint32) byCalibNum * (sint32) dwSumXY_Ii);
	
	// Increment the rail voltage normalization value until the slope is about to overrun
	// Note: 1073741824 = 0x40000000
	wNorm = 0;
	while ((sdwSlope != 0) && (sdwSlope < 1073741824) && (sdwSlope > -1073741824))
	{
		sdwSlope = (sdwSlope * 2);
		wNorm++;
	}
	
	// Finish calculating the input current slope
	sdwSlope /= sdwTemp;
	
	// Calculate the input current offset
	sdwOffset = (((sint32) dwSumY_Ii * (sint32) (1UL<<wNorm)) - sdwSlope * (sint32) dwSumX_Ii) / (sint32) byCalibNum;
	
	// Save the slope and offset values to the calibration structure
	pInCurFrmRailVol->sdwSlope = sdwSlope;
	pInCurFrmRailVol->sdwOffset = sdwOffset;
	pInCurFrmRailVol->wNorm = wNorm;
}

static void WBG_QFInit(uint8 byDeviceId, uint8 byCoilId)
{
    QF_CALIB_PARAMS* pQfParams = NVM_GetQfCalibParams(byDeviceId, byCoilId);
    
    if(pQfParams)
    {
        //Pre-works before QF_QMeasurePrepare, see comments of QF_QMeasurePrepare
        HAL_EnableCoils(byDeviceId, byCoilId, 1);
        HAL_EnableCoilDischarge(byDeviceId, byCoilId, 1);
        ST_WaitMs(gWCT_Params.wQPrepareInterval);
        if(QF_MEASURE_SUCCESS == QF_QMeasurePrepare(byDeviceId, byCoilId))
        {
            uint8 result;
            while(1)
            {
                ST_WaitMs(gWCT_Params.wQMeasureInterval);
                result = QF_QMeasure(byDeviceId, byCoilId);
                if(QF_MEASURE_CONTINUE != result)
                {
                    //Q factor measurement finished, success or fail
                    break;
                }
            }
            
            if(QF_MEASURE_SUCCESS == result)
            {
                uint32 freq, qlc;			
                if(QF_MEASURE_SUCCESS == QF_GetQFactor(byDeviceId, byCoilId, &freq, &qlc))
                {
                    //Q factor measurement success and save the parameters
                    pQfParams->dwInitResonanceFreq = freq;
                    pQfParams->dwInitQlc = qlc;
                }
                else
                {
                    pQfParams->dwInitResonanceFreq = 0;
                    pQfParams->dwInitQlc = 0;
                }
            }
            else
            {
                pQfParams->dwInitResonanceFreq = 0;
                pQfParams->dwInitQlc = 0;
            }
        }
        else
        {
            pQfParams->dwInitResonanceFreq = 0;
            pQfParams->dwInitQlc = 0;
        }
        HAL_EnableCoils(byDeviceId, byCoilId, 0);
        ST_WaitMs(gWCT_Params.wQMeasureInterval);
    }
}

void WBG_InitParams(void)
{
	uint8 byDeviceId, byCoilId;
	for(byDeviceId=0; byDeviceId<NUM_DEVICES; byDeviceId++)
	{		
		//Set an initial voltage for safety
		HAL_SetVrailVoltage(byDeviceId, gWCT_Params.wDefaultRailVoltageMv[0]);
		HAL_EnableWCT(byDeviceId, 1);
		
		WBG_CalibrateRailVoltage(byDeviceId);

		for(byCoilId=0; byCoilId<NUM_COILS_PER_DEVICE; byCoilId++)
		{
		    //Q factor initialization
			WBG_QFInit(byDeviceId, byCoilId);
		}

		for(byCoilId=0; byCoilId<NUM_COILS_PER_DEVICE; byCoilId++)
		{
		    //The vRail is the default voltage
		    //Rx remove quick detection parameter initialization
			WDB_RRQDInit(byDeviceId, byCoilId);
		}

		HAL_EnableWCT(byDeviceId, 0);
	}
}
