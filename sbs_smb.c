#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#include "sbs_smb.h"
#include "sbs_bq.h"

#define MIN(x,y)	((x < y) ? x : y)

static void _SBSParseBatteryMode(void *inPtr, size_t inSize, void *outPtr, size_t outSize);
static void _SBSParseAtRateOk(void *inPtr, size_t inSize, void *outPtr, size_t outSize);
static void _SBSParseTemperature(void *inPtr, size_t inSize, void *outPtr, size_t outSize);
static void _SBSParseBatteryStatus(void *inPtr, size_t inSize, void *outPtr, size_t outSize);
static void _SBSParseSpecificationInfo(void *inPtr, size_t inSize, void *outPtr, size_t outSize);
static void _SBSParseManufactureDate(void *inPtr, size_t inSize, void *outPtr, size_t outSize);

static sbs_smb_cmd_t cmdLUT[] =
{
	{// SBS_SMB_CMD_CODE_MANUFACTURER_ACCESS

		.writeCommand = SBS_COMMAND_MANUFACTURER_ACCESS,
		.readCommand = SBS_COMMAND_MANUFACTURER_DATA,
		.writeReadProtocol = SBS_SMB_SMBUS_PROTOCOL_WRITE_WORD_READ_BLOCK,
		.inSize	= sizeof(uint16_t),

	},
	{// SBS_SMB_CMD_CODE_MANUFACTURER_BLOCK_ACCESS

		.writeCommand = SBS_BQ_COMMAND_MANUFACTURER_BLOCK_ACCESS,
		.readCommand = SBS_BQ_COMMAND_MANUFACTURER_BLOCK_ACCESS,
		.writeReadProtocol = SBS_SMB_SMBUS_PROTOCOL_WRITE_WORD_BLOCK_READ_BLOCK,
		.inSize	= sizeof(uint16_t),

	},
	{// SBS_SMB_CMD_CODE_REMAINING_CAPACITY_ALARM

		.writeCommand = SBS_COMMAND_REMAINING_CAPACITY_ALARM,
		.readCommand = SBS_COMMAND_REMAINING_CAPACITY_ALARM,
		.writeProtocol = SBS_SMB_SMBUS_PROTOCOL_WRITE_WORD,
		.readProtocol = SBS_SMB_SMBUS_PROTOCOL_READ_WORD,
		.inSize	= sizeof(uint16_t),
		.outSize	= sizeof(uint16_t),

	},
	{// SBS_SMB_CMD_CODE_REMAINING_TIME_ALARM

		.writeCommand = SBS_COMMAND_REMAINING_TIME_ALARM,
		.readCommand = SBS_COMMAND_REMAINING_TIME_ALARM,
		.writeProtocol = SBS_SMB_SMBUS_PROTOCOL_WRITE_WORD,
		.readProtocol = SBS_SMB_SMBUS_PROTOCOL_READ_WORD,
		.inSize	= sizeof(uint16_t),
		.outSize	= sizeof(uint16_t),

	},
	{// SBS_SMB_CMD_CODE_BATTERY_MODE

		.writeCommand = SBS_COMMAND_BATTERY_MODE,
		.readCommand = SBS_COMMAND_BATTERY_MODE,
		.readProtocol = SBS_SMB_SMBUS_PROTOCOL_READ_WORD,
		.writeProtocol = SBS_SMB_SMBUS_PROTOCOL_READ_WORD,
		.outSize = sizeof(sbs_smb_battery_mode_t),
		.retFunc = _SBSParseBatteryMode,

	},
	{// SBS_SMB_CMD_CODE_AT_RATE

		.writeCommand = SBS_COMMAND_AT_RATE,
		.readCommand = SBS_COMMAND_AT_RATE,
		.writeProtocol = SBS_SMB_SMBUS_PROTOCOL_WRITE_WORD,
		.readProtocol = SBS_SMB_SMBUS_PROTOCOL_READ_WORD,
		.inSize	= sizeof(int16_t),
		.outSize	= sizeof(int16_t),

	},
	{// SBS_SMB_CMD_CODE_AT_RATE_TIME_TO_FULL

		.readCommand = SBS_COMMAND_AT_RATE_TIME_TO_FULL,
		.readProtocol = SBS_SMB_SMBUS_PROTOCOL_READ_WORD,
		.outSize	= sizeof(uint16_t),

	},
	{// SBS_SMB_CMD_CODE_AT_RATE_TIME_TO_EMPTY

		.readCommand = SBS_COMMAND_AT_RATE_TIME_TO_EMPTY,
		.readProtocol = SBS_SMB_SMBUS_PROTOCOL_READ_WORD,
		.outSize	= sizeof(uint16_t),

	},
	{// SBS_SMB_CMD_CODE_AT_RATE_OK

		.readCommand = SBS_COMMAND_AT_RATE_OK,
		.readProtocol = SBS_SMB_SMBUS_PROTOCOL_READ_WORD,
		.outSize	= sizeof(bool),
		.retFunc = _SBSParseAtRateOk,

	},
	{// SBS_SMB_CMD_CODE_TEMPERATURE

		.writeCommand = SBS_COMMAND_TEMPERATURE,
		.readProtocol = SBS_SMB_SMBUS_PROTOCOL_READ_WORD,
		.outSize	= sizeof(float),
		.retFunc = _SBSParseTemperature,

	},
	{// SBS_SMB_CMD_CODE_VOLTAGE

		.readCommand = SBS_COMMAND_VOLTAGE,
		.readProtocol = SBS_SMB_SMBUS_PROTOCOL_READ_WORD,
		.outSize	= sizeof(uint16_t),

	},
	{// SBS_SMB_CMD_CODE_CURRENT

		.readCommand = SBS_COMMAND_CURRENT,
		.readProtocol = SBS_SMB_SMBUS_PROTOCOL_READ_WORD,
		.outSize	= sizeof(int16_t),

	},
	{// SBS_SMB_CMD_CODE_AVERAGE_CURRENT

		.readCommand = SBS_COMMAND_AVERAGE_CURRENT,
		.readProtocol = SBS_SMB_SMBUS_PROTOCOL_READ_WORD,
		.outSize	= sizeof(int16_t),

	},
	{// SBS_SMB_CMD_CODE_MAX_ERROR

		.readCommand = SBS_COMMAND_MAX_ERROR,
		.readProtocol = SBS_SMB_SMBUS_PROTOCOL_READ_WORD,
		.outSize	= sizeof(uint16_t),

	},
	{// SBS_SMB_CMD_CODE_RELATIVE_STATE_OF_CHARGE

		.readCommand = SBS_COMMAND_RELATIVE_STATE_OF_CHARGE,
		.readProtocol = SBS_SMB_SMBUS_PROTOCOL_READ_WORD,
		.outSize	= sizeof(uint16_t),

	},
	{// SBS_SMB_CMD_CODE_ABSOLUTE_STATE_OF_CHARGE

		.readCommand = SBS_COMMAND_ABSOLUTE_STATE_OF_CHARGE,
		.readProtocol = SBS_SMB_SMBUS_PROTOCOL_READ_WORD,
		.outSize	= sizeof(uint16_t),

	},
	{// SBS_SMB_CMD_CODE_REMAINING_CAPACITY

		.readCommand = SBS_COMMAND_REMAINING_CAPACITY,
		.readProtocol = SBS_SMB_SMBUS_PROTOCOL_READ_WORD,
		.outSize	= sizeof(uint16_t),

	},
	{// SBS_SMB_CMD_CODE_FULL_CHARGE_CAPACITY

		.readCommand = SBS_COMMAND_FULL_CHARGE_CAPACITY,
		.readProtocol = SBS_SMB_SMBUS_PROTOCOL_READ_WORD,
		.outSize	= sizeof(uint16_t),

	},
	{// SBS_SMB_CMD_CODE_RUN_TIME_TO_EMPTY

		.readCommand = SBS_COMMAND_RUN_TIME_TO_EMPTY,
		.readProtocol = SBS_SMB_SMBUS_PROTOCOL_READ_WORD,
		.outSize	= sizeof(uint16_t),

	},
	{// SBS_SMB_CMD_CODE_AVERAGE_TIME_TO_EMPTY

		.readCommand = SBS_COMMAND_AVERAGE_TIME_TO_EMPTY,
		.readProtocol = SBS_SMB_SMBUS_PROTOCOL_READ_WORD,
		.outSize	= sizeof(uint16_t),

	},
	{// SBS_SMB_CMD_CODE_AVERAGE_TIME_TO_FULL

		.readCommand = SBS_COMMAND_AVERAGE_TIME_TO_FULL,
		.readProtocol = SBS_SMB_SMBUS_PROTOCOL_READ_WORD,
		.outSize	= sizeof(uint16_t),

	},
	{// SBS_SMB_CMD_CODE_BATTERY_STATUS

		.readCommand = SBS_COMMAND_BATTERY_STATUS,
		.readProtocol = SBS_SMB_SMBUS_PROTOCOL_READ_WORD,
		.outSize = sizeof(sbs_smb_battery_state_t),
		.retFunc = _SBSParseBatteryStatus,

	},
	{// SBS_SMB_CMD_CODE_CYCLE_COUNT

		.readCommand = SBS_COMMAND_CYCLE_COUNT,
		.readProtocol = SBS_SMB_SMBUS_PROTOCOL_READ_WORD,
		.outSize	= sizeof(uint16_t),

	},
	{// SBS_SMB_CMD_CODE_DESIGN_CAPACITY

		.readCommand = SBS_COMMAND_DESIGN_CAPACITY,
		.readProtocol = SBS_SMB_SMBUS_PROTOCOL_READ_WORD,
		.outSize	= sizeof(uint16_t),

	},
	{// SBS_SMB_CMD_CODE_DESIGN_VOLTAGE

		.readCommand = SBS_COMMAND_DESIGN_VOLTAGE,
		.readProtocol = SBS_SMB_SMBUS_PROTOCOL_READ_WORD,
		.outSize	= sizeof(uint16_t),

	},
	{// SBS_SMB_CMD_CODE_SPECIFICATION_INFO

		.readCommand = SBS_COMMAND_SPECIFICATION_INFO,
		.readProtocol = SBS_SMB_SMBUS_PROTOCOL_READ_WORD,
		.outSize	= sizeof(sbs_smb_spec_info_t),
		.retFunc = _SBSParseSpecificationInfo,

	},
	{// SBS_SMB_CMD_CODE_MANUFACTURE_DATE

		.readCommand = SBS_COMMAND_MANUFACTURE_DATE,
		.readProtocol = SBS_SMB_SMBUS_PROTOCOL_READ_WORD,
		.outSize	= sizeof(sbs_smb_date_t),
		.retFunc = _SBSParseManufactureDate,

	},
	{// SBS_SMB_CMD_CODE_SERIAL_NUMBER

		.readCommand = SBS_COMMAND_SERIAL_NUMBER,
		.readProtocol = SBS_SMB_SMBUS_PROTOCOL_READ_WORD,
		.outSize	= sizeof(uint16_t),

	},
	{// SBS_SMB_CMD_CODE_MANUFACTURER_NAME

		.readCommand = SBS_COMMAND_MANUFACTURER_NAME,
		.readProtocol = SBS_SMB_SMBUS_PROTOCOL_BLOCK_READ,

	},
	{// SBS_SMB_CMD_CODE_DEVICE_NAME

		.readCommand = SBS_COMMAND_DEVICE_NAME,
		.readProtocol = SBS_SMB_SMBUS_PROTOCOL_BLOCK_READ,

	},
	{// SBS_SMB_CMD_CODE_DEVICE_CHEMISTRY

		.readCommand = SBS_COMMAND_DEVICE_CHEMISTRY,
		.readProtocol = SBS_SMB_SMBUS_PROTOCOL_BLOCK_READ,

	},
	{// SBS_SMB_CMD_CODE_MANUFACTURER_DATA

		.writeCommand = SBS_COMMAND_MANUFACTURER_DATA,
		.readCommand = SBS_COMMAND_MANUFACTURER_DATA,
		.writeProtocol = SBS_SMB_SMBUS_PROTOCOL_BLOCK_READ,
		.readProtocol = SBS_SMB_SMBUS_PROTOCOL_BLOCK_READ,

	},
	{// SBS_SMB_CMD_CODE_CHARGING_CURRENT

		.readCommand = SBS_COMMAND_CHARGING_CURRENT,
		.readProtocol = SBS_SMB_SMBUS_PROTOCOL_READ_WORD,
		.outSize	= sizeof(uint16_t),

	},
	{// SBS_SMB_CMD_CODE_CHARGING_VOLTAGE

		.readCommand = SBS_COMMAND_CHARGING_VOLTAGE,
		.readProtocol = SBS_SMB_SMBUS_PROTOCOL_READ_WORD,
		.outSize	= sizeof(uint16_t),

	},
	{// SBS_SMB_CMD_CODE_ALARM_WARNING

		.readCommand = SBS_COMMAND_ALARM_WARNING,
		.readProtocol = SBS_SMB_SMBUS_PROTOCOL_READ_WORD,
		.outSize	= sizeof(sbs_smb_battery_state_t),
		.retFunc = _SBSParseBatteryStatus,

	},
};

static void _SBSParseBatteryMode(void *inPtr, size_t inSize, void *outPtr, size_t outSize)
{
	uint16_t* modeVal = (uint16_t*)inPtr;
	sbs_smb_battery_mode_t* mode = (sbs_smb_battery_mode_t*)outPtr;

	mode->internalChgCtrlSupport = (*modeVal & SBS_SMB_BATTERY_MODE_INTERNAL_CHARGE_CONTROLLER) ? true : false;
	mode->primaryBattSupport = (*modeVal & SBS_SMB_BATTERY_MODE_PRIMARY_BATTERY_SUPPORT) ? true : false;
	mode->conditioningRequested = (*modeVal & SBS_SMB_BATTERY_MODE_CONDITIONING_FLAG) ? true : false;
	mode->internalChgCtrlEnabled= (*modeVal & SBS_SMB_BATTERY_MODE_CHARGE_CONTROLLER_ENABLED) ? true : false;
	mode->primaryBattEnabled = (*modeVal & SBS_SMB_BATTERY_MODE_PRIMARY_BATTERY) ? true : false;
	mode->alarmBroadcastEnabled = (*modeVal & SBS_SMB_BATTERY_MODE_ALARM_MODE) ? true : false;
	mode->chargingBroadcastEnabled = (*modeVal & SBS_SMB_BATTERY_MODE_CHARGER_MODE) ? true : false;
	mode->capacityUnit = (*modeVal & SBS_SMB_BATTERY_MODE_CAPACITY_MODE) ? true : false;
}

static void _SBSParseAtRateOk(void *inPtr, size_t inSize, void *outPtr, size_t outSize)
{
	*((bool *)outPtr) = (*((uint16_t *)inPtr) == 0) ? 0 : 1;
}

static void _SBSParseTemperature(void *inPtr, size_t inSize, void *outPtr, size_t outSize)
{
	*((float *)outPtr) = *((int16_t *)inPtr) / 10.0f;
}

static void _SBSParseBatteryStatus(void *inPtr, size_t inSize, void *outPtr, size_t outSize)
{
	uint16_t* status = ((uint16_t*)inPtr);
	sbs_smb_battery_state_t *battStat = (sbs_smb_battery_state_t*)inPtr;

	battStat->overChargeAlarm = (*status & SBS_SMB_BATTERY_ALARM_OVER_CHARGED) ? 1 : 0;
	battStat->terminateChargeAlarm = (*status & SBS_SMB_BATTERY_ALARM_TERMINATE_CHARGE) ? 1 : 0;

	battStat->overTempAlarm = (*status & SBS_SMB_BATTERY_ALARM_OVER_TEMPERATURE) ? 1 : 0;
	battStat->terminateDischargeAlarm = (*status & SBS_SMB_BATTERY_ALARM_TERMINATE_DISCHARGE) ? 1 : 0;

	battStat->remainingCapacityAlarm = (*status & SBS_SMB_BATTERY_ALARM_REMAINING_CAPACITY) ? 1 : 0;
	battStat->remainingTimeAlarm = (*status & SBS_SMB_BATTERY_ALARM_REMAINING_TIME) ? 1 : 0;

	battStat->initialized = (*status & SBS_SMB_BATTERY_STATUS_INITIALIZED) ? 1 : 0;
	battStat->discharging = (*status & SBS_SMB_BATTERY_STATUS_DISCHARGING) ? 1 : 0;
	battStat->fullyCharged = (*status & SBS_SMB_BATTERY_STATUS_FULLY_CHARGED) ? 1 : 0;
	battStat->fullyDischarged = (*status & SBS_SMB_BATTERY_STATUS_BATTERY_DEPLETED) ? 1 : 0;

	battStat->error = SBS_SMB_BATTERY_ERROR_MASK(*status);
}

static void _SBSParseSpecificationInfo(void* inPtr, size_t inSize, void* outPtr, size_t outSize)
{
	uint16_t* spec = (uint16_t*)inPtr;
	sbs_smb_spec_info_t *specInfo = (sbs_smb_spec_info_t*) outPtr;

	uint8_t revision = SBS_SMB_SPEC_INFO_REVISION_MASK(*spec);
	uint8_t version = SBS_SMB_SPEC_INFO_VERSION_MASK(*spec);
	uint8_t vScale = SBS_SMB_SPEC_INFO_VSCALE_MASK(*spec);
	uint8_t iScale = SBS_SMB_SPEC_INFO_VSCALE_MASK(*spec);

	switch(version)
	{
		case SBS_SMB_SPEC_INFO_VERSION_1V0:
			sprintf(specInfo->version, "1.0");
			break;

		case SBS_SMB_SPEC_INFO_VERSION_1V1:
			sprintf(specInfo->version, "1.1");
			break;

		case SBS_SMB_SPEC_INFO_VERSION_1V1_PEC:
			sprintf(specInfo->version, "1.1+PEC");
			break;

		default:
			sprintf(specInfo->version, "Unknown");
			break;
	}

	switch (revision)
	{
		case SBS_SMB_SPEC_INFO_REVISION_1V0_1V1:
			sprintf(specInfo->version, "1.0/1.1");
			break;

		default:
			sprintf(specInfo->version, "Unknown");
			break;
	}

	specInfo->vScale = 1;
	specInfo->iScale = 1;

	// Voltage scaling factor = 10^(Scale)
	for (; vScale; vScale--)
		specInfo->vScale *= 10;
	
	// Current scaling factor = 10^(iScale)
	for (; iScale; iScale--)
		specInfo->iScale *= 10;
}

static void _SBSParseManufactureDate(void *inPtr, size_t inSize, void *outPtr, size_t outSize)
{
	uint16_t *date = (uint16_t *)inPtr;
	sbs_smb_date_t *mfgDate = (sbs_smb_date_t *)outPtr;
	mfgDate->day = SBS_SMB_DATE_DAY_MASK(*date);
	mfgDate->month = SBS_SMB_DATE_MONTH_MASK(*date);
	mfgDate->year = SBS_SMB_DATE_YEAR_MASK(*date) + SBS_SMB_DATE_BASE_YEAR;
}

void SBSLogError(smbus_err_t errCode, uint8_t* msg, uint8_t msgLen)
{
	printf("Error: ");
	switch(errCode)
	{
		case SMBUS_ERR_OK:
			printf("SMBUS_ERR_OK");
		break;
    case SMBUS_ERR_FAIL:
			printf("SMBUS_ERR_FAIL ");
		break;
    case SMBUS_ERR_INVALID_ARG:
			printf("SMBUS_ERR_INVALID_ARG");
		break;
    case SMBUS_ERR_BAD_CRC:
			printf("SMBUS_ERR_BAD_CRC");
		break;
    case SMBUS_ERR_TIMEOUT:
			printf("SMBUS_ERR_TIMEOUT");
		break;
    case SMBUS_ERR_UNEXPECTED_DATA_RECEIVED:
			printf("SMBUS_ERR_UNEXPECTED_DATA_RECEIVED");
		break;
    case SMBUS_ERR_START_TRANSMITTED:
			printf("SMBUS_ERR_START_TRANSMITTED");
		break;
    case SMBUS_ERR_REPEATED_START_TRANSMITTED:
			printf("SMBUS_ERR_REPEATED_START_TRANSMITTED");
		break;
    case SMBUS_ERR_ADDR_W_TRANSMITTED_ACK_RECIEVED:
			printf("SMBUS_ERR_ADDR_W_TRANSMITTED_ACK_RECIEVED");
		break;
    case SMBUS_ERR_ADDR_W_TRANSMITTED_NACK_RECIEVED:
			printf("SMBUS_ERR_ADDR_W_TRANSMITTED_NACK_RECIEVED");
		break;
    case SMBUS_ERR_DATA_TRANSMITTED_ACK_RECIEVED:
			printf("SMBUS_ERR_DATA_TRANSMITTED_ACK_RECIEVED");
		break;
    case SMBUS_ERR_DATA_TRANSMITTED_NACK_RECIEVED:
			printf("SMBUS_ERR_DATA_TRANSMITTED_NACK_RECIEVED");
		break;
    case SMBUS_ERR_ARBITRATION_LOST:
			printf("SMBUS_ERR_ARBITRATION_LOST");
		break;
    case SMBUS_ERR_ADDR_R_TRANSMITTED_ACK_RECIEVED:
			printf("SMBUS_ERR_ADDR_R_TRANSMITTED_ACK_RECIEVED");
		break;
    case SMBUS_ERR_ADDR_R_TRANSMITTED_NACK_RECIEVED:
			printf("SMBUS_ERR_ADDR_R_TRANSMITTED_NACK_RECIEVED");
		break;
    case SMBUS_ERR_DATA_RECIEVED_ACK_TRANSMITTED:
			printf("SMBUS_ERR_DATA_RECIEVED_ACK_TRANSMITTED");
		break;
    case SMBUS_ERR_DATA_RECIEVED_NACK_TRANSMITTED:
			printf("SMBUS_ERR_DATA_RECIEVED_NACK_TRANSMITTED");
		break;
	}
	printf(" Data: ");
	if(msg && msgLen)
		while(msgLen--)
			printf(" 0x%02X", *msg++);

	printf("\n");
}

int SBSRunCommand(sbs_smb_battery_t* battery, sbs_smb_cmd_code_t code,
									void *inPtr, size_t inSize, void *outPtr, size_t outSize)
{
	if(!battery || !battery->bus || code >= SBS_SMB_CMD_CODE_MAX || (inPtr  && !inSize) || (outPtr && !outSize))
		return SMBUS_ERR_INVALID_ARG;
	
	if(cmdLUT[code].inSize && inSize)
		if(inSize < cmdLUT[code].inSize)
			return SMBUS_ERR_INVALID_ARG;
	
	if(cmdLUT[code].outSize && outSize)
		if(outSize < cmdLUT[code].outSize)
			return SMBUS_ERR_INVALID_ARG;
		
	int ret = SMBUS_ERR_FAIL;
	uint8_t readLen = 0;
	uint8_t read8buff = 0;
	uint16_t read16Buff = 0;
	uint32_t read32Buff = 0;
	uint64_t read64Buff = 0;
	uint8_t readBlockBuff[256];
	void* readBuff = NULL;	// Holds the result of a read operation

	if(cmdLUT[code].writeReadProtocol && inPtr && outPtr)
	{
		switch(cmdLUT[code].writeReadProtocol)
		{
			case SBS_SMB_SMBUS_PROTOCOL_PROCESS_CALL:
				readBuff = &read16Buff;
				readLen = sizeof(read16Buff);
				ret = SMBusProcessCall(battery->bus, battery->busAddress, cmdLUT[code].writeCommand, *((uint16_t*)inPtr), (uint16_t *)readBuff);
				break;

			case SBS_SMB_SMBUS_PROTOCOL_BLOCK_WRITE_BLOCK_READ_PROCESS_CALL:
				readBuff = readBlockBuff;
				ret = SMBusBlockWriteBlockReadProcessCall(battery->bus, battery->busAddress, cmdLUT[code].writeCommand, (uint8_t*)inPtr, inSize, (uint8_t *)readBuff, &readLen);
				break;

			case SBS_SMB_SMBUS_PROTOCOL_WRITE_WORD_READ_BLOCK:
				readBuff = readBlockBuff;
				ret = SMBusWriteWordReadBlock(battery->bus, battery->busAddress, cmdLUT[code].writeCommand, *(uint16_t *)inPtr, cmdLUT[code].writeFlipEndianness,
																			cmdLUT[code].readCommand, (uint8_t *)readBuff, &readLen, cmdLUT[code].readWriteDelayMs);
				break;

			case SBS_SMB_SMBUS_PROTOCOL_WRITE_WORD_BLOCK_READ_BLOCK:
				readBuff = readBlockBuff;
				ret = SMBusWriteWordBlockReadBlock(battery->bus, battery->busAddress, cmdLUT[code].writeCommand, *(uint16_t *)inPtr, cmdLUT[code].readCommand,
																					 (uint8_t *)readBuff, &readLen, cmdLUT[code].readWriteDelayMs);
				break;

			default:
				break;
		}
		if(ret != SMBUS_ERR_OK)
			return ret;
	
		goto exit;
	}

	if (cmdLUT[code].writeProtocol && inPtr)
	{
		switch (cmdLUT[code].writeProtocol)
		{
			case SBS_SMB_SMBUS_PROTOCOL_QUICK_COMMAND:
				ret = SMBusQuickCommand(battery->bus, battery->busAddress, *(bool*)inPtr);
				break;

			case SBS_SMB_SMBUS_PROTOCOL_SEND_BYTE:
				ret = SMBusSendByte(battery->bus, battery->busAddress, *(uint8_t*)inPtr);
				break;

			case SBS_SMB_SMBUS_PROTOCOL_WRITE_BYTE:
				ret = SMBusWriteByte(battery->bus, battery->busAddress, cmdLUT[code].writeCommand, *(uint8_t*)inPtr);
				break;

			case SBS_SMB_SMBUS_PROTOCOL_WRITE_WORD:
				ret = SMBusWriteWord(battery->bus, battery->busAddress, cmdLUT[code].writeCommand, *(uint16_t *)inPtr);
				break;

			case SBS_SMB_SMBUS_PROTOCOL_BLOCK_WRITE:
				ret = SMBusBlockWrite(battery->bus, battery->busAddress, cmdLUT[code].writeCommand, (uint8_t *)inPtr, inSize);
				break;

			case SBS_SMB_SMBUS_PROTOCOL_HOST_NOTIFY:
				ret = SMBusHostNotify(battery->bus, *(uint8_t*)inPtr, battery->busAddress, *(uint16_t*)(inPtr + sizeof(uint8_t)));
				break;

			case SBS_SMB_SMBUS_PROTOCOL_WRITE_32:
				ret = SMBusWrite32(battery->bus, battery->busAddress, cmdLUT[code].writeCommand, *(uint32_t *)inPtr);
				break;

			case SBS_SMB_SMBUS_PROTOCOL_WRITE_64:
				ret = SMBusWrite64(battery->bus, battery->busAddress, cmdLUT[code].writeCommand, *(uint64_t *)inPtr);
				break;

			case SBS_SMB_SMBUS_PROTOCOL_WRITE_16_BLOCK:
				ret = SMBusWrite16Block(battery->bus, battery->busAddress, cmdLUT[code].writeCommand, *(uint16_t *)inPtr);
				break;

			case SBS_SMB_SMBUS_PROTOCOL_WRITE_32_BLOCK:
				ret = SMBusWrite32Block(battery->bus, battery->busAddress, cmdLUT[code].writeCommand, *(uint32_t *)inPtr);
				break;

			case SBS_SMB_SMBUS_PROTOCOL_WRITE_64_BLOCK:
				ret = SMBusWrite64Block(battery->bus, battery->busAddress, cmdLUT[code].writeCommand, *(uint64_t *)inPtr);
				break;

			case SBS_SMB_SMBUS_PROTOCOL_WRITE_RAW:
				ret = SMBusWriteRaw(battery->bus, battery->busAddress, (uint8_t *)inPtr, inSize);
				break;

			case SBS_SMB_SMBUS_PROTOCOL_WRITE_WORD_WRITE_BLOCK:
				ret = SMBusWriteWordWriteBlock(battery->bus, battery->busAddress, cmdLUT[code].writeCommand, cmdLUT[code].subCommand, cmdLUT[code].writeFlipEndianness,
																			 *(uint8_t *)inPtr, (uint8_t *)(inPtr + sizeof(uint8_t)), inSize, cmdLUT[code].readWriteDelayMs);
				break;

			default:
				break;
		}
		if (ret != SMBUS_ERR_OK)
			return ret;
	}

	if (cmdLUT[code].readProtocol && outPtr)
	{
		switch (cmdLUT[code].readProtocol)
		{
			case SBS_SMB_SMBUS_PROTOCOL_RECEIVE_BYTE:
				readBuff = &read8buff;
				readLen = sizeof(read8buff);
				ret = SMBusReceiveByte(battery->bus, battery->busAddress, (uint8_t *)readBuff);
				break;

			case SBS_SMB_SMBUS_PROTOCOL_READ_BYTE:
				readBuff = &read8buff;
				readLen = sizeof(read8buff);
				ret = SMBusReadByte(battery->bus, battery->busAddress, cmdLUT[code].readCommand, (uint8_t *)readBuff);
				break;

			case SBS_SMB_SMBUS_PROTOCOL_READ_WORD:
				readBuff = &read16Buff;
				readLen = sizeof(read16Buff);
				ret = SMBusReadWord(battery->bus, battery->busAddress, cmdLUT[code].readCommand, (uint16_t *)readBuff);
				break;

			case SBS_SMB_SMBUS_PROTOCOL_BLOCK_READ:
				readBuff = readBlockBuff;
				ret = SMBusBlockRead(battery->bus, battery->busAddress, cmdLUT[code].readCommand, (uint8_t *)readBuff, &readLen);
				break;

			case SBS_SMB_SMBUS_PROTOCOL_READ_32:
				readBuff = &read32Buff;
				readLen = sizeof(read32Buff);
				ret = SMBusRead32(battery->bus, battery->busAddress, cmdLUT[code].readCommand, (uint32_t *)readBuff);
				break;

			case SBS_SMB_SMBUS_PROTOCOL_READ_64:
				readBuff = &read64Buff;
				readLen = sizeof(read64Buff);
				ret = SMBusRead64(battery->bus, battery->busAddress, cmdLUT[code].readCommand, (uint64_t *)readBuff);
				break;

			case SBS_SMB_SMBUS_PROTOCOL_READ_16_BLOCK:
				readBuff = &read16Buff;
				readLen = sizeof(read16Buff);
				ret = SMBusRead16Block(battery->bus, battery->busAddress, cmdLUT[code].readCommand, (uint16_t *)readBuff);
				break;

			case SBS_SMB_SMBUS_PROTOCOL_READ_32_BLOCK:
				readBuff = &read32Buff;
				readLen = sizeof(read32Buff);
				ret = SMBusRead32Block(battery->bus, battery->busAddress, cmdLUT[code].readCommand, (uint32_t *)readBuff);
				break;

			case SBS_SMB_SMBUS_PROTOCOL_READ_64_BLOCK:
				readBuff = &read64Buff;
				readLen = sizeof(read64Buff);
				ret = SMBusRead64Block(battery->bus, battery->busAddress, cmdLUT[code].readCommand, (uint64_t *)readBuff);
				break;

			default:
				break;
		}

		if (ret != SMBUS_ERR_OK)
			return ret;
	}

exit:
	if(cmdLUT[code].retFunc)
		cmdLUT[code].retFunc(readBuff, readLen, outPtr, outSize);
	else if(readLen)
	{
		if (cmdLUT[code].readProtocol == SBS_SMB_SMBUS_PROTOCOL_BLOCK_READ ||
				cmdLUT[code].writeReadProtocol == SBS_SMB_SMBUS_PROTOCOL_WRITE_WORD_READ_BLOCK ||
				cmdLUT[code].writeReadProtocol == SBS_SMB_SMBUS_PROTOCOL_WRITE_WORD_BLOCK_READ_BLOCK)
		{
			*(uint8_t*)outPtr++ = readLen;
			outSize--;
		}
		memcpy(outPtr, readBuff, MIN(outSize, readLen));
	}
	return ret;
}


int SBSRunCommandBulk(sbs_smb_battery_t *battery, sbs_smb_cmd_code_t code[], uint8_t codeCount,
									void *inPtr[], size_t inSize[], void *outPtr[], size_t outSize[])
{
	int ret = 0;
	for(uint8_t i = 0; i < codeCount; i++)
	{
		ret = SBSRunCommand(battery, code[i], inPtr[i], inSize[i], outPtr[i], outSize[i]);
		if(ret != SMBUS_ERR_OK)
		{
			SBSLogError(ret, &i, sizeof(i));
			return ret;
		}
	}
	return SMBUS_ERR_OK;
}


int SBSGetBatteryInfo(sbs_smb_battery_t* battery)
{
	if(!battery)
		return SMBUS_ERR_INVALID_ARG;

	sbs_smb_cmd_code_t code[] = 
	{
		SBS_SMB_CMD_CODE_BATTERY_STATUS,
		SBS_SMB_CMD_CODE_MANUFACTURE_DATE,
		SBS_SMB_CMD_CODE_SERIAL_NUMBER,
		SBS_SMB_CMD_CODE_DEVICE_NAME,
		SBS_SMB_CMD_CODE_DEVICE_CHEMISTRY,
		SBS_SMB_CMD_CODE_MANUFACTURER_NAME,
		SBS_SMB_CMD_CODE_SPECIFICATION_INFO,
		SBS_SMB_CMD_CODE_TEMPERATURE,
		SBS_SMB_CMD_CODE_CYCLE_COUNT,
		SBS_SMB_CMD_CODE_VOLTAGE,
		SBS_SMB_CMD_CODE_RELATIVE_STATE_OF_CHARGE,
		SBS_SMB_CMD_CODE_REMAINING_CAPACITY,
	};
	uint8_t codeCount = sizeof(code) / sizeof(code[0]);

	void *inPtr[] = { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL};
	size_t inSize[] = { 	 0, 	 0, 	 0, 	 0, 	 0, 	 0, 	 0, 	 0, 	 0, 	 0, 	 0, 	 0};
	void *outPtr[] = 
	{
		&battery->status,
		&battery->manufactureDate,
		&battery->serialNumber,
		&battery->name,
		&battery->chemistry,
		&battery->manufacturer,
		&battery->specInfo,
		&battery->temperatureK,
		&battery->cycleCount,
		&battery->terminalVoltage,
		&battery->relativeStateOfCharge,
		&battery->remainingCapacity,
	};

	size_t outSize[] = 
	{
		sizeof(battery->status),
		sizeof(battery->manufactureDate),
		sizeof(battery->serialNumber),
		sizeof(battery->name),
		sizeof(battery->chemistry),
		sizeof(battery->manufacturer),
		sizeof(battery->specInfo),
		sizeof(battery->temperatureK),
		sizeof(battery->cycleCount),
		sizeof(battery->terminalVoltage),
		sizeof(battery->relativeStateOfCharge),
		sizeof(battery->remainingCapacity),
	};

	int ret = SBSRunCommandBulk(battery, code, codeCount, inPtr, inSize, outPtr, outSize);
	if(ret == SMBUS_ERR_OK)
		battery->temeratureC = battery->temperatureK - 273.15;

	return ret;
}

void SBSPrintBatteryInfo(sbs_smb_battery_t* battery)
{
	printf("|------------------Smart Battery Info------------------|\n"
				 "|------------------------------------------------------|\n"
				 "|- SMBus Address:      0x%02X\n"
				 "|- Device Name:        %s\n"
				 "|- Chemistry:          %s\n"
				 "|- Serial Number:      %d\n"
				 "|- Manufcture Date:    %02d-%02d-%04d (DD-MM-YYYY)\n"
				 "|- Manufacturer:       %s\n"
				 "|- SBS version:        %s\n"
				 "|- Temerature:         %.02fC / %.02fK\n"
				 "|- Cycle Count:        %d\n"
				 "|- Voltage:            %dmV\n"
				 "|- State of Charge:    %d%%\n"
				 "|- Remaining Capacity: %dmAH\n"
				 "|------------------------------------------------------|\n\n",
				 battery->busAddress, battery->name, battery->chemistry, battery->serialNumber,
				 battery->manufactureDate.day, battery->manufactureDate.month, battery->manufactureDate.year, 
				 battery->manufacturer, battery->specInfo.version, battery->temeratureC, battery->temperatureK, battery->cycleCount,
				 battery->terminalVoltage *  battery->specInfo.vScale, battery->relativeStateOfCharge, battery->remainingCapacity);
}
