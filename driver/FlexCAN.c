/*******************************************************************************
*
* Copyright 2012-2014 Freescale Semiconductor, Inc.
* Copyright 2017~2018 NXP.
* All rights reserved.
*******************************************************************************/
/* Including shared modules, which are used for whole project */
//#include "platformhal.h"
#include "defines.h"
#include "cpu.h"

#include "systemPrint.h"

#include "FlexCAN.h"

// # of MBs supported
#define NUMBER_OF_MB		16

/* CAN - Peripheral register structure */
typedef struct CAN_MemMap {
  uint32 MCR;                                    /*!< Module Configuration Register, offset: 0x0 */
  uint32 CTRL1;                                  /*!< Control 1 Register, offset: 0x4 */
  uint32 TIMER;                                  /*!< Free Running Timer, offset: 0x8 */
  uint8  RESERVED_0[4];
  uint32 RXMGMASK;                               /*!< Rx Mailboxes Global Mask Register, offset: 0x10 */
  uint32 RX14MASK;                               /*!< Rx 14 Mask Register, offset: 0x14 */
  uint32 RX15MASK;                               /*!< Rx 15 Mask Register, offset: 0x18 */
  uint32 ECR;                                    /*!< Error Counter, offset: 0x1C */
  uint32 ESR1;                                   /*!< Error and Status 1 Register, offset: 0x20 */
  uint32 IMASK2;                                 /*!< Interrupt Masks 2 Register, offset: 0x24 */
  uint32 IMASK1;                                 /*!< Interrupt Masks 1 Register, offset: 0x28 */
  uint32 IFLAG2;                                 /*!< Interrupt Flags 2 Register, offset: 0x2C */
  uint32 IFLAG1;                                 /*!< Interrupt Flags 1 Register, offset: 0x30 */
  uint32 CTRL2;                                  /*!< Control 2 Register, offset: 0x34 */
  uint32 ESR2;                                   /*!< Error and Status 2 Register, offset: 0x38 */
  uint32 IMEUR;                                  /*!< Individual Matching Elements Update Register, offset: 0x3C */
  uint32 LRFR;                                   /*!< Lost Rx Frames Register, offset: 0x40 */
  uint32 CRCR;                                   /*!< CRC Register, offset: 0x44 */
  uint32 RXFGMASK;                               /*!< Rx FIFO Global Mask Register, offset: 0x48 */
  uint32 RXFIR;                                  /*!< Rx FIFO Information Register, offset: 0x4C */
  uint8  RESERVED_1[48];
  struct {                                       /* offset: 0x80, array step: 0x10 */
    uint32 CS;                                   /*!< Message Buffer 0 CS Register..Message Buffer 15 CS Register, array offset: 0x80, array step: 0x10 */
    uint32 ID;                                   /*!< Message Buffer 0 ID Register..Message Buffer 15 ID Register, array offset: 0x84, array step: 0x10 */
    uint32 WORD0;                                /*!< Message Buffer 0 WORD0 Register..Message Buffer 15 WORD0 Register, array offset: 0x88, array step: 0x10 */
    uint32 WORD1;                                /*!< Message Buffer 0 WORD1 Register..Message Buffer 15 WORD1 Register, array offset: 0x8C, array step: 0x10 */
  } MB[NUMBER_OF_MB];
  uint8 RESERVED_2[1792];
  uint32 RXIMR[16];                              /*!< Rx Individual Mask Registers, array offset: 0x880, array step: 0x4 */
} volatile *CAN_MemMapPtr;


/*=================================================================================================================*/
// Definition of FlexCAN control bits
#define FLEXCAN_CLOCK_SOURCE      1       /* clock source for FlexCAN: 0 -- external oscillator, 1 -- bus clock from PLL */
#define FLEXCAN_SELF_RECEPTION    0       /* enable self-reception: 0 - disable self-reception; 1 -- enable self-reception */
#define FLEXCAN_SCAN_PRIORITY     0       /* scan prio: 0 -- RxFIFO first, 1 -- MB first */ 
#define FLEXCAN_STORE_RTR         0       /* remote request storing: 0 -- do not store RTR, 1 -- store RTR as data frame */
#define FLEXCAN_ENTIRE_ARB_CMP    1       /* entire frame arbitration filed comparision including IDE,RTR, excluding RxFIFO 
                                           * 0 -- always compare ID of Rx MB regardless of masks, but not compare RTR
                                           * 1 -- mask compare both Rx MB ID and RTR
                                           */ 

#define FLEXCAN_INDIVIDUAL_MASK   1       /* individual Rx Masking and queue enable:
                                           * 0 -- message queue is enabled, and masking with RXMGMASK/RX14MASK/RX15MASK,RXFGMASK
                                           * 1 -- message queue is disabled, and individual masking with RXIMRn
                                           */
 
#define FLEXCAN_LOCAL_PRIO        0       /* local priority enable: 0 -- disabled, 1 -- enabled */
#define FLEXCAN_LOOP_BACK         0       /* loop back for single node: 0 -- no loopback, 1 -- loopback */
/* NOTE: if use LOOPBACK, self-reception must be enabled by defining
 * #define FLEXCAN_SELF_RECEPTION  1
 */

/* # of Rx FIFO filters: 0 -- Rx FIFO disabled, or 8/16/24/32/40 */
#define FLEXCAN_NO_RXFIFO_FILTERS 0
#define FLEXCAN_ID_TAB_FORMAT     0       /* Rx FIFO ID table format: 0 -- Format A, 1 -- Format B, 2 -- Format C, 3 -- Format D */

// Configuration of MBs
#define NUMBER_OF_RX_MB		12		      /* # of Receive MBs, can be changed per application */
#define NUMBER_OF_TX_MB		(NUMBER_OF_MB-NUMBER_OF_RX_MB)	/* # of MBs are  for Transmit MBs */

// Definition of max # of custom mails
#define MAX_RX_MAILS	  20
#define MAX_TX_MAILS      20
#define MAX_RXFIFO_MAILS  10
/*=====================================================================================================================*/

// Define Acceptance IDs for ID filter table
#if (FLEXCAN_NO_RXFIFO_FILTERS > 0)

  #define ID_TABLE0_ID      (1 | CAN_MSG_IDE_MASK)       /* if the ID is in standard format, remove CAN_MSG_IDE_MASK */
  #define ID_TABLE1_ID      (2 | CAN_MSG_IDE_MASK)       /* if the ID is in standard format, remove CAN_MSG_IDE_MASK */
  #define ID_TABLE2_ID      (3 | CAN_MSG_IDE_MASK)       /* if the ID is in standard format, remove CAN_MSG_IDE_MASK */
  #define ID_TABLE3_ID      (4 | CAN_MSG_IDE_MASK)       /* if the ID is in standard format, remove CAN_MSG_IDE_MASK */
  #define ID_TABLE4_ID      (5 | CAN_MSG_IDE_MASK)       /* if the ID is in standard format, remove CAN_MSG_IDE_MASK */
  #define ID_TABLE5_ID      (6 | CAN_MSG_IDE_MASK)       /* if the ID is in standard format, remove CAN_MSG_IDE_MASK */
  #define ID_TABLE6_ID      (7 | CAN_MSG_IDE_MASK)       /* if the ID is in standard format, remove CAN_MSG_IDE_MASK */
  #define ID_TABLE7_ID      (8 | CAN_MSG_IDE_MASK)       /* if the ID is in standard format, remove CAN_MSG_IDE_MASK */

#endif

#if (FLEXCAN_NO_RXFIFO_FILTERS > 8)

  #define ID_TABLE8_ID      (9 | CAN_MSG_IDE_MASK)       /* if the ID is in standard format, remove CAN_MSG_IDE_MASK */
  #define ID_TABLE9_ID      (10 | CAN_MSG_IDE_MASK)       /* if the ID is in standard format, remove CAN_MSG_IDE_MASK */
  #define ID_TABLE10_ID     (11 | CAN_MSG_IDE_MASK)       /* if the ID is in standard format, remove CAN_MSG_IDE_MASK */
  #define ID_TABLE11_ID     (12 | CAN_MSG_IDE_MASK)       /* if the ID is in standard format, remove CAN_MSG_IDE_MASK */
  #define ID_TABLE12_ID     (13 | CAN_MSG_IDE_MASK)       /* if the ID is in standard format, remove CAN_MSG_IDE_MASK */
  #define ID_TABLE13_ID     (14 | CAN_MSG_IDE_MASK)       /* if the ID is in standard format, remove CAN_MSG_IDE_MASK */
  #define ID_TABLE14_ID     (15 | CAN_MSG_IDE_MASK)       /* if the ID is in standard format, remove CAN_MSG_IDE_MASK */
  #define ID_TABLE15_ID     (16 | CAN_MSG_IDE_MASK)       /* if the ID is in standard format, remove CAN_MSG_IDE_MASK */
#endif

#if (FLEXCAN_NO_RXFIFO_FILTERS > 16)

  #define ID_TABLE16_ID     (17 | CAN_MSG_IDE_MASK)       /* if the ID is in standard format, remove CAN_MSG_IDE_MASK */
  #define ID_TABLE17_ID     (18 | CAN_MSG_IDE_MASK)       /* if the ID is in standard format, remove CAN_MSG_IDE_MASK */
  #define ID_TABLE18_ID     (19 | CAN_MSG_IDE_MASK)       /* if the ID is in standard format, remove CAN_MSG_IDE_MASK */
  #define ID_TABLE19_ID     (20 | CAN_MSG_IDE_MASK)       /* if the ID is in standard format, remove CAN_MSG_IDE_MASK */
  #define ID_TABLE20_ID     (21 | CAN_MSG_IDE_MASK)       /* if the ID is in standard format, remove CAN_MSG_IDE_MASK */
  #define ID_TABLE21_ID     (22 | CAN_MSG_IDE_MASK)       /* if the ID is in standard format, remove CAN_MSG_IDE_MASK */
  #define ID_TABLE22_ID     (23 | CAN_MSG_IDE_MASK)       /* if the ID is in standard format, remove CAN_MSG_IDE_MASK */
  #define ID_TABLE23_ID     (24 | CAN_MSG_IDE_MASK)       /* if the ID is in standard format, remove CAN_MSG_IDE_MASK */
#endif

#if (FLEXCAN_NO_RXFIFO_FILTERS >24 )

  #define ID_TABLE24_ID     (25 | CAN_MSG_IDE_MASK)       /* if the ID is in standard format, remove CAN_MSG_IDE_MASK */
  #define ID_TABLE25_ID     (26 | CAN_MSG_IDE_MASK)       /* if the ID is in standard format, remove CAN_MSG_IDE_MASK */
  #define ID_TABLE26_ID     (27 | CAN_MSG_IDE_MASK)       /* if the ID is in standard format, remove CAN_MSG_IDE_MASK */
  #define ID_TABLE27_ID     (28 | CAN_MSG_IDE_MASK)       /* if the ID is in standard format, remove CAN_MSG_IDE_MASK */
  #define ID_TABLE28_ID     (29 | CAN_MSG_IDE_MASK)       /* if the ID is in standard format, remove CAN_MSG_IDE_MASK */
  #define ID_TABLE29_ID     (30 | CAN_MSG_IDE_MASK)       /* if the ID is in standard format, remove CAN_MSG_IDE_MASK */
  #define ID_TABLE30_ID     (31 | CAN_MSG_IDE_MASK)       /* if the ID is in standard format, remove CAN_MSG_IDE_MASK */
  #define ID_TABLE31_ID     (32 | CAN_MSG_IDE_MASK)       /* if the ID is in standard format, remove CAN_MSG_IDE_MASK */
#endif

#if (FLEXCAN_NO_RXFIFO_FILTERS >32 )

  #define ID_TABLE8_ID      (33 | CAN_MSG_IDE_MASK)       /* if the ID is in standard format, remove CAN_MSG_IDE_MASK */
  #define ID_TABLE9_ID      (34 | CAN_MSG_IDE_MASK)       /* if the ID is in standard format, remove CAN_MSG_IDE_MASK */
  #define ID_TABLE10_ID     (35 | CAN_MSG_IDE_MASK)       /* if the ID is in standard format, remove CAN_MSG_IDE_MASK */
  #define ID_TABLE11_ID     (36 | CAN_MSG_IDE_MASK)       /* if the ID is in standard format, remove CAN_MSG_IDE_MASK */
  #define ID_TABLE12_ID     (37 | CAN_MSG_IDE_MASK)       /* if the ID is in standard format, remove CAN_MSG_IDE_MASK */
  #define ID_TABLE13_ID     (38 | CAN_MSG_IDE_MASK)       /* if the ID is in standard format, remove CAN_MSG_IDE_MASK */
  #define ID_TABLE14_ID     (39 | CAN_MSG_IDE_MASK)       /* if the ID is in standard format, remove CAN_MSG_IDE_MASK */
  #define ID_TABLE15_ID     (40 | CAN_MSG_IDE_MASK)       /* if the ID is in standard format, remove CAN_MSG_IDE_MASK */
#endif


//========================= DONT touch below ==========================================//
#if ((FLEXCAN_LOOP_BACK) ^ (FLEXCAN_SELF_RECEPTION))
  #error  "FLEXCAN_LOOP_BACK must be same as FLEXCAN_SELF_RECEPTION\r\n"
#endif

// Check limit of # of Rx FIFO Filters
#if ((FLEXCAN_NO_RXFIFO_FILTERS != 0) && (FLEXCAN_NO_RXFIFO_FILTERS != 8) && (FLEXCAN_NO_RXFIFO_FILTERS != 16) && (FLEXCAN_NO_RXFIFO_FILTERS != 24) && (FLEXCAN_NO_RXFIFO_FILTERS != 32)&& (FLEXCAN_NO_RXFIFO_FILTERS != 40))
  #error  "FLEXCAN_NO_RXFIFO_FILTERS is out of range!\r\n"
#endif

// Check MB # to see if it is out of range when Rx FIFO is enabled
#if ((FLEXCAN_NO_RXFIFO_FILTERS>0))
  #if (FLEXCAN_RX_MB_START < 8u)
    #error  "FLEXCAN_RX_MB_START must be > 7 when Rx FIFO is enabled depending on FLEXCAN_NO_RXFIFO_FILTERS !\r\n"
  #endif
  #if (FLEXCAN_TX_MB_START < 8u)
  #error  "FLEXCAN_TX_MB_START must be > 7 when Rx FIFO is enabled depending on FLEXCAN_NO_RXFIFO_FILTERS!\r\n"
  #endif
#endif

/*=================================================================================================================*/

// mail box
typedef volatile struct {	
   uint8	dev_num;		    /*  FlexCAN device number */
   uint16	mailbox_number; 	/*  mailbox number */
   uint32	identifier;   		/*  message ID */
   uint8	format;				/*  mailbox format (FLEXCAN_STANDARD OR FLEXCAN_EXTENDED) */ 
   uint8	direction; 			/*  transmission or reception direction (FLEXCAN_TX or FLEXCAN_RX) */
   uint8	remote_req_flag;	/*  is remote request?   */  
   uint8	data_len;			/*  number of bytes to write to or read from the mailbox (0 to 8) */ 
   uint8	data[8]; 			/*  data bytes */
   uint8	code;				/* control code */ 
   uint8	priority;			/*  priority of the message in the mailbox */
   uint32	timestamp;			/*  timestamp */
   uint16	crc;				/*  crc of the message frame */
   uint32	imask;				/* individual mask */
 } TFCAN_MailBox, *PTFCAN_MailBox;

// Driver structure
typedef  union {
	uint16       Byte;
	struct{
		uint16        bClockSource: 1;                    /* clock source for FlexCAN: 0 -- external oscillator, 1 -- bus clock from PLL */
		uint16        bSelfRxDis:   1;                    /* disable self-reception: 0 - enable self-reception; 1 -- disable self-reception */
		uint16        bScanPrio:    1;                    /* scan prio: 0 -- RxFIFO first, 1 -- MB first */    
		uint16        bStoreRTR:    1;                    /* remote request storing */
		uint16        bEACEN:       1;                    /* entire frame arbitration filed comparision including IDE,RTR, excluding RxFIFO */
		uint16        bIRMQ:        1;                    /* individual Rx Masking and queue enable */
		uint16        bLoopBack:    1;                    /* loopback enable */
		uint16        bLocalPrio:   1;                    /* local priority enable */
		uint16        bResvd:       8;                    /* reserved */
	}Bits;
} TCtrlWord;

typedef volatile struct {
  CAN_MemMapPtr pCANRegs;                      /* pointer to CAN register structure */
  uint8        bRxFIFOFilterNo;                /* no of RxFIFO filters: valid values are 
                                                * 0 -- disable RxFIFO
                                                * 8/16/24/32/40
                                                */
  uint8        bRxFIFOIDFormat;                /* Rx FIFO ID table format: 0 -- Format A, 1 -- Format B, 2 -- Format C, 3 -- Format D */
  TCtrlWord    ctrlWord;                       /* control word */
  uint8        bRxMBStart;                     /* start index of Rx MB# */
  uint8        bRxMBEnd;                       /* end index of Rx MB# */
  uint8        bTxMBStart;                     /* start index of Tx MB# */
  uint8        bTxMBEnd;                       /* end index of Tx MB# */
} TFCANDrv_Obj, *PTFCANDrv_Obj;


#define FLEXCAN_CTRL_PROPSEG(x)        (((x)&0x00000007L)<<0)
#define FLEXCAN_CTRL_PSEG2(x)          (((x)&0x00000007L)<<16)
#define FLEXCAN_CTRL_PSEG1(x)          (((x)&0x00000007L)<<19)
#define FLEXCAN_CTRL_RJW(x)            (((x)&0x00000003L)<<22)
#define FLEXCAN_CTRL_PRESDIV(x)        (((x)&0x000000FFL)<<24)

#define FLEXCAN_MB_CODE_TX_INACTIVE	        (8)
#define FLEXCAN_MB_CODE_TX_ABORT	        (9)
#define FLEXCAN_MB_CODE_TX_ONCE		        (0x0C)
#define FLEXCAN_MB_CODE_TX_RESPONSE	        (0x0A)
#define FLEXCAN_MB_CODE_TX_RESPONSE_TEMPO   (0x0E)

#define FLEXCAN_MB_CODE_RX_EMPTY	        (4)
#define FLEXCAN_MB_CODE_RX_INACTIVE	        (0)

#define FLEXCAN_MB_CS_LENGTH(x)     ((((uint32)(x))&0x0000000F)<<16)
#define FLEXCAN_MB_CS_CODE(x)       ((((uint32)(x))&0x0000000FL)<<24)

#define FLEXCAN_MB_CS_DLC_MASK         CAN_CS0_DLC
#define FLEXCAN_MB_CS_CODE_MASK        CAN_CS0_CODE
#define FLEXCAN_MB_CS_RTR              CAN_CS0_RTR
#define FLEXCAN_MB_CS_IDE              CAN_CS0_IDE
#define FLEXCAN_MB_CS_TIMESTAMP_MASK   CAN_CS0_TIME_STAMP
#define FLEXCAN_MB_ID_EXT_MASK		   (CAN_ID0_EXT|CAN_ID0_STD)

#define FLEXCAN_GET_CODE(cs)		(((cs) & FLEXCAN_MB_CS_CODE_MASK)>>24)
#define FLEXCAN_GET_LENGTH(cs)      (((cs) & FLEXCAN_MB_CS_DLC_MASK)>>16)

// Bit definition for CAN message structure
#define	CAN_MSG_IDE_MASK	(0x80000000L)
#define CAN_MSG_IDE_BIT_NO	(31)
#define CAN_MSG_TYPE_MASK 	(0x40000000L)
#define CAN_MSG_TYPE_BIT_NO	(30)


// utilities
#define min(x,y)      ((x) < (y)? (x):(y))
#define max(x,y)      ((x) >= (y)? (x):(y))

#define swap_4bytes(ptr)	{   uint8 byte;	\
								byte = (ptr)[0]; (ptr)[0] = (ptr)[3]; (ptr)[3]=byte;	\
								byte = (ptr)[1]; (ptr)[1] = (ptr)[2]; (ptr)[2]=byte;	\
							}

// variables
static TFCAN_MailBox RxMailBox[MAX_RX_MAILS];  // receive mail box queue
static TFCAN_MailBox TxMailBox[MAX_TX_MAILS];  // transmit mail box queue
static TFCAN_MailBox RxFIFOQue[MAX_RXFIFO_MAILS];  // rx FIFO queue
static volatile uint16 rdRxMailBox,              // read pointer for RxMailBox
        wrRxMailBox,                      // write pointer for RxMailBox
        rdTxMailBox,                      // read pointer for TxMailBox
        wrTxMailBox;                      // write pointer for TxMailBox
static volatile uint16 rdRxFIFOQue;          // read pointer for RxFIFO queue
static volatile uint16 wrRxFIFOQue;          // write pointer for RxFIFO queue
static volatile uint32 nCANErrorCount;
static TFCANDrv_Obj FlexCANDrvObj;
static PTFCANDrv_Obj pFlexCANDrvObj;
static CAN_MemMapPtr pFlexCANReg = ((CAN_MemMapPtr)FCAN_BASE);
static volatile uint32 bMBFlags;   // flag if a MB is done by ISR
static volatile uint8  bMBInTransmitting;

// Callback routines
static void FlexCAN_MB_Callback(uint16 iMBNo)
{
	
}

static void FlexCAN_Err_Callback(uint32 status)
{
    if (status & CAN_ESR1_BIT0ERR)
    {
        APP_PrintString("BIT0 error occured!\n");
    }
    if (status & CAN_ESR1_BIT1ERR)
    {
    	APP_PrintString("BIT1 error occured!\n");
    }
    if (status & CAN_ESR1_STFERR)
    {
    	APP_PrintString("Stuffing error occured!\n");
    }
    if (status & CAN_ESR1_FRMERR)
    {
    	APP_PrintString("Form error occured!\n");
    }
    if (status & CAN_ESR1_CRCERR)
    {
    	APP_PrintString("CRC error occured!\n");
    }
    if (status & CAN_ESR1_ACKERR)
    {
    	APP_PrintString("Ack error occured!\n");
    }
}

static void FlexCAN_RxFIFOErr_Callback(void)
{
	
}

static void FlexCAN_WriteCode(uint16 iTxMBNo, uint8 txrxCode)
{
    uint8 bFreezeMode;

    if (iTxMBNo >= NUMBER_OF_MB)
    {
        return; // out of range
    }
    // Make sure FlexCAN is in Freeze mode
    if (!(pFlexCANReg->MCR & CAN_MCR_HALT)) //FLEXCAN_MCR_FRZ_ACK))
    {
        FlexCAN_Stop(); // halt FlexCAN safely
        bFreezeMode = 0;
    }
    else
    {
        bFreezeMode = 1;
    }
    // Start transmit/receive with specified tx/rx code
    pFlexCANReg->MB[iTxMBNo].CS = (pFlexCANReg->MB[iTxMBNo].CS
            & ~(FLEXCAN_MB_CS_CODE_MASK)) | FLEXCAN_MB_CS_CODE(txrxCode) // write code
    ;

    // Restore FlexCAN operation mode
    if (!bFreezeMode)
    {
        FlexCAN_Start();
    }
}

static uint8 FlexCAN_Is_Transfer_Done(uint16 iMB)
{
    if (bMBFlags & (1 << iMB))
    {
        return 1;
    }
    return 0;
}

static uint16 FlexCAN_CheckAbortStatus(uint16 iMB)
{
    uint8 code;
    // If it is still pending, just return pending status
    if (!(pFlexCANReg->IFLAG1 & (1 << iMB)))
    {
        return (FLEXCAN_ERROR_PENDING_TX);
    }
    // Check CODE field to see if it is aborted or not
    code = FLEXCAN_GET_CODE(pFlexCANReg->MB[iMB].CS);
    if (code == FLEXCAN_MB_CODE_TX_ABORT)
    {
        // Abort success
        return (FLEXCAN_ERROR_SUCCESS);
    }
    else if (code == FLEXCAN_MB_CODE_TX_INACTIVE)
    {
        // MB is not aborted but transmitted instead
        return (FLEXCAN_ERROR_TX_DONE);
    }
    // Other status
    return (FLEXCAN_ERROR_TX_ERR);
}

static void FlexCAN_PrepareTransmit(uint16 iTxMBNo, uint32 msgID, uint8 prio,
                             uint8 bBytes[], uint8 iNoBytes)
{
    uint16 i, wno, bno;
    uint32 word2[2] = { 0 };

    if (iTxMBNo >= NUMBER_OF_MB || iNoBytes > 8)
    {
        return; // out of range
    }
    // convert data bytes to words
    wno = (iNoBytes - 1) >> 2;
    bno = (iNoBytes - 1) % 4;
    if (wno > 0)
    {
        word2[0] = (((uint32) bBytes[0] << 24) | ((uint32) bBytes[1] << 16)
                | ((uint32) bBytes[2] << 8) | bBytes[3]);
    }
    for (i = 0; i <= bno; i++)
    {
        word2[wno] |= (uint32) bBytes[(wno << 2) + i] << (24 - (i << 3));
    }
    // Get IDE bit and RTR bit
    wno = (msgID & CAN_MSG_IDE_MASK) >> CAN_MSG_IDE_BIT_NO;
    bno = (msgID & CAN_MSG_TYPE_MASK) >> CAN_MSG_TYPE_BIT_NO;

    // Get ID bit no
    i = wno ? 0 : 18;

    // Follow 4 steps for Transmit Process
    pFlexCANReg->MB[iTxMBNo].CS =
            FLEXCAN_MB_CS_CODE(FLEXCAN_MB_CODE_TX_INACTIVE) // write inactive code
            | ((uint32) wno << 21)
            | ((uint32) bno << 20)
            | FLEXCAN_MB_CS_LENGTH(iNoBytes)  // can write size here
            ;
    pFlexCANReg->MB[iTxMBNo].ID = ((uint32) prio << 29)
            | ((msgID & ~(CAN_MSG_IDE_MASK | CAN_MSG_TYPE_MASK))<<i);
    pFlexCANReg->MB[iTxMBNo].WORD0 = word2[0];
    pFlexCANReg->MB[iTxMBNo].WORD1 = word2[1];

    // do not start transmit
}

static void FlexCAN_PrepareAndStartTransmit(uint16 iTxMBNo, uint32 msgID, uint8 prio,
                                     uint8 bBytes[], uint8 iNoBytes,
                                     uint8 txCode)
{
    uint16 i, wno, bno;

    if (iTxMBNo >= NUMBER_OF_MB || iNoBytes > 8)
    {
        return; // out of range
    }

    // Enable tx MB interrupt
    //pFlexCANReg->IMASK1 |=  (1<<iTxMBNo);    

    // Flag transfer starts
    bMBFlags &= ~((uint32) 1 << iTxMBNo);

    //
    FlexCAN_PrepareTransmit(iTxMBNo, msgID, prio, bBytes, iNoBytes);

    // Start transmit with specified tx code
    pFlexCANReg->MB[iTxMBNo].CS = (pFlexCANReg->MB[iTxMBNo].CS
            & ~(FLEXCAN_MB_CS_CODE_MASK)) | FLEXCAN_MB_CS_CODE(txCode) // write activate code
            | FLEXCAN_MB_CS_LENGTH(iNoBytes);
}



/**************************************************************
 ReadCANMB
 ** Read directly from CAN Message Buffer up to 8 bytes and return
 ** the size in bytes of data read.
 
 ** NOTE:
 ** So the ID format is:
 ** B31 30 29 28 27 26 ... 11 10 9 8 7 6 5 4 3 2 1 0
 ** |	|	 |									  |
 ** |    |    |------------------------------------|
 ** |	|					|--> 29 bits ID
 ** |	|
 ** |    |--> 1: RTR bit set, 0: RTR bit clear
 ** |
 ** |--> 1 : Extended ID, 0: Standardard ID
 **************************************************************/
static uint16 FlexCAN_ReadCANMB(uint16 iMB, uint32 *id, uint32 *timeStamp, uint8 *data,
                         uint8 bUnlock)
{
    uint8 i, wno, bno;
    uint16 code;
    uint16 length;
    uint8 leftBytes;
    uint8 format;
    uint8 *pBytes = data;

    // Lock the MB
    code = FLEXCAN_GET_CODE(pFlexCANReg->MB[iMB].CS);

    length = FLEXCAN_GET_LENGTH(pFlexCANReg->MB[iMB].CS);
    // 
    format = (pFlexCANReg->MB[iMB].CS & FLEXCAN_MB_CS_IDE) ? 1 : 0;
    *id = (pFlexCANReg->MB[iMB].ID & FLEXCAN_MB_ID_EXT_MASK);
    if (!format)
    {
        // standard ID
        *id >>= 18;
    }
    else
    {
        *id |= CAN_MSG_IDE_MASK;          // flag extended ID
    }
    format = (pFlexCANReg->MB[iMB].CS & FLEXCAN_MB_CS_RTR) ? 1 : 0;
    if (format)
    {
        *id |= CAN_MSG_TYPE_MASK;         // flag Remote Frame type
    }

    // Read message bytes
    wno = (length - 1) >> 2;
    bno = length - 1;
    if (wno > 0)
    {
        //
        (*(uint32*) pBytes) = pFlexCANReg->MB[iMB].WORD0;
        swap_4bytes(pBytes);      
        (*(uint32*) (pBytes+4)) = pFlexCANReg->MB[iMB].WORD1;
        swap_4bytes(pBytes+4);
    }
    else
    {
    	 (*(uint32*) pBytes) = pFlexCANReg->MB[iMB].WORD0;
    	 swap_4bytes(pBytes);
    }
    // Read time stamp
    *timeStamp = pFlexCANReg->MB[iMB].CS & FLEXCAN_MB_CS_TIMESTAMP_MASK;

    if (bUnlock)
    {
        // Unlock the MB
        code = pFlexCANReg->TIMER;
    }

    return (length);
}

static void FlexCAN_Poll_RxMBs(void)
{
    uint16 iMB;

    for (iMB = pFlexCANDrvObj->bRxMBStart; iMB <= pFlexCANDrvObj->bRxMBEnd;
            iMB++)
    {
        if (pFlexCANReg->IFLAG1 & (1 << iMB))
        {

            // Read the Message information
            if ((wrRxMailBox + 1) != rdRxMailBox)
            {
                RxMailBox[wrRxMailBox].data_len = FlexCAN_ReadCANMB(
                        iMB, (uint32*) &RxMailBox[wrRxMailBox].identifier,
                        (uint32*) &RxMailBox[wrRxMailBox].timestamp,
                        (uint8*) RxMailBox[wrRxMailBox].data, 1);
                wrRxMailBox++;
                if (wrRxMailBox >= MAX_RX_MAILS)
                {
                    wrRxMailBox = 0;
                }
            }

            // clear flag
            pFlexCANReg->IFLAG1 = (1 << iMB);

            bMBFlags |= (1 << iMB); // set flag to caller indicating this MB is done

            // Call custom Rx callback to do follow up processing 
            FlexCAN_MB_Callback(iMB);
        }
    }
}

static void FlexCAN_Poll_TxMBs(void)
{
    uint16 iMB;

    for (iMB = pFlexCANDrvObj->bTxMBStart; iMB <= pFlexCANDrvObj->bTxMBEnd;
            iMB++)
    {
        if (pFlexCANReg->IFLAG1 & (1 << iMB))
        {
            // Read the time stamp information
            if (rdTxMailBox != wrTxMailBox)
            {
                // if Tx MailBox not empty,do
                FlexCAN_PrepareAndStartTransmit(
                        iMB, TxMailBox[rdTxMailBox].identifier,
                        TxMailBox[rdTxMailBox].priority,
                        (uint8*) TxMailBox[rdTxMailBox].data,
                        TxMailBox[rdTxMailBox].data_len,
                        FLEXCAN_MB_CODE_TX_ONCE // tx once code
                        );
                rdTxMailBox++;
                if (rdTxMailBox >= MAX_TX_MAILS)
                {
                    rdTxMailBox = 0;
                }
            }
            else
            {
            	bMBInTransmitting = 0;  // flag to call PrepareTransmit again
                bMBFlags |= (1 << iMB); // set flag to caller indicating this MB is done by ISR
            }
            // clear flag
            pFlexCANReg->IFLAG1 = (1 << iMB);

            // Call custom Tx callback to do follow up processing
            // such as queue another message to be transmitted
            FlexCAN_MB_Callback(iMB);
        }
    }
}

static void FlexCAN_Poll_RxFIFO(void)
{
    uint16 iMatchHit;

    if (pFlexCANReg->IFLAG1 & CAN_IFLAG1_BUF5I)
    {
        // frames are available in Rx FIFO, read it into RxFIFO queue
        if ((wrRxFIFOQue + 1) != rdRxFIFOQue)
        {
            RxFIFOQue[wrRxFIFOQue].data_len = FlexCAN_ReadCANMB(
                    0, // MB0 is the read buffer of RxFIFO 
                    (uint32*) &RxFIFOQue[wrRxFIFOQue].identifier,
                    (uint32*) &RxFIFOQue[wrRxFIFOQue].timestamp,
                    (uint8*) RxFIFOQue[wrRxFIFOQue].data, 0);
            // read RXFIR
            iMatchHit = pFlexCANReg->RXFIR & 0x1FF;

            wrRxFIFOQue++;
            if (wrRxFIFOQue >= MAX_RXFIFO_MAILS)
            {
                wrRxFIFOQue = 0;
            }
        }
        // clear Rx FIFO flag
        pFlexCANReg->IFLAG1 = CAN_IFLAG1_BUF5I;
    }
    // Check Rx FIFO Warning flag
    if (pFlexCANReg->IFLAG1 & CAN_IFLAG1_BUF6I)
    {
        pFlexCANReg->IFLAG1 = CAN_IFLAG1_BUF6I;
    }
    // Check Rx FIFO Overflow flag
    if (pFlexCANReg->IFLAG1 & CAN_IFLAG1_BUF7I)
    {
        // This should be error
        nCANErrorCount++;

        pFlexCANReg->IFLAG1 = CAN_IFLAG1_BUF7I;

        // Call RxFIFO error call back
        FlexCAN_RxFIFOErr_Callback();
    }
}

void FlexCAN_Init(void)
{
    uint16 i;

    // Initialize the driver structures
    for(i = 0; i < MAX_RX_MAILS; i++)
    {
        RxMailBox[i].dev_num = 0;
    }
    for(i = 0; i < MAX_TX_MAILS; i++)
    {
        TxMailBox[i].dev_num = 0;
    }
    for(i = 0; i < MAX_RXFIFO_MAILS; i++)
    {
        RxFIFOQue[i].dev_num = 0;
    }
    rdRxMailBox = wrRxMailBox = 0;
    rdTxMailBox = wrTxMailBox = 0;
    rdRxFIFOQue = wrRxFIFOQue = 0;

    nCANErrorCount = 0;

    bMBFlags = 0;
    bMBInTransmitting = 0;

    // Initialize driver object based on custom macro definitions
    pFlexCANDrvObj = &FlexCANDrvObj;

    pFlexCANDrvObj->ctrlWord.Bits.bClockSource = (FLEXCAN_CLOCK_SOURCE? 1 : 0);
    pFlexCANDrvObj->ctrlWord.Bits.bSelfRxDis = ((FLEXCAN_SELF_RECEPTION != 0u)? 0 : 1);
    pFlexCANDrvObj->ctrlWord.Bits.bScanPrio = (FLEXCAN_SCAN_PRIORITY? 1 : 0);
    pFlexCANDrvObj->ctrlWord.Bits.bStoreRTR = (FLEXCAN_STORE_RTR? 1 : 0);
    pFlexCANDrvObj->ctrlWord.Bits.bEACEN = (FLEXCAN_ENTIRE_ARB_CMP? 1: 0);
    pFlexCANDrvObj->ctrlWord.Bits.bIRMQ = (FLEXCAN_INDIVIDUAL_MASK? 1 : 0);
    pFlexCANDrvObj->ctrlWord.Bits.bLoopBack = ((FLEXCAN_LOOP_BACK!= 0u)? 1 : 0);
    pFlexCANDrvObj->ctrlWord.Bits.bLocalPrio = (FLEXCAN_LOCAL_PRIO? 1 : 0);

    pFlexCANDrvObj->bRxMBStart = FLEXCAN_RX_MB_START; /* start index of Rx MB# */
    pFlexCANDrvObj->bRxMBEnd = FLEXCAN_RX_MB_END; /* end index of Rx MB# */
    pFlexCANDrvObj->bTxMBStart = FLEXCAN_TX_MB_START; /* start index of Tx MB# */
    pFlexCANDrvObj->bTxMBEnd = FLEXCAN_TX_MB_END; /* end index of Tx MB# */

    pFlexCANDrvObj->bRxFIFOFilterNo = FLEXCAN_NO_RXFIFO_FILTERS;
    pFlexCANDrvObj->bRxFIFOIDFormat = FLEXCAN_ID_TAB_FORMAT;

    pFlexCANDrvObj->pCANRegs = pFlexCANReg;
}

FLEXCAN_ERROR_E FlexCAN_Open(uint32 baudrateKHz)
{
    uint16 i;

    // Select clock source for CAN first
    // NOTE: must ensure that LPM_ACK = 1 before changing clock source bit
    if (pFlexCANDrvObj->ctrlWord.Bits.bClockSource)
    {
        pFlexCANReg->CTRL1 |= CAN_CTRL1_CLKSRC; //Source --> bus clock
    }
    else
    {
        pFlexCANReg->CTRL1 &= ~CAN_CTRL1_CLKSRC; //Source --> external oscillator
    }
    // Enable CAN module
    pFlexCANReg->MCR |= CAN_MCR_FRZ;          // enable HALT feature
    pFlexCANReg->MCR |= CAN_MCR_HALT;  
    pFlexCANReg->MCR &= ~CAN_MCR_MDIS;

    while ((CAN_MCR_LPMACK & pFlexCANReg->MCR))
    {
        ;
    }
    // Now can apply Soft Reset
    pFlexCANReg->MCR ^= CAN_MCR_SOFTRST;
    while (CAN_MCR_SOFTRST & pFlexCANReg->MCR)
    {
        ;
    }

    // Now it should be in Freeze mode  
    while (!(CAN_MCR_FRZACK & pFlexCANReg->MCR))
    {
        ;
    }

    // Disable self-reception
    if (pFlexCANDrvObj->ctrlWord.Bits.bSelfRxDis)
    {
        pFlexCANReg->MCR |= CAN_MCR_SRXDIS;
    }

    // Enable individual masking and queue
    if (pFlexCANDrvObj->ctrlWord.Bits.bIRMQ)
    {
        pFlexCANReg->MCR |= CAN_MCR_IRMQ;
    }
    // Set local priority
    if (pFlexCANDrvObj->ctrlWord.Bits.bLocalPrio)
    {
        pFlexCANReg->MCR |= CAN_MCR_LPRIOEN;
    }

    /* Initialize all 16 MBs */
    for (i = 0; i < NUMBER_OF_MB; i++)
    {
        pFlexCANReg->MB[i].CS = 0x00000000;
        pFlexCANReg->MB[i].ID = 0x00000000;
        pFlexCANReg->MB[i].WORD0 = 0x00000000;
        pFlexCANReg->MB[i].WORD1 = 0x00000000;
    }
    // Set RRS and EACEN bits in CTRL2
    if (pFlexCANDrvObj->ctrlWord.Bits.bEACEN)
    {
        pFlexCANReg->CTRL2 |= CAN_CTRL2_EACEN;
    }
    else
    {
        pFlexCANReg->CTRL2 &= ~CAN_CTRL2_EACEN;
    }
    if (pFlexCANDrvObj->ctrlWord.Bits.bStoreRTR)
    {
        pFlexCANReg->CTRL2 |= CAN_CTRL2_RRS;
    }
    else
    {
        pFlexCANReg->CTRL2 &= ~CAN_CTRL2_RRS;
    }
    // Set Scan priority (Mailbxes reception priority)
    if (pFlexCANDrvObj->ctrlWord.Bits.bScanPrio)
    {
        pFlexCANReg->CTRL2 |= CAN_CTRL2_MRP;
    }
    else
    {
        pFlexCANReg->CTRL2 &= ~CAN_CTRL2_MRP;
    }

    // Eanble RxFIFO if # of RxFIFO filters is >0
    if (pFlexCANDrvObj->bRxFIFOFilterNo)
    {
        pFlexCANReg->MCR |= CAN_MCR_RFEN;

        // Configure RFFN field with # of Rx FIFO filters
		pFlexCANReg->CTRL2 = (pFlexCANReg->CTRL2 & (~CAN_CTRL2_RFFN)) | 
		       ((((pFlexCANDrvObj->bRxFIFOFilterNo>>3)-1) & 0x0F) << 24);
    }
    // Set ID filter table format
    pFlexCANReg->MCR |= (pFlexCANReg->MCR & ~CAN_MCR_IDAM)
            | ((pFlexCANDrvObj->bRxFIFOIDFormat&0x03)<<8);

    // Configure bit rate
    switch (baudrateKHz)
    {
        case (33):	// 33.33K
            if (pFlexCANReg->CTRL1 & CAN_CTRL1_CLKSRC)
            {
                /* 
                 ** 100M/250= 400k sclock, 12Tq
                 ** PROPSEG = 3, LOM = 0x0, LBUF = 0x0, TSYNC = 0x0, SAMP = 1
                 ** RJW = 3, PSEG1 = 4, PSEG2 = 4,PRESDIV = 250
                 */
                pFlexCANReg->CTRL1 = (0 | FLEXCAN_CTRL_PROPSEG(2)
                        | FLEXCAN_CTRL_RJW(2) | FLEXCAN_CTRL_PSEG1(3)
                        | FLEXCAN_CTRL_PSEG2(3)
                        | FLEXCAN_CTRL_PRESDIV(249));
            }
            else
            {
                /* 
                 ** 8M/20= 400k sclock, 12Tq
                 ** PROPSEG = 1, LOM = 0x0, LBUF = 0x0, TSYNC = 0x0, SAMP = 1
                 ** RJW = 4, PSEG1 = 5, PSEG2 = 5,PRESDIV = 20
                 */
                pFlexCANReg->CTRL1 = (0 | FLEXCAN_CTRL_PROPSEG(0)
                        | FLEXCAN_CTRL_RJW(3) | FLEXCAN_CTRL_PSEG1(4)
                        | FLEXCAN_CTRL_PSEG2(4)
                        | FLEXCAN_CTRL_PRESDIV(19));

            }
            break;
        case (83):	// 83.33K
            if (pFlexCANReg->CTRL1 & CAN_CTRL1_CLKSRC)
            {

                /* 
                 ** 100M/100 = 1M sclock, 12Tq
                 ** PROPSEG = 3, LOM = 0x0, LBUF = 0x0, TSYNC = 0x0, SAMP = 1
                 ** RJW = 3, PSEG1 = 4, PSEG2 = 4,PRESDIV = 100
                 */
                pFlexCANReg->CTRL1 = (0 | FLEXCAN_CTRL_PROPSEG(2)
                        | FLEXCAN_CTRL_RJW(2) | FLEXCAN_CTRL_PSEG1(3)
                        | FLEXCAN_CTRL_PSEG2(3)
                        | FLEXCAN_CTRL_PRESDIV(99));
            }
            else
            {
                /* 
                 ** 8M/8= 1M sclock, 12Tq
                 ** PROPSEG = 1, LOM = 0x0, LBUF = 0x0, TSYNC = 0x0, SAMP = 1
                 ** RJW = 4, PSEG1 = 5, PSEG2 = 5,PRESDIV = 8
                 */
                pFlexCANReg->CTRL1 = (0 | FLEXCAN_CTRL_PROPSEG(0)
                        | FLEXCAN_CTRL_RJW(3) | FLEXCAN_CTRL_PSEG1(4)
                        | FLEXCAN_CTRL_PSEG2(4)
                        | FLEXCAN_CTRL_PRESDIV(7));

            }
            break;
        case (50):
            if (pFlexCANReg->CTRL1 & CAN_CTRL1_CLKSRC)
            {
                /* 100M/200=500K sclock, 10Tq
                 ** PROPSEG = 0x1, LOM = 0x0, LBUF = 0x0, TSYNC = 0x0, SAMP = 0x0 
                 ** RJW = 0x4, PSEG1 = 0x4, PSEG2 = 0x4, PRESDIV = 200
                 */
                pFlexCANReg->CTRL1 = (0 | FLEXCAN_CTRL_PROPSEG(0)
                        | FLEXCAN_CTRL_RJW(3) | FLEXCAN_CTRL_PSEG1(3)
                        | FLEXCAN_CTRL_PSEG2(3)
                        | FLEXCAN_CTRL_PRESDIV(199));
            }
            else
            {
                /* 
                 ** 8M/16= 0.5M sclock, 10Tq
                 ** PROPSEG = 1, LOM = 0x0, LBUF = 0x0, TSYNC = 0x0, SAMP = 1
                 ** RJW = 4, PSEG1 = 4, PSEG2 = 4, PRESDIV = 16
                 */
                pFlexCANReg->CTRL1 = (0 | FLEXCAN_CTRL_PROPSEG(0)
                        | FLEXCAN_CTRL_RJW(3) | FLEXCAN_CTRL_PSEG1(3)
                        | FLEXCAN_CTRL_PSEG2(3)
                        | FLEXCAN_CTRL_PRESDIV(15));
            }
            break;
        case (100):
            if (pFlexCANReg->CTRL1 & CAN_CTRL1_CLKSRC)
            {
                /* 
                 ** 100M/100=1M sclock, 10Tq
                 ** PROPSEG = 0x1, LOM = 0x0, LBUF = 0x0, TSYNC = 0x0, SAMP = 0x0 
                 ** RJW = 0x4, PSEG1 = 0x4, PSEG2 = 0x4, PRESDIV = 100
                 */
                pFlexCANReg->CTRL1 = (0 | FLEXCAN_CTRL_PROPSEG(0)
                        | FLEXCAN_CTRL_RJW(3) | FLEXCAN_CTRL_PSEG1(3)
                        | FLEXCAN_CTRL_PSEG2(3)
                        | FLEXCAN_CTRL_PRESDIV(99));
            }
            else
            {
                /* 
                 ** 8M/8= 1M sclock, 10Tq
                 ** PROPSEG = 1, LOM = 0x0, LBUF = 0x0, TSYNC = 0x0, SAMP = 1
                 ** RJW = 4, PSEG1 = 4, PSEG2 = 4, PRESDIV = 8
                 */
                pFlexCANReg->CTRL1 = (0 | FLEXCAN_CTRL_PROPSEG(0)
                        | FLEXCAN_CTRL_RJW(3) | FLEXCAN_CTRL_PSEG1(3)
                        | FLEXCAN_CTRL_PSEG2(3)
                        | FLEXCAN_CTRL_PRESDIV(7));
            }
            break;
        case (125):
            if (pFlexCANReg->CTRL1 & CAN_CTRL1_CLKSRC)
            {
                /* 
                 ** 100M/100=1M sclock,8Tq
                 ** PROPSEG = 0x1, LOM = 0x0, LBUF = 0x0, TSYNC = 0x0, SAMP = 0x0 
                 ** RJW = 0x3, PSEG1 = 0x3, PSEG2 = 0x3, PRESDIV = 100
                 */
                pFlexCANReg->CTRL1 = (0 | FLEXCAN_CTRL_PROPSEG(0)
                        | FLEXCAN_CTRL_RJW(2) | FLEXCAN_CTRL_PSEG1(2)
                        | FLEXCAN_CTRL_PSEG2(2)
                        | FLEXCAN_CTRL_PRESDIV(99));
            }
            else
            {
                /* 
                 * 8M/8= 1M sclock, 8Tq
                 ** PROPSEG = 1, LOM = 0x0, LBUF = 0x0, TSYNC = 0x0, SAMP = 1
                 ** RJW = 3, PSEG1 = 3, PSEG2 = 3, PRESDIV = 8
                 */
                pFlexCANReg->CTRL1 = (0 | FLEXCAN_CTRL_PROPSEG(0)
                        | FLEXCAN_CTRL_RJW(2) | FLEXCAN_CTRL_PSEG1(2)
                        | FLEXCAN_CTRL_PSEG2(2)
                        | FLEXCAN_CTRL_PRESDIV(7));
            }
            break;
        case (250):
            if (pFlexCANReg->CTRL1 & CAN_CTRL1_CLKSRC)
            {
                /* 
                 **	100M/16= 6.25M sclock,25Tq
                 ** PROPSEG = 0x8, LOM = 0x0, LBUF = 0x0, TSYNC = 0x0, SAMP = 0x0 
                 ** RJW = 0x4, PSEG1 = 0x8, PSEG2 = 0x8, PRESDIV = 16
                 */
                pFlexCANReg->CTRL1 = (0 | FLEXCAN_CTRL_PROPSEG(7)
                        | FLEXCAN_CTRL_RJW(3) | FLEXCAN_CTRL_PSEG1(7)
                        | FLEXCAN_CTRL_PSEG2(7)
                        | FLEXCAN_CTRL_PRESDIV(15));
            }
            else
            {
                /* 
                 ** 8M/4= 2M sclock, 8Tq
                 ** PROPSEG = 1, LOM = 0x0, LBUF = 0x0, TSYNC = 0x0, SAMP = 1
                 ** RJW = 3, PSEG1 = 3, PSEG2 = 3, PRESDIV = 4
                 */
                pFlexCANReg->CTRL1 = (0 | FLEXCAN_CTRL_PROPSEG(0)
                        | FLEXCAN_CTRL_RJW(2) | FLEXCAN_CTRL_PSEG1(2)
                        | FLEXCAN_CTRL_PSEG2(2)
                        | FLEXCAN_CTRL_PRESDIV(3));
            }
            break;
        case (500):
            if (pFlexCANReg->CTRL1 & CAN_CTRL1_CLKSRC)
            {
                /* 
                 **	100M/8= 12.5M sclock,25Tq
                 ** PROPSEG = 0x8, LOM = 0x0, LBUF = 0x0, TSYNC = 0x0, SAMP = 0x0 
                 ** RJW = 0x4, PSEG1 = 0x8, PSEG2 = 0x8, PRESDIV = 8
                 */
                pFlexCANReg->CTRL1 = (0 | FLEXCAN_CTRL_PROPSEG(7)
                        | FLEXCAN_CTRL_RJW(3) | FLEXCAN_CTRL_PSEG1(7)
                        | FLEXCAN_CTRL_PSEG2(7)
                        | FLEXCAN_CTRL_PRESDIV(7));
            }
            else
            {
                /* 
                 ** 8M/2=4M sclock, 8Tq
                 ** PROPSEG = 1, LOM = 0x0, LBUF = 0x0, TSYNC = 0x0, SAMP = 1
                 ** RJW = 3, PSEG1 = 3, PSEG2 = 3, PRESDIV = 2
                 */
                pFlexCANReg->CTRL1 = (0 | FLEXCAN_CTRL_PROPSEG(0)
                        | FLEXCAN_CTRL_RJW(2) | FLEXCAN_CTRL_PSEG1(2)
                        | FLEXCAN_CTRL_PSEG2(2)
                        | FLEXCAN_CTRL_PRESDIV(1));
            }
            break;
        case (1000):
            if (pFlexCANReg->CTRL1 & CAN_CTRL1_CLKSRC)
            {
                /* 
                 **	100M/4= 25M sclock,25Tq
                 ** PROPSEG = 0x8, LOM = 0x0, LBUF = 0x0, TSYNC = 0x0, SAMP = 0x0 
                 ** RJW = 0x4, PSEG1 = 0x8, PSEG2 = 0x8, PRESDIV = 4
                 */
                pFlexCANReg->CTRL1 = (0 | FLEXCAN_CTRL_PROPSEG(7)
                        | FLEXCAN_CTRL_RJW(3) | FLEXCAN_CTRL_PSEG1(7)
                        | FLEXCAN_CTRL_PSEG2(7)
                        | FLEXCAN_CTRL_PRESDIV(3));
            }
            else
            {
                /*  
                 ** 8M/1=8M sclock,8Tq
                 ** PROPSEG = 1, LOM = 0x0, LBUF = 0x0, TSYNC = 0x0, SAMP = 1
                 ** RJW = 3, PSEG1 = 3, PSEG2 = 3, PRESDIV = 1
                 */
                pFlexCANReg->CTRL1 = (0 | FLEXCAN_CTRL_PROPSEG(0)
                        | FLEXCAN_CTRL_RJW(2) | FLEXCAN_CTRL_PSEG1(2)
                        | FLEXCAN_CTRL_PSEG2(2)
                        | FLEXCAN_CTRL_PRESDIV(0));

            }
            break;
        default:
            return (FLEXCAN_ERROR_INVALID_BAUD);
    }

    if (pFlexCANDrvObj->ctrlWord.Bits.bLoopBack)
    {
        // use loopback for single node
        pFlexCANReg->CTRL1 |= CAN_CTRL1_LPB;
    }
    else
    {
        // use external CAN bus
        pFlexCANReg->CTRL1 &= ~CAN_CTRL1_LPB;
        
    }

    /* Initialize mask registers */
    pFlexCANReg->RXMGMASK = 0x1FFFFFFF;
    pFlexCANReg->RX14MASK = 0x1FFFFFFF;
    pFlexCANReg->RX15MASK = 0x1FFFFFFF;

    /* Initialize individual mask registers for the queue */
    if (pFlexCANReg->MCR & CAN_MCR_IRMQ)
    {
        for (i = 0; i < NUMBER_OF_MB; i++)
        {
            pFlexCANReg->RXIMR[i] = 0x1FFFFFFFL;
        }
    }

    // Enable interrupts
    //pFlexCANReg->IMASK1 |=  (1<<FLEXCAN_RX_MB_START) | (1<<FLEXCAN_TX_MB_START);

    // Start communication
    FlexCAN_Start();

    return (FLEXCAN_ERROR_SUCCESS);
}

void FlexCAN_Start(void)
{
    // De-assert Freeze Mode 
    pFlexCANReg->MCR &= ~(CAN_MCR_FRZ);
    pFlexCANReg->MCR &= ~(CAN_MCR_HALT);

    // Wait till exit of freeze mode
    while (pFlexCANReg->MCR & CAN_MCR_FRZACK)
    {
        ;
    }

    // Wait till ready 
    while (pFlexCANReg->MCR & CAN_MCR_NOTRDY)
    {
        ;
    }
}

void FlexCAN_Stop(void)
{
    // Assert Freeze Mode
    pFlexCANReg->MCR |= (CAN_MCR_FRZ);
    pFlexCANReg->MCR |= (CAN_MCR_HALT);

    // Wait till enter freeze mode
    while (!(pFlexCANReg->MCR & CAN_MCR_FRZACK))
    {
        ;
    }
}

uint16 FlexCAN_ConfigureRxFIFO_IDTable(uint32 idList[], uint16 idListNo)
{
    uint16 nTotalValidTabIDNo; // total # of Rx FIFO table RxIDs
    uint16 nIDTab;
    uint8 bIDAM;
    uint8 bIsExtID, bIsRTR;
    uint32 id;
    uint32 *pIDTabElement;
    uint16 i, j;
    uint8 bFreezeMode;
    uint16 status = FLEXCAN_ERROR_SUCCESS;

    // Calculate the total # of valid table elements
    bIDAM = (pFlexCANReg->MCR & CAN_MCR_IDAM) >> 8;
    if (bIDAM == 3)
    {
        // all frames will be rejected, so just return without configuration
        status = FLEXCAN_ERROR_SUCCESS;
        return (status);
    }
    nTotalValidTabIDNo = (pFlexCANReg->CTRL2 & CAN_CTRL2_RFFN) >> 24;
    nTotalValidTabIDNo = ((nTotalValidTabIDNo + 1) << 3) * (1 << bIDAM);

    if ((idListNo) > nTotalValidTabIDNo)
    {
        status = (FLEXCAN_ERROR_PARAM_OUT_OF_RANGE);
        return (status);
    }

    nIDTab = min(idListNo, nTotalValidTabIDNo);

    // Calculate the address of the starting ID Table Element corresponding startIDTabNo which bases 0
    pIDTabElement = (uint32*) &pFlexCANReg->MB[6]; // ID Filter Table Elements starts from MB6

    // Make sure FlexCAN is in Freeze mode before writing Filter tables
    if (!(pFlexCANReg->MCR & CAN_MCR_HALT))
    {
        FlexCAN_Stop(); // halt FlexCAN safely
        bFreezeMode = 0;
    }
    else
    {
        bFreezeMode = 1;
    }
    // Configure the RX FIFO ID filter Tab elements
    for (i = 0; i < nIDTab;)
    {
        // Get IDE bit and RTR bit
        id = idList[i] & ~(CAN_MSG_IDE_MASK | CAN_MSG_TYPE_MASK);
        bIsExtID = (idList[i] & CAN_MSG_IDE_MASK) >> CAN_MSG_IDE_BIT_NO;
        bIsRTR = (idList[i] & CAN_MSG_TYPE_MASK) >> CAN_MSG_TYPE_BIT_NO;

        if (bIDAM == 1)
        {
            // Format B two IDs
            *pIDTabElement = ((id & 0x03fff) << (16 + (1 - bIsExtID) * 3))
                    | ((uint32) bIsExtID << 30) | ((uint32) bIsRTR << 31); // RXIDB_0  
            i++;
            if (i < nIDTab)
            {
                id = idList[i] & ~(CAN_MSG_IDE_MASK | CAN_MSG_TYPE_MASK);
                bIsExtID = (idList[i] & CAN_MSG_IDE_MASK) >> CAN_MSG_IDE_BIT_NO;
                bIsRTR = (idList[i] & CAN_MSG_TYPE_MASK) >> CAN_MSG_TYPE_BIT_NO;
                *pIDTabElement |= ((id & 0x03fff) << ((1 - bIsExtID) * 3))
                        | ((uint32) bIsExtID << 14) | ((uint32) bIsRTR << 15); // RXIDB_1     
                i++;
            }
        }
        else if (bIDAM == 2)
        {
            // Format C
            j = 0;
            *pIDTabElement = (id & 0x00ff) << (24 - (j << 3));    // RXIDC_0
            i++;
            j++;
            do
            {
                if (i < nIDTab)
                {
                    id = idList[i] & ~(CAN_MSG_IDE_MASK | CAN_MSG_TYPE_MASK);
                    bIsExtID = (idList[i] & CAN_MSG_IDE_MASK)
                            >> CAN_MSG_IDE_BIT_NO;
                    bIsRTR = (idList[i] & CAN_MSG_TYPE_MASK)
                            >> CAN_MSG_TYPE_BIT_NO;
                    *pIDTabElement |= ((id & 0x00ff) << (24 - (j << 3))); // RXIDC_1 .. RXIDC_3
                    j++;
                    i++;
                }
                else
                {
                    break;
                }
            }
            while (j <= 3);
        }
        else if (!bIDAM)
        {
            if (bIsExtID)
            {   // Format A with extended ID
                *pIDTabElement = (id << 1) | ((uint32) bIsExtID << 30)
                        | ((uint32) bIsRTR << 31); // single ID acceptance codes          
            }
            else
            {   // Format A with standard ID
                *pIDTabElement = (id << 19) | ((uint32) bIsRTR << 31); // single ID acceptance codes    RXIDA                 
            }
            i++;
        }
        pIDTabElement++;  // advance to the next ID table element
    }
    // Enable Rx FIFO interrupt
    // pFlexCANReg->IMASK1 |=  FLEXCAN_IFLAG1_BUF5I;         

    end:
    // Restore FlexCAN operation mode
    if (!bFreezeMode)
    {
        FlexCAN_Start();
    }
    return (status);
}

uint16 FlexCAN_ConfigureRxMBMask(uint16 iMB, uint32 mask)
{
    uint8 bFreezeMode;

    if (iMB > NUMBER_OF_MB)
    {
        return (FLEXCAN_ERROR_PARAM_OUT_OF_RANGE);
    }

    // Make sure FlexCAN is in Freeze mode
    if (!(pFlexCANReg->MCR & CAN_MCR_HALT))
    {
        FlexCAN_Stop(); // halt FlexCAN safely
        bFreezeMode = 0;
    }
    else
    {
        bFreezeMode = 1;
    }
    // Check if individual masking is enabled
    if (pFlexCANReg->MCR & CAN_MCR_IRMQ)
    {
        // individual masks
        pFlexCANReg->RXIMR[iMB] = mask;
    }
    else
    {   // Legacy support with global masks
        if (iMB == 14)
        {
            pFlexCANReg->RX14MASK = mask;
        }
        else if (iMB == 15)
        {
            pFlexCANReg->RX15MASK = mask;
        }
        else
        {
            pFlexCANReg->RXMGMASK = mask;
        }
    }
    
    
    // Restore FlexCAN operation mode
    if (!bFreezeMode)
    {
        FlexCAN_Start();
    }
    return (FLEXCAN_ERROR_SUCCESS);
}

uint16 FlexCAN_ConfigureRxFIFOFilterTabElementMask(uint16 iTabElement, uint32 mask)
{
    uint8 bFreezeMode;
    uint8 bIDAM;
    uint16 nTotalValidTabNo;

    bIDAM = (pFlexCANReg->MCR & CAN_MCR_IDAM) >> 8;

    nTotalValidTabNo = (pFlexCANReg->CTRL2 & CAN_CTRL2_RFFN) >> 24;
    nTotalValidTabNo = ((nTotalValidTabNo + 1) << 3);

    if (iTabElement > nTotalValidTabNo)
    {
        return (FLEXCAN_ERROR_PARAM_OUT_OF_RANGE);
    }

    // Make sure FlexCAN is in Freeze mode
    if (!(pFlexCANReg->MCR & CAN_MCR_HALT))
    {
        FlexCAN_Stop(); // halt FlexCAN safely
        bFreezeMode = 0;
    }
    else
    {
        bFreezeMode = 1;
    }
    // Configure Rx FIFO table element filter
    // Check if individual masking is enabled
    if (pFlexCANReg->MCR & CAN_MCR_IRMQ)
    {
        // individual masks
        pFlexCANReg->RXIMR[iTabElement] = mask; // similar to MB RX IMR
    }
    else
    {   // Legacy support with global masks
        pFlexCANReg->RXFGMASK = mask;
    }

    // Restore FlexCAN operation mode
    if (!bFreezeMode)
    {
        FlexCAN_Start();
    }
    return (FLEXCAN_ERROR_SUCCESS);
}

uint16 FlexCAN_ConfigureRxFIFOGlobalMask(uint32 mask)
{
    uint8 bFreezeMode;

    // Make sure FlexCAN is in Freeze mode
    if (!(pFlexCANReg->MCR & CAN_MCR_HALT)) //FLEXCAN_MCR_FRZ_ACK))
    {
        FlexCAN_Stop(); // halt FlexCAN safely
        bFreezeMode = 0;
    }
    else
    {
        bFreezeMode = 1;
    }
    pFlexCANReg->RXFGMASK = mask;

    // Restore FlexCAN operation mode
    if (!bFreezeMode)
    {
        FlexCAN_Start();
    }
    return (FLEXCAN_ERROR_SUCCESS);
}

/**************************************************************
 PrepareRxCANMB
 ** Prepare a CAN MB for rx.
 
 ** NOTE:
 ** If using FCANMB_SET_ID or FCANMB_GET_ID, consider the condition:
 ** #define FCAN_ID_EXT 0x80000000	// specifies extended ID anywhere the ID is required
 ** #define FCAN_ID_RTR 0x40000000	// turns on the RTR bit when calling FCANMB_SET_ID (only)
 ** So the ID format is:
 ** B31 30 29 28 27 26 ... 11 10 9 8 7 6 5 4 3 2 1 0
 ** |	|	 |									  |
 ** |    |    |------------------------------------|
 ** |	|					|--> 29 bits ID
 ** |	|
 ** |    |--> 1: RTR bit set, 0: RTR bit clear
 ** |
 ** |--> 1 : Extended ID, 0: Standardard ID
 **************************************************************/
void FlexCAN_PrepareRxCANMB(uint16 iMB, uint32 id)
{
    uint32 id2;
    uint32 cs = FLEXCAN_MB_CS_CODE(FLEXCAN_MB_CODE_RX_EMPTY);

    // Deactivate the rx MB for cpu write 
    pFlexCANReg->MB[iMB].CS =
            FLEXCAN_MB_CS_CODE(FLEXCAN_MB_CODE_RX_INACTIVE); // write inactive code	

    // Write ID
    id2 = id & ~(CAN_MSG_IDE_MASK | CAN_MSG_TYPE_MASK);
    if (id & CAN_MSG_IDE_MASK)
    {
        pFlexCANReg->MB[iMB].ID = id2;
        cs |= FLEXCAN_MB_CS_IDE;
    }
    else
    {
        pFlexCANReg->MB[iMB].ID = id2 << 18;
    }

    // Activate the MB for rx 
    pFlexCANReg->MB[iMB].CS = cs;

    // flag to caller transfer is started
    bMBFlags &= ~((uint32) 1 << iMB);
}

void FlexCAN_Write(uint32 msgID, uint8 prio,uint8 bBytes[], uint8 iNoBytes)
{
    uint16 i;

    if((wrTxMailBox+1) == rdTxMailBox)
    {
        // if queue full, return
        return;
    }
    TxMailBox[wrTxMailBox].identifier = msgID;
    TxMailBox[wrTxMailBox].priority = prio;
    TxMailBox[wrTxMailBox].data_len = iNoBytes;

    for(i = 0; i < iNoBytes; i++)
    {
        TxMailBox[wrTxMailBox].data[i] = bBytes[i];
    }

    wrTxMailBox++;
    if(wrTxMailBox >=MAX_TX_MAILS)
    {
        wrTxMailBox = 0;
    }

    // Prepare transmit for the first time
    if(!bMBInTransmitting)
    {
    	bMBInTransmitting = 1;
        FlexCAN_PrepareAndStartTransmit(pFlexCANDrvObj->bTxMBStart,
                TxMailBox[rdTxMailBox].identifier,
                TxMailBox[rdTxMailBox].priority,
                (uint8*)TxMailBox[rdTxMailBox].data,
                TxMailBox[rdTxMailBox].data_len,
                FLEXCAN_MB_CODE_TX_ONCE              // tx once
        );
        rdTxMailBox++;
        if(rdTxMailBox >=MAX_TX_MAILS)
        {
            rdTxMailBox = 0;
        }
    }
}

uint16 FlexCAN_Read(uint16 iMailBoxQue, uint32 *id, uint8 data[])
{
    uint16 i;
    uint16 noBytes = 0;
 
    if(rdRxMailBox == wrRxMailBox)
    {
        // if queue empty, return
        return (noBytes);
    }

    *id = RxMailBox[rdRxMailBox].identifier;
    noBytes = RxMailBox[rdRxMailBox].data_len;

    for(i = 0; i < noBytes; i++)
    {
        data[i] = RxMailBox[rdRxMailBox].data[i];
    }
    rdRxMailBox++;
    if(rdRxMailBox >=MAX_RX_MAILS)
    {
        rdRxMailBox = 0;
    }
    return (noBytes);
}

uint16 FlexCAN_Read_RxFIFOQueue(uint32 *id, uint8 data[])
{
    uint16 i;
    uint16 noBytes = 0;

    if(rdRxFIFOQue == wrRxFIFOQue)
    {
        // if queue empty, return
        return (noBytes);
    }

    *id = RxFIFOQue[rdRxFIFOQue].identifier;
    noBytes = RxFIFOQue[rdRxFIFOQue].data_len;

    for(i = 0; i < noBytes; i++)
    {
        data[i] = RxFIFOQue[rdRxFIFOQue].data[i];
    }
    rdRxFIFOQue++;
    if(rdRxFIFOQue >=MAX_RXFIFO_MAILS)
    {
        rdRxFIFOQue = 0;
    }
    return (noBytes);
}

void FlexCAN_EnableMB_Interrupt(uint16 iMB)
{
    pFlexCANReg->IMASK1 |= (1 << iMB);
}

void FlexCAN_DisableMB_Interrupt(uint16 iMB)
{
    pFlexCANReg->IMASK1 &= ~(1 << iMB);
}

void FlexCAN_EnableBusOff_Interrupt(void)
{
    pFlexCANReg->CTRL1 |= CAN_CTRL1_BOFFMSK;
}

void FlexCAN_DisableBusOff_Interrupt(void)
{
    pFlexCANReg->CTRL1 &= ~CAN_CTRL1_BOFFMSK;
}

void FlexCAN_EnableError_Interrupt(void)
{
    pFlexCANReg->CTRL1 |= CAN_CTRL1_ERRMSK;
}

void FlexCAN_DisableError_Interrupt(void)
{
    pFlexCANReg->CTRL1 &= ~CAN_CTRL1_ERRMSK;
}

void FlexCAN_EnableTxWarning_Interrupt(void)
{
    pFlexCANReg->CTRL1 |= CAN_CTRL1_TWRNMSK;
}

void FlexCAN_DisableTxWarning_Interrupt(void)
{
    pFlexCANReg->CTRL1 &= ~CAN_CTRL1_TWRNMSK;
}

void FlexCAN_EnableRxWarning_Interrupt(void)
{
    pFlexCANReg->CTRL1 |= CAN_CTRL1_RWRNMSK;
}

void FlexCAN_DisableRxWarning_Interrupt(void)
{
    pFlexCANReg->CTRL1 &= ~CAN_CTRL1_RWRNMSK;
}

void FlexCAN_EnableWakeup_Interrupt(void)
{
    pFlexCANReg->MCR |= CAN_MCR_WAKMSK;
}

void FlexCAN_DisableWakeup_Interrupt(void)
{
    pFlexCANReg->MCR &= ~CAN_MCR_WAKMSK;
}

void FlexCAN_MB_ISRAck(void)
{
    // Check Rx MB flags, so Rx MBs has highest priority than Tx MBs
    FlexCAN_Poll_RxMBs();

    // Check Tx MB flags
    FlexCAN_Poll_TxMBs();

    // Check Rx FIFO status and handle it
    // If Rx FIFO is enabled, check FIFO flag
    if(pFlexCANReg->MCR & CAN_MCR_RFEN)
    {
        FlexCAN_Poll_RxFIFO();
    }
}

void FlexCAN_BusOff_ISRAck(void)
{
    pFlexCANReg->ESR1 = CAN_ESR1_BOFFINT;
}

void FlexCAN_Error_ISRAck(void)
{
    // check error flags
    uint32_t status;

    status = pFlexCANReg->ESR1;

    if (status & CAN_ESR1_ERRINT)
    {
        nCANErrorCount++;

        // Call CAN error call back                		
        FlexCAN_Err_Callback(status);

        // clear the error flag
        pFlexCANReg->ESR1 = status;
    }
}

void FlexCAN_TxWarning_ISRAck(void)
{
    pFlexCANReg->ESR1 = CAN_ESR1_TXWRN;
}

void FlexCAN_RxWarning_ISRAck(void)
{
    pFlexCANReg->ESR1 = CAN_ESR1_RXWRN;
}

void FlexCAN_Wakeup_ISRAck(void)
{
    pFlexCANReg->ESR1 = CAN_ESR1_WAKINT;    // clear wakeup interrupt
}
