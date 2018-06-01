/*******************************************************************************
*
* Copyright 2012-2014 Freescale Semiconductor, Inc.
* Copyright 2017~2018 NXP.
* All rights reserved.
*******************************************************************************/

#include "defines.h"
#include "cpu.h"

#include "i2c.h"

#define POLLING_TRIALS      0xFFF0U

static void I2C_SetAddr(uint8 addr)
{
    while(I2C0_S & I2C0_S_BUSY) {}
    I2C0_C1 |= I2C0_C1_TX;
    if(I2C0_C1 & I2C0_C1_MST)
    {
        I2C0_C1 |= I2C0_C1_RSTA;
    }
    else
    {
        I2C0_C1 |= I2C0_C1_MST;
    }
    I2C0_D = addr;
}

static boolean I2C_SendBlock(uint8 *pBlock, uint16 wLen)
{
    boolean ret = FALSE;
    uint16 Tr;
    uint16 status;

    for(Tr = 0U; Tr < POLLING_TRIALS; Tr++)
    {
        if(I2C0_S & I2C0_S_IICIF)
        {
            status = I2C0_S;
            I2C0_S |= I2C0_S_IICIF;
            Tr = 0;

            if(status & I2C0_S_RXAK)
            {
                //No Rx ACK signal, error
                I2C0_C1 &= ~I2C0_C1_MST; //stop signal sent
                I2C0_C1 &= ~I2C0_C1_TX;  //switch to Rx mode
                break;
            }

            if(wLen != 0U)
            {
                wLen--; //Decrese number of chars for the transmit
                I2C0_D = *pBlock++;
            }
            else
            {
                //Finished data sent
                I2C0_C1 &= ~I2C0_C1_MST; //stop signal sent
                I2C0_C1 &= ~I2C0_C1_TX;  //switch to Rx mode
                for(Tr = POLLING_TRIALS;Tr != 0U; Tr--)
                {
                    if((I2C0_S & I2C0_S_BUSY) == 0U)
                    {
                        ret = TRUE;
                        break;
                    }
                }
                break;
            }
        }
    }
    return ret;
}

static boolean I2C_RecvBlock(uint8 *pBlock, uint16 wLen)
{
    boolean ret = FALSE;
    uint16 Tr;
    uint16 status;
    
    for(Tr = 0U; Tr < POLLING_TRIALS; Tr++)
    {
        if(I2C0_S & I2C0_S_IICIF)
        {
            status = I2C0_S;
            I2C0_S |= I2C0_S_IICIF;
            Tr = 0;
            
            if(I2C0_C1 & I2C0_C1_TX)
            {
                if(status & I2C0_S_RXAK)
                {
                    //No Rx ACK signal, error
                    I2C0_C1 &= ~I2C0_C1_MST; //stop signal sent
                    I2C0_C1 &= ~I2C0_C1_TX;  //switch to Rx mode
                    break;
                }
                else
                {
                    if(wLen != 0)
                    {
                        uint16 dummy;
                        if(wLen == 1U)
                        {
                            //only one byte, not send ACK
                            I2C0_C1 |= I2C0_C1_TXAK;
                        }
                        else
                        {
                            I2C0_C1 &= ~I2C0_C1_TXAK;
                        }
                        I2C0_C1 &= ~I2C0_C1_TX; //switch to Rx mode
                        dummy = I2C0_D;         /* Dummy read character */
                    }
                    else
                    {
                        //no data needs read, issue STOP signal
                        I2C0_C1 &= ~I2C0_C1_MST; //stop signal sent
                        I2C0_C1 &= ~I2C0_C1_TX; //switch to Rx mode
                        for(Tr = POLLING_TRIALS;Tr != 0U; Tr--)
                        {
                            if((I2C0_S & I2C0_S_BUSY) == 0U)
                            {
                                ret = TRUE;
                                break;
                            }
                        }
                        break;
                    }
                }
            }
            else
            {
                wLen--;
                if(wLen != 0U)
                {
                    if(wLen == 1U)
                    {
                        //One byte left, not send ACK
                        I2C0_C1 |= I2C0_C1_TXAK;
                    }
                }
                else
                {
                    I2C0_C1 &= ~I2C0_C1_MST;  //stop signal sent
                    for(Tr = POLLING_TRIALS;Tr != 0U; Tr--)
                    {
                        if((I2C0_S & I2C0_S_BUSY) == 0U)
                        {
                            ret = TRUE;
                            break;
                        }
                    }
                }
                *pBlock++ = (uint8) I2C0_D;
            }
        }
    }
	return ret;
}

void I2C_Init(void)
{
    I2C0_C1  = 0U;
    I2C0_S   = 0x12U;
    I2C0_FLT = 0U;
    I2C0_F   = 0x33U; //0x33: 100K, 0x23: 400K
    I2C0_SMB = 0x08U;
    I2C0_C1 |= I2C0_C1_IICEN;
}

void I2C_Enable(uint8 byIsEn)
{
    if(byIsEn)
    {
        I2C0_C1 |= I2C0_C1_IICEN;
    }
    else
    {
        I2C0_C1 &= ~I2C0_C1_IICEN;
    }
}

boolean I2C_Write(uint8 addr, uint8 *pCmd, uint16 Cmd_size)
{
    I2C_SetAddr((addr<<1));
    return I2C_SendBlock(pCmd, Cmd_size);
}

boolean I2C_Read(uint8 addr, uint8 *pRsp, uint16 Rsp_size)
{
    I2C_SetAddr((addr<<1)+1);
    return I2C_RecvBlock(pRsp, Rsp_size);
}
