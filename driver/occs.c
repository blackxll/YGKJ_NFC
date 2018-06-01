/*******************************************************************************
*
* Copyright 2012-2014 Freescale Semiconductor, Inc.
* Copyright 2017~2018 NXP.
* All rights reserved.
*******************************************************************************/

//---------------------------
// STANDARD LIBRARY INCLUDES
//---------------------------


//----------------------
// APPLICATION INCLUDES
//----------------------
#include "defines.h"
#include "cpu.h"

#include "occs.h"

void OCCS_Init(void)
{
	uint16 wtemp;
	uint16 wFlagTemp1 = 0u;
	uint16 wFlagTemp2 = 0u;
	
	// load factory trimming value of the internal relaxation oscillator
	OCCS_OSCTL1 &= ~OCCS_OSCTL1_FREQ_TRIM8M;
	wtemp = ((uint16) (SIM_NVMOPT2H & ((uint16) SIM_NVMOPT2H_ROSC_8M_FTRIM) ));
	OCCS_OSCTL1 |= ((vuint16_t) wtemp);
	
	OCCS_OSCTL1 &= ~OCCS_OSCTL1_CLK_MODE; /* Power-up the crystal oscillator */
	OCCS_OSCTL1 &= ~OCCS_OSCTL1_EXT_SEL;  /* Select an external crystal oscillator to be used */
	OCCS_OSCTL1 |= OCCS_OSCTL1_COHL;

	OCCS_OSCTL2 &= ~(OCCS_OSCTL2_COPD); /* Enable crystal oscillator */	
	while(0 == (OCCS_STAT & OCCS_STAT_OSC_OK)) {}  /* Wait for external oscillator stabilization */
	
	OCCS_CTRL |= OCCS_CTRL_PRECS_0; /* switch to external clock source (set PRESC=1) */
	
	/* Wait 6 NOPs for the synchronizing circuit to change clocks */
	asm(nop);
	asm(nop);
	asm(nop);
	asm(nop);
	asm(nop);
	asm(nop);
	
	//OCCS_OSCTL1 |= OCCS_OSCTL1_ROPD; /* shut down internal oscillator if required */
	OCCS_CTRL &= ~ OCCS_CTRL_PLLPD; /* Enable PLL and LCKON */
	OCCS_CTRL |= OCCS_CTRL_LCKON;
	OCCS_DIVBY = 0x2031U;  /* Set the clock prescalers */ 
	
	// wait for PLL lock
	do
	{
		wFlagTemp1 = ((uint16) (OCCS_STAT & ((uint16) OCCS_STAT_LCK1) ));
		wFlagTemp2 = ((uint16) (OCCS_STAT & ((uint16) OCCS_STAT_LCK0) ));
	} while (!((wFlagTemp1 != 0u) && (wFlagTemp2 != 0u)));
	
	OCCS_CTRL |= OCCS_CTRL_ZSRC; /* PLL is locked: final PLL setup */
	
	// Wait for clock transition to complete
	while( ( OCCS_STAT & OCCS_STAT_ZSRCS ) != OCCS_STAT_ZSRCS_0 )
	{
		;
	}
}


void OCCS_SetHighSpeed(void)
{
	//LPRUN -> RUN
	SIM_PWRMODE &= ~SIM_PWRMODE_LPMODE; /* Exit low power mode */
	//Restore to internal 8MHz clock
	OCCS_OSCTL1 &= ~OCCS_OSCTL1_ROPD; /* Power up ROSC 8MHz. */
	OCCS_CTRL &= ~OCCS_CTRL_PRECS; /* Select an internal 8MHz clock source for the CPU core */
	OCCS_DIVBY &= ~OCCS_DIVBY_COD; 
	//Restore to 100MHz bus clock
	OCCS_Init();
}

void OCCS_SetLowSpeed(void)
{
	//RUN -> LPRUN
	OCCS_CTRL &= ~OCCS_CTRL_ZSRC; /* Select clock source to MSTR_OSC */
	OCCS_CTRL |= OCCS_CTRL_PLLPD; /* Disable PLL */
	OCCS_OSCTL2 &= ~OCCS_OSCTL2_ROPD32K; /* Power up low speed oscillator */
	OCCS_CTRL = (OCCS_CTRL & ~OCCS_CTRL_PRECS) | (2u << 2); /* Select internal 32KHz clock */
	/* Wait 6 NOPs for resynchronization */
	asm(nop);
	asm(nop);
	asm(nop);
	asm(nop);
	asm(nop);
	asm(nop);
	OCCS_OSCTL1 |= OCCS_OSCTL1_ROPD; /* Power down ROSC 8MHz. */
	OCCS_OSCTL2 |= OCCS_OSCTL2_COPD;	//power down Crystal Oscillator
	OCCS_DIVBY &= ~OCCS_DIVBY_COD;
	
	SIM_PWRMODE |= SIM_PWRMODE_LPMODE; /* Enter low power mode */
}

