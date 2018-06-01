/*******************************************************************************
 *
 * Copyright 2012-2014 Freescale Semiconductor, Inc.
 * Copyright 2017~2018 NXP.
 * All rights reserved.
 *******************************************************************************/

#include "defines.h"
#include "cpu.h"

#include "appCfg.h"

#include "freemaster.h"
#include "freemaster_cfg.h"

void FMSTR_Cfg(void)
{
    //Enable interrupts of FreeMASTER tool 
#if FMSTR_USE_SCI
#if FMSTR_SCI_BASE == 0xE080
    INTC_IPR5 = ( (INTC_IPR5 & ~INTC_IPR5_QSCI0_RCV) | (INTC_IPR5_QSCI0_RCV_1) );
    INTC_IPR5 = ( (INTC_IPR5 & ~INTC_IPR5_QSCI0_TDRE) | (INTC_IPR5_QSCI0_TDRE_1) );
    QSCI0_CTRL2 = 0; //disable fifo
#elif FMSTR_SCI_BASE == 0xE090
    INTC_IPR4 = ( (INTC_IPR4 & ~INTC_IPR4_QSCI1_RCV) | (INTC_IPR4_QSCI1_RCV_1) );
    INTC_IPR5 = ( (INTC_IPR5 & ~INTC_IPR5_QSCI1_TDRE) | (INTC_IPR5_QSCI1_TDRE_1) );
    QSCI1_CTRL2 = 0; //disable fifo
#endif
    
#elif FMSTR_USE_JTAG
    //set "EOnCE Transmit Register Empty"  and "EOnCE Receive Register Full"
    INTC_IPR0 = ( (INTC_IPR0 & ~INTC_IPR0_TX_REG) | (INTC_IPR0_TX_REG_0) );
    INTC_IPR0 = ( (INTC_IPR0 & ~INTC_IPR0_RX_REG) | (INTC_IPR0_RX_REG_0) );
#endif
}

