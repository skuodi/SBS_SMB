/**
 * 
 * @file:   sbs_smb.h - C library for interfacing with Smart Battery v1.1 compliant batteries over SMBus
 * @author: skuodi
 * @date:   17 January, 2023. Updated 04 May 2024
 * 
 * */

#ifndef _SBS_SMB_H_
#define _SBS_SMB_H_ 

#include <stdint.h>
#include <stdbool.h>

#include "platform/smbus_platform.h"

#define SBS_BATTERY_DEFAULT_ADDRESS                     0x0B

/***********************************SBS Command definitions*********************************************/

#define SBS_COMMAND_MANUFACTURER_ACCESS                 0x00
#define SBS_COMMAND_REMAINING_CAPACITY_ALARM            0x01
#define SBS_COMMAND_REMAINING_TIME_ALARM                0x02
#define SBS_COMMAND_BATTERY_MODE                        0x03
#define SBS_COMMAND_AT_RATE                             0x04    
#define SBS_COMMAND_AT_RATE_TIME_TO_FULL                0x05
#define SBS_COMMAND_AT_RATE_TIME_TO_EMPTY               0x06
#define SBS_COMMAND_AT_RATE_OK                          0x07
#define SBS_COMMAND_TEMPERATURE                         0x08
#define SBS_COMMAND_VOLTAGE                             0x09
#define SBS_COMMAND_CURRENT                             0x0A
#define SBS_COMMAND_AVERAGE_CURRENT                     0x0B
#define SBS_COMMAND_MAX_ERROR                           0x0C
#define SBS_COMMAND_RELATIVE_STATE_OF_CHARGE            0x0D
#define SBS_COMMAND_ABSOLUTE_STATE_OF_CHARGE            0x0E
#define SBS_COMMAND_REMAINING_CAPACITY                  0x0F
#define SBS_COMMAND_FULL_CHARGE_CAPACITY                0x10
#define SBS_COMMAND_RUN_TIME_TO_EMPTY                   0x11
#define SBS_COMMAND_AVERAGE_TIME_TO_EMPTY               0x12
#define SBS_COMMAND_AVERAGE_TIME_TO_FULL                0x13
#define SBS_COMMAND_CHARGING_CURRENT                    0x14
#define SBS_COMMAND_CHARGING_VOLTAGE                    0x15
#define SBS_COMMAND_BATTERY_STATUS                      0x16
#define SBS_COMMAND_ALARM_WARNING                       0x16
#define SBS_COMMAND_CYCLE_COUNT                         0x17
#define SBS_COMMAND_DESIGN_CAPACITY                     0x18
#define SBS_COMMAND_DESIGN_VOLTAGE                      0x19
#define SBS_COMMAND_SPECIFICATION_INFO                  0x1A
#define SBS_COMMAND_MANUFACTURE_DATE                    0x1B
#define SBS_COMMAND_SERIAL_NUMBER                       0x1C
#define SBS_COMMAND_MANUFACTURER_NAME                   0x20
#define SBS_COMMAND_DEVICE_NAME                         0x21
#define SBS_COMMAND_DEVICE_CHEMISTRY                    0x22
#define SBS_COMMAND_MANUFACTURER_DATA                   0x23
#define SBS_COMMAND_OPTIONAL_MFG_FUNCTION5              0x2F
#define SBS_COMMAND_OPTIONAL_MFG_FUNCTION4              0x3C
#define SBS_COMMAND_OPTIONAL_MFG_FUNCTION3              0x3D
#define SBS_COMMAND_OPTIONAL_MFG_FUNCTION2              0x3E
#define SBS_COMMAND_OPTIONAL_MFG_FUNCTION1              0x3F

/***********************************SBS BATTERY MODE BITS*********************************************/

#define SBS_SMB_BATTERY_MODE_INTERNAL_CHARGE_CONTROLLER (1 <<  0) // Whether Battery Contains Internal Charge Controller Function
#define SBS_SMB_BATTERY_MODE_PRIMARY_BATTERY_SUPPORT    (1 <<  1) // Whether Battery Can Act as Primary Or Secondary Battery
#define SBS_SMB_BATTERY_MODE_CONDITIONING_FLAG          (1 <<  7) // Whether Battery Controller Requires Conditioning Cycle
#define SBS_SMB_BATTERY_MODE_CHARGE_CONTROLLER_ENABLED  (1 <<  8) // Whether Battery's Internal Charge Controller Is Enabled
#define SBS_SMB_BATTERY_MODE_PRIMARY_BATTERY            (1 <<  9) // Whether Battery is Configured as A Primary Battery
#define SBS_SMB_BATTERY_MODE_ALARM_MODE                 (1 << 13) // Whether Battery Will Broadcast Warnings of Alarm Conditions
#define SBS_SMB_BATTERY_MODE_CHARGER_MODE               (1 << 14) // Whether Battery Broadcasts Voltage and Current When Queried
#define SBS_SMB_BATTERY_MODE_CAPACITY_MODE              (1 << 15) // Whether Battery Reports Capacity in 10mAH or in mAH

typedef enum
{
  SBS_SMB_CAPACITY_UNIT_CURRENT = 0,  // mAH
  SBS_SMB_CAPACITY_UNIT_POWER,        // mW
} sbs_smb_capacity_unit_t;

typedef struct 
{
  uint8_t internalChgCtrlSupport :1;
  uint8_t primaryBattSupport :1;
  uint8_t conditioningRequested :1;
  uint8_t internalChgCtrlEnabled :1;
  uint8_t primaryBattEnabled :1;
  uint8_t alarmBroadcastEnabled :1;
  uint8_t chargingBroadcastEnabled :1;
  sbs_smb_capacity_unit_t capacityUnit : 1;

} sbs_smb_battery_mode_t;

/***********************************SBS BATTERY STATUS*********************************************/

#define SBS_SMB_BATTERY_ALARM_MASK(x)                   (x & 0xFF00)

typedef enum
{
  SBS_SMB_BATTERY_ALARM_OVER_CHARGED              =   (1 << 15),
  SBS_SMB_BATTERY_ALARM_TERMINATE_CHARGE          =   (1 << 14),
  SBS_SMB_BATTERY_ALARM_OVER_TEMPERATURE          =   (1 << 12),
  SBS_SMB_BATTERY_ALARM_TERMINATE_DISCHARGE       =   (1 << 11),
  SBS_SMB_BATTERY_ALARM_REMAINING_CAPACITY        =   (1 <<  9),
  SBS_SMB_BATTERY_ALARM_REMAINING_TIME            =   (1 <<  8),
}sbs_smb_battery_alarm_t;

#define SBS_SMB_BATTERY_STATUS_MASK(x)                  (x & 0x00F0)

typedef enum 
{
  SBS_SMB_BATTERY_STATUS_INITIALIZED              =   (1 <<  7),
  SBS_SMB_BATTERY_STATUS_DISCHARGING              =   (1 <<  6),
  SBS_SMB_BATTERY_STATUS_FULLY_CHARGED            =   (1 <<  5),
  SBS_SMB_BATTERY_STATUS_BATTERY_DEPLETED         =   (1 <<  4),
}sbs_smb_battery_status_t;

#define SBS_SMB_BATTERY_ERROR_MASK(x) (x & 0x000F)

typedef enum
{
  SBS_SMB_BATTERY_ERROR_NONE                      =   0x00,
  SBS_SMB_BATTERY_ERROR_BUSY                      =   0x01,
  SBS_SMB_BATTERY_ERROR_RESERVED_CMD              =   0x02,
  SBS_SMB_BATTERY_ERROR_UNSUPPORTED_CMD           =   0x03,
  SBS_SMB_BATTERY_ERROR_ACCESS_DENIED             =   0x04,
  SBS_SMB_BATTERY_ERROR_OVERFLOW_UNDERFLOW        =   0x05,
  SBS_SMB_BATTERY_ERROR_BAD_SIZE                  =   0x06,
  SBS_SMB_BATTERY_ERROR_UNKNOWN                   =   0x07,
}sbs_smb_battery_error_t;

typedef struct
{
  uint16_t overChargeAlarm                        :   1;
  uint16_t terminateChargeAlarm                   :   1;
  uint16_t overTempAlarm                          :   1;
  uint16_t terminateDischargeAlarm                :   1;
  uint16_t remainingCapacityAlarm                 :   1;
  uint16_t remainingTimeAlarm                     :   1;
  uint16_t initialized                            :   1;
  uint16_t discharging                            :   1;
  uint16_t fullyCharged                           :   1;
  uint16_t fullyDischarged                        :   1;
  sbs_smb_battery_error_t error;
}sbs_smb_battery_state_t;

#define SBS_SMB_SPEC_INFO_VERSION_MASK(s)             ((s >> 4)& 0x0F)
#define SBS_SMB_SPEC_INFO_REVISION_MASK(s)            (s & 0x0F)
#define SBS_SMB_SPEC_INFO_VSCALE_MASK(s)              ((s >> 8) & 0x0F)
#define SBS_SMB_SPEC_INFO_ISCALE_MASK(s)              ((s >> 12) & 0x0F)

#define SBS_SMB_SPEC_INFO_VERSION_1V0                 0b0001
#define SBS_SMB_SPEC_INFO_VERSION_1V1                 0b0010
#define SBS_SMB_SPEC_INFO_VERSION_1V1_PEC             0b0011
#define SBS_SMB_SPEC_INFO_REVISION_1V0_1V1            0x01

typedef struct
{
  char version[8];
  char revision[8];
  uint16_t vScale;
  uint16_t iScale;
}sbs_smb_spec_info_t;

#define SBS_SMB_DATE_DAY_MASK(d)                      (d & 0x1F)
#define SBS_SMB_DATE_MONTH_MASK(d)                    ((d >> 5) & 0x0F)
#define SBS_SMB_DATE_YEAR_MASK(d)                     ((d >> 9) & 0x7F)

#define SBS_SMB_DATE_BASE_YEAR                        1980

typedef struct
{
  uint8_t day;
  uint8_t month;
  uint16_t year;
}sbs_smb_date_t;

typedef enum
{
  SBS_SMB_CMD_CODE_MANUFACTURER_ACCESS,       // Read/Write Battery Manufacturer Word (optional, implementation specific)
  SBS_SMB_CMD_CODE_MANUFACTURER_BLOCK_ACCESS, // Read/Write Battery Manufacturer Word (optional, implementation specific)
  SBS_SMB_CMD_CODE_REMAINING_CAPACITY_ALARM,  // Read/Write Low Power Warning Threshold
  SBS_SMB_CMD_CODE_REMAINING_TIME_ALARM,      // Read/Write the remaining time threshold below which the battery sends low power AlarmWarning() messages
  SBS_SMB_CMD_CODE_BATTERY_MODE,              // Read/Write Battery Mode bits
  SBS_SMB_CMD_CODE_AT_RATE,                   // Read/Write the AtRate value used in calculations made by the AtRateTimeToFull(), AtRateTimeToEmpty(), and AtRateOK() functions.
  SBS_SMB_CMD_CODE_AT_RATE_TIME_TO_FULL,      // Read the predicted remaining time to fully charge the battery at the previously written AtRate value in mA.
  SBS_SMB_CMD_CODE_AT_RATE_TIME_TO_EMPTY,     // Returns the predicted remaining time to fully charge the battery at the previously written AtRate value in mA.
  SBS_SMB_CMD_CODE_AT_RATE_OK,                // Returns a Boolean value that indicates whether or not the battery can deliver the previously written AtRate value of additional energy for 10 seconds (Boolean).
  SBS_SMB_CMD_CODE_TEMPERATURE,               // Read Battery Temperature
  SBS_SMB_CMD_CODE_VOLTAGE,                   // Battery terminal voltage in mV
  SBS_SMB_CMD_CODE_CURRENT,                   // Battery Current in mA. 0 ~ 32767 for Charge, 0 ~ -32767 for Discharge
  SBS_SMB_CMD_CODE_AVERAGE_CURRENT,           // Read one minute rolling average of current through battery terminals
  SBS_SMB_CMD_CODE_MAX_ERROR,                 // Read Expected Maximum error in state of charge calculation. Range 0 - 100%
  SBS_SMB_CMD_CODE_RELATIVE_STATE_OF_CHARGE,  // Read Battery Capacity as a Percentage of Full Charge Capacity
  SBS_SMB_CMD_CODE_ABSOLUTE_STATE_OF_CHARGE,  // Read Battery Capacity as a Percentage of Design Capacity
  SBS_SMB_CMD_CODE_REMAINING_CAPACITY,        // Read the Predicted Remaining Battery Capacity at C/5 discharge rate
  SBS_SMB_CMD_CODE_FULL_CHARGE_CAPACITY,      // Read the Predicted Battery capacity when fully charged
  SBS_SMB_CMD_CODE_RUN_TIME_TO_EMPTY,         // Read predicted remaining battery life at present discharge rate. Range 0 - 65535 min
  SBS_SMB_CMD_CODE_AVERAGE_TIME_TO_EMPTY,     // Read one minute rolling average of remaining battery life at present discharge rate. Range 0 - 65535 min
  SBS_SMB_CMD_CODE_AVERAGE_TIME_TO_FULL,      // Read one minute rolling average of predicted time to fill battery at present charge rate. Range 0 - 65535 min
  SBS_SMB_CMD_CODE_BATTERY_STATUS,            // Read Battery Status bits
  SBS_SMB_CMD_CODE_CYCLE_COUNT,               // Read the number of cycles the battery has experienced.
  SBS_SMB_CMD_CODE_DESIGN_CAPACITY,           // Read the theoretical capacity of a new pack at C/5 discharge rate
  SBS_SMB_CMD_CODE_DESIGN_VOLTAGE,            // Read the theoretical voltage of a new pack
  SBS_SMB_CMD_CODE_SPECIFICATION_INFO,        // Read the version number of the Smart Battery specification the battery pack supports, as well as voltage and current and capacity scaling information
  SBS_SMB_CMD_CODE_MANUFACTURE_DATE,          // Read the date the cell pack was manufactured.
  SBS_SMB_CMD_CODE_SERIAL_NUMBER,             // Read the serial number of the pack
  SBS_SMB_CMD_CODE_MANUFACTURER_NAME,         // Read the name of the pack manufacturer
  SBS_SMB_CMD_CODE_DEVICE_NAME,               // Read the name of the pack
  SBS_SMB_CMD_CODE_DEVICE_CHEMISTRY,          // Read the chemistry of the pack
  SBS_SMB_CMD_CODE_MANUFACTURER_DATA,          // Read the manufacturer data contained in the battery
  SBS_SMB_CMD_CODE_CHARGING_CURRENT,          // Read the Smart Battery's desired charging rate (mA)
  SBS_SMB_CMD_CODE_CHARGING_VOLTAGE,          // Read the Smart Battery's desired charging voltage (mV).
  SBS_SMB_CMD_CODE_ALARM_WARNING,             // Reveive Alarms generated by the Smart Battery
  SBS_SMB_CMD_CODE_MAX
} sbs_smb_cmd_code_t;

typedef enum
{
  SBS_SMB_SMBUS_PROTOCOL_QUICK_COMMAND = 1,
  SBS_SMB_SMBUS_PROTOCOL_SEND_BYTE,
  SBS_SMB_SMBUS_PROTOCOL_RECEIVE_BYTE,
  SBS_SMB_SMBUS_PROTOCOL_WRITE_BYTE,
  SBS_SMB_SMBUS_PROTOCOL_WRITE_WORD,
  SBS_SMB_SMBUS_PROTOCOL_READ_BYTE,
  SBS_SMB_SMBUS_PROTOCOL_READ_WORD,
  SBS_SMB_SMBUS_PROTOCOL_PROCESS_CALL,
  SBS_SMB_SMBUS_PROTOCOL_BLOCK_WRITE,
  SBS_SMB_SMBUS_PROTOCOL_BLOCK_READ,
  SBS_SMB_SMBUS_PROTOCOL_BLOCK_WRITE_BLOCK_READ_PROCESS_CALL,
  SBS_SMB_SMBUS_PROTOCOL_HOST_NOTIFY,
  SBS_SMB_SMBUS_PROTOCOL_WRITE_32,
  SBS_SMB_SMBUS_PROTOCOL_READ_32,
  SBS_SMB_SMBUS_PROTOCOL_WRITE_64,
  SBS_SMB_SMBUS_PROTOCOL_READ_64,
  SBS_SMB_SMBUS_PROTOCOL_WRITE_16_BLOCK,
  SBS_SMB_SMBUS_PROTOCOL_READ_16_BLOCK,
  SBS_SMB_SMBUS_PROTOCOL_WRITE_32_BLOCK,
  SBS_SMB_SMBUS_PROTOCOL_READ_32_BLOCK,
  SBS_SMB_SMBUS_PROTOCOL_WRITE_64_BLOCK,
  SBS_SMB_SMBUS_PROTOCOL_READ_64_BLOCK,
  SBS_SMB_SMBUS_PROTOCOL_WRITE_RAW,
  SBS_SMB_SMBUS_PROTOCOL_WRITE_WORD_READ_BLOCK,
  SBS_SMB_SMBUS_PROTOCOL_WRITE_WORD_WRITE_BLOCK,
  SBS_SMB_SMBUS_PROTOCOL_WRITE_WORD_BLOCK_READ_BLOCK,
}sbs_smb_smbus_protocol_t;

typedef void (*SBSReturnFuction_t)(void *inPtr, size_t inSize, void *outPtr, size_t outSize);

typedef struct
{
  uint8_t writeCommand;    // Actual command code sent to the SBS
  uint8_t subCommand;
  uint8_t readCommand;     // Actual command code sent to the SBS
  sbs_smb_smbus_protocol_t readProtocol;
  sbs_smb_smbus_protocol_t writeProtocol;
  sbs_smb_smbus_protocol_t writeReadProtocol;
  bool writeFlipEndianness;
  uint16_t readWriteDelayMs;
  size_t inSize;
  size_t outSize;
  void *inPtr;
  void *outPtr;
  SBSReturnFuction_t retFunc;
} sbs_smb_cmd_t;

typedef struct
{
  smbus_handle_t bus;
  uint8_t busAddress;
  sbs_smb_battery_state_t status;
  sbs_smb_date_t manufactureDate;
  uint16_t serialNumber;
  char name[64];
  char chemistry[64];
  char manufacturer[64];
  sbs_smb_spec_info_t specInfo;
  float temperatureK;
  float temeratureC;
  uint16_t cycleCount;
  uint16_t terminalVoltage;
  uint16_t relativeStateOfCharge;
  uint16_t remainingCapacity;
  uint16_t chargingVoltage;
  uint16_t chargingCurrent;
}sbs_smb_battery_t;

void SBSLogError(smbus_err_t errCode, uint8_t* msg, uint8_t msgLen);

int SBSRunCommand(sbs_smb_battery_t *battery, sbs_smb_cmd_code_t code,
                  void *inPtr, size_t inSize, void *outPtr, size_t outSize);

int SBSRunCommandBulk(sbs_smb_battery_t *battery, sbs_smb_cmd_code_t code[], uint8_t codeCount,
									void *inPtr[], size_t inSize[], void *outPtr[], size_t outSize[]);

int SBSGetBatteryInfo(sbs_smb_battery_t* battery);

void SBSPrintBatteryInfo(sbs_smb_battery_t* battery);

#endif