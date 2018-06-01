/*******************************************************************************
*
* Copyright 2012-2016 Freescale Semiconductor, Inc.
* Copyright 2017~2018 NXP.
* All rights reserved.
*******************************************************************************/
#include "defines.h"

#include "appCfg.h"
#include "wct_lib.h"

#include "systemPrint.h"
#include "hal.h"

#include "wct_debug.h"

#include "NVM.h"

//Bootloader parameters
typedef struct
{	
	uint16	wReserved;	//Make uint32 alignment
    uint16	wEntryPlaceL;
    uint16	wEntryPlaceH;
    uint16	wDelayTime;
} BOOTLOADER_PARAMS;

void PARAM_WctParamInit(NVM_PARAMS *pNvmParam);
void PROT_Init(NVM_PARAMS *pNvmParam);

#define NVM_OP_PARAMS_FLASH_BASE_ADDRESS_FIRST_DEVICE		(NVM_BASE_ADDR)
#define NVM_CAL_PARAMS_FLASH_BASE_ADDRESS 					(NVM_OP_PARAMS_FLASH_BASE_ADDRESS_FIRST_DEVICE + ( sizeof( OP_PARAMS ) * (NUM_DEVICES) ))

NVM_PARAMS NvmParams;

static void NVM_DebugReportChecksumFault(uint8* chBuffer, uint8 byChecksum, uint8 byCalcChecksum)
{
	APP_PrintString("\r\n:");
	APP_PrintString(chBuffer);
	APP_PrintString(" Checksum Fault, Read: 0x");
	APP_PrintHexChar(byChecksum);
	APP_PrintString(", Calculated: 0x");
	APP_PrintHexChar(byCalcChecksum);
	APP_PrintString("\r\n");
}

static uint8 NVM_SystemComputeDataBlockChecksum(uint8* pbyDataBlock, uint16 wSizeOfDataBlock)
{
	uint16 wDataBlockIndex = 0;
	uint32 dwChecksum = 0;	
	uint8 byData;
	
	// Calculate Checksum - do not include the last byte (checksum)
	while( wDataBlockIndex < (wSizeOfDataBlock - 1) )
	{
		byData = *pbyDataBlock;
		byData ^= (uint8) wDataBlockIndex;
		dwChecksum += (uint32) (byData);
		wDataBlockIndex++;
		*pbyDataBlock++;
	}
	
	// Checksum is sum of all XORed values modulo-256
	return (uint8) (dwChecksum & 0x000000FF);
}

static boolean NVM_InitOpParams(NVM_BYTE_POINTER pNvmOperatingParams, uint8 *pRamOperatingParams, uint16 wNumParamBytes )
{
	boolean ret = TRUE;
	uint8 byCalcChecksum;
	OP_PARAMS *pOperatingParams = (OP_PARAMS*)pRamOperatingParams;

	// Load Operating Params from NVM
	HAL_ReadFlashData(pRamOperatingParams, pNvmOperatingParams, wNumParamBytes);
	byCalcChecksum = NVM_SystemComputeDataBlockChecksum( pRamOperatingParams, wNumParamBytes );
	if( ( pOperatingParams->ChecksumParams.byChecksum & 0xFF ) != byCalcChecksum )
	{
		ret = FALSE;
		NVM_DebugReportChecksumFault( (uint8*)"Op Params", pOperatingParams->ChecksumParams.byChecksum, byCalcChecksum );
	}
	return ret;
}

static boolean NVM_InitCalParams(NVM_BYTE_POINTER pNvmCalibratingParams, uint8 *pRamCalibratingParams, uint16 wNumParamBytes )
{
	boolean ret = TRUE;	
	uint8 byCalcChecksum;
	CAL_PARAMS *pCalibratingParams = (CAL_PARAMS*)pRamCalibratingParams;
	
	// Load Calibrate Params from NVM
	HAL_ReadFlashData(pRamCalibratingParams, pNvmCalibratingParams, wNumParamBytes);
	byCalcChecksum = NVM_SystemComputeDataBlockChecksum( pRamCalibratingParams, wNumParamBytes );
	if( ( pCalibratingParams->ChecksumParams.byChecksum & 0xFF ) != byCalcChecksum )
	{
		ret = FALSE;
		NVM_DebugReportChecksumFault( (uint8*)"Cal Params", pCalibratingParams->ChecksumParams.byChecksum, byCalcChecksum );
	}
	return ret;
}

//return FALSE means NVM error
boolean NVM_ReadParams(boolean bNeedReloadNVM)
{
	boolean ret = TRUE;

	if(bNeedReloadNVM)
	{
		if (NVM_InitOpParams((NVM_BYTE_POINTER) (NVM_OP_PARAMS_FLASH_BASE_ADDRESS_FIRST_DEVICE), (uint8*) &NvmParams.OpParams, 
				sizeof(OP_PARAMS)) == FALSE)
		{
			ret = FALSE;
		}
		else if (NVM_InitCalParams((NVM_BYTE_POINTER) (NVM_CAL_PARAMS_FLASH_BASE_ADDRESS), (uint8*) &NvmParams.CalParams, 
				sizeof(CAL_PARAMS)) == FALSE)
		{
			ret = FALSE;
		}
	}
    
    if(TRUE == ret)
    {
        //Init parameter for library from NVM, some library parameter is hard coded
        PARAM_WctParamInit(&NvmParams);
        //Init parameter for protection from NVM
        PROT_Init(&NvmParams);
        
        if (0 == NvmParams.OpParams.OpStateParams.dwAutoCalibrated)
        {
            //Have not done auto calibration, do it. Generally performed at first power on after firmware download
            //Note: during auto calibration, there should be no object on Tx coil surface
            WBG_InitParams();
            NvmParams.OpParams.OpStateParams.dwAutoCalibrated = 1;
            NVM_WriteParams();  //save auto calibrated parameter to NVM
        }
    }

	return ret;
}

void NVM_ReCalibration(void)
{
    //For debug purpose
	WBG_InitParams();
	NvmParams.OpParams.OpStateParams.dwAutoCalibrated = 1;
	NVM_WriteParams();
}

void NVM_WriteParams(void)
{
	BOOTLOADER_PARAMS BootloaderParams;
	uint8 byCalcChecksum;
	
	//update operation params checksum
	byCalcChecksum = NVM_SystemComputeDataBlockChecksum((uint8 *) &NvmParams.OpParams, sizeof(OP_PARAMS));
	NvmParams.OpParams.ChecksumParams.byChecksum = byCalcChecksum;

	//update calibration checksum
	byCalcChecksum = NVM_SystemComputeDataBlockChecksum((uint8 *) &NvmParams.CalParams, sizeof(CAL_PARAMS));
	NvmParams.CalParams.ChecksumParams.byChecksum = byCalcChecksum;
	
#if BOOTLOADER_USED
	//Save the bootloader parameter
	HAL_ReadFlashData((uint8*)&BootloaderParams, (NVM_BYTE_POINTER)(BOOTLOADER_PARAMS_ADDR), sizeof(BOOTLOADER_PARAMS));
#endif
	
	// Erase the entire data flash
	HAL_EraseFlashSector(NVM_BASE_ADDR);
	
	//Re-program with all valid parameter data
	HAL_WriteFlashData((uint32*)&NvmParams, NVM_BASE_ADDR, sizeof(NVM_PARAMS));
	
#if BOOTLOADER_USED
	//Restore bootloader parameter
	HAL_WriteFlashData((uint32*)&BootloaderParams, BOOTLOADER_PARAMS_ADDR, sizeof(BOOTLOADER_PARAMS));
#endif
}

ANALOG_CAL_PARAMS* NVM_GetAnalogCalibrationParam(uint8 byDeviceId)
{
	return &NvmParams.CalParams.AnalogParams[byDeviceId];
}

QF_CALIB_PARAMS *NVM_GetQfCalibParams(uint8 byDeviceId, uint8 byCoilId)
{
	return &(NvmParams.CalParams.QfCalibParams[byDeviceId][byCoilId]);
}

FOD_CHARACTERIZATION_PARAMS* NVM_GetCharactParams(uint8 byDeviceId, uint8 byCoilId, uint8 byControlType)
{
	return &(NvmParams.CalParams.PowerLossParams[byDeviceId][byCoilId][byControlType].FodCharacterizationParams);
}

FOD_NORMALIZATION_PARAMS* NVM_GetNormalizationParams(uint8 byDeviceId, uint8 byCoilId, uint8 byControlType)
{
	return &(NvmParams.CalParams.PowerLossParams[byDeviceId][byCoilId][byControlType].FodNormalizationParams);
}

CURRENT_FITTING_PARAMS* NVM_GetRRQDCoilCurrParams(uint8 byDeviceId, uint8 byCoilId)
{
	return &(NvmParams.CalParams.QuickRemovalParams[byDeviceId][byCoilId].CoilCurFrmRailVol);
}

CURRENT_FITTING_PARAMS* NVM_GetRRQDInputCurrParams(uint8 byDeviceId, uint8 byCoilId)
{
	return &(NvmParams.CalParams.QuickRemovalParams[byDeviceId][byCoilId].InCurFrmRailVol);
}
