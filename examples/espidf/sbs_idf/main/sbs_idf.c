#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c.h"
#include "driver/gpio.h"

#include "sbs_smb.h"
#include "sbs_bq.h"

#define I2C_PORT        I2C_NUM_0
#define SDA_PIN         10
#define SCL_PIN         11
#define I2C_SPEED       100000
#define I2C_TIMEOUT_MS  1000

sbs_smb_battery_t battery;

// scan for devices present on the bus. A smart battery typically appears at address 0x0B
int I2CTest()
{
  printf("Scanning...");
  for (int i = 0; i < 128; i++)
  {
    if (SMBusQuickCommand(battery.bus, i, 0) == SMBUS_ERR_OK)
    {
      printf("Found 0x%02X\n", i);
      return i;
    }
    else
      printf(".");
  }
  printf("\n");
  return 0;
}

void app_main(void)
{
  battery.bus = SMBusInit(I2C_PORT, 0, I2C_SPEED, SDA_PIN, SCL_PIN, -1, 1000, true);
  if(!battery.bus)
    while(1)
      {
        printf("Couldn't init SMBus!\n");
        vTaskDelay(pdMS_TO_TICKS(3000));
      }

  battery.busAddress = (uint8_t)I2CTest();

  vTaskDelay(pdMS_TO_TICKS(3000));
  printf("Attempting to unseal...\n");

  uint8_t unsealKey[] =     {0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF, 0xFE, 0xDC, 0xBA, 0x98, 0x76, 0x54, 0x32, 0x10};
  uint8_t fullAccessKey[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
  uint16_t unsealKey2[] =   {0x0414, 0x3672};
  uint16_t fullAccessKey2[] = {0xFFFF, 0xFFFF};

  int ret;

  if((ret = SBSBqAccessSha1Hmac(&battery, SBS_BQ_COMMAND_UNSEAL_DEVICE, unsealKey)) == SMBUS_ERR_OK)
  {
    printf("Unsealed with SBSBqAccessSha1Hmac()\n");
    if ((ret = SBSBqAccessSha1Hmac(&battery, SBS_BQ_COMMAND_FULL_ACCESS_DEVICE, fullAccessKey)) == SMBUS_ERR_OK)
      printf("Full Access obtained with SBSBqAccessSha1Hmac()\n");
    else
      printf("Error %d. Could not obtain Full Access\n", ret);

    if ((ret = SBSBqSeal(&battery)) == SMBUS_ERR_OK)
      printf("Device sealed successfully!");
    else
      printf("Error %d. Device sealing failed!", ret);
  }
  else
  if ((ret = SBSBqAccess2WordKey(&battery, SBS_BQ_COMMAND_UNSEAL_DEVICE, unsealKey2)) == SMBUS_ERR_OK)
  {
    printf("Unsealed with SBSBqAccess2WordKey()\n");
    if ((ret = SBSBqAccess2WordKey(&battery, SBS_BQ_COMMAND_FULL_ACCESS_DEVICE, fullAccessKey2) == SMBUS_ERR_OK))
      printf("Full Access obtained with SBSBqAccess2WordKey()\n");
    else
      printf("Error %d. Could not obtain Full Access\n", ret);

    if ((ret = SBSBqSeal(&battery)) == SMBUS_ERR_OK)
      printf("Device sealed successfully!");
    else
      printf("Error %d. Device sealing failed!", ret);
  }
  else
    printf("Error %d. Unseal failed\n", ret);

  while (1)
  {
    vTaskDelay(pdMS_TO_TICKS(3000));

    ret = SBSGetBatteryInfo(&battery);  

   if(ret == SMBUS_ERR_OK)
      SBSPrintBatteryInfo(&battery);
    else
      printf("Error %d. Could not get device info\n", ret);

  }
}
