#include "sbs_smb.h"
#include "string.h"

extern void EchoPrint(char *buf); //function for 

#define SBS_PRINT_LOG //comment out to disable priniting of logs

#ifdef SBS_PRINT_LOG

char Buffer[100] = { 0 };

#endif


/**
 * @brief:	ReadManufacturerAccess	-	Read Battery Manufacturer Byte (optional, implementation specific)
 * @param:	none
 * @retval:	uint8_t					-	Manufacturer Access Byte
 * */
uint8_t ReadManufacturerAccess(void)
{
	uint16_t buf = 0;

	if (SMBusReadWord(SBS_BATTERY_ADDRESS, SBS_COMMAND_MANUFACTURER_ACCESS, &buf) == SMBUS_STATUS_OK)
		return buf;
	return SMBUS_STATUS_OK;
}

/**
 * @brief:	WriteManufacturerAccess	-	Write Battery Manufacturer Word (optional, implementation specific)
 * @param:	manufacturerWord		-	Manufacturer Data to be written
 * 
 * @retval:	uint8_t					-	returns 0 if transaction was successful
 * 									-	returns 1 if transaction was unsuccessful
 * */
uint8_t WriteManufacturerAccess(uint16_t manufacturerWord)
{
	uint16_t buf;
	if (SMBusWriteWord(SBS_BATTERY_ADDRESS, SBS_COMMAND_MANUFACTURER_ACCESS, manufacturerWord)== SMBUS_STATUS_OK)
	{
		return SMBUS_STATUS_OK;
	}
	
	return SMBUS_STATUS_ERROR;

}

/**
 * @brief:	ReadRemainingCapacityAlarm	-	Read Low Power Warning Threshold
 * @param:	none
 * 
 * @retval:	uint16_t					-	Returned Low Power Warning Threshold Value
 * 
 * */

uint16_t ReadRemainingCapacityAlarm(void)
{
	uint16_t ret = 0;
	if(SMBusReadWord(SBS_BATTERY_ADDRESS,SBS_COMMAND_REMAINING_CAPACITY_ALARM, &ret) == SMBUS_STATUS_OK)
	{
		#ifdef SBS_PRINT_LOG
		
		sprintf(Buffer, "Low Power Warning Threshold: %d mAH\r\n", ret);
		EchoPrint(Buffer);

		#endif
	}
	else
	{

		#ifdef SBS_PRINT_LOG

		sprintf(Buffer, "ReadRemainingCapacityAlarm(void) Error!!\r\n");
		EchoPrint(Buffer);

		#endif

		return 0;

	}

	return ret;


}


/**
 * @brief:	WriteRemainingCapacityAlarm		- 	Set Low Power Warning Threshold
 * @param:	remainingCapacityAlarmThreshold	-	threshold to be set in mAH
 * 
 * @retval:	uint8_t							-	returns 0 if transaction was successful
 * 											-	returns 1 if transaction was unsuccessful
 * */

uint8_t WriteRemainingCapacityAlarm(uint16_t remainingCapacityAlarmThreshold)
{
	uint16_t buf = 0;
	if(SMBusWriteWord(SBS_BATTERY_ADDRESS,SBS_COMMAND_REMAINING_CAPACITY_ALARM, remainingCapacityAlarmThreshold) == SMBUS_STATUS_OK)
	{
		SBS_READOUT_DELAY(10);
		if (SMBusReadWord(SBS_BATTERY_ADDRESS,SBS_COMMAND_REMAINING_CAPACITY_ALARM, &buf) == SMBUS_STATUS_OK)
		{
			if(buf == remainingCapacityAlarmThreshold)
				return SMBUS_STATUS_OK;
		}
	}
	
	return SMBUS_STATUS_ERROR;
}

/**
 * @brief:	ReadRemainingTimeAlarm	-	Read the time threshold below which the battery send low power AlarmWarning() messages 
 * @param:	none
 * 
 * @retval:	uint16_t				-	Remaining time alarm threshold in minutes
 * 
 * */
uint16_t ReadRemainingTimeAlarm(void)
{
	
	uint16_t ret;
	if(SMBusReadWord(SBS_BATTERY_ADDRESS, SBS_COMMAND_REMAINING_TIME_ALARM, &ret) == SMBUS_STATUS_OK)
	{
		#ifdef SBS_PRINT_LOG
		
		sprintf(Buffer, "Remaining Time Warning Threshold: %d min\r\n", ret);
		EchoPrint(Buffer);

		#endif
	}
	else
	{

		#ifdef SBS_PRINT_LOG

		sprintf(Buffer, "ReadRemainingTimeAlarm(void) Error!!\r\n");
		EchoPrint(Buffer);

		#endif

		return 0;

	}

}

/**
 * @brief:	WriteRemainingTimeAlarm		-	Set the remaining time alarm threshold
 * @param:	remainingTimeAlarmThreshold	-	remaining time alarm threshold value to be set in minutes
 * 
 * @retval:	uint8_t						-	returns 0 if transaction was successful
 * 										-	returns 1 if transaction was unsuccessful 
 * */
uint8_t WriteRemainingTimeAlarm(uint16_t remainingTimeAlarmThreshold)
{
	uint16_t buf;
	if(SMBusWriteWord(SBS_BATTERY_ADDRESS, SBS_COMMAND_REMAINING_TIME_ALARM, remainingTimeAlarmThreshold) == SMBUS_STATUS_OK)
	{
		SBS_READOUT_DELAY(10);
		if(SMBusReadWord(SBS_BATTERY_ADDRESS, SBS_COMMAND_REMAINING_TIME_ALARM, &buf) == SMBUS_STATUS_OK)
		{
			if (buf == remainingTimeAlarmThreshold)
				return SMBUS_STATUS_OK;
		}
	}
	return SMBUS_STATUS_ERROR;
}

/**
 * @brief:	ReadBatteryMode		-	Read Battery Mode bits
 * @param:	none
 * @retval:	uint16_t			-	battery mode bits
 * 
 * */
uint16_t ReadBatteryMode(void)
{
	uint16_t ret = 0;
	if (SMBusReadWord(SBS_BATTERY_ADDRESS, SBS_COMMAND_BATTERY_MODE, &ret) == SMBUS_STATUS_OK)
	{
		#ifdef SBS_PRINT_LOG

			// check INTERNAL_CHARGE_CONTROLLER bit
			if (ret & 0x01)
				sprintf(Buffer, "Battery Contains Internal Charge Controller Function\r\n");
			else
				sprintf(Buffer, "Battery Contains No Internal Charge Controller Function\r\n");
			EchoPrint(Buffer);

			// check PRIMARY_BATTERY_SUPPORT bit
			if (ret & (0x01 << 1))
				sprintf(Buffer, "Battery Can Act as Primary Or Secondary Battery\r\n"); //
			else
				sprintf(Buffer, "Battery Can Only Act as Standalone Battery\r\n");
			EchoPrint(Buffer);

			// check CONDITIONING_FLAG bit
			if (ret & (0x01 << 7))
				sprintf(Buffer, "Battery Controller Requires Conditioning Cycle\r\n"); // controller is limited in its ability to determine current state-of-charge of the battery
			else
				sprintf(Buffer, "Battery Controller is Accurate\r\n");
			EchoPrint(Buffer);

			// check (settable, DEFAULT = 0) CHARGE_CONTROLLER_ENABLED bit
			if (ret & (0x01 << 8))
				sprintf(Buffer, "Battery's Internal Charge Controller Is Enabled\r\n");
			else
				sprintf(Buffer, "Battery's Internal Charge Controller Is Disabled\r\n");
			EchoPrint(Buffer);

			// check (settable, DEFAULT = 0) PRIMARY_BATTERY bit
			if (ret & (0x01 << 9))
				sprintf(Buffer, "Battery is Configured as A Primary Battery\r\n");
			else
				sprintf(Buffer, "Battery Is Configured as A Secondary Battery\r\n");
			EchoPrint(Buffer);

			// check (settable, DEFAULT = 0) ALARM_MODE bit
			if (ret & (0x01 << 13))
				sprintf(Buffer, "Battery Will NOT Broadcast Warnings of Alarm Conditions\r\n");
			else
				sprintf(Buffer, "Battery Will Broadcast Warnings of Alarm Conditions\r\n");
			EchoPrint(Buffer);

			// check (settable, DEFAULT = 0) CHARGER_MODE bit
			if (ret & (0x01 << 14))
				sprintf(Buffer, "Battery Only Broadcasts Voltage and Current When Queried\r\n");
			else
				sprintf(Buffer, "Battery Broadcasts Its Voltage and Current every 5-60sec\r\n");
			EchoPrint(Buffer);

			// check (settable, DEFAULT = 0) CAPACITY_MODE bit
			if (ret & (0x01 << 15))
				sprintf(Buffer, "Battery Reports Capacity in 10mwH\r\n");
			else
				sprintf(Buffer, "Battery Reports Capacity in mAH\r\n\n");
			EchoPrint(Buffer);

		#endif
	}
	else
	{
		#ifdef SBS_PRINT_LOG

			sprintf(Buffer, "BatteryMode(void) Error!!\r\n");
			EchoPrint(Buffer);

		#endif

		ret = 0;

	}
	return ret;
}

/**@brief:	ReadTemperature		-		Read Battery Temperature
 * @param:	none
 * @retval:	uint16_t			-		Battery temperature in 0.1K units
 * 								-		Divide by 10 to obtain temperature in K
 * 
 * */

uint16_t ReadTemperature(void)
{
	uint16_t ret;
	
	if (SMBusReadWord(SBS_BATTERY_ADDRESS, SBS_COMMAND_TEMPERATURE, &ret) == SMBUS_STATUS_OK)
	{
		#ifdef SBS_PRINT_LOG
		
		sprintf(Buffer, "Battery Temperature: %d/10 deg. K\r\n", ret);
		EchoPrint(Buffer);

		#endif
	}
	else
	{

		#ifdef SBS_PRINT_LOG

		sprintf(Buffer, "Temperature(void) Error!!\r\n");
		EchoPrint(Buffer);

		#endif

		return 0;

	}

	return ret;

}


/**@brief:	ReadVoltage			-		Read Battery Terminal Voltage
 * @param:	none
 * @retval:	uint16_t			-		Battery temperature in mV
 * 
 * */
uint16_t ReadVoltage(void)
{
	uint16_t ret;

	if (SMBusReadWord(SBS_BATTERY_ADDRESS, SBS_COMMAND_VOLTAGE, &ret) == SMBUS_STATUS_OK)
	{
		#ifdef SBS_PRINT_LOG

		sprintf(Buffer, "Battery Voltage: %d mV\r\n", ret);
		EchoPrint(Buffer);

		#endif
	}
	else
	{
		ret = 0;
		#ifdef SBS_PRINT_LOG

		sprintf(Buffer, "ReadVoltage(void) Error!!\r\n");
		EchoPrint(Buffer);

		#endif
	}
	return ret;
}


/**@brief:	ReadCurrent			-		Read Battery Current
 * @param:	none
 * @retval:	int16_t				-		Battery Current in mA
 * 								-		0 to 32767 for Charge
 * 								-		0 to -32767 for discharge
 * */
int16_t ReadCurrent(void)
{
	uint16_t ret;

	if (SMBusReadWord(SBS_BATTERY_ADDRESS, SBS_COMMAND_CURRENT, &ret) == SMBUS_STATUS_OK)
	{
		#ifdef SBS_PRINT_LOG

		sprintf(Buffer, "Battery Current: %d mA\r\n", (int16_t)ret);
		EchoPrint(Buffer);
		
		#endif
	}
	else
	{
		ret = 0;
		#ifdef SBS_PRINT_LOG

		sprintf(Buffer, "ReadCurrent(void) Error!!\r\n");
		EchoPrint(Buffer);
		
		#endif
	}
	return (int16_t)ret;
}


/**@brief:	ReadAverageCurrent	-		Read one minute rolling average of current through battery terminals
 * @param:	none
 * @retval:	int16_t				-		Battery Current in mA
 * 								-		0 to 32767 for Charge
 * 								-		0 to -32767 for discharge
 * */
int16_t ReadAverageCurrent(void)
{
	uint16_t ret;

	if (SMBusReadWord(SBS_BATTERY_ADDRESS, SBS_COMMAND_AVERAGE_CURRENT, &ret) == SMBUS_STATUS_OK)
	{
		#ifdef SBS_PRINT_LOG

		sprintf(Buffer, "Battery Average Current: %d mA\r\n", (int16_t)ret);
		EchoPrint(Buffer);
		
		#endif
	}
	else
	{
		ret = -1;
		#ifdef SBS_PRINT_LOG

		sprintf(Buffer, "ReadAverageCurrent(void) Error!!\r\n");
		EchoPrint(Buffer);
		
		#endif
	}
	return (int16_t)ret;
}

/**
 * @brief:	ReadMaxError		-	Reads Expected Maximum error in state of charge calculation
 * @param:	none
 * @retval:	int16_t				-	percentage uncertainty
 * 								-	range 0 - 100%
 * */
uint16_t ReadMaxError(void)
{
	uint16_t ret;

	if (SMBusReadWord(SBS_BATTERY_ADDRESS, SBS_COMMAND_MAX_ERROR, &ret) == SMBUS_STATUS_OK)
	{
		#ifdef SBS_PRINT_LOG

		sprintf(Buffer, "Maximum Expected Parameter Error: %d perc. \r\n", (int16_t)ret);
		EchoPrint(Buffer);
		
		#endif
	}
	else
	{
		ret = -1;
		#ifdef SBS_PRINT_LOG

		sprintf(Buffer, "ReadMaxError(void) Error!!\r\n");
		EchoPrint(Buffer);
		
		#endif
	}
	return (int16_t)ret;
}


/**
 * @brief:	ReadRelativeStateOfCharge	-	Read Battery Capacity as a Percentage of Full Charge Capacity
 * @param:	none
 * @retval:	int16_t							-	percentage 
 * 											-	range 0 - 100
 * */

int16_t ReadRelativeStateOfCharge(void)
{
	uint16_t ret;

	if (SMBusReadWord(SBS_BATTERY_ADDRESS, SBS_COMMAND_RELATIVE_STATE_OF_CHARGE, &ret) == SMBUS_STATUS_OK)
	{
		#ifdef SBS_PRINT_LOG

		sprintf(Buffer, "Remaining Charged Battery Percentage: %d perc. \r\n", (int16_t)ret);
		EchoPrint(Buffer);
		
		#endif
	}
	else
	{
		ret = -1;
		#ifdef SBS_PRINT_LOG

		sprintf(Buffer, "ReadRelativeStateOfCharge(void) Error!!\r\n");
		EchoPrint(Buffer);
		
		#endif
	}
	return (int16_t)ret;
}



/**
 * @brief:	ReadAbsoluteStateOfCharge		-	Read Battery Capacity as a Percentage of Design Capacity
 * @param:	none
 * @retval:	int16_t							-	percentage 
 * 											-	range 0 - 100
 * */

int16_t ReadAbsoluteStateOfCharge(void)
{
	uint16_t ret;

	if (SMBusReadWord(SBS_BATTERY_ADDRESS, SBS_COMMAND_ABSOLUTE_STATE_OF_CHARGE, &ret) == SMBUS_STATUS_OK)
	{
		#ifdef SBS_PRINT_LOG

		sprintf(Buffer, "Remaining Design Battery Percentage: %d perc. \r\n", (int16_t)ret);
		EchoPrint(Buffer);
		
		#endif
	}
	else
	{
		ret = -1;
		#ifdef SBS_PRINT_LOG

		sprintf(Buffer, "ReadAbsoluteStateOfCharge(void) Error!!\r\n");
		EchoPrint(Buffer);
		
		#endif
	}
	return (int16_t)ret;
}


/**
 * @brief:	ReadRemainingCapacity		-	Read the Predicted Remaining Battery Capacity at C/5 discharge rate
 * @param:	none
 * @retval:	uint16_t					-	remaining capacity in mA
 * 										-	valid 0 - 65535mA
 * 										
 * */
uint16_t ReadRemainingCapacity(void)
{
	uint16_t ret;

	if (SMBusReadWord(SBS_BATTERY_ADDRESS, SBS_COMMAND_REMAINING_CAPACITY, &ret) == SMBUS_STATUS_OK)
	{
		#ifdef SBS_PRINT_LOG

		sprintf(Buffer, "Predicted Remaining Charged Battery Capacity: %d mAH\r\n", ret);
		EchoPrint(Buffer);
		
		#endif
	}
	else
	{
		ret = 0;
		#ifdef SBS_PRINT_LOG

		sprintf(Buffer, "ReadRemainingCapacity(void) Error!!\r\n");
		EchoPrint(Buffer);
		
		#endif
	}
	return ret;
}


/**
 * @brief:	ReadFullChargeCapacity		-	Read the Predicted Battery capacity when fully charged
 * @param:	none
 * @retval:	uint16_t					-	remaining capacity in mA
 * 										-	valid 0 - 65535mA
 * */
uint16_t ReadFullChargeCapacity(void)
{
	uint16_t ret;

	if (SMBusReadWord(SBS_BATTERY_ADDRESS, SBS_COMMAND_FULL_CHARGE_CAPACITY, &ret) == SMBUS_STATUS_OK)
	{
		#ifdef SBS_PRINT_LOG

		sprintf(Buffer, "Predicted Battery Full Charge Capacity: %d mAH\r\n", ret);
		EchoPrint(Buffer);
		
		#endif
	}
	else
	{
		ret = 0;
		#ifdef SBS_PRINT_LOG

		sprintf(Buffer, "ReadFullChargeCapacity(void) Error!!\r\n");
		EchoPrint(Buffer);
		
		#endif
	}
	return ret;
}


/**
 * @brief:	ReadRunTimeToEmpty		-	Read predicted remaining battery life at present discharge rate
 * @param:	none
 * @retval:	uint16_t				-	remaining operation time in minutes
 * 									-	valid 0 - 65535min
 * */
uint16_t ReadRunTimeToEmpty(void)
{
	uint16_t ret;

	if (SMBusReadWord(SBS_BATTERY_ADDRESS, SBS_COMMAND_RUN_TIME_TO_EMPTY, &ret) == SMBUS_STATUS_OK)
	{
		#ifdef SBS_PRINT_LOG

		sprintf(Buffer, "Predicted Remaining Run Time: %d min\r\n", ret);
		EchoPrint(Buffer);
		
		#endif
	}
	else
	{
		ret = 0;
		#ifdef SBS_PRINT_LOG

		sprintf(Buffer, "ReadRunTimeToEmpty(void) Error!!\r\n");
		EchoPrint(Buffer);
		
		#endif
	}
	return ret;
}


/**
 * @brief:	ReadAverageTimeToEmpty	-	Read one minute rolling average of remaining battery life at present discharge rate
 * @param:	none
 * @retval:	uint16_t				-	remaining operation time in minutes
 * 									-	valid 0 - 65535min
 * */
uint16_t ReadAverageTimeToEmpty(void)
{
	uint16_t ret;

	if (SMBusReadWord(SBS_BATTERY_ADDRESS, SBS_COMMAND_AVERAGE_TIME_TO_EMPTY, &ret) == SMBUS_STATUS_OK)
	{
		#ifdef SBS_PRINT_LOG

		sprintf(Buffer, "Calculated Average Remaining Run Time: %d min\r\n", ret);
		EchoPrint(Buffer);
		
		#endif
	}
	else
	{
		ret = 0;
		#ifdef SBS_PRINT_LOG

		sprintf(Buffer, "ReadAverageTimeToEmpty(void) Error!!\r\n");
		EchoPrint(Buffer);
		
		#endif
	}
	return ret;
}


// /*Read predicted time to fill battery at present charge rate*/


/**
 * @brief:	ReadAverageTimeToFull	-	Read one minute rolling average of predicted time to fill battery at present charge rate
 * @param:	none
 * @retval:	uint16_t				-	remaining operation time in minutes
 * 									-	valid 0 - 65535min
 * */
uint16_t ReadAverageTimeToFull(void)
{
	uint16_t ret;

	if (SMBusReadWord(SBS_BATTERY_ADDRESS, SBS_COMMAND_AVERAGE_TIME_TO_FULL, &ret) == SMBUS_STATUS_OK)
	{
		#ifdef SBS_PRINT_LOG

		sprintf(Buffer, "Calculated Average Remaining Charge Time: %d min\r\n", ret);
		EchoPrint(Buffer);
		
		#endif
	}
	else
	{
		ret = 0;
		#ifdef SBS_PRINT_LOG

		sprintf(Buffer, "ReadAverageTimeToFull(void) Error!!\r\n");
		EchoPrint(Buffer);
		
		#endif
	}
	return ret;
}



/**
 * @brief:	ReadBatteryStatus	-	Read Battery Status bits
 * @param:	none
 * @retval:	uint16_t			-	battery status bits
 * 
 * */
uint16_t ReadBatteryStatus(void)
{
	uint16_t ret = 0;
	if (SMBusReadWord(SBS_BATTERY_ADDRESS, SBS_COMMAND_BATTERY_MODE, &ret) == SMBUS_STATUS_OK)
	{
		#ifdef SBS_PRINT_LOG

			//ERROR CODE bits
			if (ret & 0b1111 == 0x00)
				sprintf(Buffer, "Command Processed Successfully\r\n");
			else if (ret & 0b1111 == 0x01)
				sprintf(Buffer, "Battery Busy\r\n");
			else if (ret & 0b1111 == 0x02)
				sprintf(Buffer, "Reserved Command\r\n");
			else if (ret & 0b1111 == 0x03)
				sprintf(Buffer, "Unsupported Command\r\n");
			else if (ret & 0b1111 == 0x04)
				sprintf(Buffer, "Access Denied: Attempt to write a read-only function\r\n");
			else if (ret & 0b1111 == 0x05)
				sprintf(Buffer, "Data Overflow/Underflow detected\r\n");
			else if (ret & 0b1111 == 0x06)
				sprintf(Buffer, "Attempted to write function with incorrect data block size\r\n");
			else if (ret & 0b1111 == 0x07)
				sprintf(Buffer, "Undefinable error detected\r\n");
			EchoPrint(Buffer);

			// FULLY CHARGED bit
			if (ret & (0x01 << 4))
				sprintf(Buffer, "Battery Capacity is Depleted\r\n");
			else
				sprintf(Buffer, "Battery Capacity is not Depleted\r\n");
			EchoPrint(Buffer);

			// FULLY CHARGED bit
			if (ret & (0x01 << 5))
				sprintf(Buffer, "Battery is Fully Charged\r\n");
			else
				sprintf(Buffer, "Battery is not Fully Charged\r\n");
			EchoPrint(Buffer);

			// DISCHARGING bit
			if (ret & (0x01 << 6))
				sprintf(Buffer, "Battery is Discharging\r\n");
			else
				sprintf(Buffer, "Battery is not Discharging\r\n");
			EchoPrint(Buffer);

			// INITIALIZED bit
			if (ret & (0x01 << 7))
				sprintf(Buffer, "Battery Needs Manufacturer Calibration\r\n"); //
			else
				sprintf(Buffer, "Battery Does not need Manufacturer Calibration\r\n");
			EchoPrint(Buffer);

			// REMAINING TIME ALARM bit
			if (ret & (0x01 << 8))
				sprintf(Buffer, "Value of AverageTimeToEmpty is less than the Remaining Time alarm threshold\r\n");
			else
				sprintf(Buffer, "Value of AverageTimeToEmpty is not less than the Remaining Time alarm threshold\r\n");
			EchoPrint(Buffer);

			// REMAINING CAPACITY ALARM bit
			if (ret & (0x01 << 9))
				sprintf(Buffer, "Value of Remaining Capacity is less than the Remaining Capacity alarm threshold\r\n");
			else
				sprintf(Buffer, "Value of Remaining Capacity is not less than the Remaining Capacity alarm threshold\r\n");
			EchoPrint(Buffer);

			// TERMINATE_DISCHARGE_ALARM bit
			if (ret & (0x01 << 11))
				sprintf(Buffer, "Battery Capacity Depleted! Discharging should stop as soon as possible.\r\n");
			else
				sprintf(Buffer, "No Alarm condition. Discharging is allowed.\r\n");
			EchoPrint(Buffer);

			// OVER TEMPERATURE ALARM bit
			if (ret & (0x01 << 12))
				sprintf(Buffer, "Battery Temperature is Above pre-set limit!\r\n");
			else
				sprintf(Buffer, "Battery Temperature is not Above pre-set limit!\r\n");
			EchoPrint(Buffer);

			// TERMINATE_CHARGE_ALARM bit
			if (ret & (0x01 << 14))
				sprintf(Buffer, "Alarm Condition! Charging should be temporarily suspended.\r\n");
			else
				sprintf(Buffer, "No Alarm condition. Charging is allowed.\r\n");
			EchoPrint(Buffer);

			// OVER CHARGED ALARM bit
			if (ret & (0x01 << 15))
				sprintf(Buffer, "Battery Is in OVERCHARGE State!\r\n");
			else
				sprintf(Buffer, "Battery Is in not OVERCHARGE State\r\n\n");
			EchoPrint(Buffer);

		#endif
	}
	else
	{
		#ifdef SBS_PRINT_LOG

			sprintf(Buffer, "ReadBatteryStatus(void) Error!!\r\n");
			EchoPrint(Buffer);

		#endif

		ret = 0;

	}
	return ret;
}


/**
 * @brief:	ReadCycleCount	-	returns the  number of cycles the battery has experienced
 * @param:	none
 * @retval:	uint16_t		-	number of cycles experienced
 * 							-	a cycle is a discharge amount approximately equal to DesignCapacity
 * 							-	valid 0 thru 65535, 
 * 							-	65535 indicates the battery experienced 65535 or more cycles
 * 
 * */
uint16_t ReadCycleCount(void)
{
	uint16_t ret;

	if (SMBusReadWord(SBS_BATTERY_ADDRESS, SBS_COMMAND_CYCLE_COUNT, &ret) == SMBUS_STATUS_OK)
	{
		#ifdef SBS_PRINT_LOG

		sprintf(Buffer, "Experienced cycles: %d \r\n", ret);
		EchoPrint(Buffer);
		
		#endif
	}
	else
	{
		ret = 0;
		#ifdef SBS_PRINT_LOG

		sprintf(Buffer, "ReadCycleCount(void) Error!!\r\n");
		EchoPrint(Buffer);
		
		#endif
	}
	return ret;
}



/**
 * @brief:	ReadDesignCapacity			-	returns the theoretical capacity of a new pack at C/5 discharge rate
 * @param:	none
 * @retval:	uint16_t					-	Design capacity in mA
 * 										-	valid 0 - 65535mA
 * 										
 * */
uint16_t ReadDesignCapacity(void)
{
	uint16_t ret;

	if (SMBusReadWord(SBS_BATTERY_ADDRESS, SBS_COMMAND_DESIGN_CAPACITY, &ret) == SMBUS_STATUS_OK)
	{
		#ifdef SBS_PRINT_LOG

		sprintf(Buffer, "Theoretical New Pack Capacity: %d mAH\r\n", ret);
		EchoPrint(Buffer);
		
		#endif
	}
	else
	{
		ret = 0;
		#ifdef SBS_PRINT_LOG

		sprintf(Buffer, "ReadDesignCapacity(void) Error!!\r\n");
		EchoPrint(Buffer);
		
		#endif
	}
	return ret;
}



/**
 * @brief:	ReadDesignVoltage			-	returns the theoretical voltage of a new pack
 * @param:	none
 * @retval:	uint16_t					-	Design Voltage in mV
 * 										-	valid 0 - 65535mV
 * 										
 * */
uint16_t ReadDesignVoltage(void)
{
	uint16_t ret;

	if (SMBusReadWord(SBS_BATTERY_ADDRESS, SBS_COMMAND_DESIGN_VOLTAGE, &ret) == SMBUS_STATUS_OK)
	{
		#ifdef SBS_PRINT_LOG

		sprintf(Buffer, "Theoretical New Pack Voltage: %d mV\r\n", ret);
		EchoPrint(Buffer);
		
		#endif
	}
	else
	{
		ret = 0;
		#ifdef SBS_PRINT_LOG

		sprintf(Buffer, "ReadDesignVoltage(void) Error!!\r\n");
		EchoPrint(Buffer);
		
		#endif
	}
	return ret;
}

/**
 * @brief:	ReadSpecificationInfo	-	Returns the version number of the Smart Battery specification the battery pack supports, as well as voltage and current and capacity scaling information
 * @param:	none
 * @retval:	uint16_t				-	 packed unsigned integer with specification and scaling information
 * 
 * */

uint16_t ReadSpecificationInfo(void)
{
	uint16_t ret;

	if (SMBusReadWord(SBS_BATTERY_ADDRESS, SBS_COMMAND_SPECIFICATION_INFO, &ret) == SMBUS_STATUS_OK)
	{
		#ifdef SBS_PRINT_LOG

		sprintf(Buffer, "All read voltages scaled (multiplied) by: %d\r\n", (ret >> 12));
		EchoPrint(Buffer);

		sprintf(Buffer, "All read currents and capacites scaled (multiplied) by: %d\r\n", (ret >> 8));
		EchoPrint(Buffer);

		if(((ret >> 4) & 0x0F) == 0x01)
		sprintf(Buffer, "Battery Compies to Smart Battery Specification v1.0\r\n");
		else if(((ret >> 4) & 0x0F) == 0x02)
		sprintf(Buffer, "Battery Compies to Smart Battery Specification v1.1\r\n");
		else if(((ret >> 4) & 0x0F) == 0x03)
		sprintf(Buffer, "Battery Compies to Smart Battery Specification v1.1 with optional PEC\r\n");
		EchoPrint(Buffer);
		
		#endif
	}
	else
	{
		ret = 0;
		#ifdef SBS_PRINT_LOG

		sprintf(Buffer, "ReadSpecificationInfo(void) Error!!\r\n");
		EchoPrint(Buffer);
		
		#endif
	}
	return ret;
}


/**
 * @brief:	ReadManufactureDate		-	Returns the date the cell pack was manufactured in a packed integer
 * @param:	none
 * @retval:	uint16_t				-	 packed unsigned integer with manufacture date
 * 
 * */
uint16_t ReadManufactureDate(void)
{
	uint16_t ret;

	if (SMBusReadWord(SBS_BATTERY_ADDRESS, SBS_COMMAND_MANUFACTURE_DATE, &ret) == SMBUS_STATUS_OK)
	{
		#ifdef SBS_PRINT_LOG

		sprintf(Buffer, "Battery Was Manufactured on (DD-MM-YYYY): %02d-%02d-%d \r\n", (ret & 31), ((ret >> 5) & 15), (ret >> 9) + 1980);
		EchoPrint(Buffer);
		
		#endif
	}
	else
	{
		ret = 0;
		#ifdef SBS_PRINT_LOG

		sprintf(Buffer, "ReadManufactureDate(void) Error!!\r\n");
		EchoPrint(Buffer);
		
		#endif
	}
	return ret;
}

/**
 * @brief:	ReadSerialNumber	-	Returns the serial number of the pack
 * @param:	none
 * @retval:	uint16_t			-	serial number
 * 
 * */
uint16_t ReadSerialNumber(void)
{
	uint16_t ret;

	if (SMBusReadWord(SBS_BATTERY_ADDRESS, SBS_COMMAND_SERIAL_NUMBER, &ret) == SMBUS_STATUS_OK)
	{
		#ifdef SBS_PRINT_LOG

		sprintf(Buffer, "Battery Serial Number: %d\r\n", ret);
		EchoPrint(Buffer);
		
		#endif
	}
	else
	{
		ret = 0;
		#ifdef SBS_PRINT_LOG

		sprintf(Buffer, "ReadSerialNumber(void) Error!!\r\n");
		EchoPrint(Buffer);
		
		#endif
	}
	return ret;
}


/**
 * @brief:	ReadManufacturerName	-	Returns the name of the pack manufacturer
 * @param:	none
 * @retval:	uint16_t				-	serial number
 * 
 * */
uint8_t ReadManufacturerName(uint8_t* nameBuf)
{
	if(SMBusBlockRead(SBS_BATTERY_ADDRESS, SBS_COMMAND_MANUFACTURER_NAME, nameBuf) == SMBUS_STATUS_OK)
	{
		#ifdef SBS_PRINT_LOG

		sprintf(Buffer, "Battery Was Manufactured by: %s \r\n", nameBuf);
		EchoPrint(Buffer);

		#endif

		return SMBUS_STATUS_OK;
	}else
	{
		#ifdef SBS_PRINT_LOG

		sprintf(Buffer, "ReadManufacturerName() Error!!\r\n");
		EchoPrint(Buffer);

		#endif

	}
	return SMBUS_STATUS_ERROR;


}


/**
 * @brief:	ReadDeviceName		-	Returns the name of the pack 
 * @param:	none
 * @retval:	uint8_t				-	Return status
 * 
 * */
uint8_t ReadDeviceName(uint8_t* nameBuf)
{
	if(SMBusBlockRead(SBS_BATTERY_ADDRESS, SBS_COMMAND_DEVICE_NAME, nameBuf) == SMBUS_STATUS_OK)
	{
		#ifdef SBS_PRINT_LOG

		sprintf(Buffer, "Battery Name: %s \r\n", nameBuf);
		EchoPrint(Buffer);

		#endif

		return SMBUS_STATUS_OK;
	}else
	{
		#ifdef SBS_PRINT_LOG

		sprintf(Buffer, "ReadDeviceName() Error!!\r\n");
		EchoPrint(Buffer);

		#endif

	}
	return SMBUS_STATUS_ERROR;
}


/**
 * @brief:	ReadDeviceChemistry	-	read the chemistry of the pack 
 * @param:	none
 * @retval:	uint16_t			-	serial number
 * 
 * */
uint8_t ReadDeviceChemistry(uint8_t* nameBuf)
{
	if(SMBusBlockRead(SBS_BATTERY_ADDRESS, SBS_COMMAND_DEVICE_CHEMISTRY, nameBuf) == SMBUS_STATUS_OK)
	{
		#ifdef SBS_PRINT_LOG

		sprintf(Buffer, "Battery Chemistry: %s \r\n", nameBuf);
		EchoPrint(Buffer);

		#endif

		return SMBUS_STATUS_OK;
	}else
	{
		#ifdef SBS_PRINT_LOG

		sprintf(Buffer, "ReadDeviceChemistry() Error!!\r\n");
		EchoPrint(Buffer);

		#endif

	}
	return SMBUS_STATUS_ERROR;
}


/**
 * @brief:	ReadManufacturerData	-	Returns the name of the pack 
 * @param:	none
 * @retval:	uint8_t					-	Return status
 * 
 * */
uint8_t ReadManufacturerData(uint8_t* dataBuf)
{
	if(SMBusBlockRead(SBS_BATTERY_ADDRESS, SBS_COMMAND_MANUFACTURER_DATA, dataBuf) == SMBUS_STATUS_OK)
	{
		return SMBUS_STATUS_OK;
	}else
	{
		#ifdef SBS_PRINT_LOG

		sprintf(Buffer, "ReadManufacturerData() Error!!\r\n");
		EchoPrint(Buffer);

		#endif

	}
	return SMBUS_STATUS_ERROR;
}
