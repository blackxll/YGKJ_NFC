/*******************************************************************************
*
* Copyright 2012-2015 Freescale Semiconductor, Inc.
* Copyright 2017~2018 NXP.
* All rights reserved.
*******************************************************************************/

#include "defines.h"

#include "wct_lib.h"

#include "hal.h"

#include "systemPrint.h"

#define INVALID_HEX_VALUE   (0xFFU)

static uint8 SPRT_HexCharToDecValue( uint8 byHexChar )
{
    // Change a hex nibble to a decimal value
    uint8 byDecValue = INVALID_HEX_VALUE;

    if ( ( byHexChar >= '0' ) && ( byHexChar <= '9' ) )
    {
        byDecValue = byHexChar - 48;
    }
    else
    {
        if ( ( byHexChar >= 'A' ) && ( byHexChar <= 'F' ) )
        {
            byDecValue = byHexChar - 55;
        }
        else
        {
            if ( ( byHexChar >= 'a' ) && ( byHexChar <= 'f' ) )
            {
                byDecValue = byHexChar - 87;
            }
        }
    }
    return( byDecValue );
}

static void SPRT_HexValueToHexCharString( uint8 byValue, uint8* chBuffer )
{
    // Converts a number to a hex character string
    uint8 byIndex = 0;
    uint8 byDigit;
    uint8 byCounter;

    // Convert digits
    for( byCounter = 2; byCounter > 0; byCounter-- )
    {
        // Get the digit value
        byDigit = ( byValue >> ( ( byCounter - 1 ) * 4 ) ) & 0x0F;

        if ( byDigit < 10 )
        {
            byDigit += 48;
        }
        else
        {
            byDigit += 55;
        }
        chBuffer[ byIndex++ ] = byDigit;
    }

    // Add NULL terminator
    chBuffer[ byIndex ] = '\0';
}

static void SPRT_DoubleWordValueToDecString( uint32 dwValue, uint8* chBuffer )
{
    uint8 byIndex = 0;
    uint8 byDigit;           // Value of the current calculated digit
    uint8 byCounter;         // Variable for the counter loop
    uint32 dwDivisionValue;  // The value used to divide the dwValue by to get the current digit

    // Set the division value to the max digit place for a long number
    dwDivisionValue = 1000000000UL;

    if( dwValue == 0 )
    {
        // The only digit is a zero - display it
        chBuffer[ byIndex++ ] = '0';
    }
    else
    {
        // Loop through all 10 possible digits
        for( byCounter = 0; byCounter < 10; byCounter++ )
        {
            // Calculate the digit value
            byDigit = dwValue / dwDivisionValue;         

            if( ( byDigit != 0 ) || ( byIndex != 0 ) )
            {
                // The value is not a leading zero... save the digit
                chBuffer[ byIndex++ ] = ( byDigit + 0x30 );
            }

            // Subtract out the current digit
            dwValue -= ( dwDivisionValue * byDigit );

            // Move the division value one digit
            dwDivisionValue /= 10;
        }
    }

    // Add NULL terminator
    chBuffer[ byIndex ] = '\0';
}

//------------------
// PUBLIC FUNCTIONS
//------------------
void SPRT_PrintChar(uint8 byChar)
{
	HAL_PrintChar(byChar);
}

void SPRT_PrintString(uint8 *pbyStr)
{
	// Write out the string
	while ((*pbyStr) != 0x00)
	{
		SPRT_PrintChar((uint8) *pbyStr);
		pbyStr++;
	}
}

void SPRT_PrintDecChar(uint8 byChar)
{
    uint8 byBuffer[4];

    // Put out a byte as a decimal value
    SPRT_DoubleWordValueToDecString( ( uint32 ) byChar, &byBuffer[0] );
    SPRT_PrintString( byBuffer );
}

void SPRT_PrintHexChar(uint8 byChar)
{
    // Put out a byte as a Hex value
    uint8 byBuffer[3];

    SPRT_HexValueToHexCharString( byChar, &byBuffer[0] );
    SPRT_PrintString( byBuffer );
}

void SPRT_PrintSignedDecChar(uint8 byChar)
{
	uint8 byBuffer[6];

    // Put out a byte as a signed decimal value
    if( byChar & 0x80 )
    {
        byBuffer[ 0 ] = '-';
        byChar = 0x100 - byChar;
    }
    else
    {
        byBuffer[ 0 ] = '+';
    }
    SPRT_DoubleWordValueToDecString( ( uint32 ) byChar, &byBuffer[1] );

    SPRT_PrintString( byBuffer );
}

void SPRT_PrintSignedDecWord(uint16 wValue)
{
	uint8 byBuffer[10];

    // Put out a byte as a signed decimal value
    if( wValue & 0x8000 )
    {
        byBuffer[ 0 ] = '-';
        wValue = 0x10000UL - wValue;
    }
    else
    {
        byBuffer[ 0 ] = '+';
    }
    SPRT_DoubleWordValueToDecString( ( uint32 ) wValue, &byBuffer[1] );

    SPRT_PrintString( byBuffer );
}

void SPRT_PrintDoubleWordValue(uint32 dwValue)
{
    uint8 byBuffer[11];

    SPRT_DoubleWordValueToDecString( dwValue, &byBuffer[0] );

    SPRT_PrintString( byBuffer );
}

