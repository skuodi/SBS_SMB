/**
 * @file    smbus_espidf.c  -   SMBUS data link layer protocol bus controller implementation using ESP32 I2C
 *                          -   Tested on ESP32S3
 * @author  skuodi
 * @date    23 April 2024.
 * 
 * **/
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "esp_err.h"
#include "driver/i2c.h"
#include "driver/gpio.h"
#include "smbus_platform.h"

#define I2C_RW_READ     0x01                    ///< I2C RW bit read mode
#define I2C_RW_WRITE    0x00                    ///< I2C RW bit write mode
#define I2C_CHECK_ACK   0x01                    ///< I2C controller will check ack from target
#define I2C_NO_ACK      0x00                    ///< I2C controller will not check ack from target
#define I2C_SEND_ACK    I2C_MASTER_ACK          ///< I2C ack value
#define I2C_SEND_NACK   I2C_MASTER_NACK         ///< I2C nack value
#define I2C_LAST_NACK   I2C_MASTER_LAST_NACK    ///< I2C nack value

#define PLATFORM_MAX_I2C_SPEED 800000

#define CRC_INIT_VALUE  0x00

struct smbus_handle
{
    int i2cPort;    // Implementation specific handle to an I2C peripheral
    smbus_info_t info;
};

static uint8_t SMBusCrc8(uint8_t crc8, uint8_t const *data, uint16_t dataLength);
                   
smbus_handle_t SMBusInit(void* i2cPort, int8_t myAddress, uint32_t i2cSpeed, int sdaPin,
                         int sclPin, int intPin, long timeoutMs, bool usePec)
{
    int port = (int)i2cPort;
    
    if (port >= SOC_I2C_NUM || i2cSpeed > PLATFORM_MAX_I2C_SPEED)
        return NULL;
    
    i2c_config_t config = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = sdaPin,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_io_num = sclPin,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = i2cSpeed,
        .clk_flags = 0,
    };

    if (i2c_param_config(port, &config) != ESP_OK)
        return NULL;

    if (i2c_driver_install(port, I2C_MODE_MASTER, 0, 0, 0) != ESP_OK)
        return NULL;

    /// Adjust the I2C clock duty cycle so that it is closer to 50%
    if (i2c_set_period(port, 250, 200) != ESP_OK)
    {
        i2c_driver_delete(port);
        return NULL;
    }

    struct smbus_handle* busHandle = (struct smbus_handle*) malloc(sizeof(struct smbus_handle));
    if(!busHandle)
    {
        i2c_driver_delete(port);
        return NULL;
    }

    busHandle->i2cPort = port;
    busHandle->info.myAddress = myAddress;
    busHandle->info.i2cSpeed = i2cSpeed;
    busHandle->info.sdaPin = sdaPin;
    busHandle->info.sclPin = sclPin;
    busHandle->info.intPin = intPin;
    busHandle->info.timeoutMs = (timeoutMs) ? timeoutMs : portMAX_DELAY;
    busHandle->info.usePEC = usePec;

    return busHandle;
}

smbus_status_t SMBusDeinit(smbus_handle_t handle)
{
    if(!handle)
        return SMBUS_ERR_INVALID_ARG;
    
    if(i2c_driver_delete(handle->i2cPort) != ESP_OK)
        return SMBUS_ERR_FAIL;

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
    info->timeoutMs = ((struct smbus_handle*)handle)->info.timeoutMs;
    
    return SMBUS_ERR_OK;
}

smbus_status_t SMBusQuickCommand(smbus_handle_t handle, uint8_t devAddr, bool readWriteBit)
{
    if (!handle)
        return SMBUS_ERR_INVALID_ARG;

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (devAddr << 1) | (readWriteBit & 1), I2C_CHECK_ACK);
    i2c_master_stop(cmd);
    int ret = i2c_master_cmd_begin(handle->i2cPort, cmd, pdMS_TO_TICKS(handle->info.timeoutMs));
    i2c_cmd_link_delete(cmd);

    return (ret == ESP_OK) ? SMBUS_ERR_OK : (ret == ESP_ERR_TIMEOUT) ? SMBUS_ERR_TIMEOUT : SMBUS_ERR_FAIL;
}

smbus_status_t SMBusSendByte(smbus_handle_t handle, uint8_t devAddr, uint8_t data)
{
    if (!handle)
        return SMBUS_ERR_INVALID_ARG;

    uint8_t sendBuff[] = {(devAddr << 1) | I2C_RW_WRITE, data, 0};
    sendBuff[sizeof(sendBuff) - 1] = SMBusCrc8(CRC_INIT_VALUE, sendBuff, sizeof(sendBuff) - 1);

    int ret = i2c_master_write_to_device(handle->i2cPort, devAddr, sendBuff,
                                         (handle->info.usePEC) ? sizeof(sendBuff) - 1 : sizeof(sendBuff) - 2, pdMS_TO_TICKS(handle->info.timeoutMs));

    return (ret == ESP_OK) ? SMBUS_ERR_OK : (ret == ESP_ERR_TIMEOUT) ? SMBUS_ERR_TIMEOUT : SMBUS_ERR_FAIL;
}

smbus_status_t SMBusReceiveByte(smbus_handle_t handle, uint8_t devAddr, uint8_t* data)
{
    if (!handle)
        return SMBUS_ERR_INVALID_ARG;

    uint8_t recvBuff[] = {(devAddr << 1) | I2C_RW_READ, 0, 0};

    int ret = i2c_master_read_from_device(handle->i2cPort, devAddr, &recvBuff[1], (handle->info.usePEC) ? 2 : 1,
                                          pdMS_TO_TICKS(handle->info.timeoutMs));

    if(ret == ESP_ERR_TIMEOUT)
        return SMBUS_ERR_TIMEOUT;
    else if(ret == ESP_OK)
    {
        if (handle->info.usePEC)
        {
            uint8_t refCrc = SMBusCrc8(CRC_INIT_VALUE, recvBuff, sizeof(recvBuff) - 1);
            if(refCrc != recvBuff[sizeof(recvBuff) - 1])
                return SMBUS_ERR_BAD_CRC;
        }
        return SMBUS_ERR_OK;
    }
    
    return SMBUS_ERR_FAIL;
}

smbus_status_t SMBusWriteByte(smbus_handle_t handle, uint8_t devAddr, uint8_t command, uint8_t data)
{
    if (!handle)
        return SMBUS_ERR_INVALID_ARG;

    uint8_t sendBuff[] = {(devAddr << 1) | I2C_RW_WRITE, command, data, 0};
    sendBuff[sizeof(sendBuff) - 1] = SMBusCrc8(CRC_INIT_VALUE, sendBuff, sizeof(sendBuff) - 1);

    int ret =  i2c_master_write_to_device(handle->i2cPort, devAddr, &sendBuff[1],
                                          (handle->info.usePEC)? sizeof(sendBuff) - 1 : sizeof(sendBuff) - 2, pdMS_TO_TICKS(handle->info.timeoutMs));

    return (ret == ESP_OK) ? SMBUS_ERR_OK : (ret == ESP_ERR_TIMEOUT) ? SMBUS_ERR_TIMEOUT : SMBUS_ERR_FAIL;
}

smbus_status_t SMBusWriteWord(smbus_handle_t handle, uint8_t devAddr, uint8_t command, uint16_t data)
{
    if (!handle)
        return SMBUS_ERR_INVALID_ARG;

    uint8_t sendBuff[] = {(devAddr << 1) | I2C_RW_WRITE, command, (data & 0xFF), (data >> 8), 0};
    sendBuff[sizeof(sendBuff) - 1] = SMBusCrc8(CRC_INIT_VALUE, sendBuff, sizeof(sendBuff) - 1);

    int ret =  i2c_master_write_to_device(handle->i2cPort, devAddr, &sendBuff[1], 
                                         (handle->info.usePEC) ? sizeof(sendBuff) - 1 : sizeof(sendBuff) - 2, pdMS_TO_TICKS(handle->info.timeoutMs));

    return (ret == ESP_OK) ? SMBUS_ERR_OK : (ret == ESP_ERR_TIMEOUT) ? SMBUS_ERR_TIMEOUT : SMBUS_ERR_FAIL;
}

smbus_status_t SMBusReadByte(smbus_handle_t handle, uint8_t devAddr, uint8_t command, uint8_t* data)
{
    if (!handle || !data)
        return SMBUS_ERR_INVALID_ARG;

    uint8_t recvBuff[] = {(devAddr << 1) | I2C_RW_WRITE, command, (devAddr << 1) | I2C_RW_READ, 0, 0};

    int ret = i2c_master_write_read_device(handle->i2cPort, devAddr, &command, 1, &recvBuff[3],
                                          (handle->info.usePEC) ? 2 : 1, pdMS_TO_TICKS(handle->info.timeoutMs));

    if(ret == ESP_ERR_TIMEOUT)
        return SMBUS_ERR_TIMEOUT;
    else if(ret == ESP_OK)
    {
        if (handle->info.usePEC)
        {
            uint8_t refCrc = SMBusCrc8(CRC_INIT_VALUE, recvBuff, sizeof(recvBuff) - 1);
            if(refCrc != recvBuff[sizeof(recvBuff) - 1])
                return SMBUS_ERR_BAD_CRC;
        }
        *data = recvBuff[3];
        return SMBUS_ERR_OK;
    }
    
    return SMBUS_ERR_FAIL;
}

smbus_status_t SMBusReadWord(smbus_handle_t handle, uint8_t devAddr, uint8_t command, uint16_t* data)
{
    if(!handle || !data)
        return SMBUS_ERR_INVALID_ARG;

    uint8_t recvBuff[] = {(devAddr << 1) | I2C_RW_WRITE, command, (devAddr << 1) | I2C_RW_READ, 0, 0, 0};

    int ret = i2c_master_write_read_device( handle->i2cPort, devAddr, &command, 1, &recvBuff[3],
                                            (handle->info.usePEC) ? 3 : 2, pdMS_TO_TICKS(handle->info.timeoutMs));

    if(ret == ESP_ERR_TIMEOUT)
        return SMBUS_ERR_TIMEOUT;
    else if(ret == ESP_OK)
    {
        if (handle->info.usePEC)
        {
            uint8_t refCrc = SMBusCrc8(CRC_INIT_VALUE, recvBuff, sizeof(recvBuff) - 1);
            if(refCrc != recvBuff[sizeof(recvBuff) - 1])
                return SMBUS_ERR_BAD_CRC;
        }
    *data = ((uint16_t)recvBuff[4] << 8) | recvBuff[3];
        return SMBUS_ERR_OK;
    }
    
    return SMBUS_ERR_FAIL;
}

smbus_status_t SMBusProcessCall(smbus_handle_t handle, uint8_t devAddr, uint8_t command, uint16_t dataSent, uint16_t* dataRecv)
{
    if(!handle || !dataRecv)
        return SMBUS_ERR_INVALID_ARG;

    uint8_t recvBuff[] = {(devAddr << 1) | I2C_RW_WRITE, command, (dataSent & 0xFF), (dataSent >> 8), (devAddr << 1) | I2C_RW_READ, 0, 0, 0};

    int ret = i2c_master_write_read_device( handle->i2cPort, devAddr, &recvBuff[1], 3, &recvBuff[5],
                                            (handle->info.usePEC) ? 3 : 2, pdMS_TO_TICKS(handle->info.timeoutMs));

    if(ret == ESP_ERR_TIMEOUT)
        return SMBUS_ERR_TIMEOUT;
    else if(ret == ESP_OK)
    {
        if (handle->info.usePEC)
        {
            uint8_t refCrc = SMBusCrc8(CRC_INIT_VALUE, recvBuff, sizeof(recvBuff) - 1);
            if(refCrc != recvBuff[sizeof(recvBuff) - 1])
                return SMBUS_ERR_BAD_CRC;
        }
    *dataRecv = (recvBuff[6] << 8) | recvBuff[5];
        return SMBUS_ERR_OK;
    }
    
    return SMBUS_ERR_FAIL;
}

smbus_status_t SMBusBlockWrite(smbus_handle_t handle, uint8_t devAddr, uint8_t command, uint8_t* dataSent, uint8_t dataLength)
{
    if(!handle || !dataSent || !dataLength)
        return SMBUS_ERR_INVALID_ARG;

    uint8_t sendBuff[3 + dataLength + 1];
    sendBuff[0] = (devAddr << 1) | I2C_RW_WRITE;
    sendBuff[1] = command;
    sendBuff[2] = dataLength;
    memcpy(&sendBuff[3], dataSent, dataLength);
    sendBuff[sizeof(sendBuff) - 1] = SMBusCrc8(CRC_INIT_VALUE, sendBuff, sizeof(sendBuff) - 1);

    int ret = i2c_master_write_to_device(handle->i2cPort, devAddr, &sendBuff[1], (handle->info.usePEC) ? sizeof(sendBuff) - 1 : sizeof(sendBuff) - 2,
                                         pdMS_TO_TICKS(handle->info.timeoutMs));

    return (ret == ESP_OK) ? SMBUS_ERR_OK : (ret == ESP_ERR_TIMEOUT) ? SMBUS_ERR_TIMEOUT : SMBUS_ERR_FAIL;
}

smbus_status_t SMBusBlockRead(smbus_handle_t handle, uint8_t devAddr, uint8_t command, uint8_t* dataRecv, uint8_t *dataLength)
{   
    if(!handle || !dataRecv || !dataLength)
        return SMBUS_ERR_INVALID_ARG;

    uint8_t dataBuff[3 + 1 + 256 + 1];
    dataBuff[0] = (devAddr << 1) | I2C_RW_WRITE;
    dataBuff[1] = command;
    dataBuff[2] = (devAddr << 1) | I2C_RW_READ;

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_cmd_handle_t cmd2 = i2c_cmd_link_create();

    i2c_master_start(cmd);
    i2c_master_write(cmd, dataBuff, 2, I2C_CHECK_ACK);
    i2c_master_start(cmd);
    i2c_master_write(cmd, &dataBuff[2], 1, I2C_CHECK_ACK);
    i2c_master_read(cmd, &dataBuff[3], 1, I2C_SEND_ACK);
    int ret = i2c_master_cmd_begin(handle->i2cPort, cmd, pdMS_TO_TICKS(handle->info.timeoutMs));
    i2c_cmd_link_delete(cmd);

    if(ret != ESP_OK || !dataBuff[3])
        return (ret == ESP_ERR_TIMEOUT) ? SMBUS_ERR_TIMEOUT : SMBUS_ERR_FAIL;
    
    i2c_master_read(cmd2, &dataBuff[4], (handle->info.usePEC) ? dataBuff[3] + 1 : dataBuff[3] , I2C_LAST_NACK);
    i2c_master_stop(cmd2);
    ret = i2c_master_cmd_begin(handle->i2cPort, cmd2, pdMS_TO_TICKS(handle->info.timeoutMs));
    i2c_cmd_link_delete(cmd2);

    if(ret == ESP_ERR_TIMEOUT)
        return SMBUS_ERR_TIMEOUT;
    else if(ret == ESP_OK)
    {
        if (handle->info.usePEC)
        {
            uint8_t refCrc = SMBusCrc8(CRC_INIT_VALUE, dataBuff, 3 + 1 + dataBuff[3]);
            if (refCrc != dataBuff[3 + 1 + dataBuff[3]])
                return SMBUS_ERR_BAD_CRC;
        }
        memcpy(dataRecv, &dataBuff[4], dataBuff[3]);
        *dataLength = dataBuff[3];
        return SMBUS_ERR_OK;
    }

    return SMBUS_ERR_FAIL;
}

smbus_status_t SMBusBlockWriteBlockReadProcessCall(smbus_handle_t handle, uint8_t devAddr, uint8_t command, uint8_t* dataSent,
                                                    uint8_t dataSentLength, uint8_t* dataRecv, uint8_t* dataRecvLength)
{   
    if(!handle || !dataSent || !dataSentLength || !dataRecv || !dataRecvLength)
        return SMBUS_ERR_INVALID_ARG;

    // (add/w + cmd + [sendLength]) + dataToSend(dataSentLength bytes) + addr/r + [recvLength] + dataToReceive(max 256 bytes) + PEC
    uint8_t dataBuff[3 + dataSentLength + 1 + 1 + 256 + 1];
    uint8_t* buffP = dataBuff;
    *buffP++ = (devAddr << 1) | I2C_RW_WRITE;
    *buffP++ = command;
    *buffP++ = dataSentLength;
    memcpy(buffP, dataSent, dataSentLength);
    buffP += dataSentLength;
    *buffP++ = (devAddr << 1) | I2C_RW_READ;

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();

    i2c_master_start(cmd);
    i2c_master_write(cmd, dataBuff, 3 + dataSentLength, I2C_CHECK_ACK);
    i2c_master_start(cmd);
    i2c_master_write(cmd, &dataBuff[3 + dataSentLength], 1, I2C_CHECK_ACK);
    i2c_master_read (cmd, &dataBuff[3 + dataSentLength + 1], 1, I2C_SEND_ACK);
    int ret = i2c_master_cmd_begin(handle->i2cPort, cmd, pdMS_TO_TICKS(handle->info.timeoutMs));
    i2c_cmd_link_delete(cmd);

    uint8_t recvLen = dataBuff[3 + dataSentLength + 1];

    if(ret != ESP_OK || !recvLen)
        return (ret == ESP_ERR_TIMEOUT) ? SMBUS_ERR_TIMEOUT : SMBUS_ERR_FAIL;
    
    cmd = i2c_cmd_link_create();
    i2c_master_read(cmd, &dataBuff[3 + dataSentLength + 1 + 1], (handle->info.usePEC) ? recvLen + 1 : recvLen, I2C_LAST_NACK);
    i2c_master_stop(cmd);
    ret = i2c_master_cmd_begin(handle->i2cPort, cmd, pdMS_TO_TICKS(handle->info.timeoutMs));
    i2c_cmd_link_delete(cmd);

    if(ret == ESP_ERR_TIMEOUT)
        return SMBUS_ERR_TIMEOUT;
    else if(ret == ESP_OK)
    {
        if (handle->info.usePEC)
        {
            uint8_t refCrc = SMBusCrc8(CRC_INIT_VALUE, dataBuff, 3 + dataSentLength + 1 + recvLen);
            if(refCrc != dataBuff[sizeof(dataBuff) - 1])
                return SMBUS_ERR_BAD_CRC;
        }
        memcpy(dataRecv, &dataBuff[3 + dataSentLength + 1 + 1], recvLen);
        *dataRecvLength = recvLen;
        return SMBUS_ERR_OK;
    }
    
    return SMBUS_ERR_FAIL;
}

smbus_status_t SMBusHostNotify(smbus_handle_t handle, uint8_t hostAddr, uint8_t devAddr, uint16_t data)
{
    if (!handle)
        return SMBUS_ERR_INVALID_ARG;
    
    uint8_t sendBuff[] = { (devAddr << 1), data & 0xFF, data >> 8};
    int ret =  i2c_master_write_to_device(handle->i2cPort, hostAddr, sendBuff, 3, pdMS_TO_TICKS(handle->info.timeoutMs));
    return (ret == ESP_OK) ? SMBUS_ERR_OK : (ret == ESP_ERR_TIMEOUT) ? SMBUS_ERR_TIMEOUT : SMBUS_ERR_FAIL;
}

smbus_status_t SMBusWrite32(smbus_handle_t handle, uint8_t devAddr, uint8_t command, uint32_t dataSent)
{
    if(!handle)
        return SMBUS_ERR_INVALID_ARG;

    uint8_t sendBuff[] = {  (devAddr << 1) | I2C_RW_WRITE,
                            command,
                            (dataSent >>  0) & 0xFF,
                            (dataSent >>  8) & 0xFF,
                            (dataSent >> 16) & 0xFF,
                            (dataSent >> 24) & 0xFF,
                            0 };

    sendBuff[sizeof(sendBuff) - 1] = SMBusCrc8(CRC_INIT_VALUE, sendBuff, sizeof(sendBuff) - 1);

    int ret = i2c_master_write_to_device(handle->i2cPort, devAddr, &sendBuff[1], (handle->info.usePEC) ? sizeof(sendBuff) - 1 : sizeof(sendBuff) - 2,
                                         pdMS_TO_TICKS(handle->info.timeoutMs));

    
    return (ret == ESP_OK) ? SMBUS_ERR_OK : (ret == ESP_ERR_TIMEOUT) ? SMBUS_ERR_TIMEOUT : SMBUS_ERR_FAIL;
}

smbus_status_t SMBusRead32(smbus_handle_t handle, uint8_t devAddr, uint8_t command, uint32_t* dataRecv)
{
    if(!handle)
        return SMBUS_ERR_INVALID_ARG;

    uint8_t dataBuff[] = {(devAddr << 1) | I2C_RW_WRITE, command, (devAddr << 1) | I2C_RW_READ, 0, 0, 0, 0, 0};

    int ret = i2c_master_write_read_device(handle->i2cPort, devAddr, &command, 1, &dataBuff[3],
                                          (handle->info.usePEC) ? 5 : 4, pdMS_TO_TICKS(handle->info.timeoutMs));

    if(ret == ESP_ERR_TIMEOUT)
        return SMBUS_ERR_TIMEOUT;
    else if(ret == ESP_OK)
    {
        if (handle->info.usePEC)
        {
            uint8_t refCrc = SMBusCrc8(CRC_INIT_VALUE, dataBuff, sizeof(dataBuff) - 1);
            if(refCrc != dataBuff[sizeof(dataBuff) - 1])
                return SMBUS_ERR_BAD_CRC;
        }
        *dataRecv = ((uint32_t)dataBuff[3 + 3] << 24) |
                    ((uint32_t)dataBuff[3 + 2] << 16) |
                    ((uint32_t)dataBuff[3 + 1] <<  8) |
                    ((uint32_t)dataBuff[3 + 0] <<  0);
        return SMBUS_ERR_OK;
    }
    
    return SMBUS_ERR_FAIL;
}

smbus_status_t SMBusWrite64(smbus_handle_t handle, uint8_t devAddr, uint8_t command, uint64_t dataSent)
{
    if(!handle)
        return SMBUS_ERR_INVALID_ARG;

    uint8_t sendBuff[] = {  (devAddr << 1) | I2C_RW_WRITE,
                            command,
                            (dataSent >>  0) & 0xFF,
                            (dataSent >>  8) & 0xFF,
                            (dataSent >> 16) & 0xFF,
                            (dataSent >> 24) & 0xFF,
                            (dataSent >> 32) & 0xFF,
                            (dataSent >> 40) & 0xFF,
                            (dataSent >> 48) & 0xFF,
                            (dataSent >> 54) & 0xFF,
                            0};

    sendBuff[sizeof(sendBuff) - 1] = SMBusCrc8(CRC_INIT_VALUE, sendBuff, sizeof(sendBuff) - 1);

    int ret = i2c_master_write_to_device(handle->i2cPort, devAddr, &sendBuff[1],(handle->info.usePEC) ? sizeof(sendBuff) - 1 : sizeof(sendBuff) - 2,
                                         pdMS_TO_TICKS(handle->info.timeoutMs));

    
    return (ret == ESP_OK) ? SMBUS_ERR_OK : (ret == ESP_ERR_TIMEOUT) ? SMBUS_ERR_TIMEOUT : SMBUS_ERR_FAIL;
}

smbus_status_t SMBusRead64(smbus_handle_t handle, uint8_t devAddr, uint8_t command, uint64_t* dataRecv)
{
    if(!handle)
        return SMBUS_ERR_INVALID_ARG;

    uint8_t dataBuff[3 + 8 + 1] = {0};
    dataBuff[0] = (devAddr << 1) | I2C_RW_WRITE;
    dataBuff[1] = command;
    dataBuff[2] = (devAddr << 1) | I2C_RW_READ;

    int ret = i2c_master_write_read_device( handle->i2cPort, devAddr, &command, 1, &dataBuff[3],
                                            (handle->info.usePEC) ? 9 : 8, pdMS_TO_TICKS(handle->info.timeoutMs));

    if(ret == ESP_ERR_TIMEOUT)
        return SMBUS_ERR_TIMEOUT;
    else if(ret == ESP_OK)
    {
        if (handle->info.usePEC)
        {
            uint8_t refCrc = SMBusCrc8(CRC_INIT_VALUE, dataBuff, sizeof(dataBuff) - 1);
            if(refCrc != dataBuff[sizeof(dataBuff) - 1])
                return SMBUS_ERR_BAD_CRC;
        }

        *dataRecv = ((uint64_t)dataBuff[3 + 7] << 54) |
                    ((uint64_t)dataBuff[3 + 6] << 48) |
                    ((uint64_t)dataBuff[3 + 5] << 40) |
                    ((uint64_t)dataBuff[3 + 4] << 32) |
                    ((uint64_t)dataBuff[3 + 3] << 24) |
                    ((uint64_t)dataBuff[3 + 2] << 16) |
                    ((uint64_t)dataBuff[3 + 1] <<  8) |
                    ((uint64_t)dataBuff[3 + 0] <<  0);

        return SMBUS_ERR_OK;
    }
    
    return SMBUS_ERR_FAIL;
}

smbus_status_t SMBusWrite32Block(smbus_handle_t handle, uint8_t devAddr, uint8_t command, uint32_t dataSent)
{
    uint8_t buff[] = {(dataSent >> 0) & 0xFF,
                      (dataSent >> 8) & 0xFF,
                      (dataSent >> 16) & 0xFF,
                      (dataSent >> 24) & 0xFF};

    return SMBusBlockWrite(handle, devAddr, command, buff, sizeof(uint32_t));
}

smbus_status_t SMBusRead16Block(smbus_handle_t handle, uint8_t devAddr, uint8_t command, uint16_t *dataRecv)
{
    uint8_t dataBuff[256];
    uint8_t recvLen;
    smbus_status_t ret = SMBusBlockRead(handle, devAddr, command, dataBuff, &recvLen);

    if (ret != SMBUS_ERR_OK || recvLen != sizeof(uint16_t))
        return ret;

    *dataRecv = ((uint16_t)dataBuff[1] << 8) |
                ((uint16_t)dataBuff[0] << 0);

    return SMBUS_ERR_OK;
}

smbus_status_t SMBusWrite16Block(smbus_handle_t handle, uint8_t devAddr, uint8_t command, uint16_t dataSent)
{
    uint8_t dataBuff[] = {(dataSent >> 0) & 0xFF,
                          (dataSent >> 8) & 0xFF};

    return SMBusBlockWrite(handle, devAddr, command, dataBuff, sizeof(uint16_t));
}

smbus_status_t SMBusRead32Block(smbus_handle_t handle, uint8_t devAddr, uint8_t command, uint32_t *dataRecv)
{
    uint8_t dataBuff[256];
    uint8_t recvLen;
    smbus_status_t ret = SMBusBlockRead(handle, devAddr, command, dataBuff, &recvLen);

    if (ret != SMBUS_ERR_OK || recvLen != sizeof(uint32_t))
        return ret;

    *dataRecv = ((uint32_t)dataBuff[3] << 24) |
                ((uint32_t)dataBuff[2] << 16) |
                ((uint32_t)dataBuff[1] <<  8) |
                ((uint32_t)dataBuff[0] <<  0);

    return SMBUS_ERR_OK;
}

smbus_status_t SMBusWrite64Block(smbus_handle_t handle, uint8_t devAddr, uint8_t command, uint64_t dataSent)
{
    uint8_t dataBuff[] = {(dataSent >> 0) & 0xFF,
                      (dataSent >> 8) & 0xFF,
                      (dataSent >> 16) & 0xFF,
                      (dataSent >> 24) & 0xFF,
                      (dataSent >> 32) & 0xFF,
                      (dataSent >> 40) & 0xFF,
                      (dataSent >> 48) & 0xFF,
                      (dataSent >> 54) & 0xFF};

    return SMBusBlockWrite(handle, devAddr, command, dataBuff, sizeof(uint64_t));
}

smbus_status_t SMBusRead64Block(smbus_handle_t handle, uint8_t devAddr, uint8_t command, uint64_t *dataRecv)
{
    uint8_t dataBuff[256];
    uint8_t recvLen;
    smbus_status_t ret = SMBusBlockRead(handle, devAddr, command, dataBuff, &recvLen);

    if (ret != SMBUS_ERR_OK || recvLen != sizeof(uint64_t))
        return ret;

    *dataRecv = ((uint64_t)dataBuff[3 + 7] << 54) |
                ((uint64_t)dataBuff[3 + 6] << 48) |
                ((uint64_t)dataBuff[3 + 5] << 40) |
                ((uint64_t)dataBuff[3 + 4] << 32) |
                ((uint64_t)dataBuff[3 + 3] << 24) |
                ((uint64_t)dataBuff[3 + 2] << 16) |
                ((uint64_t)dataBuff[3 + 1] <<  8) |
                ((uint64_t)dataBuff[3 + 0] <<  0);

    return SMBUS_ERR_OK;
}

smbus_status_t SMBusWriteRaw(smbus_handle_t handle, uint8_t devAddr, uint8_t *dataSent, uint8_t dataLength)
{

    int ret = i2c_master_write_to_device(handle->i2cPort, devAddr, dataSent, dataLength, pdMS_TO_TICKS(handle->info.timeoutMs));

    return (ret == ESP_OK) ? SMBUS_ERR_OK : (ret == ESP_ERR_TIMEOUT) ? SMBUS_ERR_TIMEOUT
                                                                     : SMBUS_ERR_FAIL;
}

smbus_status_t SMBusWriteWordReadBlock(smbus_handle_t handle, uint8_t devAddr, uint8_t command, uint16_t word, bool wordFlipEndianness,
                                       uint8_t responseCommand, uint8_t *dataRecv, uint8_t *dataLength, int delayMs)
{
    if(wordFlipEndianness)
        word = (word << 8) | (word >> 8);

    smbus_status_t ret = SMBusWriteWord(handle, devAddr, command, word);
    if (ret != SMBUS_ERR_OK)
        return ret;
    //If the delay is greater than the RTOS defalt tick resolution
    if(delayMs > 10)
        SMBusPlatformDelayMs(delayMs);

    return SMBusBlockRead(handle, devAddr, responseCommand, dataRecv, dataLength);
}

smbus_status_t SMBusWriteWordWriteBlock(smbus_handle_t handle, uint8_t devAddr, uint8_t command, uint16_t word, bool wordFlipEndianness,
                                        uint8_t responseCommand, uint8_t *dataSent, uint8_t dataLength, int delayMs)
{
    if (wordFlipEndianness)
        word = (word << 8) | (word >> 8);

    smbus_status_t ret = SMBusWriteWord(handle, devAddr, command, word);
    if (ret != SMBUS_ERR_OK)
        return ret;
    //If the delay is greater than the RTOS defalt tick resolution
    if(delayMs > 10)
        SMBusPlatformDelayMs(delayMs);

    return SMBusBlockWrite(handle, devAddr, responseCommand, dataSent, dataLength);
}

smbus_status_t SMBusWriteWordBlockReadBlock(smbus_handle_t handle, uint8_t devAddr, uint8_t command, uint16_t word,
                                            uint8_t responseCommand, uint8_t *dataRecv, uint8_t *dataLength, int delayMs)
{
    smbus_status_t ret = SMBusWrite16Block(handle, devAddr, command, word);
    if (ret != SMBUS_ERR_OK)
        return ret;
    //If the delay is greater than the RTOS defalt tick resolution
    if(delayMs > 10)
        SMBusPlatformDelayMs(delayMs);

    return SMBusBlockRead(handle, devAddr, responseCommand, dataRecv, dataLength);
}


void SMBusPlatformDelayMs(uint32_t delayMs)
{
    vTaskDelay(pdMS_TO_TICKS(delayMs));
}

static uint8_t SMBusCrc8(uint8_t crc8, uint8_t const *data, uint16_t dataLength)
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