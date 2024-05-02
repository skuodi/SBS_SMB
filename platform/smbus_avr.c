/**
 * @file    smbus_avr.c  -   SMBUS data link layer protocol bus controller implementation using AVR TWI
 *                       -   Tested on ATMEGA32U4
 * @author  skuodi
 * @date    17 January, 2023. Revised on 7 April 2024.
 * @brief   The @sequence in each function description denotes the sequence of transaction protocol elements that make up the I2C bus protocol
 *          Uppercase denotes an element sent from the bus controller and lowercase denotes an element sent from a peripheral device
 *          e.g. ...,data byte,A,... means a peripheral device sent an 8-bit byte of data, which was received and acknowledged by the bus controller
 *          e.g. ...,DATA BYTE,a,... means the bus controller device sent an 8-bit byte of data, which was received and acknowledged by a peripheral device.
 * 
 * @note    This SMBus AVR implementation does not support PEC and the handle's usePEC field is igonored
 * 
 * **/
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

#include "avr/io.h"
#include "smbus_platform.h"

#define FCPU 16000000

struct smbus_handle
{
    void* i2cPort;    // Implementation specific handle to an I2C peripheral
    smbus_info_t info;
};

/****Implementation specific definitions - For AVR*****/

#define TWI_STATUS                                      (TWSR & 0xF8)

#define TWI_STATUS_START_TRANSMITTED                    (1<<3)
#define TWI_STATUS_REPEATED_START_TRANSMITTED           (2<<3)
#define TWI_STATUS_ADDR_W_TRANSMITTED_ACK_RECIEVED      (3<<3)
#define TWI_STATUS_ADDR_W_TRANSMITTED_NACK_RECIEVED     (4<<3)
#define TWI_STATUS_DATA_TRANSMITTED_ACK_RECIEVED        (5<<3)
#define TWI_STATUS_DATA_TRANSMITTED_NACK_RECIEVED       (6<<3)
#define TWI_STATUS_ARBITRATION_LOST                     (7<<3)
#define TWI_STATUS_ADDR_R_TRANSMITTED_ACK_RECIEVED      (8<<3)
#define TWI_STATUS_ADDR_R_TRANSMITTED_NACK_RECIEVED     (9<<3)
#define TWI_STATUS_DATA_RECIEVED_ACK_TRANSMITTED        (10<<3)
#define TWI_STATUS_DATA_RECIEVED_NACK_TRANSMITTED       (11<<3)

#define TWI_STATUS_TO_SMBUS_ERR()                       -(TWSR >> 3)

#define TWI_WAIT()              while(!(TWCR & (1<<TWINT)))//Wait for the last initiated operation to be executed

#define TWI_START()             do{ \
                                    TWCR = ( 1 << TWINT) | (1 << TWSTA) | (1 << TWEN);\
                                    TWI_WAIT();\
                                    if (TWI_STATUS != TWI_STATUS_START_TRANSMITTED)\
                                        return TWI_STATUS_TO_SMBUS_ERR();\
                                }while(0) //Send start condition = clear interrupt flag, set start bit, enable TWI

#define TWI_START_REPEATED()    do{ \
                                    TWCR = ( 1 << TWINT) | (1 << TWSTA) | (1 << TWEN);\
                                    TWI_WAIT();\
                                    if (TWI_STATUS != TWI_STATUS_REPEATED_START_TRANSMITTED)\
                                        return TWI_STATUS_TO_SMBUS_ERR();\
                                }while(0) //Send repeated start condition = clear interrupt flag, set start bit, enable TWI

#define TWI_SEND_BYTE(d)        do{\
                                    TWDR = d;\
                                    TWCR = (1<<TWINT) | (1<<TWEN);\
                                    TWI_WAIT();\
                                }while(0) // Place data in data register and wait for it to be sent

#define TWI_SEND_DATA_ACK(d)    do{\
                                    TWI_SEND_BYTE(d);\
                                    if (TWI_STATUS != TWI_STATUS_DATA_TRANSMITTED_ACK_RECIEVED)\
                                        return TWI_STATUS_TO_SMBUS_ERR();\
                                }while(0) //Send one data byte and check for ACK

#define TWI_SEND_DATA_NACK(d)   do{\
                                    TWI_SEND_BYTE(d);\
                                    if (TWI_STATUS != TWI_STATUS_DATA_TRANSMITTED_NACK_RECIEVED)\
                                        return TWI_STATUS_TO_SMBUS_ERR();\
                                }while(0) //Send one data byte and check for NACK

#define TWI_RECV_DATA_ACK(dp)    do{\
                                    TWCR = (1 << TWINT) | (1 << TWEN) | (1 << TWEA);\
                                    TWI_WAIT();\
                                    *dp = TWDR;\
                                    if (TWI_STATUS != TWI_STATUS_DATA_RECIEVED_ACK_TRANSMITTED)\
                                        return TWI_STATUS_TO_SMBUS_ERR();\
                                }while(0) //Receive one data byte and reply with ACK

#define TWI_RECV_DATA_NACK(dp)  do{\
                                    TWCR = (1 << TWINT) | (1<<TWEN);\
                                    TWI_WAIT();\
                                    *dp = TWDR;\
                                    if (TWI_STATUS != TWI_STATUS_DATA_RECIEVED_NACK_TRANSMITTED)\
                                        return TWI_STATUS_TO_SMBUS_ERR();\
                                }while(0) //Receive one data byte and reply with NACK

#define TWI_SEND_ADDR_W_ACK(a)  do{\
                                    TWI_SEND_BYTE(a << 1);\
                                    if (TWI_STATUS != TWI_STATUS_ADDR_W_TRANSMITTED_ACK_RECIEVED)\
                                        return TWI_STATUS_TO_SMBUS_ERR();\
                                }while(0) //Send device address with RW bit = 0, and check for ACK

#define TWI_SEND_ADDR_R_ACK(a)  do{\
                                    TWI_SEND_BYTE( (a << 1) | 1);\
                                    if (TWI_STATUS != TWI_STATUS_ADDR_R_TRANSMITTED_ACK_RECIEVED)\
                                        return TWI_STATUS_TO_SMBUS_ERR();\
                                }while(0) //Send device address with RW bit = 1, and check for ACK

#define TWI_STOP()              do{\
                                    TWCR = (1 << TWINT) | (1 << TWSTO) | (1 << TWEN);\
                                }while(0) //Send stop condition = clear interrupt flag, set stop bit, enable TWI


#define TWI_MAX_SPEED           400000

                   
smbus_handle_t SMBusInit(void* i2cPort, int8_t myAddress, uint32_t i2cSpeed, int sdaPin, int sclPin, int intPin, long timeoutMs)
{
    if(!i2cPort || i2cSpeed > TWI_MAX_SPEED)
        return NULL;
    
    uint8_t twbr;

    // Enable internal pullup resistors.
    // The pullup strength of these resistors is not adequate for SMBUS on AVR
    // and external resistors (1k-10k) are required for proper bus funtionality.
    MCUCR &= ~( 1 << PUD);

    //Set Fscl = Fcpu / ( 16 + 2( TWBR ))*4^TWPS
    //TWPS = 0; For simplicity, eliminate TWPS from the equation
    twbr = FCPU / (2 * i2cSpeed);
    if(twbr == 0)
        return NULL;

    TWBR = twbr;   
    TWSR |=  1;
    TWCR |= (1 << TWEN);//Enable TWI

    struct smbus_handle* busHandle = (struct smbus_handle*)malloc(sizeof(struct smbus_handle));
    if(!busHandle)
        return NULL;

    busHandle->i2cPort = i2cPort;
    busHandle->info.myAddress = myAddress;
    busHandle->info.i2cSpeed = FCPU /  (16 + (2 * twbr ));   // Set to the actual speed after integer truncation
    busHandle->info.sdaPin = sdaPin;
    busHandle->info.sclPin = sclPin;
    busHandle->info.intPin = intPin;
    busHandle->info.timeoutMs = timeoutMs;

    return busHandle;
}

smbus_status_t SMBusDeinit(smbus_handle_t handle)
{
    if(!handle)
        return SMBUS_ERR_INVALID_ARG;
    
    free(handle);
    return SMBUS_ERR_OK;
    
}

smbus_status_t SMBusGetInfo(smbus_handle_t handle, smbus_info_t *info)
{
    if(!handle || !info)
        return SMBUS_ERR_INVALID_ARG;
    
    info->myAddress = ((struct smbus_handle*)handle)->info.myAddress;
    info->i2cSpeed  = ((struct smbus_handle*)handle)->info.i2cSpeed;
    info->sdaPin    = ((struct smbus_handle*)handle)->info.sdaPin;
    info->sclPin    = ((struct smbus_handle*)handle)->info.sclPin;
    info->intPin    = ((struct smbus_handle*)handle)->info.intPin;
    
    return SMBUS_ERR_OK;
    
}


/**
 * @note    This function does not comply with the SMBus standard on AVR as the TWI phy
 *          cannot transmit an address with the RW bit set, followed by a stop condition.
 *          Thus this command is always sent with R/W = 0 on AVR
 * 
 **/
smbus_status_t SMBusQuickCommand(smbus_handle_t handle, uint8_t devAddr, bool readWriteBit)
{
    TWI_START();
    TWI_SEND_ADDR_W_ACK(devAddr);
    TWI_STOP();

    return SMBUS_ERR_OK;
}

smbus_status_t SMBusSendByte(smbus_handle_t handle, uint8_t devAddr, uint8_t data)
{
    TWI_START();
    TWI_SEND_ADDR_W_ACK(devAddr);
    TWI_SEND_DATA_ACK(data);
    TWI_STOP();

    return SMBUS_ERR_OK;
}

smbus_status_t SMBusReceiveByte(smbus_handle_t handle, uint8_t devAddr, uint8_t* data)
{
    if(!handle || !data)
        return SMBUS_ERR_INVALID_ARG;

    TWI_START();
    TWI_SEND_ADDR_R_ACK(devAddr);
    TWI_RECV_DATA_NACK(data);
    TWI_STOP();

    return SMBUS_ERR_OK;
}

smbus_status_t SMBusWriteByte(smbus_handle_t handle, uint8_t devAddr, uint8_t command, uint8_t data)
{
    TWI_START();
    TWI_SEND_ADDR_W_ACK(devAddr);
    TWI_SEND_DATA_ACK(command);
    TWI_SEND_DATA_ACK(data);
    TWI_STOP();

    return SMBUS_ERR_OK;
}

smbus_status_t SMBusWriteWord(smbus_handle_t handle, uint8_t devAddr, uint8_t command, uint16_t data)
{
    TWI_START();
    TWI_SEND_ADDR_W_ACK(devAddr);
    TWI_SEND_DATA_ACK(command);
    TWI_SEND_DATA_ACK(data & 0xFF);
    TWI_SEND_DATA_ACK(data >> 8);
    TWI_STOP();

    return SMBUS_ERR_OK;

}

smbus_status_t SMBusReadByte(smbus_handle_t handle, uint8_t devAddr, uint8_t command, uint8_t* data)
{
    if(!handle || !data)
        return SMBUS_ERR_INVALID_ARG;

    TWI_START();
    TWI_SEND_ADDR_W_ACK(devAddr);
    TWI_SEND_DATA_ACK(command);
    TWI_START_REPEATED();
    TWI_SEND_ADDR_R_ACK(devAddr);
    TWI_RECV_DATA_ACK(data);
    TWI_STOP();

    return SMBUS_ERR_OK;
}

smbus_status_t SMBusReadWord(smbus_handle_t handle, uint8_t devAddr, uint8_t command, uint16_t* data)
{ 
    if(!handle || !data)
        return SMBUS_ERR_INVALID_ARG;

    uint8_t dataLow, dataHigh;

    TWI_START();
    TWI_SEND_ADDR_W_ACK(devAddr);
    TWI_SEND_DATA_ACK(command);
    TWI_START_REPEATED();
    TWI_SEND_ADDR_R_ACK(devAddr);
    TWI_RECV_DATA_ACK((&dataLow));
    TWI_RECV_DATA_NACK((&dataHigh));
    *data = (dataHigh << 8) | dataLow;
    TWI_STOP();

    return SMBUS_ERR_OK;
}

smbus_status_t SMBusProcessCall(smbus_handle_t handle, uint8_t devAddr, uint8_t command, uint16_t dataSent, uint16_t* dataRecv)
{
    if(!handle || !dataRecv)
        return SMBUS_ERR_INVALID_ARG;

    uint8_t dataRecvHigh, dataRecvLow;

    TWI_START();
    TWI_SEND_ADDR_W_ACK(devAddr);
    TWI_SEND_DATA_ACK(command);
    TWI_SEND_DATA_ACK(dataSent & 0xFF);
    TWI_SEND_DATA_ACK(dataSent >> 8);
    TWI_START_REPEATED();
    TWI_SEND_ADDR_R_ACK(devAddr);
    TWI_RECV_DATA_ACK((&dataRecvLow));
    TWI_RECV_DATA_NACK((&dataRecvHigh));
    *dataRecv = (dataRecvHigh << 8) | dataRecvLow;
    TWI_STOP();

    return SMBUS_ERR_OK;

}

smbus_status_t SMBusBlockWrite(smbus_handle_t handle, uint8_t devAddr, uint8_t command, uint8_t* dataSent, uint8_t dataLength)
{
    if(!handle || !dataSent)
        return SMBUS_ERR_INVALID_ARG;
    
    TWI_START();
    TWI_SEND_ADDR_W_ACK(devAddr);
    TWI_SEND_DATA_ACK(command);
    TWI_SEND_DATA_ACK(dataLength);

    for(int i = 0; i < dataLength; i++)
        TWI_SEND_DATA_ACK(dataSent[i]);

    TWI_STOP();

    return SMBUS_ERR_OK;
}

smbus_status_t SMBusBlockRead(smbus_handle_t handle, uint8_t devAddr, uint8_t command, uint8_t* dataRecv, uint8_t *dataLength)
{
    if(!handle || !dataRecv || !dataLength)
        return SMBUS_ERR_INVALID_ARG;
    
    TWI_START();
    TWI_SEND_ADDR_W_ACK(devAddr);
    TWI_SEND_DATA_ACK(command);
    TWI_START_REPEATED();
    TWI_SEND_ADDR_R_ACK(devAddr);
    TWI_RECV_DATA_ACK(dataLength);

    for(int i = 0; i < *dataLength - 1; i++)
        TWI_RECV_DATA_ACK((&dataRecv[i]));
    
    TWI_RECV_DATA_NACK((&dataRecv[*dataLength - 1]));
    TWI_STOP();

    return SMBUS_ERR_OK;

}

smbus_status_t SMBusBlockWriteBlockReadProcessCall(smbus_handle_t handle, uint8_t devAddr, uint8_t command, uint8_t* dataSent,
                                                    uint8_t dataSentLength, uint8_t* dataRecv, uint8_t* dataRecvLength)
{
    if(!handle || !dataSent  || !dataRecv || !dataRecvLength)
        return SMBUS_ERR_INVALID_ARG;

    TWI_START();
    TWI_SEND_ADDR_W_ACK(devAddr);
    TWI_SEND_DATA_ACK(command);
    TWI_SEND_DATA_ACK(dataSentLength);

    for(int i = 0; i < dataSentLength; i++)
        TWI_SEND_DATA_ACK(dataSent[i]);

    TWI_START_REPEATED();
    TWI_SEND_ADDR_R_ACK(devAddr);

    //Receive Data
    TWI_RECV_DATA_ACK(dataRecvLength);

    for(int i = 0; i < *dataRecvLength - 1; i++)
        TWI_RECV_DATA_ACK((&dataRecv[i]));
    
    TWI_RECV_DATA_NACK((&dataRecv[*dataRecvLength - 1]));

    TWI_STOP();

    return SMBUS_ERR_OK;

}

smbus_status_t SMBusHostNotify(smbus_handle_t handle, uint8_t hostAddr, uint8_t devAddr, uint16_t data)
{
   return SMBusWriteWord(handle, hostAddr, devAddr << 1, data);
}

smbus_status_t SMBusWrite32(smbus_handle_t handle, uint8_t devAddr, uint8_t command, uint32_t dataSent)
{
    TWI_START();

    TWI_SEND_ADDR_W_ACK(devAddr);
    TWI_SEND_DATA_ACK(command);

    for(int i = 0; i < 4; i++)
        TWI_SEND_DATA_ACK((dataSent >> (i*8)) & 0xFF);

    TWI_STOP();

    return SMBUS_ERR_OK;
}

smbus_status_t SMBusRead32(smbus_handle_t handle, uint8_t devAddr, uint8_t command, uint32_t* dataRecv)
{
    if(!handle || !dataRecv)
        return SMBUS_ERR_INVALID_ARG;

    uint8_t dataBuf[4];
    TWI_START();
    TWI_SEND_ADDR_W_ACK(devAddr);
    TWI_SEND_DATA_ACK(command);
    TWI_START_REPEATED();
    TWI_SEND_ADDR_R_ACK(devAddr);

    for(int i = 0; i < 3; i++)
        TWI_RECV_DATA_ACK((&dataBuf[i]));
    
    TWI_RECV_DATA_NACK((&dataBuf[3]));

    *dataRecv = ((uint32_t)dataBuf[3] << 24) |
                ((uint32_t)dataBuf[2] << 16) |
                ((uint32_t)dataBuf[1] <<  8) |
                ((uint32_t)dataBuf[0] <<  0);

    TWI_STOP();

    return SMBUS_ERR_OK;

}

smbus_status_t SMBusWrite64(smbus_handle_t handle, uint8_t devAddr, uint8_t command, uint64_t dataSent)
{
    TWI_START();
    TWI_SEND_ADDR_W_ACK(devAddr);
    TWI_SEND_DATA_ACK(command);

    for(int i = 0; i < 8; i++)
        TWI_SEND_DATA_ACK((dataSent >> (i*8)) & 0xFF);

    TWI_STOP();

    return SMBUS_ERR_OK;
}

smbus_status_t SMBusRead64(smbus_handle_t handle, uint8_t devAddr, uint8_t command, uint64_t* dataRecv)
{
    if(!handle || !dataRecv)
        return SMBUS_ERR_INVALID_ARG;
    
    uint8_t dataBuf[8];

    TWI_START();
    TWI_SEND_ADDR_W_ACK(devAddr);
    TWI_SEND_DATA_ACK(command);
    TWI_START_REPEATED();
    TWI_SEND_ADDR_R_ACK(devAddr);

    for(int i = 0; i < 8 - 1; i++)
        TWI_RECV_DATA_ACK((&dataBuf[i]));
    
    TWI_RECV_DATA_NACK((&dataBuf[8 - 1]));

    *dataRecv = ((uint64_t)dataBuf[7] << 54) |
                ((uint64_t)dataBuf[6] << 48) |
                ((uint64_t)dataBuf[5] << 40) |
                ((uint64_t)dataBuf[4] << 32) |
                ((uint64_t)dataBuf[3] << 24) |
                ((uint64_t)dataBuf[2] << 16) |
                ((uint64_t)dataBuf[1] <<  8) |
                ((uint64_t)dataBuf[0] <<  0);

    TWI_STOP();

    return SMBUS_ERR_OK;

}

uint8_t SMBusCrc8(uint8_t crc8, uint8_t const *data, uint16_t dataLength)
{
    if (!data)
        return 0xff;

    while (dataLength--)
	{
        crc8 ^= *data++;
        for (uint8_t k = 0; k < 8; k++)
            crc8 = crc8 & 0x80 ? (crc8 << 1) ^ 0x07 : crc8 << 1;
    }

    return crc8;
}