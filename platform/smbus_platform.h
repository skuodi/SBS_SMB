/**
 * @file    smbus_platform.h  -   smbus data link layer implementation header file
 * @author  skuodi
 * @date    7 April 2024.
 * 
 * **/


#ifndef _SMBUS_PLATFORM_H
#define _SMBUS_PLATFORM_H 

#include <stdint.h>
#include <stdbool.h>

//millisecond delay
#ifdef __AVR_ARCH__
#define SMBUS_PLATFORM_MS_DELAY(x)    _delay_ms(x) 
#endif

typedef enum
{
    SMBUS_ERR_OK                                 =   0,
    SMBUS_ERR_FAIL                               =  -20,
    SMBUS_ERR_INVALID_ARG                        =  -21,
    SMBUS_ERR_BAD_CRC                            =  -22,
    SMBUS_ERR_TIMEOUT                            =  -23,
    SMBUS_ERR_UNEXPECTED_DATA_RECEIVED           =  -24,
    SMBUS_ERR_START_TRANSMITTED                  =  -1,
    SMBUS_ERR_REPEATED_START_TRANSMITTED         =  -2,
    SMBUS_ERR_ADDR_W_TRANSMITTED_ACK_RECIEVED    =  -3,
    SMBUS_ERR_ADDR_W_TRANSMITTED_NACK_RECIEVED   =  -4,
    SMBUS_ERR_DATA_TRANSMITTED_ACK_RECIEVED      =  -5,
    SMBUS_ERR_DATA_TRANSMITTED_NACK_RECIEVED     =  -6,
    SMBUS_ERR_ARBITRATION_LOST                   =  -7,
    SMBUS_ERR_ADDR_R_TRANSMITTED_ACK_RECIEVED    =  -8,
    SMBUS_ERR_ADDR_R_TRANSMITTED_NACK_RECIEVED   =  -9,
    SMBUS_ERR_DATA_RECIEVED_ACK_TRANSMITTED      =  -10,
    SMBUS_ERR_DATA_RECIEVED_NACK_TRANSMITTED     =  -11,
}smbus_err_t;


typedef struct 
{
    uint8_t myAddress;  // The 7-bit I2C address that this device responds to, either as a host or a peripheral device.
    uint32_t i2cSpeed;
    long timeoutMs;
    int sdaPin;
    int sclPin;
    int intPin;         // I2C Interrupt/SMBALERT pin
    int usePEC;         // Whether to append/check a checksum byte when sending/receiving
}smbus_info_t;

typedef struct smbus_handle* smbus_handle_t;

/****************************************************SMBus Protocol Functions***************************************/

/**
 * @brief           Initialize the provided I2C peripheral and return an SMBUS instance attached to it
 * @param myAddress The 7-bit I2C address that this device responds to,
 *                  either as a host or a peripheral device.
 **/
smbus_handle_t SMBusInit(void* i2cPort, int8_t myAddress, uint32_t i2cSpeed, int sdaPin, int sclPin, int intPin, long timeoutMs, bool usePec);

/**
 * @brief Deinitialize the I2C peripheral attached to the SMBUS \a handle and free allocated resources
 **/
smbus_err_t SMBusDeinit(smbus_handle_t handle);

smbus_err_t SMBusGetInfo(smbus_handle_t handle, smbus_info_t *info);

/**
 * @brief                       Send the device address along with a read/write bit
 * @param       devAddr         7-bit peripheral device address
 * @param       readWriteBit    The state of the read/write bit
 *  
 * @sequence:                   S,ADDRESS,W,a,P
 * 
 **/
smbus_err_t SMBusQuickCommand(smbus_handle_t handle, uint8_t devAddr, bool readWriteBit);

/**
 * @brief                   send 8-bit data
 * @param       devAddr     7-bit peripheral device address
 * 
 * @sequence:               S,ADDRESS,W,a,DATA BYTE,a,P 
 * 
 **/
smbus_err_t SMBusSendByte(smbus_handle_t handle, uint8_t devAddr, uint8_t data);

/**
 * @brief                   receive 8-bit data
 * @param       devAddr     7-bit peripheral device address
 * 
 * @sequence:               S,ADDRESS,R,a,data byte,N,P 
 * 
 **/
smbus_err_t SMBusReceiveByte(smbus_handle_t handle, uint8_t devAddr, uint8_t* data);

/**
 * @brief                   send an 8-bit command followed by 8-bit data
 * @param       devAddr     7-bit peripheral device address
 * 
 * @sequence:               S,ADDRESS+W,a,COMMAND BYTE,a,DATA BYTE,a,P
 * 
 **/
smbus_err_t SMBusWriteByte(smbus_handle_t handle, uint8_t devAddr, uint8_t command, uint8_t data);

/**
 * @brief                   send an 8-bit command followed by 16-bit data
 * @param       devAddr     7-bit peripheral device address
 * 
 * @sequence:               S,ADDRESS+W,a,COMMAND BYTE,a,DATA LOW BYTE,a,DATA HIGH BYTE,a,P
 * 
 **/
smbus_err_t SMBusWriteWord(smbus_handle_t handle, uint8_t devAddr, uint8_t command, uint16_t data);

/**
 * @brief                   send an 8-bit command then receive 8-bit data
 * @param       devAddr     7-bit peripheral device address
 * 
 * @sequence:               S,ADDRESS+W,a,COMMAND BYTE,a,Sr,ADDRESS+R,a,data byte,N,P
 * 
 **/
smbus_err_t SMBusReadByte(smbus_handle_t handle, uint8_t devAddr, uint8_t command, uint8_t* data);

/**
 * @brief                   send an 8-bit command then receive 16-bit data
 * @param       devAddr     7-bit peripheral device address
 * 
 * @sequence:               S,ADDRESS+W,a,COMMAND BYTE,a,Sr,ADDRESS+R,a,data low byte,A,data high byte,N,P
 * 
 **/
smbus_err_t SMBusReadWord(smbus_handle_t handle, uint8_t devAddr, uint8_t command, uint16_t* data);

/**
 * @brief                   send an 8-bit command followed by 16-bit data then receive 16-bit data
 * @param       devAddr     7-bit peripheral device address
 * 
 * @sequence:               S,ADDRESS+W,a,COMMAND,a,DATA LOW BYTE,a,DATA HIGH BYTE,a,Sr,ADDRESS+R,a,data low byte,A,data high byte,N,P
 * 
 **/
smbus_err_t SMBusProcessCall(smbus_handle_t handle, uint8_t devAddr, uint8_t command, uint16_t dataSent,   uint16_t* dataRecv);

/**
 * @brief                   send an 8-bit command followed by 8-bit datalength then [datalength] consecutive bytes of data 
 * @param       devAddr     7-bit peripheral device address
 * 
 * @sequence:               S,ADDRESS+W,a,COMMAND,a,BYTE COUNT = N,a,DATA BYTE 1,a,DATA BYTE 2,a,...,DATA BYTE N,a,P
 * 
 **/
smbus_err_t SMBusBlockWrite(smbus_handle_t handle, uint8_t devAddr, uint8_t command, uint8_t* dataSent, uint8_t dataLength);


/**
 * @brief                   send an 8-bit command followed by 8-bit datalength then receive [datalength] consecutive bytes of data   
 * @param       devAddr     7-bit peripheral device address
 * 
 * @sequence:               S,ADDRESS+W,a,COMMAND,a,Sr,ADDRESS+R,a,byte count = n,A,data byte 1,A,data byte 2,A,...,data byte n,N,P
 *
 **/
smbus_err_t SMBusBlockRead(smbus_handle_t handle, uint8_t devAddr, uint8_t command, uint8_t* dataRecv, uint8_t *dataLength);

/**
 * @brief                   send an 8-bit command followed by 8-bit sendlength then [sendlength] consecutive bytes of data
 *                          then receive 8-bit recvlength followed by [recvlength] consecutive bytes of data
 * @param       devAddr     7-bit peripheral device address
 * 
 * @sequence:               S,ADDRESS+W,a,COMMAND,a,BYTE COUNT = N,a,DATA BYTE 1,a,DATA BYTE 2,a,...,DATA BYTE N,a,
 *                          Sr,ADDRESS+R,a,byte count = n,A,data byte 1,A,data byte 2,A,...,data byte n,N,P
 * 
 **/
smbus_err_t SMBusBlockWriteBlockReadProcessCall(smbus_handle_t handle, uint8_t devAddr, uint8_t command,
                                                    uint8_t* dataSent, uint8_t dataSentLength, uint8_t* dataRecv, uint8_t* dataRecvLength);

/**
 * @brief                   send an 8-bit host address followed by 8-bit device address then 16-bit data to a host device on the bus.
 * @note                    The device executing this function must be in control of the clock line for the duration of this transmission.
 * @param       hostAddr    7-bit host address
 * @param       devAddr     7-bit peripheral device address
 * 
 * @sequence:               S,HOST ADDRESS+W,a,DEV ADDRESS,a,DATA LOW BYTE,a,DATA HIGH BYTE,a,P
 * 
 **/
smbus_err_t SMBusHostNotify(smbus_handle_t handle, uint8_t hostAddr, uint8_t devAddr, uint16_t data);

/**
 * @brief                   send an 8-bit command followed by 32-bit data 
 * @param       devAddr     7-bit peripheral device address
 * 
 * @sequence:               S,ADDRESS+W,a,COMMAND,a,DATA BYTE[7:0],a,DATA BYTE[15:8],a,DATA BYTE[23:16],a,DATA BYTE[31:24],a,P
 * 
 **/
smbus_err_t SMBusWrite32(smbus_handle_t handle, uint8_t devAddr, uint8_t command, uint32_t dataSent);

/**
 * @brief                   send an 8-bit command then receive 32-bit data
 * @param       devAddr     7-bit peripheral device address
 *
 * @sequence:               S,ADDRESS+W,a,COMMAND,a,Sr,ADDRESS+R,a,data byte[7:0],A,data byte[15:8],A,data byte[23:16],A,data byte[31:24],N,P
 *
 **/
smbus_err_t SMBusRead32(smbus_handle_t handle, uint8_t devAddr, uint8_t command, uint32_t *dataRecv);

/**
 * @brief                   send an 8-bit command followed by 64-bit data
 * @param       devAddr     7-bit peripheral device address
 *
 * @sequence:               S,ADDRESS+W,a,COMMAND,a,DATA BYTE[7:0],a,DATA BYTE[15:8],a,DATA BYTE[23:16],a,DATA BYTE[31:24],a,
 *                                                  DATA BYTE[39:32],a,DATA BYTE[47:40],a,DATA BYTE[55:48],a,DATA BYTE[63:56],a,P
 *
 **/
smbus_err_t SMBusWrite64(smbus_handle_t handle, uint8_t devAddr, uint8_t command, uint64_t dataSent);

/**
 * @brief                   send an 8-bit command then receive 64-bit data
 * @param       devAddr     7-bit peripheral device address
 *
 * @sequence:               S,ADDRESS+W,a,COMMAND,a,Sr,ADDRESS+R,a,data byte[7:0],A,data byte[15:8],A,data byte[23:16],A,data byte[31:24],A,
 *                                                              data byte[39:32],A,data byte[47:40],A,data byte[55:48],A,data byte[63:56],N,P
 *
 * */
smbus_err_t SMBusRead64(smbus_handle_t handle, uint8_t devAddr, uint8_t command, uint64_t *dataRecv);

/**
 * @brief                   send an 8-bit command then receive 16-bit data using BlockWrite() protocol
 * @param       devAddr     7-bit peripheral device address
 *
 * @sequence:               S,ADDRESS+W,a,COMMAND,a,Sr,ADDRESS+R,a,data byte[7:0],A,data byte[15:8],N,P
 *
 **/
smbus_err_t SMBusWrite16Block(smbus_handle_t handle, uint8_t devAddr, uint8_t command, uint16_t dataSent);

/**
 * @brief                   send an 8-bit command then receive 16-bit data using ReadBlock() protocol
 * @param       devAddr     7-bit peripheral device address
 *
 * @sequence:               S,ADDRESS+W,a,COMMAND,a,Sr,ADDRESS+R,a,data byte[7:0],A,data byte[15:8],N,P
 *
 **/
smbus_err_t SMBusRead16Block(smbus_handle_t handle, uint8_t devAddr, uint8_t command, uint16_t *dataRecv);

/**
 * @brief                   send an 8-bit command then receive 32-bit data using BlockWrite() protocol
 * @param       devAddr     7-bit peripheral device address
 *
 * @sequence:               S,ADDRESS+W,a,COMMAND,a,Sr,ADDRESS+R,a,data byte[7:0],A,data byte[15:8],A,data byte[23:16],A,data byte[31:24],N,P
 *
 **/
smbus_err_t SMBusWrite32Block(smbus_handle_t handle, uint8_t devAddr, uint8_t command, uint32_t dataSent);

/**
 * @brief                   send an 8-bit command then receive 32-bit data using ReadBlock() protocol
 * @param       devAddr     7-bit peripheral device address
 *
 * @sequence:               S,ADDRESS+W,a,COMMAND,a,Sr,ADDRESS+R,a,data byte[7:0],A,data byte[15:8],A,data byte[23:16],A,data byte[31:24],N,P
 *
 **/
smbus_err_t SMBusRead32Block(smbus_handle_t handle, uint8_t devAddr, uint8_t command, uint32_t *dataRecv);

/**
 * @brief                   send an 8-bit command followed by 64-bit data using WriteBlock() protocol
 * @param       devAddr     7-bit peripheral device address
 *
 * @sequence:               S,ADDRESS+W,a,COMMAND,a,DATA BYTE[7:0],a,DATA BYTE[15:8],a,DATA BYTE[23:16],a,DATA BYTE[31:24],a,
 *                                                  DATA BYTE[39:32],a,DATA BYTE[47:40],a,DATA BYTE[55:48],a,DATA BYTE[63:56],a,P
 *
 **/
smbus_err_t SMBusWrite64Block(smbus_handle_t handle, uint8_t devAddr, uint8_t command, uint64_t dataSent);

/**
 * @brief                   send an 8-bit command then receive 64-bit data using ReadBlock() protocol
 * @param       devAddr     7-bit peripheral device address
 *
 * @sequence:               S,ADDRESS+W,a,COMMAND,a,Sr,ADDRESS+R,a,data byte[7:0],A,data byte[15:8],A,data byte[23:16],A,data byte[31:24],A,
 *                                                              data byte[39:32],A,data byte[47:40],A,data byte[55:48],A,data byte[63:56],N,P
 *
 * */
smbus_err_t SMBusRead64Block(smbus_handle_t handle, uint8_t devAddr, uint8_t command, uint64_t *dataRecv);

/**
 * @brief                   send [datalength] consecutive bytes of data using standard I2C
 * @param       devAddr     7-bit peripheral device address
 *
 * @sequence:               S,ADDRESS+W,a,DATA BYTE 1,a,DATA BYTE 2,a,...,DATA BYTE N,a,P
 *
 **/
smbus_err_t SMBusWriteRaw(smbus_handle_t handle, uint8_t devAddr, uint8_t *dataSent, uint8_t dataLength);

/**
 * @brief                   send an 8-bit command followed by an 16-bit word then receive a chunk of data using ReadBlock() protocol
 * @param   devAddr         7-bit peripheral device address
 * @param   responseCommand Command used for the ReadBlock() to receive a chunk of data
 * @param   dataLength      If the function was successfull, contains the number of bytes received with ReadBlock()
 * @param   delayMs         Millosecond delay between the WriteWord() and the ReadBlock()
 *
 * */
smbus_err_t SMBusWriteWordReadBlock(smbus_handle_t handle, uint8_t devAddr, uint8_t command, uint16_t word, bool wordFlipEndianness,
                                       uint8_t responseCommand, uint8_t *dataRecv, uint8_t *dataLength, int delayMs);

/**
 * @brief                   send an 8-bit command followed by an 16-bit word then a chunk of data using ReadBlock() protocol
 *
 * @param   devAddr         7-bit peripheral device address
 * @param   responseCommand Command used for the WriteBlock() to send a chunk of data
 * @param   dataLength      Number of bytes to be sent with WriteBlock()
 * @param   delayMs         Millosecond delay between the WriteWord() and the WriteBlock()
 * */
smbus_err_t SMBusWriteWordWriteBlock(smbus_handle_t handle, uint8_t devAddr, uint8_t command, uint16_t word, bool wordFlipEndianness,
                                       uint8_t responseCommand, uint8_t *dataSent, uint8_t dataLength, int delayMs);

/**
 * @brief                   send an 8-bit command followed by an 16-bit word using WriteBlock() then receive a chunk of data using ReadBlock() protocol
 * @param   devAddr         7-bit peripheral device address
 * @param   responseCommand Command used for the ReadBlock() to receive a chunk of data
 * @param   dataLength      If the function was successfull, contains the number of bytes received with ReadBlock()
 * @param   delayMs         Millosecond delay between the WriteWordBlock() and the ReadBlock()
 * */
smbus_err_t SMBusWriteWordBlockReadBlock(smbus_handle_t handle, uint8_t devAddr, uint8_t command, uint16_t word,
                                            uint8_t responseCommand, uint8_t *dataRecv, uint8_t *dataLength, int delayMs);

void SMBusPlatformDelayMs(uint32_t delayMs);

#endif