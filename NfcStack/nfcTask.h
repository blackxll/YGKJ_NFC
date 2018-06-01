/* *************************
* Copyright 2018 NXP
* All rights reserved.
**************************** */

#ifndef NFCTASK_H_
#define NFCTASK_H_

#define NFC_RD     1
#define NFC_WR     2
#define NFC_P2P    3
#define NFC_CE     4

#define ST_WCH 				(uint8_t) 0x00u
#define ST_CARD_DETECTION	(uint8_t) 0x01u
#define ST_APDU_EXCHANGE	(uint8_t) 0x02u
#define ST_BT_P2P			(uint8_t) 0x03u
#define ST_CE				(uint8_t) 0x04u
#define ST_NFC_WCH			(uint8_t) 0x05u
#define ST_NFC_WCH_TAG		(uint8_t) 0x06u
#define ST_WCH_NO_NFC		(uint8_t) 0x07u
#define ST_BT_P2P_TRANSMIT	(uint8_t) 0x08u
#define ST_CE_TRANSMIT	(uint8_t) 0x09u

#define APDU_ST_INIT			(uint8_t) 0x00
#define APDU_ST_AID_SELECT_SUCC	(uint8_t) 0x01
#define APDU_ST_AID_SELECT_ERR	(uint8_t) 0x02
#define APDU_ST_RDM_BYTES_SUCC	(uint8_t) 0x03
#define APDU_ST_RDM_BYTES_ERR	(uint8_t) 0x04
#define APDU_ST_AES_SUCC		(uint8_t) 0x05
#define APDU_ST_AES_ERR			(uint8_t) 0x06

typedef enum
{
    NFC_START_DISCOVERY = 0x00, 
	NFC_WAIT_DISCOVERY_NTF,
	NFC_DISCOVERY_NTF,
	NFC_CARD_INFO, 
	NFC_PRESENCE_CHECK,
	NFC_STOP_DISCOVERY,
	NFC_INVALID,
    NFC_IDLE
}NfcState_t;

extern NfcState_t state;
extern uint8_t FMSTR_Uid[18];
extern uint8_t FMSTR_UidLength;
extern uint8_t FMSTR_Atqa[2];
extern uint8_t FMSTR_Sak[2];
extern uint8_t FMSTR_NfcTechnology;
extern uint8_t FMSTR_TagDetected;
extern uint8_t FMSTR_AppState;
extern uint8_t FMSTR_TagType;

void init_nfc(unsigned char);
boolean lpcd_calib(void);
boolean lpcd_wakeup(void);
void task_nfc(void);
void NDEFMessage_Init(void);

#endif /* NFCTASK_H_ */
