/**
 * @file:   smbus_if.h  -   smbus data link layer implementation using AVR TWI - header file
 * @author: skuodi
 * @date:   
 * 
 * **/


#ifndef _SMBUS_IF_H
#define _SMBUS_IF_H 

#include "stdio.h"
#include "stdint.h"

/***********************************************SMBus STATUS Defines for AVR TWI *****************************************************/

#define SMBUS_STATUS_OK     0
#define SMBUS_STATUS_ERROR  1
#define SMBUS_STATUS_START_TRANSMITTED                     (1<<3)
#define SMBUS_STATUS_REPEATED_START_TRANSMITTED            (2<<3)
#define SMBUS_STATUS_ADDR_W_TRANSMITTED_ACK_RECIEVED       (3<<3)
#define SMBUS_STATUS_ADDR_W_TRANSMITTED_NACK_RECIEVED      (4<<3)
#define SMBUS_STATUS_DATA_TRANSMITTED_ACK_RECIEVED         (5<<3)
#define SMBUS_STATUS_DATA_TRANSMITTED_NACK_RECIEVED        (6<<3)
#define SMBUS_STATUS_ARBITRATION_LOST                      (7<<3)
#define SMBUS_STATUS_ADDR_R_TRANSMITTED_ACK_RECIEVED       (8<<3)
#define SMBUS_STATUS_ADDR_R_TRANSMITTED_NACK_RECIEVED      (9<<3)
#define SMBUS_STATUS_DATA_RECIEVED_ACK_TRANSMITTED         (10<<3)
#define SMBUS_STATUS_DATA_RECIEVED_NACK_TRANSMITTED        (11<<3)

/****************************************************SMBus Protocol Functions***************************************/
uint8_t SMBusInit(void);
uint8_t SMBusQuickCommand(uint8_t devAddr, uint8_t rwBit);
uint8_t SMBusQuickCommand_NonCompliant(uint8_t devAddr);
uint8_t SMBusSendByte(uint8_t devAddr, uint8_t data);
uint8_t SMBusReceiveByte(uint8_t devAddr, uint8_t* data);
uint8_t SMBusWriteByte(uint8_t devAddr, uint8_t command, uint8_t data);
uint8_t SMBusWriteWord(uint8_t devAddr, uint8_t command, uint16_t data);
uint8_t SMBusReadByte(uint8_t devAddr, uint8_t command, uint8_t* data);
uint8_t SMBusReadWord(uint8_t devAddr, uint8_t command, uint16_t* data);
uint8_t SMBusProcessCall(uint8_t devAddr, uint8_t command, uint16_t dataSent,   uint16_t* dataRecv);
uint8_t SMBusBlockWrite(uint8_t devAddr, uint8_t command, uint8_t* dataSent, uint8_t dataLength);
uint8_t SMBusBlockRead(uint8_t devAddr, uint8_t command, uint8_t* dataRecv);
uint8_t SMBusBlockWriteBlockReadProcessCall(uint8_t devAddr, uint8_t command, uint8_t* dataSent, uint8_t dataSentLength, uint8_t* dataRecv, uint8_t* dataRecvLength);
uint8_t SMBusWrite32(uint8_t devAddr, uint8_t command, uint32_t dataSent);
uint8_t SMBusRead32(uint8_t devAddr, uint8_t command, uint32_t* dataRecv);
uint8_t SMBusWrite64(uint8_t devAddr, uint8_t command, uint64_t dataSent);
uint8_t SMBusRead64(uint8_t devAddr, uint8_t command, uint64_t* dataRecv);

#endif