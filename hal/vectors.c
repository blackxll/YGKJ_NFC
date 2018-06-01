/*******************************************************************************
*
* Copyright 2012-2016 Freescale Semiconductor, Inc.
* Copyright 2017~2018 NXP.
* All rights reserved.
*******************************************************************************/

#include "defines.h"
#include "appcfg.h"
#include "event.h"


#define MWCT10xx_ISR_COP_RESET                   init_MWCT10xx_ISR_HW_RESET // COP reset vector is same as HW reset
#define MWCT10xx_ISR_ILLEGALOP                   MWCT10xx_ISR_UNDEFINED
#define MWCT10xx_ISR_SWI3                        MWCT10xx_ISR_UNDEFINED
#define MWCT10xx_ISR_OVERFLOW                    MWCT10xx_ISR_UNDEFINED
#define MWCT10xx_ISR_MISALIGNED                  MWCT10xx_ISR_UNDEFINED
#define MWCT10xx_ISR_STPCNT                      MWCT10xx_ISR_UNDEFINED
#define MWCT10xx_ISR_BKPT                        MWCT10xx_ISR_UNDEFINED
#define MWCT10xx_ISR_TRBUF                       MWCT10xx_ISR_UNDEFINED
//#define MWCT10xx_ISR_TX_REG                      MWCT10xx_ISR_UNDEFINED
//#define MWCT10xx_ISR_RX_REG                      MWCT10xx_ISR_UNDEFINED
#define MWCT10xx_ISR_BUS_ERR                     MWCT10xx_ISR_UNDEFINED
#define MWCT10xx_ISR_UNUSED                      MWCT10xx_ISR_UNDEFINED
#define MWCT10xx_ISR_SWI2                        MWCT10xx_ISR_UNDEFINED
#define MWCT10xx_ISR_SWI1                        MWCT10xx_ISR_UNDEFINED
#define MWCT10xx_ISR_SWI0                        MWCT10xx_ISR_UNDEFINED
#define MWCT10xx_ISR_XBARA                       MWCT10xx_ISR_UNDEFINED
#define MWCT10xx_ISR_LVI1                        MWCT10xx_ISR_UNDEFINED
#define MWCT10xx_ISR_OCCS                        MWCT10xx_ISR_UNDEFINED
#define MWCT10xx_ISR_TMRA_3                      MWCT10xx_ISR_UNDEFINED
//#define MWCT10xx_ISR_TMRA_2                      MWCT10xx_ISR_UNDEFINED
#if !DIGITAL_DCDC
#define MWCT10xx_ISR_TMRA_1                      MWCT10xx_ISR_UNDEFINED
#endif
//#define MWCT10xx_ISR_TMRA_0                      MWCT10xx_ISR_UNDEFINED
#define MWCT10xx_ISR_TMRB_0                      MWCT10xx_ISR_UNDEFINED
#if DIGITAL_DCDC
#define MWCT10xx_ISR_ADC_CC0                     MWCT10xx_ISR_UNDEFINED
#endif
#define MWCT10xx_ISR_ADC_CC1                     MWCT10xx_ISR_UNDEFINED
#define MWCT10xx_ISR_ADC_ERR                     MWCT10xx_ISR_UNDEFINED
#define MWCT10xx_ISR_DMA3                        MWCT10xx_ISR_UNDEFINED
#define MWCT10xx_ISR_DMA2                        MWCT10xx_ISR_UNDEFINED
#define MWCT10xx_ISR_DMA1                        MWCT10xx_ISR_UNDEFINED
//#define MWCT10xx_ISR_DMA0                        MWCT10xx_ISR_UNDEFINED

#if CAN_SUPPORTED
#else
#define MWCT10xx_ISR_CAN_MB                      MWCT10xx_ISR_UNDEFINED
#define MWCT10xx_ISR_CAN_BUSOFF                  MWCT10xx_ISR_UNDEFINED
#define MWCT10xx_ISR_CAN_ERROR                   MWCT10xx_ISR_UNDEFINED
#define MWCT10xx_ISR_CAN_TX_WARN                 MWCT10xx_ISR_UNDEFINED
#define MWCT10xx_ISR_CAN_RX_WARN                 MWCT10xx_ISR_UNDEFINED
#define MWCT10xx_ISR_CAN_WAKEUP                  MWCT10xx_ISR_UNDEFINED
#endif
#define MWCT10xx_ISR_QSCI1_RERR                  MWCT10xx_ISR_UNDEFINED
#define MWCT10xx_ISR_QSCI1_TIDLE                 MWCT10xx_ISR_UNDEFINED
#if DIGITAL_DCDC
#define MWCT10xx_ISR_QSCI1_RCV                   MWCT10xx_ISR_UNDEFINED
#define MWCT10xx_ISR_QSCI1_TDRE                  MWCT10xx_ISR_UNDEFINED
#endif
#define MWCT10xx_ISR_QSCI0_RERR                  MWCT10xx_ISR_UNDEFINED
#define MWCT10xx_ISR_QSCI0_TIDLE                 MWCT10xx_ISR_UNDEFINED
//#define MWCT10xx_ISR_QSCI0_RCV                   MWCT10xx_ISR_UNDEFINED
//#define MWCT10xx_ISR_QSCI0_TDRE                  MWCT10xx_ISR_UNDEFINED
#define MWCT10xx_ISR_QSPI1_XMIT                  MWCT10xx_ISR_UNDEFINED
#define MWCT10xx_ISR_QSPI1_RCV                   MWCT10xx_ISR_UNDEFINED
#define MWCT10xx_ISR_QSPI0_XMIT                  MWCT10xx_ISR_UNDEFINED
#define MWCT10xx_ISR_QSPI0_RCV                   MWCT10xx_ISR_UNDEFINED 
#define MWCT10xx_ISR_IIC0                        MWCT10xx_ISR_UNDEFINED
#define MWCT10xx_ISR_PWMA_FAULT                  MWCT10xx_ISR_UNDEFINED
#define MWCT10xx_ISR_PWMA_RERR                   MWCT10xx_ISR_UNDEFINED
#define MWCT10xx_ISR_PWMA_RELOAD3                MWCT10xx_ISR_UNDEFINED
#define MWCT10xx_ISR_PWMA_CAP                    MWCT10xx_ISR_UNDEFINED
#define MWCT10xx_ISR_PWMA_CMP3                   MWCT10xx_ISR_UNDEFINED
#define MWCT10xx_ISR_PWMA_RELOAD2                MWCT10xx_ISR_UNDEFINED
#define MWCT10xx_ISR_PWMA_CMP2                   MWCT10xx_ISR_UNDEFINED
#define MWCT10xx_ISR_PWMA_RELOAD1                MWCT10xx_ISR_UNDEFINED
#define MWCT10xx_ISR_PWMA_CMP1                   MWCT10xx_ISR_UNDEFINED
#define MWCT10xx_ISR_PWMA_RELOAD0                MWCT10xx_ISR_UNDEFINED
#define MWCT10xx_ISR_PWMA_CMP0                   MWCT10xx_ISR_UNDEFINED
#define MWCT10xx_ISR_FTFA_RDCOL                  MWCT10xx_ISR_UNDEFINED
#define MWCT10xx_ISR_FTFA_CC                     MWCT10xx_ISR_UNDEFINED
#define MWCT10xx_ISR_CMPD                        MWCT10xx_ISR_UNDEFINED
#define MWCT10xx_ISR_CMPC                        MWCT10xx_ISR_UNDEFINED
#define MWCT10xx_ISR_CMPB                        MWCT10xx_ISR_UNDEFINED
#define MWCT10xx_ISR_CMPA                        MWCT10xx_ISR_UNDEFINED
//#define MWCT10xx_ISR_PIT1_ROLLOVER 				 MWCT10xx_ISR_UNDEFINED
#define MWCT10xx_ISR_PIT0_ROLLOVER               MWCT10xx_ISR_UNDEFINED
#define MWCT10xx_ISR_GPIOE                       MWCT10xx_ISR_UNDEFINED
#define MWCT10xx_ISR_GPIOD                       MWCT10xx_ISR_UNDEFINED
#define MWCT10xx_ISR_GPIOC                       MWCT10xx_ISR_UNDEFINED
#define MWCT10xx_ISR_GPIOB                       MWCT10xx_ISR_UNDEFINED
#define MWCT10xx_ISR_GPIOA                       MWCT10xx_ISR_UNDEFINED
#define MWCT10xx_ISR_GPIOF                       MWCT10xx_ISR_UNDEFINED
#define MWCT10xx_ISR_COP_INT                     MWCT10xx_ISR_UNDEFINED
#define MWCT10xx_ISR_EWM_INT                     MWCT10xx_ISR_UNDEFINED
#define MWCT10xx_ISR_SWILP                       MWCT10xx_ISR_UNDEFINED

void init_MWCT10xx_ISR_HW_RESET(void);
volatile asm void _vect(void);

#pragma define_section interrupt_vectors "interrupt_vectors.text"  RX
#pragma section interrupt_vectors begin
volatile asm void _vect(void)
{
#if (BOOTLOADER_USED)
	JMP 0x1F400
	JMP 0x1F400
#else    	
	JMP init_MWCT10xx_ISR_HW_RESET    // Reserved for Reset Overlay                     0x00  0
    JMP MWCT10xx_ISR_COP_RESET        // Reserved for COP Reset Overlay                 0x02  1
#endif    
    JSR MWCT10xx_ISR_ILLEGALOP        // Illegal Instruction                       3    0x04  2
    JSR MWCT10xx_ISR_SWI3             // SW Interrupt 3                            3    0x06  3
    JSR MWCT10xx_ISR_OVERFLOW         // Hardware Stack Overflow                   3    0x08  4
    JSR MWCT10xx_ISR_MISALIGNED       // Misaligned Data Access                    3    0x0A  5
    JSR MWCT10xx_ISR_STPCNT           // EOnCE Step Counter Interrupt              1-3  0x0C  6
    JSR MWCT10xx_ISR_BKPT             // EOnCE Breakpoint Unit                     1-3  0x0E  7
    JSR MWCT10xx_ISR_TRBUF            // EOnCE Trace Buffer Interrupt              1-3  0x10  8
    JSR MWCT10xx_ISR_TX_REG           // EOnCE Transmit Register Empty             1-3  0x12  9
    JSR MWCT10xx_ISR_RX_REG           // EOnCE Receive Register Full               1-3  0x14  10
    JSR MWCT10xx_ISR_BUS_ERR          // Bus Error Interrupt                       1-3  0x16  11
    JSR MWCT10xx_ISR_UNUSED           // Reserved                                  1-3  0x18  12
    JSR MWCT10xx_ISR_UNUSED           // Reserved                                  1-3  0x1A  13
    JSR MWCT10xx_ISR_UNUSED           // Reserved                                  1-3  0x1C  14
    JSR MWCT10xx_ISR_SWI2             // SW Interrupt 2                            1-3  0x1E  15
    JSR MWCT10xx_ISR_SWI1             // SW Interrupt 1                            1-3  0x20  16
    JSR MWCT10xx_ISR_SWI0             // SW Interrupt 0                            1-3  0x22  17
    JSR MWCT10xx_ISR_XBARA            // Crossbar Interupt                         1-3  0x24  18
    JSR MWCT10xx_ISR_LVI1             // Low Voltage Interrupt                     1-3  0x26  19
    JSR MWCT10xx_ISR_OCCS             // PLL Error Interrupt                       1-3  0x28  20
    JSR MWCT10xx_ISR_UNUSED           // Reserved                                  0-2  0x2A  21
    JSR MWCT10xx_ISR_UNUSED           // Reserved                                  0-2  0x2C  22
    JSR MWCT10xx_ISR_UNUSED           // Reserved                                  0-2  0x2E  23
    JSR MWCT10xx_ISR_TMRB_0           // Timer B0 Interrupt                        0-2  0x30  24
    JSR MWCT10xx_ISR_TMRA_3           // Timer A3 Interrupt                        0-2  0x32  25
    JSR MWCT10xx_ISR_TMRA_2           // Timer A2 Interrupt                        0-2  0x34  26
    JSR MWCT10xx_ISR_TMRA_1           // Timer A1 Interrupt                        0-2  0x36  27
    JSR MWCT10xx_ISR_TMRA_0           // Timer A0 Interrupt                        0-2  0x38  28
    JSR MWCT10xx_ISR_ADC_CC1          // Cyclic ADC Conversion Complete            0-2  0x3A  29
    JSR MWCT10xx_ISR_ADC_CC0          // Cyclic ADC Conversion Complete            0-2  0x3C  30
    JSR MWCT10xx_ISR_ADC_ERR          // Cyclic ADC Zero Corssing or Limit         0-2  0x3E  31
    JSR MWCT10xx_ISR_UNUSED           // Reserved                                  0-2  0x40  32
    JSR MWCT10xx_ISR_DMA3             // DMA 3 Service Request                     0-2  0x42  33
    JSR MWCT10xx_ISR_DMA2             // DMA 2 Service Request                     0-2  0x44  34
    JSR MWCT10xx_ISR_DMA1             // DMA 1 Service Request                     0-2  0x46  35
    JSR MWCT10xx_ISR_DMA0             // DMA 0 Service Request                     0-2  0x48  36
    JSR MWCT10xx_ISR_CAN_MB           // Reserved                                  0-2  0x4A  37
    JSR MWCT10xx_ISR_CAN_BUSOFF       // Reserved                                  0-2  0x4C  38
    JSR MWCT10xx_ISR_CAN_ERROR        // CAN Error                                 0-2  0x4E  39
    JSR MWCT10xx_ISR_CAN_TX_WARN      // CAN Transmiter Warning                    0-2  0x50  40
    JSR MWCT10xx_ISR_CAN_RX_WARN      // CAN Receive Warning                       0-2  0x52  41
    JSR MWCT10xx_ISR_CAN_WAKEUP       // CAN Wakeup                                0-2  0x54  42
    JSR MWCT10xx_ISR_UNUSED           // Reserved                                  0-2  0x56  43
    JSR MWCT10xx_ISR_UNUSED           // Reserved                                  0-2  0x58  44
    JSR MWCT10xx_ISR_UNUSED           // Reserved                                  0-2  0x5A  45
    JSR MWCT10xx_ISR_UNUSED           // Reserved                                  0-2  0x5C  46
    JSR MWCT10xx_ISR_QSCI1_RERR       // SCI1 Receiver Error                       0-2  0x5E  47
    JSR MWCT10xx_ISR_QSCI1_RCV        // SCI1 Data Received                        0-2  0x60  48
    JSR MWCT10xx_ISR_QSCI1_TIDLE      // SCI1 Transmitter Idle                     0-2  0x62  49
    JSR MWCT10xx_ISR_QSCI1_TDRE       // SCI1 Transmit Data Register Empty         0-2  0x64  50
    JSR MWCT10xx_ISR_QSCI0_RERR       // SCI0 Receiver Error                       0-2  0x66  51
    JSR MWCT10xx_ISR_QSCI0_RCV        // SCI0 Data Received                        0-2  0x68  52
    JSR MWCT10xx_ISR_QSCI0_TIDLE      // SCI0 Transmitter Idle                     0-2  0x6A  53
    JSR MWCT10xx_ISR_QSCI0_TDRE       // SCI0 Transmit Data Register Empty         0-2  0x6C  54
    JSR MWCT10xx_ISR_UNUSED           // Reserved                                  0-2  0x6E  55
    JSR MWCT10xx_ISR_UNUSED           // Reserved                                  0-2  0x70  56
    JSR MWCT10xx_ISR_QSPI1_XMIT       // SPI1 Transmitter                          0-2  0x72  57
    JSR MWCT10xx_ISR_QSPI1_RCV        // SPI1 Receiver Ful                         0-2  0x74  58
    JSR MWCT10xx_ISR_QSPI0_XMIT       // SPI0 Transmitter                          0-2  0x76  59
    JSR MWCT10xx_ISR_QSPI0_RCV        // SPI0 Receiver Ful                         0-2  0x78  60
    JSR MWCT10xx_ISR_UNUSED           // Reserved                                  0-2  0x7A  61
    JSR MWCT10xx_ISR_IIC0             // I2C0 Interrupt                            0-2  0x7C  62
    JSR MWCT10xx_ISR_UNUSED           // Reserved                                  0-2  0x7E  63
    JSR MWCT10xx_ISR_UNUSED           // Reserved                                  0-2  0x80  64
    JSR MWCT10xx_ISR_UNUSED           // Reserved                                  0-2  0x82  65
    JSR MWCT10xx_ISR_UNUSED           // Reserved                                  0-2  0x84  66
    JSR MWCT10xx_ISR_UNUSED           // Reserved                                  0-2  0x86  67
    JSR MWCT10xx_ISR_UNUSED           // Reserved                                  0-2  0x88  68
    JSR MWCT10xx_ISR_UNUSED           // Reserved                                  0-2  0x8A  69
    JSR MWCT10xx_ISR_UNUSED           // Reserved                                  0-2  0x8C  70
    JSR MWCT10xx_ISR_UNUSED           // Reserved                                  0-2  0x8E  71
    JSR MWCT10xx_ISR_UNUSED           // Reserved                                  0-2  0x90  72
    JSR MWCT10xx_ISR_UNUSED           // Reserved                                  0-2  0x92  73
    JSR MWCT10xx_ISR_UNUSED           // Reserved                                  0-2  0x94  74
    JSR MWCT10xx_ISR_UNUSED           // Reserved                                  0-2  0x96  75
    JSR MWCT10xx_ISR_UNUSED           // Reserved                                  0-2  0x98  76
    JSR MWCT10xx_ISR_PWMA_FAULT       // PWMA Fault                                0-2  0x9A  77
    JSR MWCT10xx_ISR_PWMA_RERR        // PWMA Reload Error                         0-2  0x9C  78
    JSR MWCT10xx_ISR_PWMA_RELOAD3     // PWMA Submodule 3 Reload                   0-2  0x9E  79
    JSR MWCT10xx_ISR_PWMA_CAP         // PWMA Input Capture                        0-2  0xA0  80
    JSR MWCT10xx_ISR_PWMA_CMP3        // PWMA Submodule 3 Compare                  0-2  0xA2  81
    JSR MWCT10xx_ISR_PWMA_RELOAD2     // PWMA Submodule 2 Reload                   0-2  0xA4  82
    JSR MWCT10xx_ISR_PWMA_CMP2        // PWMA Submodule 2 Compare                  0-2  0xA6  83
    JSR MWCT10xx_ISR_PWMA_RELOAD1     // PWMA Submodule 1 Reload                   0-2  0xA8  84
    JSR MWCT10xx_ISR_PWMA_CMP1        // PWMA Submodule 1 Compare                  0-2  0xAA  85
    JSR MWCT10xx_ISR_PWMA_RELOAD0     // PWMA Submodule 0 Reload                   0-2  0xAC  86
    JSR MWCT10xx_ISR_PWMA_CMP0        // PWMA Submodule 0 Compare                  0-2  0xAE  87
    JSR MWCT10xx_ISR_FTFA_RDCOL       // Flash Access Error                        0-2  0xB0  88
    JSR MWCT10xx_ISR_FTFA_CC          // Flash Command Complete                    0-2  0xB2  89
    JSR MWCT10xx_ISR_CMPD             // Comparator D Interrupt                    0-2  0xB4  90
    JSR MWCT10xx_ISR_CMPC             // Comparator C Interrupt                    0-2  0xB6  91
    JSR MWCT10xx_ISR_CMPB             // Comparator B Interrupt                    0-2  0xB8  92
    JSR MWCT10xx_ISR_CMPA             // Comparator A Interrupt                    0-2  0xBA  93
    JSR MWCT10xx_ISR_PIT1_ROLLOVER    // Programable Int Timer 1 Rollover          0-2  0xBC  94
    JSR MWCT10xx_ISR_PIT0_ROLLOVER    // Programable Int Timer 0 Rollover          0-2  0xBE  95
    JSR MWCT10xx_ISR_UNUSED           // Reserved                                  0-2  0xC0  96
    JSR MWCT10xx_ISR_UNUSED           // Reserved                                  0-2  0xC2  97
    JSR MWCT10xx_ISR_UNUSED           // Reserved                                  0-2  0xC4  98
    JSR MWCT10xx_ISR_UNUSED           // Reserved                                  0-2  0xC6  99
    JSR MWCT10xx_ISR_UNUSED           // Reserved                                  0-2  0xC8  100
    JSR MWCT10xx_ISR_UNUSED           // Reserved                                  0-2  0xCA  101
    JSR MWCT10xx_ISR_GPIOF            // GPIOF Interrupt                           0-2  0xCC  102
    JSR MWCT10xx_ISR_GPIOE            // GPIOE Interrupt                           0-2  0xCE  103
    JSR MWCT10xx_ISR_GPIOD            // GPIOD Interrupt                           0-2  0xD0  104
    JSR MWCT10xx_ISR_GPIOC            // GPIOC Interrupt                           0-2  0xD2  105
    JSR MWCT10xx_ISR_GPIOB            // GPIOB Interrupt                           0-2  0xD4  106
    JSR MWCT10xx_ISR_GPIOA            // GPIOA Interrupt                           0-2  0xD6  107
    JSR MWCT10xx_ISR_COP_INT          // COP Warning                               0-2  0xD8  108
    JSR MWCT10xx_ISR_EWM_INT          // External Watchdog Indicate                0-2  0xDA  109
    JSR MWCT10xx_ISR_SWILP            // SWILP                                     -1   0xDC  110
}
#pragma section interrupt_vectors end

#pragma define_section FlashConfig "flash_config.text"  RX
#pragma section FlashConfig begin
const uint8 flashConfig[] =
{
	0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF,
	0xFE, 0xFF, 0xFF, 0xFF
};
#pragma section FlashConfig end

