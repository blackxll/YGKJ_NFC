/*******************************************************************************
*
* Copyright 2012-2016 Freescale Semiconductor, Inc.
* Copyright 2017~2018 NXP.
* All rights reserved.
*******************************************************************************/

#ifndef __NVM_H__
#define __NVM_H__

#define NUM_FOD_TYPE                            1U
#define NOM_CAL_SCALE_FACTOR_SIZE_IN_BITS       15

// Checksum Parameters
typedef struct
{
	uint8 byPaddingByte1;
	uint8 byPaddingByte2;
	uint8 byPaddingByte3;
	uint8 byChecksum;
} CHECKSUM_PARAMS;

// NOTE: This structure definition MUST reside on a 32-bit address boundary!
typedef struct
{
	// Digital Ping Operational Parameters
	uint32 dwPingFrequency;
	uint16 wPingPulseDurationTimeMs;
	uint16 wPingIntervalMs;
	
	// Analog Ping Operational Parameters
	uint16 wAnalogPingCoilCurrentThreshold;
	uint8  byDigitalPingRetryIntervalSeconds;
	uint8  byReserved;
	
	// Signal Detection Parameters
	uint16 wOverCoilCurrentThreshold;
	uint16 wOverRailVoltageThreshold;
	uint16 wOverInputVoltageThreshold;
	uint16 wUnderInputVoltageThreshold;
	uint16 wOverInputCurrentThreshold;
	sint16 swOverTemperatureThreshold;

	// Operating Frequencies
	uint32 dwMinFreq;
	uint32 dwMaxFreq;

	// Rail Control Parameters
	uint16 wMinRailVoltageMv;
	uint16 wMaxRailVoltageMv;
    uint32 dwDefaultRailVoltageMv[NUM_COILS_PER_DEVICE];
	
	uint32 dwAutoCalibrated;
} OPSTATE_OP_PARAMS;

typedef struct
{
	uint32 dwPowerLossIndicationToPwrCessationMs;
	uint16 wPowerLossLPThreshold;
	uint16 wPowerLossMPThreshold;
	uint8  byNumFodTripsToIndication;
	uint8  byReserved;
	uint16 wReserved;
} POWERLOSS_OP_PARAMS;

// Operating Parameters
typedef struct
{
	OPSTATE_OP_PARAMS OpStateParams;
	POWERLOSS_OP_PARAMS PowerLossParams;  
	CHECKSUM_PARAMS ChecksumParams;
} OP_PARAMS;

typedef struct
{
	uint16 wMinRailVoltageMv;
	uint16 wMaxRailVoltageMv;
	sint32 sdwRailVoltageSlope;
	sint32 sdwInputCurrentSlope;
	sint32 sdwRailVoltageOffset;
	sint32 sdwInputCurrentOffset;
	uint16 wRailVoltageNorm;
	uint16 wInputCurrentNorm;
	uint16 wInputVoltageCalibration;
	uint16 wInputCurrentCalibration;
} ANALOG_CAL_PARAMS;

typedef struct
{
	// FOD Characterization Params
	FOD_CHARACTERIZATION_PARAMS FodCharacterizationParams;
	// FOD Normalization Params
	FOD_NORMALIZATION_PARAMS FodNormalizationParams;
} POWERLOSS_CAL_PARAMS;

typedef struct
{
	sint32  sdwSlope;
	sint32  sdwOffset;
	uint16  wNorm;
	uint16  wReserved;
}CURRENT_FITTING_PARAMS;

typedef struct
{
	CURRENT_FITTING_PARAMS	InCurFrmRailVol;
	CURRENT_FITTING_PARAMS	CoilCurFrmRailVol;
} QUICK_REMOVAL_PARAMS;

typedef struct
{
	ANALOG_CAL_PARAMS AnalogParams[NUM_DEVICES];
	POWERLOSS_CAL_PARAMS PowerLossParams[NUM_DEVICES][NUM_COILS_PER_DEVICE][NUM_FOD_TYPE];
	QUICK_REMOVAL_PARAMS QuickRemovalParams[NUM_DEVICES][ NUM_COILS_PER_DEVICE ];
	QF_CALIB_PARAMS QfCalibParams[NUM_DEVICES][NUM_COILS_PER_DEVICE];
	CHECKSUM_PARAMS ChecksumParams;
} CAL_PARAMS;

typedef struct
{
	OP_PARAMS  OpParams;
	CAL_PARAMS CalParams;
} NVM_PARAMS;


boolean NVM_ReadParams(boolean bNeedReloadNVM);
void NVM_WriteParams(void);
void NVM_ReCalibration(void);

ANALOG_CAL_PARAMS* NVM_GetAnalogCalibrationParam(uint8 byDeviceId);
QF_CALIB_PARAMS *NVM_GetQfCalibParams(uint8 byDeviceId, uint8 byCoilId);
FOD_CHARACTERIZATION_PARAMS* NVM_GetCharactParams(uint8 byDeviceId, uint8 byCoilId, uint8 byControlType);
FOD_NORMALIZATION_PARAMS* NVM_GetNormalizationParams(uint8 byDeviceId, uint8 byCoilId, uint8 byControlType);
CURRENT_FITTING_PARAMS* NVM_GetRRQDCoilCurrParams(uint8 byDeviceId, uint8 byCoilId);
CURRENT_FITTING_PARAMS* NVM_GetRRQDInputCurrParams(uint8 byDeviceId, uint8 byCoilId);

#endif
