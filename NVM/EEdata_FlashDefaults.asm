 ;/******************************************************************************
 ;*
 ;* Freescale Semiconductor Inc.
 ;* (c) Copyright 2013 Freescale Semiconductor
 ;* ALL RIGHTS RESERVED.
 ;*
 ;*******************************************************************************/
 
 
    SECTION dflash
    org p:
 ; DFLASH Parameter Initialization Table:
 ;  Coil Configuration 
 DC $F31C    ;0x00
 DC $0001    ;0x02
 DC $0041    ;0x04
 DC $0190    ;0x06
 DC $0005    ;0x08
 DC $0005    ;0x0A
 DC $1964    ;0x0C
 DC $59D8    ;0x0E
 DC $4650    ;0x10
 DC $1F40    ;0x12
 DC $0FA0    ;0x14
 DC $003C    ;0x16
 DC $F31C    ;0x18
 DC $0001    ;0x1A
 DC $F31C    ;0x1C
 DC $0001    ;0x1E
 DC $03E8    ;0x20
 DC $61A8    ;0x22
 DC $1B58    ;0x24
 DC $0000    ;0x26
 DC $1B58    ;0x28
 DC $0000    ;0x2A
 DC $1B58    ;0x2C
 DC $0000    ;0x2E
 DC $0000    ;0x30
 DC $0000    ;0x32
 DC $0200    ;0x34
 DC $0000    ;0x36
 DC $0258    ;0x38
 DC $03E8    ;0x3A
 DC $0003    ;0x3C
 DC $0000    ;0x3E
 DC $0000    ;0x40
 DC $E000    ;0x42
 ;  Calibration configuration 
 DC $0F9A    ;0x44
 DC $5BC6    ;0x46
 DC $0020    ;0x48
 DC $0000    ;0x4A
 DC $01B4    ;0x4C
 DC $0000    ;0x4E
 DC $FDC0    ;0x50
 DC $FFFF    ;0x52
 DC $6DAC    ;0x54
 DC $0064    ;0x56
 DC $0005    ;0x58
 DC $0012    ;0x5A
 DC $7EE8    ;0x5C
 DC $79C3    ;0x5E
 DC $548A    ;0x60
 DC $001A    ;0x62
 DC $82B4    ;0x64
 DC $0013    ;0x66
 DC $007A    ;0x68
 DC $0000    ;0x6A
 DC $79A7    ;0x6C
 DC $0021    ;0x6E
 DC $65E8    ;0x70
 DC $0014    ;0x72
 DC $0011    ;0x74
 DC $0000    ;0x76
 DC $5551    ;0x78
 DC $001A    ;0x7A
 DC $9FE8    ;0x7C
 DC $0012    ;0x7E
 DC $0097    ;0x80
 DC $0000    ;0x82
 DC $89B1    ;0x84
 DC $0023    ;0x86
 DC $58BF    ;0x88
 DC $0013    ;0x8A
 DC $0016    ;0x8C
 DC $0000    ;0x8E
 DC $59AB    ;0x90
 DC $001A    ;0x92
 DC $A63F    ;0x94
 DC $0012    ;0x96
 DC $008A    ;0x98
 DC $0000    ;0x9A
 DC $8F0D    ;0x9C
 DC $0021    ;0x9E
 DC $4D3A    ;0xA0
 DC $0012    ;0xA2
 DC $FF98    ;0xA4
 DC $0000    ;0xA6
 DC $0321    ;0xA8
 DC $0000    ;0xAA
 DC $FD24    ;0xAC
 DC $0001    ;0xAE
 DC $000D    ;0xB0
 DC $0000    ;0xB2
 DC $01D9    ;0xB4
 DC $0000    ;0xB6
 DC $D804    ;0xB8
 DC $FFFF    ;0xBA
 DC $0008    ;0xBC
 DC $0000    ;0xBE
 DC $0313    ;0xC0
 DC $0000    ;0xC2
 DC $A5C5    ;0xC4
 DC $0001    ;0xC6
 DC $000D    ;0xC8
 DC $0000    ;0xCA
 DC $01D6    ;0xCC
 DC $0000    ;0xCE
 DC $D118    ;0xD0
 DC $FFFF    ;0xD2
 DC $0008    ;0xD4
 DC $0000    ;0xD6
 DC $01A9    ;0xD8
 DC $0000    ;0xDA
 DC $FF77    ;0xDC
 DC $0000    ;0xDE
 DC $000C    ;0xE0
 DC $0000    ;0xE2
 DC $01E1    ;0xE4
 DC $0000    ;0xE6
 DC $CFBE    ;0xE8
 DC $FFFF    ;0xEA
 DC $0008    ;0xEC
 DC $0000    ;0xEE
 DC $3AC4    ;0xF0
 DC $0001    ;0xF2
 DC $0C6A    ;0xF4
 DC $0000    ;0xF6
 DC $39C1    ;0xF8
 DC $0001    ;0xFA
 DC $0CBA    ;0xFC
 DC $0000    ;0xFE
 DC $3BC8    ;0x100
 DC $0001    ;0x102
 DC $0BAC    ;0x104
 DC $0000    ;0x106
 DC $0000    ;0x108
 DC $1B00    ;0x10A
 
    ENDSEC 
 
     end
