/*******************************************************************************
*
* Copyright 2012-2014 Freescale Semiconductor, Inc.
* Copyright 2017~2018 NXP.
* All rights reserved.
*******************************************************************************/

#include "defines.h"
#include "cpu.h"

#include "flash.h"

// Actual program flash is from 0x0000 to 0x1FFFF Words
#define PP_FLASH_END							0x001FFFFUL // program p-flash end address
#define PP_FLASH_END_IN_BYTES					(PP_FLASH_END << 1)

//---------------- Control Register definitions ----------------
// FTFA_FSTAT, flash module status register definitions
#define COMMAND_COMPLETE_INT_FLAG               ( FTFL_FSTAT & 0x0080u )
#define START_COMMAND_SEQUENCE                  ( FTFL_FSTAT |= 0x0080u )
#define READ_COLLISION_ERROR_FLAG               ( FTFL_FSTAT & 0x0040u )
#define CLEAR_READ_COLLISION_ERROR_FLAG         ( FTFL_FSTAT |= 0x0040u )
#define ACCESS_ERROR_FLAG                       ( (uint16)FTFL_FSTAT & 0x0020u )
#define CLEAR_ACCESS_ERROR_FLAG                 ( FTFL_FSTAT |= 0x0020u )
#define PROTECTION_VIOLATION_FLAG               ( (uint16) FTFL_FSTAT & 0x0010u )
#define CLEAR_PROTECTION_VIOLATION_FLAG         ( FTFL_FSTAT |= 0x0010u )

// Flash memory commands
#define READ_1S_SECTION                         0x01u
#define PROGRAM_CHECK                           0x02u
#define READ_RESOURCE                           0x03u
#define PROGRAM_LONGWORD                        0x06u
#define ERASE_FLASH_SECTOR                      0x09u
#define READ_1S_ALL_BLOCKS                      0x40u
#define READ_ONCE                               0x41u
#define PROGRAM_ONCE                            0x43u
#define ERASE_ALL_BLOCKS                        0x44u
#define VERIFY_BACKDOOR_ACCESS_KEY              0x45u

#pragma define_section XRAM_CODE ".code_in_ram"
#pragma section XRAM_CODE begin
static void FLASH_LaunchCmd(void)
{
    // Clear CCIF to execute
    START_COMMAND_SEQUENCE;

    // Wait until command complete flag is set
    while( !COMMAND_COMPLETE_INT_FLAG )
    {
        ;
    }
}
#pragma section XRAM_CODE end


boolean FLASH_SectorDataErase(uint32 dwAddr)
{
	// NOTE: This command will erase all bytes in a program flash sector 
		
	// NOTE: Flash command addressing is allocated on uint8 boundaries. This differs from
	// the uint16 sized boundary default for the part.
	/* dwAddr: uint8 address*/
	boolean bStatus = TRUE;
	uint16 wFlagTemp1 = 0u;
	uint16 wFlagTemp2 = 0u;

	if( dwAddr <= PP_FLASH_END_IN_BYTES )
	{
		// Wait until command complete flag is set
		while( !COMMAND_COMPLETE_INT_FLAG )
		{
			;
		}
	
		// Set command to erase a sector, write address c
		FTFL_FCCOB0 = ERASE_FLASH_SECTOR;
	
		// Write address in flash sector to be erased
		FTFL_FCCOB1 = (vuint8_t) ( ( dwAddr >> 16) & 0xFFu  );     // Flash address [23:16]
		FTFL_FCCOB2 = (vuint8_t) ( ( dwAddr >> 8) & 0xFFu  );      // Flash address [15:8]
		FTFL_FCCOB3 = (vuint8_t) ( dwAddr & 0xFFu );               // Flash address [7:0]
	
		// Clear all flags to allow next command
		CLEAR_PROTECTION_VIOLATION_FLAG;
		CLEAR_ACCESS_ERROR_FLAG;		

		FLASH_LaunchCmd();
	
		wFlagTemp1 = (PROTECTION_VIOLATION_FLAG);
		wFlagTemp2 = (ACCESS_ERROR_FLAG);
		
		if( (wFlagTemp1 != 0u) || (wFlagTemp2 != 0u) )
		{
			// An error has occurred... clear the flags to allow future operations
			CLEAR_PROTECTION_VIOLATION_FLAG;
			CLEAR_ACCESS_ERROR_FLAG;
			bStatus = FALSE;
		}
	}
	else
	{
		// Illegal program flash address
		bStatus = FALSE;
	}
	
	return bStatus;
}

boolean FLASH_WriteData(uint32 dwAddr, uint32 dwData)
{
	// NOTE: Data can only be written to flash in uint32 blocks

	// NOTE: Flash command addressing is allocated on uint8 boundaries. This differs from
	// the uint16 sized boundary default for the part.
	boolean bStatus = TRUE;
	uint16 wFlagTemp1 = 0u;
	uint16 wFlagTemp2 = 0u;
	    
	// DO NOT attempt write if address is invalid
	if( dwAddr <= PP_FLASH_END_IN_BYTES )
	{
		// Wait until command complete flag is set
		while( !COMMAND_COMPLETE_INT_FLAG )
		{
			;
		}

		// Write address in flash sector to be erased
		FTFL_FCCOB1 = (vuint8_t) ( ( dwAddr >> 16) & 0xFFu  );      // Flash address [23:16]
		FTFL_FCCOB2 = (vuint8_t) ( ( dwAddr >> 8) & 0xFFu  );       // Flash address [15:8]
		FTFL_FCCOB3 = (vuint8_t) ( dwAddr & 0xFFu );                // Flash address [7:0]

		// Set command to program long word, write address c
		FTFL_FCCOB0 = PROGRAM_LONGWORD;

		// Write data to be saved to flash
		FTFL_FCCOB7 = ((vuint8_t) ( ( dwData ) & 0xFFu )) ;           // uint8 1
		FTFL_FCCOB6 = ((vuint8_t) ( ( dwData >> 8 ) & 0xFFu ));       // uint8 2
		FTFL_FCCOB5 = ((vuint8_t) ( ( dwData >> 16 ) & 0xFFu ));      // uint8 3
		FTFL_FCCOB4 = ((vuint8_t) ( ( dwData >> 24 ) & 0xFFu ));      // uint8 4

		// Clear all flags to allow next command
		CLEAR_PROTECTION_VIOLATION_FLAG;
		CLEAR_ACCESS_ERROR_FLAG;

		FLASH_LaunchCmd();

		wFlagTemp1 = (PROTECTION_VIOLATION_FLAG);
		wFlagTemp2 = (ACCESS_ERROR_FLAG);
		
		if( (wFlagTemp1 != 0u)|| (wFlagTemp2 != 0u) )
		{
			// An error has occurred... clear the flags to allow future operations
			CLEAR_PROTECTION_VIOLATION_FLAG;
			CLEAR_ACCESS_ERROR_FLAG;
			bStatus = FALSE;
		}
	}
	else
	{
		// Illegal address requested
		bStatus = FALSE;
	}

	return bStatus;
}

void FLASH_ReadData(uint8 * pbyBuf, NVM_BYTE_POINTER pbyAddr, uint16 wLen)
{
	uint16 i;
	NVM_BYTE_POINTER pbyDataFlashAddress = pbyAddr;

	for(i=0; i<wLen; i++)
	{
		*pbyBuf++ = *pbyDataFlashAddress++;
	}
}

void FLASH_Init(void)
{
    FTFL_FPROT0 = 0xFFU;
    FTFL_FPROT1 = 0xFFU;
    FTFL_FPROT2 = 0xFFU;
    FTFL_FPROT3 = 0xFFU;
}

