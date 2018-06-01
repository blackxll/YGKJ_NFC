/* *************************
* Copyright 2018 NXP
* All rights reserved.
**************************** */

#ifdef NFC_STACK_USED
#include "nfc_driver_config.h"
#include "i2c.h"
#include "tml.h"

/* wait for RX is blocking. That is why we need to call freeMASTER poll here */
#include "freemaster.h"

static uint8_t tml_Init(void) {
	I2C_Init();

	//GPIO has been initialized in peripheralInit.c
	//gpio_Init(); 

	/* Configure GPIO for IRQ pin */
	//gpio_SetDir(PORT_IRQ, PIN_IRQ, SET_IN);

	/* Configure GPIO for VEN pin */
	//gpio_SetDir(PORT_VEN, PIN_VEN, SET_OUT);

	return SUCCESS;
}

static uint8_t tml_Reset(void) 
{
	/* Apply VEN reset */
	SET_VEN_HIGH;
	SLEEP(10);
	SET_VEN_LOW;
	SLEEP(10);
	SET_VEN_HIGH;

	return SUCCESS;
}

static uint8_t tml_Tx(uint8_t *pBuff, uint16_t buffLen) 
{
    if (I2C_Write(NXPNCI_I2C_ADDR, pBuff, buffLen) != SUCCESS)
    {
    	SLEEP(10);
    	if(I2C_Write(NXPNCI_I2C_ADDR, pBuff, buffLen) != SUCCESS)
    	{
    		return ERROR;
    	}
    }

	return SUCCESS;
}

static uint8_t tml_Rx(uint8_t *pBuff, uint16_t buffLen, uint16_t *pBytesRead) 
{
	uint8_t ret;

	ret = I2C_Read(NXPNCI_I2C_ADDR, pBuff, 3);

	if (ret == SUCCESS) 
	{
		if (pBuff[2] != 0) 
		{
			ret = I2C_Read(NXPNCI_I2C_ADDR, &pBuff[3], pBuff[2]);
			if (ret == SUCCESS) 
			{
				*pBytesRead = pBuff[2] + 3;
			} 
			else 
			{
				*pBytesRead = 0;
			}
		} 
		else 
		{
			*pBytesRead = 3;
		}
	} 
	else 
	{
		*pBytesRead = 0;
	}

	return ret;

}

static uint8_t tml_WaitForRx(uint16_t timeout) 
{
	if (timeout == 0) 
	{
		//while ((gpio_GetValue(PORT_IRQ, PIN_IRQ) == LOW));
		while (GET_IRQ_VALUE == LOW)
		{
			/* wait for irq */
			
			/* this is blocking. That is why we need to call freeMASTER poll here */
			/* if interrupt based FMSTR is activated, this call has no effect */
			FMSTR_Poll();
		}
	} 
	else 
	{
		//while ((gpio_GetValue(PORT_IRQ, PIN_IRQ) == LOW)) 
		while (GET_IRQ_VALUE == LOW)
		{
			/* this is blocking. That is why we need to call freeMASTER poll here */
			/* if interrupt based FMSTR is activated, this call has no effect */
			FMSTR_Poll();
			
			SLEEP(10);  
			timeout -= 10;
			if (timeout <= 0) return ERROR;
		}
	}
	return SUCCESS;
}

void tml_Connect(void) 
{
	tml_Init();
	tml_Reset();
}

void tml_Send(uint8_t *pBuffer, uint16_t BufferLen, uint16_t *pBytesSent) 
{
	if(tml_Tx(pBuffer, BufferLen) == ERROR)
	{
		*pBytesSent = 0;
	}
	else
	{
		*pBytesSent = BufferLen;
	}
}

void tml_Receive(uint8_t *pBuffer, uint16_t BufferLen, uint16_t *pBytes, uint16_t timeout) 
{
	if (tml_WaitForRx(timeout) == ERROR)
		*pBytes = 0;
	else
		tml_Rx(pBuffer, BufferLen, pBytes);
}

#endif
