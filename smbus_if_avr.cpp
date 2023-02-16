/**
 * @file:   smbus_if.c  -   smbus data link layer protocol implementation using AVR TWI - source file 
 *                      -   Tested on ATMEGA32U4
 * @author: skuodi
 * @date:   
 * @brief:  the @sequence in each function description denotes the sequence of transaction protocol elements that make up the specific bus protocol
 *          Uppercase denotes an element sent from the host and lowercase denotes an element sent from the peripheral device
 *          e.g. ...,data byte,A,... means the peripheral device sent [data byte], which was then acknowledged by the host device
 *          e.g. ...,DATA BYTE,a,... means the host device sent [data byte], which was then acknowledged by the peripheral device
 * 
 * **/
#include "smbus_if.h"
#include "avr/io.h"

/****Implementation specific definitions - For AVR*****/

#define TWI_STATUS          (TWSR & 0xF8)
#define TWI_WAIT            while(!(TWCR & (1<<TWINT)))


/**
 * @brief:  SMBusInit   -   Initialize SMBus Peripheral
 * @param:  none
 * @retval: uint8_t     -   SMBUS_STATUS
 * */
uint8_t SMBusInit(void)
{

    //Enable internal pullup resistors
    MCUCR &= ~( 1 << PUD);

    //Set clock frequency = F / ( 16 + 2( TWBR ))*4^TWPS
    TWBR = 12;    //200KHz
    TWSR |=  1;

    //Enable TWI
    TWCR |= (1 << TWEN);

    return SMBUS_STATUS_OK;

}


/**
 * @brief:      SMBusQuickCommand   -   send SMBus Quick Command
 *                                  -   This function does not comply with the SMBus standard as it cannot
 *                                      transmit a read bit due to limitations in the way the TWI phy works
 * @param:      devAddr             -   7-bit peripheral device address, left shifted by 1
 * 
 * @retval:     uint8_t             -   SMBUS status
 * 
 * @sequence:   S,ADDRESS,W,a,P
 * 
 * **/
uint8_t SMBusQuickCommand_NonCompliant(uint8_t devAddr)
{
    //Send Start Condition
    TWCR = ( 1 << TWINT) | (1 << TWSTA) | (1 << TWEN);  //clear interrupt flag, set start bit, enable TWI
    TWI_WAIT;   //wait for start condition to be transmitted

    if (TWI_STATUS != SMBUS_STATUS_START_TRANSMITTED )  //error check
        return (TWI_STATUS >> 3);

    //Send Address + W
    devAddr &= ~(1);         //address + W bits
    TWDR = devAddr;         //place address and RW bits in data register
    TWCR = (1<<TWINT) | (1<<TWEN);//send data register contents
    TWI_WAIT;

    if (TWI_STATUS != SMBUS_STATUS_ADDR_W_TRANSMITTED_ACK_RECIEVED)   //error check
        return (TWI_STATUS >> 3);

    //Send Stop Condition
    TWCR = (1 << TWINT) | (1 << TWSTO) | (1 << TWEN);

    return SMBUS_STATUS_OK;
}


/**
 * 
 * @brief:      SMBusSendByte       -   send 8-bit value
 * @param:      devAddr             -   7-bit peripheral device address, left shifted by 1
 * @param:      data                -   data to be sent
 * 
 * @retval:     uint8_t             -   SMBUS status
 * 
 * @sequence:   S,ADDRESS,W,a,DATA BYTE,a,P 
 * 
 * */
uint8_t SMBusSendByte(uint8_t devAddr, uint8_t data)
{
    //Send Start Condition
    TWCR = ( 1 << TWINT) | (1 << TWSTA) | (1 << TWEN);  //clear interrupt flag, set start bit, enable TWI
    TWI_WAIT;   //wait for start condition to be transmitted

    if (TWI_STATUS != SMBUS_STATUS_START_TRANSMITTED )  //error check
        return (TWI_STATUS >> 3);

    //Send Address + W
    devAddr &= ~(1);         //address + W
    TWDR = devAddr;         //place address + W in data register
    TWCR = (1<<TWINT) | (1<<TWEN);                 //send data register contents
    TWI_WAIT;

    if (TWI_STATUS != SMBUS_STATUS_ADDR_W_TRANSMITTED_ACK_RECIEVED)   //error check
        return (TWI_STATUS >> 3);

    //Send Data
    TWDR = data;         //place data in data register
    TWCR = (1<<TWINT) | (1<<TWEN);                 //send data register contents
    TWI_WAIT;

    if (TWI_STATUS != SMBUS_STATUS_DATA_TRANSMITTED_ACK_RECIEVED)   //error check
        return (TWI_STATUS >> 3);

    //Send Stop Condition
    TWCR = (1 << TWINT) | (1 << TWSTO) | (1 << TWEN);

    return SMBUS_STATUS_OK;
}


/**
 * @brief:      SMBusReceiveByte    -   receive 8-bit data
 * @param:      devAddr             -   7-bit peripheral device address, left shifted by 1
 * @param:      data                -   pointer to hold received value
 * 
 * @retval:     uint8_t             -   SMBUS status
 * 
 * @sequence:   S,ADDRESS,R,a,data byte,N,P 
 * */
uint8_t SMBusReceiveByte(uint8_t devAddr, uint8_t* data)
{
    //Send Start Condition
    TWCR = ( 1 << TWINT) | (1 << TWSTA) | (1 << TWEN);  //clear interrupt flag, set start bit, enable TWI
    TWI_WAIT;   //wait for start condition to be transmitted

    if (TWI_STATUS != SMBUS_STATUS_START_TRANSMITTED )  //error check
        return (TWI_STATUS >> 3);

    //Send Address + R
    devAddr |= 1;           // address + R
    TWDR = devAddr;         //place address and RW bits in data register
    TWCR = (1<<TWINT) | (1<<TWEN);                 //send data register contents
    TWI_WAIT;

    if (TWI_STATUS != SMBUS_STATUS_ADDR_R_TRANSMITTED_ACK_RECIEVED)   //error check
        return (TWI_STATUS >> 3);

    //Receive Data
    TWCR = (1<<TWINT) | (1<<TWEN);//initiate data recepiton
    TWI_WAIT;   //wait for data to be received

    *data = TWDR;//retrieve data

    if (TWI_STATUS != SMBUS_STATUS_DATA_RECIEVED_NACK_TRANSMITTED)   //error check
        return (TWI_STATUS >> 3);

    //Send Stop Condition
    TWCR = (1 << TWINT) | (1 << TWSTO) | (1 << TWEN);

    return SMBUS_STATUS_OK;
}

/**
 * @brief:      SMBusWriteByte      -   send an 8-bit command followed by 8-bit data
 * @param:      devAddr             -   7-bit peripheral device address, left shifted by 1
 * @param:      command             -   8-bit command to be sent
 * @param:      data                -   8-bit data to be sent 
 * 
 * @retval:     uint8_t             -   SMBUS status
 * 
 * @sequence:   S,ADDRESS+W,a,COMMAND BYTE,a,DATA BYTE,a,P
 * */
uint8_t SMBusWriteByte(uint8_t devAddr, uint8_t command, uint8_t data)
{
    //Send Start Condition
    TWCR = ( 1 << TWINT) | (1 << TWSTA) | (1 << TWEN);  //clear interrupt flag, set start bit, enable TWI
    TWI_WAIT;   //wait for start condition to be transmitted

    if (TWI_STATUS != SMBUS_STATUS_START_TRANSMITTED )  //error check
        return (TWI_STATUS >> 3);

    //Send Address + W
    devAddr &= ~(1);         //address + W
    TWDR = devAddr;         //place address + W in data register
    TWCR = (1<<TWINT) | (1<<TWEN);                 //send data register contents
    TWI_WAIT;

    if (TWI_STATUS != SMBUS_STATUS_ADDR_W_TRANSMITTED_ACK_RECIEVED)   //error check
        return (TWI_STATUS >> 3);

    //Send command
    TWDR = command;         //place command in data register
    TWCR = (1<<TWINT) | (1<<TWEN); //send data register contents
    TWI_WAIT;

    if (TWI_STATUS != SMBUS_STATUS_DATA_TRANSMITTED_ACK_RECIEVED)   //error check
        return (TWI_STATUS >> 3);

    //Send Data
    TWDR = data;         //place data in data register
    TWCR = (1<<TWINT) | (1<<TWEN);                 //send data register contents
    TWI_WAIT;

    if (TWI_STATUS != SMBUS_STATUS_DATA_TRANSMITTED_ACK_RECIEVED)   //error check
        return (TWI_STATUS >> 3);

    //Send Stop Condition
    TWCR = (1 << TWINT) | (1 << TWSTO) | (1 << TWEN);

    return SMBUS_STATUS_OK;
}

/**
 * @brief:      SMBusWriteWord  -   send an 8-bit command along with 16-bit data
 * @param:      devAddr             -   7-bit peripheral device address, left shifted by 1
 * @param:      command             -   8-bit command to be sent
 * @param:      data                -   16-bit data to be sent 
 * 
 * @retval:     uint8_t             -   SMBUS status
 * 
 * @sequence:   S,ADDRESS+W,a,COMMAND BYTE,a,DATA LOW BYTE,a,DATA HIGH BYTE,a,P
 * */
uint8_t SMBusWriteWord(uint8_t devAddr, uint8_t command, uint16_t data)
{
    //Send Start Condition
    TWCR = ( 1 << TWINT) | (1 << TWSTA) | (1 << TWEN);  //clear interrupt flag, set start bit, enable TWI
    TWI_WAIT;   //wait for start condition to be transmitted

    if (TWI_STATUS != SMBUS_STATUS_START_TRANSMITTED )  //error check
        return (TWI_STATUS >> 3);

    //Send Address + W
    devAddr &= ~(1);         //address + W
    TWDR = devAddr;         //place address + W in data register
    TWCR = (1<<TWINT) | (1<<TWEN);                 //send data register contents
    TWI_WAIT;

    if (TWI_STATUS != SMBUS_STATUS_ADDR_W_TRANSMITTED_ACK_RECIEVED)   //error check
        return (TWI_STATUS >> 3);

    //Send command
    TWDR = command;         //place command in data register
    TWCR = (1<<TWINT) | (1<<TWEN); //send data register contents
    TWI_WAIT;

    if (TWI_STATUS != SMBUS_STATUS_DATA_TRANSMITTED_ACK_RECIEVED)   //error check
        return (TWI_STATUS >> 3);

    //Send Data low byte
    TWDR = (data & 0xFF);         //place data low byte in data register
    TWCR = (1<<TWINT) | (1<<TWEN);                 //send data register contents
    TWI_WAIT;

    if (TWI_STATUS != SMBUS_STATUS_DATA_TRANSMITTED_ACK_RECIEVED)   //error check
        return (TWI_STATUS >> 3);

    //Send Data high byte
    TWDR = (data >> 8);         //place data in data register
    TWCR = (1<<TWINT) | (1<<TWEN);                 //send data register contents
    TWI_WAIT;

    if (TWI_STATUS != SMBUS_STATUS_DATA_TRANSMITTED_ACK_RECIEVED)   //error check
        return (TWI_STATUS >> 3);

    //Send Stop Condition
    TWCR = (1 << TWINT) | (1 << TWSTO) | (1 << TWEN);

    return SMBUS_STATUS_OK;

}

/**
 * @brief:      SMBusReadByte       -   send an 8-bit command then receive a 8-bit data
 * @param:      devAddr             -   7-bit peripheral device address, left shifted by 1
 * @param:      command             -   8-bit command to be sent
 * @param:      data                -   pointer to hold received 8-bit data  
 * 
 * @retval:     uint8_t             -   returns 0 if the transaction was acknowledged
 *                                  -   returns -1 if the transaction was not acknowledged
 * 
 * @sequence:   S,ADDRESS+W,a,COMMAND BYTE,a,Sr,ADDRESS+R,a,data byte,N,P
 * */
uint8_t SMBusReadByte(uint8_t devAddr, uint8_t command, uint8_t* data)
{
    
    //Send Start Condition
    TWCR = ( 1 << TWINT) | (1 << TWSTA) | (1 << TWEN);  //clear interrupt flag, set start bit, enable TWI
    TWI_WAIT;   //wait for start condition to be transmitted

    if (TWI_STATUS != SMBUS_STATUS_START_TRANSMITTED )  //error check
        return (TWI_STATUS >> 3);

    //Send Address + W
    devAddr &= ~(1);         //address + W
    TWDR = devAddr;         //place address + W in data register
    TWCR = (1<<TWINT) | (1<<TWEN);                 //send data register contents
    TWI_WAIT;

    if (TWI_STATUS != SMBUS_STATUS_ADDR_W_TRANSMITTED_ACK_RECIEVED)   //error check
        return (TWI_STATUS >> 3);

    //Send command
    TWDR = command;         //place command in data register
    TWCR = (1<<TWINT) | (1<<TWEN); //send data register contents
    TWI_WAIT;

    if (TWI_STATUS != SMBUS_STATUS_DATA_TRANSMITTED_ACK_RECIEVED)   //error check
        return (TWI_STATUS >> 3);

    //Send Repeated Start Condition
    TWCR = ( 1 << TWINT) | (1 << TWSTA) | (1 << TWEN);  //clear interrupt flag, set start bit, enable TWI
    TWI_WAIT;   //wait for repeated start condition to be transmitted
    TWI_WAIT;

    if (TWI_STATUS != SMBUS_STATUS_REPEATED_START_TRANSMITTED )  //error check
        return (TWI_STATUS >> 3);

    //Send Address + R
    devAddr |= 1;           // address + R
    TWDR = devAddr;         //place address and RW bits in data register
    TWCR = (1<<TWINT) | (1<<TWEN);                 //send data register contents
    TWI_WAIT;

    if (TWI_STATUS != SMBUS_STATUS_ADDR_R_TRANSMITTED_ACK_RECIEVED)   //error check
        return (TWI_STATUS >> 3);

    //Receive Data
    TWCR = (1<<TWINT) | (1<<TWEN);//initiate data recepiton
    TWI_WAIT;   //wait for data to be received

    *data = TWDR;//retrieve data

    if (TWI_STATUS != SMBUS_STATUS_DATA_RECIEVED_NACK_TRANSMITTED)   //error check
        return (TWI_STATUS >> 3);

    //Send Stop Condition
    TWCR = (1 << TWINT) | (1 << TWSTO) | (1 << TWEN);

    return SMBUS_STATUS_OK;
}

/**
 * @brief:      SMBusReadWord       -   send an 8-bit command then receive 16-bit data
 * @param:      devAddr             -   7-bit peripheral device address, left shifted by 1
 * @param:      command             -   8-bit command to be sent
 * @param:      data                -   pointer to hold received 16-bit data  
 * 
 * @retval:     uint8_t             -   returns 0 if the transaction was acknowledged
 *                                  -   returns -1 if the transaction was not acknowledged
 * 
 * @sequence:   S,ADDRESS+W,a,COMMAND BYTE,a,Sr,ADDRESS+R,a,data low byte,A,data high byte,N,P
 * */
uint8_t SMBusReadWord(uint8_t devAddr, uint8_t command, uint16_t* data)
{ 
    //Send Start Condition
    TWCR = ( 1 << TWINT) | (1 << TWSTA) | (1 << TWEN);  //clear interrupt flag, set start bit, enable TWI
    TWI_WAIT;   //wait for start condition to be transmitted

    if (TWI_STATUS != SMBUS_STATUS_START_TRANSMITTED )  //error check
        return (TWI_STATUS >> 3);

    //Send Address + W
    devAddr &= ~(1);         //address + W
    TWDR = devAddr;         //place address + W in data register
    TWCR = (1<<TWINT) | (1<<TWEN);                 //send data register contents
    TWI_WAIT;   //wait for address + W to be transmitted

    if (TWI_STATUS != SMBUS_STATUS_ADDR_W_TRANSMITTED_ACK_RECIEVED)   //error check
        return (TWI_STATUS >> 3);

    //Send command
    TWDR = command;         //place command in data register
    TWCR = (1<<TWINT) | (1<<TWEN); //send data register contents
    TWI_WAIT;   //wait for command to be transmitted

    if (TWI_STATUS != SMBUS_STATUS_DATA_TRANSMITTED_ACK_RECIEVED)   //error check
        return (TWI_STATUS >> 3);

    //Send Repeated Start Condition
    TWCR = ( 1 << TWINT) | (1 << TWSTA) | (1 << TWEN);  //clear interrupt flag, set start bit, enable TWI
    TWI_WAIT;   //wait for repeated start condition to be transmitted

    if (TWI_STATUS != SMBUS_STATUS_REPEATED_START_TRANSMITTED )  //error check
        return (TWI_STATUS >> 3);

    //Send Address + R
    devAddr |= 1;           // address + R
    TWDR = devAddr;         //place address and RW bits in data register
    TWCR = (1<<TWINT) | (1<<TWEN);                 //send data register contents
    TWI_WAIT;   //wait for address + R to be transmitted

    if (TWI_STATUS != SMBUS_STATUS_ADDR_R_TRANSMITTED_ACK_RECIEVED)   //error check
        return (TWI_STATUS >> 3);

    //Receive Data low byte
    TWCR = (1<<TWINT) | (1<<TWEN) | (1<<TWEA);//initiate data recepiton
    TWI_WAIT;   //wait for data to be received

    *data = TWDR;//retrieve data low byte

    if (TWI_STATUS != SMBUS_STATUS_DATA_RECIEVED_ACK_TRANSMITTED)   //error check
        return (TWI_STATUS >> 3);

    //Receive Data high byte
    TWCR = (1<<TWINT) | (1<<TWEN);//initiate data recepiton
    TWI_WAIT;   //wait for data high byte to be received

    *data |= (TWDR << 8); //retrieve data high byte

    if (TWI_STATUS != SMBUS_STATUS_DATA_RECIEVED_NACK_TRANSMITTED)   //error check
        return (TWI_STATUS >> 3);

    //Send Stop Condition
    TWCR = (1 << TWINT) | (1 << TWSTO) | (1 << TWEN);

    return SMBUS_STATUS_OK;
}

/**
 * @brief:      SMBusProcessCall    -   send an 8-bit command followed by 16-bit data then receive 16-bit data
 * @param:      devAddr             -   7-bit peripheral device address, left shifted by 1
 * @param:      command             -   8-bit command to be sent
 * @param:      dataSent            -   16-bit data to be sent
 * @param:      dataRecv            -   pointer to hold received 16-bit data  
 * 
 * @retval:     uint8_t             -   returns 0 if the transaction was acknowledged
 *                                  -   returns -1 if the transaction was not acknowledged
 * 
 * @sequence:   S,ADDRESS+W,a,COMMAND,a,DATA LOW BYTE,a,DATA HIGH BYTE,a,Sr,ADDRESS+R,a,data low byte,A,data high byte,N,P
 * */
uint8_t SMBusProcessCall(uint8_t devAddr, uint8_t command, uint16_t dataSent,   uint16_t* dataRecv)
{

    //Send Start Condition
    TWCR = ( 1 << TWINT) | (1 << TWSTA) | (1 << TWEN);  //clear interrupt flag, set start bit, enable TWI
    TWI_WAIT;   //wait for start condition to be transmitted

    if (TWI_STATUS != SMBUS_STATUS_START_TRANSMITTED )  //error check
        return (TWI_STATUS >> 3);

    //Send Address + W
    devAddr &= ~(1);         //address + W
    TWDR = devAddr;         //place address + W in data register
    TWCR = (1<<TWINT) | (1<<TWEN);                 //send data register contents
    TWI_WAIT;

    if (TWI_STATUS != SMBUS_STATUS_ADDR_W_TRANSMITTED_ACK_RECIEVED)   //error check
        return (TWI_STATUS >> 3);

    //Send command
    TWDR = command;         //place command in data register
    TWCR = (1<<TWINT) | (1<<TWEN); //send data register contents
    TWI_WAIT;

    if (TWI_STATUS != SMBUS_STATUS_DATA_TRANSMITTED_ACK_RECIEVED)   //error check
        return (TWI_STATUS >> 3);

    //Send Data low byte
    TWDR = (dataSent & 0xFF);         //place data low byte in data register
    TWCR = (1<<TWINT) | (1<<TWEN);                 //send data register contents
    TWI_WAIT;

    if (TWI_STATUS != SMBUS_STATUS_DATA_TRANSMITTED_ACK_RECIEVED)   //error check
        return (TWI_STATUS >> 3);

    //Send Data high byte
    TWDR = (dataSent >> 8);         //place data in data register
    TWCR = (1<<TWINT) | (1<<TWEN);                 //send data register contents
    TWI_WAIT;

    if (TWI_STATUS != SMBUS_STATUS_DATA_TRANSMITTED_ACK_RECIEVED)   //error check
        return (TWI_STATUS >> 3);


    //Send Repeated Start Condition
    TWCR = ( 1 << TWINT) | (1 << TWSTA) | (1 << TWEN);  //clear interrupt flag, set start bit, enable TWI
    TWI_WAIT;   //wait for repeated start condition to be transmitted
    TWI_WAIT;

    if (TWI_STATUS != SMBUS_STATUS_REPEATED_START_TRANSMITTED )  //error check
        return (TWI_STATUS >> 3);

    //Send Address + R
    devAddr |= 1;           // address + R
    TWDR = devAddr;         //place address and RW bits in data register
    TWCR = (1<<TWINT) | (1<<TWEN);                 //send data register contents
    TWI_WAIT;

    if (TWI_STATUS != SMBUS_STATUS_ADDR_R_TRANSMITTED_ACK_RECIEVED)   //error check
        return (TWI_STATUS >> 3);

    //Receive Data low byte
    TWCR = (1<<TWINT) | (1<<TWEN)| (1<<TWEA);//initiate data recepiton with ACK
    TWI_WAIT;   //wait for data to be received

    *dataRecv = TWDR;//retrieve data low byte

    if (TWI_STATUS != SMBUS_STATUS_DATA_RECIEVED_ACK_TRANSMITTED)   //error check
        return (TWI_STATUS >> 3);

    //Receive Data high byte
    TWCR = (1<<TWINT) | (1<<TWEN);//initiate data recepiton
    TWI_WAIT;   //wait for data to be received

    *dataRecv = (TWDR << 8); //retrieve data high byte

    if (TWI_STATUS != SMBUS_STATUS_DATA_RECIEVED_NACK_TRANSMITTED)   //error check
        return (TWI_STATUS >> 3);

    //Send Stop Condition
    TWCR = (1 << TWINT) | (1 << TWSTO) | (1 << TWEN);

    return SMBUS_STATUS_OK;

}

/**
 * @brief:      SMBusBlockWrite     -   send an 8-bit command followed by [datalength] consecutive bytes of data 
 * @param:      command             -   8-bit command to be sent
 * @param:      dataSent            -   data to be sent as 8-bit bytes. The byte at index 0 is sent first
 * @param:      dataLength          -   number of bytes to send
 * 
 * @retval:     uint8_t             -   SMBUS status

 * @sequence:   S,ADDRESS+W,a,COMMAND,a,BYTE COUNT = N,a,DATA BYTE 1,a,DATA BYTE 2,a,...,DATA BYTE N,a,P
 * */
uint8_t SMBusBlockWrite(uint8_t devAddr, uint8_t command, uint8_t* dataSent, uint8_t dataLength)
{
    //Send Start Condition
    TWCR = ( 1 << TWINT) | (1 << TWSTA) | (1 << TWEN);  //clear interrupt flag, set start bit, enable TWI
    TWI_WAIT;   //wait for start condition to be transmitted

    if (TWI_STATUS != SMBUS_STATUS_START_TRANSMITTED )  //error check
        return (TWI_STATUS >> 3);

    //Send Address + W
    devAddr &= ~(1);         //address + W
    TWDR = devAddr;         //place address + W in data register
    TWCR = (1<<TWINT) | (1<<TWEN);                 //send data register contents
    TWI_WAIT;

    if (TWI_STATUS != SMBUS_STATUS_ADDR_W_TRANSMITTED_ACK_RECIEVED)   //error check
        return (TWI_STATUS >> 3);

    //Send command
    TWDR = command;         //place command in data register
    TWCR = (1<<TWINT) | (1<<TWEN); //send data register contents
    TWI_WAIT;

    if (TWI_STATUS != SMBUS_STATUS_DATA_TRANSMITTED_ACK_RECIEVED)   //error check
        return (TWI_STATUS >> 3);

    //Send Data length
    TWDR = dataLength;         //place data in data register
    TWCR = (1<<TWINT) | (1<<TWEN);                 //send data register contents
    TWI_WAIT;

    if (TWI_STATUS != SMBUS_STATUS_DATA_TRANSMITTED_ACK_RECIEVED)   //error check
        return (TWI_STATUS >> 3);

    for(int i = 0; i < dataLength; i++)
    {

        //Send Data 
        TWDR = dataSent[i];         //place data in data register
        TWCR = (1<<TWINT) | (1<<TWEN);                 //send data register contents
        TWI_WAIT;

        if (TWI_STATUS != SMBUS_STATUS_DATA_TRANSMITTED_ACK_RECIEVED)   //error check
            return (TWI_STATUS >> 3);

    }

    //Send Stop Condition
    TWCR = (1 << TWINT) | (1 << TWSTO) | (1 << TWEN);

    return SMBUS_STATUS_OK;
}

/**
 * @brief:      SMBusBlockRead      -   send an 8-bit command then receive [datalength] consecutive bytes of data   
 * @param:      devAddr             -   7-bit peripheral device address, left shifted by 1
 * @param:      command             -   8-bit command to be sent
 * @param:      dataRecv            -   pointer to hold received 8-bit bytes
 * 
 * @retval:     uint8_t             -   SMBUS status
 * 
 * @sequence:   S,ADDRESS+W,a,COMMAND,a,Sr,ADDRESS+R,a,byte count = n,A,data byte 1,A,data byte 2,A,...,data byte n,N,P
 * 
 * */
uint8_t SMBusBlockRead(uint8_t devAddr, uint8_t command, uint8_t* dataRecv)
{
    uint8_t dataLength = 0;
    //Send Start Condition
    TWCR = ( 1 << TWINT) | (1 << TWSTA) | (1 << TWEN);  //clear interrupt flag, set start bit, enable TWI
    TWI_WAIT;   //wait for start condition to be transmitted

    if (TWI_STATUS != SMBUS_STATUS_START_TRANSMITTED )  //error check
        return (TWI_STATUS >> 3);

    //Send Address + W
    devAddr &= ~(1);         //address + W
    TWDR = devAddr;         //place address + W in data register
    TWCR = (1<<TWINT) | (1<<TWEN);                 //send data register contents
    TWI_WAIT;   //wait for address + W to be transmitted

    if (TWI_STATUS != SMBUS_STATUS_ADDR_W_TRANSMITTED_ACK_RECIEVED)   //error check
        return (TWI_STATUS >> 3);

    //Send command
    TWDR = command;         //place command in data register
    TWCR = (1<<TWINT) | (1<<TWEN); //send data register contents
    TWI_WAIT;   //wait for command to be transmitted

    if (TWI_STATUS != SMBUS_STATUS_DATA_TRANSMITTED_ACK_RECIEVED)   //error check
        return (TWI_STATUS >> 3);

    //Send Repeated Start Condition
    TWCR = ( 1 << TWINT) | (1 << TWSTA) | (1 << TWEN);  //clear interrupt flag, set start bit, enable TWI
    TWI_WAIT;   //wait for repeated start condition to be transmitted

    if (TWI_STATUS != SMBUS_STATUS_REPEATED_START_TRANSMITTED )  //error check
        return (TWI_STATUS >> 3);

    //Send Address + R
    devAddr |= 1;           // address + R
    TWDR = devAddr;         //place address and RW bits in data register
    TWCR = (1<<TWINT) | (1<<TWEN);                 //send data register contents
    TWI_WAIT;   //wait for address + R to be transmitted

    if (TWI_STATUS != SMBUS_STATUS_ADDR_R_TRANSMITTED_ACK_RECIEVED)   //error check
        return (TWI_STATUS >> 3);

    //Receive Data
    TWCR = (1<<TWINT) | (1<<TWEN) | (1 << TWEA);//initiate data recepiton
    TWI_WAIT;   //wait for data to be received

    dataLength = TWDR;//retrieve data

    if (TWI_STATUS != SMBUS_STATUS_DATA_RECIEVED_ACK_TRANSMITTED)   //error check
        return (TWI_STATUS >> 3);

    for(int i = 0; i < dataLength; i++)
    {
        //Receive Data
        TWCR = (1<<TWINT) | (1<<TWEN) | (1 << TWEA);//initiate data recepiton
        if (i + 1 == dataLength)    //if this is the last byte being received
            TWCR &= ~(1 << TWEA);   //don't acknowledge

        TWI_WAIT;   //wait for data to be received

        dataRecv[i] = TWDR;//retrieve data

        if (TWI_STATUS != SMBUS_STATUS_DATA_RECIEVED_ACK_TRANSMITTED && (i + 1 != dataLength))   //error check
            return (TWI_STATUS >> 3);
    }

    //Send Stop Condition
    TWCR = (1 << TWINT) | (1 << TWSTO) | (1 << TWEN);

    return SMBUS_STATUS_OK;

}

/**
 * @brief:      SMBusBlockWrite     -   send an 8-bit command followed by [datalength] consecutive bytes of data then receive block data
 * @param:      command             -   8-bit command to be sent
 * @param:      dataSent            -   data to be sent as 8-bit bytes. The byte at index 0 is sent first
 * @param:      dataLength          -   number of bytes to send
 * @param:      dataRecv            -   will hold received data
 * 
 * @retval:     uint8_t             -   SMBUS status
 * 
 * @sequence:   S,ADDRESS+W,a,COMMAND,a,BYTE COUNT = N,a,DATA BYTE 1,a,DATA BYTE 2,a,...,DATA BYTE N,a,Sr,ADDRESS+R,a,byte count = n,A,data byte 1,A,data byte 2,A,...,data byte n,N,P
 * 
 * */
uint8_t SMBusBlockWriteBlockReadProcessCall(uint8_t devAddr, uint8_t command, uint8_t* dataSent, uint8_t dataSentLength, uint8_t* dataRecv, uint8_t* dataRecvLength)
{
    //Send Start Condition
    TWCR = ( 1 << TWINT) | (1 << TWSTA) | (1 << TWEN);  //clear interrupt flag, set start bit, enable TWI
    TWI_WAIT;   //wait for start condition to be transmitted

    if (TWI_STATUS != SMBUS_STATUS_START_TRANSMITTED )  //error check
        return (TWI_STATUS >> 3);

    //Send Address + W
    devAddr &= ~(1);         //address + W
    TWDR = devAddr;         //place address + W in data register
    TWCR = (1<<TWINT) | (1<<TWEN);                 //send data register contents
    TWI_WAIT;

    if (TWI_STATUS != SMBUS_STATUS_ADDR_W_TRANSMITTED_ACK_RECIEVED)   //error check
        return (TWI_STATUS >> 3);

    //Send command
    TWDR = command;         //place command in data register
    TWCR = (1<<TWINT) | (1<<TWEN); //send data register contents
    TWI_WAIT;

    if (TWI_STATUS != SMBUS_STATUS_DATA_TRANSMITTED_ACK_RECIEVED)   //error check
        return (TWI_STATUS >> 3);

    //Send Data length
    TWDR = dataSentLength;         //place data in data register
    TWCR = (1<<TWINT) | (1<<TWEN);                 //send data register contents
    TWI_WAIT;

    if (TWI_STATUS != SMBUS_STATUS_DATA_TRANSMITTED_ACK_RECIEVED)   //error check
        return (TWI_STATUS >> 3);

    for(int i = 0; i < dataSentLength; i++)
    {

        //Send Data 
        TWDR = dataSent[i];         //place data in data register
        TWCR = (1<<TWINT) | (1<<TWEN);                 //send data register contents
        TWI_WAIT;

        if (TWI_STATUS != SMBUS_STATUS_DATA_TRANSMITTED_ACK_RECIEVED)   //error check
            return (TWI_STATUS >> 3);

    }

    //Send Repeated Start Condition
    TWCR = ( 1 << TWINT) | (1 << TWSTA) | (1 << TWEN);  //clear interrupt flag, set start bit, enable TWI
    TWI_WAIT;   //wait for repeated start condition to be transmitted

    if (TWI_STATUS != SMBUS_STATUS_REPEATED_START_TRANSMITTED )  //error check
        return (TWI_STATUS >> 3);

    //Send Address + R
    devAddr |= 1;           // address + R
    TWDR = devAddr;         //place address and RW bits in data register
    TWCR = (1<<TWINT) | (1<<TWEN);                 //send data register contents
    TWI_WAIT;   //wait for address + R to be transmitted

    if (TWI_STATUS != SMBUS_STATUS_ADDR_R_TRANSMITTED_ACK_RECIEVED)   //error check
        return (TWI_STATUS >> 3);

    //Receive Data
    TWCR = (1<<TWINT) | (1<<TWEN) | (1 << TWEA);//initiate data recepiton
    TWI_WAIT;   //wait for data to be received

    *dataRecvLength = TWDR;//retrieve data

    if (TWI_STATUS != SMBUS_STATUS_DATA_RECIEVED_ACK_TRANSMITTED)   //error check
        return (TWI_STATUS >> 3);

    for(int i = 0; i < *dataRecvLength; i++)
    {
        //Receive Data
        TWCR = (1<<TWINT) | (1<<TWEN) | (1 << TWEA);//initiate data recepiton
        if (i + 1 == *dataRecvLength)   //if the current byte is the last being received
            TWCR &= ~(1 << TWEA);       //don't acknowledge

        TWI_WAIT;   //wait for data to be received

        dataRecv[i] = TWDR;//retrieve data

        if (TWI_STATUS != SMBUS_STATUS_DATA_RECIEVED_ACK_TRANSMITTED && (i + 1 != *dataRecvLength))   //error check
            return (TWI_STATUS >> 3);
    }

    //Send Stop Condition
    TWCR = (1 << TWINT) | (1 << TWSTO) | (1 << TWEN);

    return SMBUS_STATUS_OK;

}

/**
 * @brief:      SMBusHostNotify     -   send 16-bit data to a host device on the bus
 * @param:      hostAddr            -   7-bit host address left shifted by 1
 * @param:      devAddr             -   7-bit address of the device executing this function, left shifted by 1
 * @param:      data                -   16-bit data to be sent 
 * 
 * @retval:     uint8_t             -   SMBUS status
 * 
 * @sequence:   S,HOST ADDRESS+W,a,DEV ADDRESS,a,DATA LOW BYTE,a,DATA HIGH BYTE,a,P
 * */
uint8_t SMBusHostNotify(uint8_t hostAddr, uint8_t devAddr, uint16_t data)
{
    //Send Start Condition
    TWCR = ( 1 << TWINT) | (1 << TWSTA) | (1 << TWEN);  //clear interrupt flag, set start bit, enable TWI
    TWI_WAIT;   //wait for start condition to be transmitted

    if (TWI_STATUS != SMBUS_STATUS_START_TRANSMITTED )  //error check
        return (TWI_STATUS >> 3);

    //Send Address + W
    hostAddr &= ~(1);         //address + W
    TWDR = hostAddr;         //place address + W in data register
    TWCR = (1<<TWINT) | (1<<TWEN);                 //send data register contents
    TWI_WAIT;

    if (TWI_STATUS != SMBUS_STATUS_ADDR_W_TRANSMITTED_ACK_RECIEVED)   //error check
        return (TWI_STATUS >> 3);

    //Send device address
    TWDR = devAddr;         //place device address in data register
    TWCR = (1<<TWINT) | (1<<TWEN); //send data register contents
    TWI_WAIT;

    if (TWI_STATUS != SMBUS_STATUS_DATA_TRANSMITTED_ACK_RECIEVED)   //error check
        return (TWI_STATUS >> 3);

    //Send Data low byte
    TWDR = (data & 0xFF);         //place data low byte in data register
    TWCR = (1<<TWINT) | (1<<TWEN);                 //send data register contents
    TWI_WAIT;

    if (TWI_STATUS != SMBUS_STATUS_DATA_TRANSMITTED_ACK_RECIEVED)   //error check
        return (TWI_STATUS >> 3);

    //Send Data high byte
    TWDR = (data >> 8);         //place data in data register
    TWCR = (1<<TWINT) | (1<<TWEN);                 //send data register contents
    TWI_WAIT;

    if (TWI_STATUS != SMBUS_STATUS_DATA_TRANSMITTED_ACK_RECIEVED)   //error check
        return (TWI_STATUS >> 3);

    //Send Stop Condition
    TWCR = (1 << TWINT) | (1 << TWSTO) | (1 << TWEN);

    return SMBUS_STATUS_OK;

}


/**
 * @brief:      SMBusWrite32        -   send an 8-bit command followed by 32-bit data 
 * @param:      command             -   8-bit command to be sent
 * @param:      dataSent            -   32-bit data to be sent as 8-bit bytes.
 * 
 * @retval:     uint8_t             -   SMBUS status
 * 
 * @sequence:   S,ADDRESS+W,a,COMMAND,a,DATA BYTE[7:0],a,DATA BYTE[15:8],a,DATA BYTE[23:16],a,DATA BYTE[31:24],a,P
 * */
uint8_t SMBusWrite32(uint8_t devAddr, uint8_t command, uint32_t dataSent)
{
    //Send Start Condition
    TWCR = ( 1 << TWINT) | (1 << TWSTA) | (1 << TWEN);  //clear interrupt flag, set start bit, enable TWI
    TWI_WAIT;   //wait for start condition to be transmitted

    if (TWI_STATUS != SMBUS_STATUS_START_TRANSMITTED )  //error check
        return (TWI_STATUS >> 3);

    //Send Address + W
    devAddr &= ~(1);         //address + W
    TWDR = devAddr;         //place address + W in data register
    TWCR = (1<<TWINT) | (1<<TWEN);                 //send data register contents
    TWI_WAIT;

    if (TWI_STATUS != SMBUS_STATUS_ADDR_W_TRANSMITTED_ACK_RECIEVED)   //error check
        return (TWI_STATUS >> 3);

    //Send command
    TWDR = command;         //place command in data register
    TWCR = (1<<TWINT) | (1<<TWEN); //send data register contents
    TWI_WAIT;

    if (TWI_STATUS != SMBUS_STATUS_DATA_TRANSMITTED_ACK_RECIEVED)   //error check
        return (TWI_STATUS >> 3);

    for(int i = 0; i < 4; i++)
    {

        //Send Data 
        TWDR = (dataSent >> (i*8)) & 0xFF;         //place data in data register
        TWCR = (1<<TWINT) | (1<<TWEN);                 //send data register contents
        TWI_WAIT;

        if (TWI_STATUS != SMBUS_STATUS_DATA_TRANSMITTED_ACK_RECIEVED)   //error check
            return (TWI_STATUS >> 3);

    }

    //Send Stop Condition
    TWCR = (1 << TWINT) | (1 << TWSTO) | (1 << TWEN);

    return SMBUS_STATUS_OK;
}

/**
 * @brief:      SMBusRead32 -   send an 8-bit command then receive 32-bit data   
 * @param:      devAddr     -   7-bit peripheral device address, left shifted by 1
 * @param:      command     -   8-bit command to be sent
 * @param:      dataRecv    -   pointer to hold received 32-bit data
 * 
 * @retval:     uint8_t     -   SMBUS status
 * 
 * @sequence:   S,ADDRESS+W,a,COMMAND,a,Sr,ADDRESS+R,a,data byte[7:0],A,data byte[15:8],A,data byte[23:16],A,data byte[31:24],N,P
 * */
uint8_t SMBusRead32(uint8_t devAddr, uint8_t command, uint32_t* dataRecv)
{
    uint8_t dataLength = 0;
    uint8_t dataBuf[4];
    //Send Start Condition
    TWCR = ( 1 << TWINT) | (1 << TWSTA) | (1 << TWEN);  //clear interrupt flag, set start bit, enable TWI
    TWI_WAIT;   //wait for start condition to be transmitted

    if (TWI_STATUS != SMBUS_STATUS_START_TRANSMITTED )  //error check
        return (TWI_STATUS >> 3);

    //Send Address + W
    devAddr &= ~(1);         //address + W
    TWDR = devAddr;         //place address + W in data register
    TWCR = (1<<TWINT) | (1<<TWEN);                 //send data register contents
    TWI_WAIT;   //wait for address + W to be transmitted

    if (TWI_STATUS != SMBUS_STATUS_ADDR_W_TRANSMITTED_ACK_RECIEVED)   //error check
        return (TWI_STATUS >> 3);

    //Send command
    TWDR = command;         //place command in data register
    TWCR = (1<<TWINT) | (1<<TWEN); //send data register contents
    TWI_WAIT;   //wait for command to be transmitted

    if (TWI_STATUS != SMBUS_STATUS_DATA_TRANSMITTED_ACK_RECIEVED)   //error check
        return (TWI_STATUS >> 3);

    //Send Repeated Start Condition
    TWCR = ( 1 << TWINT) | (1 << TWSTA) | (1 << TWEN);  //clear interrupt flag, set start bit, enable TWI
    TWI_WAIT;   //wait for repeated start condition to be transmitted

    if (TWI_STATUS != SMBUS_STATUS_REPEATED_START_TRANSMITTED )  //error check
        return (TWI_STATUS >> 3);

    //Send Address + R
    devAddr |= 1;           // address + R
    TWDR = devAddr;         //place address and RW bits in data register
    TWCR = (1<<TWINT) | (1<<TWEN);                 //send data register contents
    TWI_WAIT;   //wait for address + R to be transmitted

    if (TWI_STATUS != SMBUS_STATUS_ADDR_R_TRANSMITTED_ACK_RECIEVED)   //error check
        return (TWI_STATUS >> 3);

    for(int i = 0; i < 4; i++)
    {
        //Receive Data
        TWCR = (1<<TWINT) | (1<<TWEN) | (1 << TWEA);//initiate data recepiton
        if (i + 1 == 4)
            TWCR &= ~(1 << TWEA);

        TWI_WAIT;   //wait for data to be received

        *dataRecv |= (TWDR << (8*i));//retrieve data

        if (TWI_STATUS != SMBUS_STATUS_DATA_RECIEVED_ACK_TRANSMITTED && (i + 1 != 4))   //error check
            return (TWI_STATUS >> 3);

    }

    //Send Stop Condition
    TWCR = (1 << TWINT) | (1 << TWSTO) | (1 << TWEN);

    return SMBUS_STATUS_OK;

}


/**
 * @brief:      SMBusWrite64        -   send an 8-bit command followed by 64-bit data 
 * @param:      command             -   8-bit command to be sent
 * @param:      dataSent            -   64-bit data to be sent as 8-bit bytes.
 * 
 * @retval:     uint8_t             -   SMBUS status
 * 
 * @sequence:   S,ADDRESS+W,a,COMMAND,a,DATA BYTE[7:0],a,DATA BYTE[15:8],a,DATA BYTE[23:16],a,DATA BYTE[31:24],a,DATA BYTE[39:32],a,DATA BYTE[47:40],a,DATA BYTE[55:48],a,DATA BYTE[63:56],a,P
 * */
uint8_t SMBusWrite64(uint8_t devAddr, uint8_t command, uint64_t dataSent)
{
    //Send Start Condition
    TWCR = ( 1 << TWINT) | (1 << TWSTA) | (1 << TWEN);  //clear interrupt flag, set start bit, enable TWI
    TWI_WAIT;   //wait for start condition to be transmitted

    if (TWI_STATUS != SMBUS_STATUS_START_TRANSMITTED )  //error check
        return (TWI_STATUS >> 3);

    //Send Address + W
    devAddr &= ~(1);         //address + W
    TWDR = devAddr;         //place address + W in data register
    TWCR = (1<<TWINT) | (1<<TWEN);                 //send data register contents
    TWI_WAIT;

    if (TWI_STATUS != SMBUS_STATUS_ADDR_W_TRANSMITTED_ACK_RECIEVED)   //error check
        return (TWI_STATUS >> 3);

    //Send command
    TWDR = command;         //place command in data register
    TWCR = (1<<TWINT) | (1<<TWEN); //send data register contents
    TWI_WAIT;

    if (TWI_STATUS != SMBUS_STATUS_DATA_TRANSMITTED_ACK_RECIEVED)   //error check
        return (TWI_STATUS >> 3);

    for(int i = 0; i < 8; i++)
    {

        //Send Data 
        TWDR = (dataSent >> (i*8)) & 0xFF;         //place data in data register
        TWCR = (1<<TWINT) | (1<<TWEN);                 //send data register contents
        TWI_WAIT;

        if (TWI_STATUS != SMBUS_STATUS_DATA_TRANSMITTED_ACK_RECIEVED)   //error check
            return (TWI_STATUS >> 3);

    }

    //Send Stop Condition
    TWCR = (1 << TWINT) | (1 << TWSTO) | (1 << TWEN);

    return SMBUS_STATUS_OK;
}


/**
 * @brief:      SMBusRead64 -   send an 8-bit command then receive 64-bit data   
 * @param:      devAddr     -   7-bit peripheral device address, left shifted by 1
 * @param:      command     -   8-bit command to be sent
 * @param:      dataRecv    -   pointer to hold received 64-bit data
 * 
 * @retval:     uint8_t     -   SMBUS status
 * 
 * @sequence:   S,ADDRESS+W,a,COMMAND,a,Sr,ADDRESS+R,a,data byte[7:0],A,data byte[15:8],A,data byte[23:16],A,data byte[31:24],A,data byte[39:32],A,data byte[47:40],A,data byte[55:48],A,data byte[63:56],N,P
 * 
 * */
uint8_t SMBusRead64(uint8_t devAddr, uint8_t command, uint32_t* dataRecv)
{
    uint8_t dataLength = 0;
    uint8_t dataBuf[8];

    //Send Start Condition
    TWCR = ( 1 << TWINT) | (1 << TWSTA) | (1 << TWEN);  //clear interrupt flag, set start bit, enable TWI
    TWI_WAIT;   //wait for start condition to be transmitted

    if (TWI_STATUS != SMBUS_STATUS_START_TRANSMITTED )  //error check
        return (TWI_STATUS >> 3);

    //Send Address + W
    devAddr &= ~(1);         //address + W
    TWDR = devAddr;         //place address + W in data register
    TWCR = (1<<TWINT) | (1<<TWEN);                 //send data register contents
    TWI_WAIT;   //wait for address + W to be transmitted

    if (TWI_STATUS != SMBUS_STATUS_ADDR_W_TRANSMITTED_ACK_RECIEVED)   //error check
        return (TWI_STATUS >> 3);

    //Send command
    TWDR = command;         //place command in data register
    TWCR = (1<<TWINT) | (1<<TWEN); //send data register contents
    TWI_WAIT;   //wait for command to be transmitted

    if (TWI_STATUS != SMBUS_STATUS_DATA_TRANSMITTED_ACK_RECIEVED)   //error check
        return (TWI_STATUS >> 3);

    //Send Repeated Start Condition
    TWCR = ( 1 << TWINT) | (1 << TWSTA) | (1 << TWEN);  //clear interrupt flag, set start bit, enable TWI
    TWI_WAIT;   //wait for repeated start condition to be transmitted

    if (TWI_STATUS != SMBUS_STATUS_REPEATED_START_TRANSMITTED )  //error check
        return (TWI_STATUS >> 3);

    //Send Address + R
    devAddr |= 1;           // address + R
    TWDR = devAddr;         //place address and RW bits in data register
    TWCR = (1<<TWINT) | (1<<TWEN);                 //send data register contents
    TWI_WAIT;   //wait for address + R to be transmitted

    if (TWI_STATUS != SMBUS_STATUS_ADDR_R_TRANSMITTED_ACK_RECIEVED)   //error check
        return (TWI_STATUS >> 3);

    for(int i = 0; i < 8; i++)
    {
        //Receive Data
        TWCR = (1<<TWINT) | (1<<TWEN) | (1 << TWEA);//initiate data recepiton
        if (i + 1 == 8)
            TWCR &= ~(1 << TWEA);

        TWI_WAIT;   //wait for data to be received
        
        *dataRecv |= (TWDR << (8*i));//retrieve data

        if (TWI_STATUS != SMBUS_STATUS_DATA_RECIEVED_ACK_TRANSMITTED && (i + 1 != 8))   //error check
            return (TWI_STATUS >> 3);

    }

    //Send Stop Condition
    TWCR = (1 << TWINT) | (1 << TWSTO) | (1 << TWEN);

    return SMBUS_STATUS_OK;

}
