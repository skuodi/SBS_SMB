/**
 * 
 * @file:   sbs_smb.h - C library for interfacing with Smart Battery v1.1 compliant batteries over SMBus
 * @author: skuodi
 * @date:   17 January, 2023
 * 
 * */

#ifndef _SBS_SMB_H_
#define _SBS_SMB_H_ 

#include "smbus_if.h"

#define SBS_BATTERY_ADDRESS                     0x0B << 1

/***********************************SBS Command definitions*********************************************/

#define SBS_COMMAND_MANUFACTURER_ACCESS         0x00
#define SBS_COMMAND_REMAINING_CAPACITY_ALARM        0x01
#define SBS_COMMAND_REMAINING_TIME_ALARM        0x02
#define SBS_COMMAND_BATTERY_MODE            0x03
#define SBS_COMMAND_AT_RATE                     0x04    
#define SBS_COMMAND_AT_RATE_TIME_TO_FULL        0x05
#define SBS_COMMAND_AT_RATE_TIME_TO_EMPTY       0x06
#define SBS_COMMAND_AT_RATE_OK                  0x07
#define SBS_COMMAND_TEMPERATURE                 0x08
#define SBS_COMMAND_VOLTAGE                 0x09
#define SBS_COMMAND_CURRENT                 0x0A
#define SBS_COMMAND_AVERAGE_CURRENT             0x0B
#define SBS_COMMAND_MAX_ERROR               0x0C
#define SBS_COMMAND_RELATIVE_STATE_OF_CHARGE        0x0D
#define SBS_COMMAND_ABSOLUTE_STATE_OF_CHARGE        0x0E
#define SBS_COMMAND_REMAINING_CAPACITY          0x0F
#define SBS_COMMAND_FULL_CHARGE_CAPACITY        0x10
#define SBS_COMMAND_RUN_TIME_TO_EMPTY           0x11
#define SBS_COMMAND_AVERAGE_TIME_TO_EMPTY       0x12
#define SBS_COMMAND_AVERAGE_TIME_TO_FULL        0x13
#define SBS_COMMAND_CHARGING_CURRENT            0x14
#define SBS_COMMAND_CHARGING_VOLTAGE            0x15
#define SBS_COMMAND_BATTERY_STATUS          0x16
#define SBS_COMMAND_ALARM_WARNING           0x16
#define SBS_COMMAND_CYCLE_COUNT             0x17
#define SBS_COMMAND_DESIGN_CAPACITY         0x18
#define SBS_COMMAND_DESIGN_VOLTAGE          0x19
#define SBS_COMMAND_SPECIFICATION_INFO          0x1A
#define SBS_COMMAND_MANUFACTURE_DATE            0x1B
#define SBS_COMMAND_SERIAL_NUMBER           0x1C
#define SBS_COMMAND_MANUFACTURER_NAME           0x20
#define SBS_COMMAND_DEVICE_NAME             0x21
#define SBS_COMMAND_DEVICE_CHEMISTRY            0x22
#define SBS_COMMAND_MANUFACTURER_DATA           0x23

//millisecond delay
extern uint8_t _delay_ms(unsigned long);
#define SBS_READOUT_DELAY(x)    _delay_ms(x) 

/***********************************SBS Communication functions*********************************************/

uint8_t ReadManufacturerAccess(void);
uint8_t WriteManufacturerAccess(uint16_t manufacturerWord);
uint16_t ReadRemainingCapacityAlarm(void);
uint8_t WriteRemainingCapacityAlarm(uint16_t remainingCapacityAlarmThreshold);
uint16_t ReadRemainingTimeAlarm(void);
uint8_t WriteRemainingTimeAlarm(uint16_t remainingTimeAlarmThreshold);
uint16_t ReadBatteryMode(void);
uint16_t ReadTemperature(void);
uint16_t ReadVoltage(void);
int16_t ReadCurrent(void);
int16_t ReadAverageCurrent(void);
uint16_t ReadMaxError(void);
int16_t ReadRelativeStateOfCharge(void);
int16_t ReadAbsoluteStateOfCharge(void);
uint16_t ReadRemainingCapacity(void);
uint16_t ReadFullChargeCapacity(void);
uint16_t ReadRunTimeToEmpty(void);
uint16_t ReadAverageTimeToEmpty(void);
uint16_t ReadAverageTimeToFull(void);
uint16_t ReadBatteryStatus(void);
uint16_t ReadCycleCount(void);
uint16_t ReadDesignCapacity(void);
uint16_t ReadDesignVoltage(void);
uint16_t ReadSpecificationInfo(void);
uint16_t ReadManufactureDate(void);
uint16_t ReadSerialNumber(void);
uint8_t ReadManufacturerName(uint8_t* nameBuf);
uint8_t ReadDeviceName(uint8_t* nameBuf);
uint8_t ReadDeviceChemistry(uint8_t* nameBuf);
uint8_t ReadManufacturerData(uint8_t* dataBuf);

#endif