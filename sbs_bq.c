#include <stdint.h>
#include <string.h>

#include "libs/WjCryptLib/lib/WjCryptLib_Sha1.h"

#include "platform/smbus_platform.h"
#include "sbs_bq.h"

int SBSBqAccessSha1Hmac(sbs_smb_battery_t *battery, uint8_t accessCmd, uint8_t *key)
{
  if (!battery || !battery->bus || !key)
    return SMBUS_ERR_INVALID_ARG;

  uint8_t msgLen;
  uint8_t dataBuff[256 + 1];
  uint8_t flipBuff[20];

  SHA1_HASH hash;
  memset(dataBuff, 0, sizeof(dataBuff));

  // Send the unseal command and receive a 20-byte challenge message
  int ret = SBSRunCommand(battery, SBS_SMB_CMD_CODE_MANUFACTURER_ACCESS, &accessCmd,
                          sizeof(accessCmd), dataBuff + 16, sizeof(dataBuff) - 16);
  if (ret != SMBUS_ERR_OK)
    return ret;
  msgLen = *(dataBuff + 16);  // The first value returned is the size of the data received
  if (msgLen != 20)
    return SMBUS_ERR_UNEXPECTED_DATA_RECEIVED;
  
  for(uint8_t i = 0; i < msgLen; i++)
    (dataBuff + 16)[i] = (dataBuff + 16)[i + 1];

  // Data is transferred LSByte first but hash is calculated MSByte first so reverse the order of received bytes
  for (int i = 20; i > 0; i--)
    flipBuff[sizeof(flipBuff) - i] = (dataBuff + 16)[i - 1];

  // H1 = H(K + M)
  memcpy(dataBuff, key, 16);
  memcpy(dataBuff + 16, flipBuff, sizeof(flipBuff));

#ifdef SBS_BQ_LOG
  printf("B1 : ");
  for (int i = 0; i < 36; i++)
    printf("%2.2X", dataBuff[i]);
  printf("\n");
#endif

  Sha1Calculate(dataBuff, 36, &hash);

#ifdef SBS_BQ_LOG
  printf("H1 : ");
  for (int i = 0; i < sizeof(hash); i++)
    printf("%2.2X", hash.bytes[i]);
  printf("\n");
#endif

  // H2 = H(K + H1)
  memcpy(dataBuff + 16, hash.bytes, 20);

#ifdef SBS_BQ_LOG
  printf("B2 : ");
  for (int i = 0; i < 36; i++)
    printf("%2.2X", dataBuff[i]);
  printf("\n");
#endif

  Sha1Calculate(dataBuff, 36, &hash);

#ifdef SBS_BQ_LOG
  printf("H2 : ");
  for (int i = 0; i < sizeof(hash); i++)
    printf("%2.2X", hash.bytes[i]);
  printf("\n");
#endif

  // Data is transferred LSByte first but hash is calculated MSByte first so reverse the order before sending

  for (int i = 20; i > 0; i--)
    flipBuff[sizeof(flipBuff) - i] = hash.bytes[i - 1];

  ret = SMBusBlockWrite(battery->bus, battery->busAddress, SBS_COMMAND_OPTIONAL_MFG_FUNCTION5, hash.bytes, 20);
  if (ret != SMBUS_ERR_OK)
    return ret;

  // Takes at least 250ms for the setting to take effect
  SMBusPlatformDelayMs(500);

  uint16_t temp = SBS_BQ_COMMAND_OPERATION_STATUS;
  ret = SBSRunCommand(battery, SBS_SMB_CMD_CODE_MANUFACTURER_ACCESS,
                      &temp, sizeof(temp), dataBuff, sizeof(dataBuff));

  msgLen = *dataBuff;
  if (ret != SMBUS_ERR_OK || msgLen != 3)
    return SMBUS_ERR_UNEXPECTED_DATA_RECEIVED;

  for(uint8_t i = 0; i < msgLen; i++)
    dataBuff[i] = dataBuff[i + 1];

  if (accessCmd == SBS_BQ_COMMAND_UNSEAL_DEVICE)
    return ((dataBuff[1] & 1) && !(dataBuff[0] & 0x08)) ? SMBUS_ERR_OK : SMBUS_ERR_FAIL;
  else if (accessCmd == SBS_BQ_COMMAND_FULL_ACCESS_DEVICE)
    return (!(dataBuff[1] & 1) && (dataBuff[0] & 0x08)) ? SMBUS_ERR_OK : SMBUS_ERR_FAIL;

  return SMBUS_ERR_FAIL;
}

int SBSBqBlockAccessSha1Hmac(sbs_smb_battery_t *battery, uint8_t accessCmd, uint8_t *key)
{
  if (!battery->bus || !key)
    return SMBUS_ERR_INVALID_ARG;

  uint8_t msgLen;
  uint8_t dataBuff[256];
  uint8_t flipBuff[20];

  SHA1_HASH hash;
  memset(dataBuff, 0, sizeof(dataBuff));

  // Send the unseal command and receive a 20-byte challenge message
  int ret = SBSRunCommand(battery, SBS_SMB_CMD_CODE_MANUFACTURER_BLOCK_ACCESS, &accessCmd,
                          sizeof(accessCmd), dataBuff + 16, sizeof(dataBuff) - 16);
  if (ret != SMBUS_ERR_OK)
    return ret;
  msgLen = *(dataBuff + 16);  // The first value returned is the size of the data received
  if (msgLen != 20)
    return SMBUS_ERR_UNEXPECTED_DATA_RECEIVED;
  
  for(uint8_t i = 0; i < msgLen; i++)
    (dataBuff + 16)[i] = (dataBuff + 16)[i + 1];

  // Data is transferred LSByte first but hash is calculated MSByte first so reverse the order of received bytes
  for (int i = 20; i > 0; i--)
    flipBuff[sizeof(flipBuff) - i] = (dataBuff + 16)[i - 1];

  // H1 = H(K + M)
  memcpy(dataBuff, key, 16);
  memcpy(dataBuff + 16, flipBuff, sizeof(flipBuff));

#ifdef SBS_BQ_LOG
  printf("B1 : ");
  for (int i = 0; i < 36; i++)
    printf("%2.2X", dataBuff[i]);
  printf("\n");
#endif

  Sha1Calculate(dataBuff, 36, &hash);

#ifdef SBS_BQ_LOG
  printf("H1 : ");
  for (int i = 0; i < sizeof(hash); i++)
    printf("%2.2X", hash.bytes[i]);
  printf("\n");
#endif

  // H2 = H(K + H1)
  memcpy(dataBuff + 16, hash.bytes, 20);

#ifdef SBS_BQ_LOG
  printf("B2 : ");
  for (int i = 0; i < 36; i++)
    printf("%2.2X", dataBuff[i]);
  printf("\n");
#endif

  Sha1Calculate(dataBuff, 36, &hash);

#ifdef SBS_BQ_LOG
  printf("H2 : ");
  for (int i = 0; i < sizeof(hash); i++)
    printf("%2.2X", hash.bytes[i]);
  printf("\n");
#endif

  // Data is transferred LSByte first but hash is calculated MSByte first so reverse the order before sending

  for (int i = 20; i > 0; i--)
    flipBuff[sizeof(flipBuff) - i] = hash.bytes[i - 1];

  ret = SMBusBlockWrite(battery->bus, battery->busAddress, SBS_COMMAND_OPTIONAL_MFG_FUNCTION5, hash.bytes, 20);
  if (ret != SMBUS_ERR_OK)
    return ret;

  // Takes at least 250ms for the setting to take effect
  SMBusPlatformDelayMs(500);

  uint16_t temp = SBS_BQ_COMMAND_OPERATION_STATUS;
  ret = SBSRunCommand(battery, SBS_SMB_CMD_CODE_MANUFACTURER_BLOCK_ACCESS,
                      &temp, sizeof(temp), dataBuff, sizeof(dataBuff));

  msgLen = *dataBuff;
  if (ret != SMBUS_ERR_OK || msgLen != 3)
    return SMBUS_ERR_UNEXPECTED_DATA_RECEIVED;

  for(uint8_t i = 0; i < msgLen; i++)
    dataBuff[i] = dataBuff[i + 1];

  if (accessCmd == SBS_BQ_COMMAND_UNSEAL_DEVICE)
    return ((dataBuff[1] & 1) && !(dataBuff[0] & 0x08)) ? SMBUS_ERR_OK : SMBUS_ERR_FAIL;
  else if (accessCmd == SBS_BQ_COMMAND_FULL_ACCESS_DEVICE)
    return (!(dataBuff[1] & 1) && (dataBuff[0] & 0x08)) ? SMBUS_ERR_OK : SMBUS_ERR_FAIL;

  return SMBUS_ERR_FAIL;
}

int SBSBqAccess2WordKey(sbs_smb_battery_t *battery, uint8_t accessCmd, uint16_t *key)
{
  if (!battery->bus || !key)
    return SMBUS_ERR_INVALID_ARG;

  uint8_t recvCount;
  uint8_t dataBuff[256];

  int ret = SMBusWriteWord(battery->bus, battery->busAddress, SBS_COMMAND_MANUFACTURER_ACCESS, key[0]);
  if (ret != SMBUS_ERR_OK)
    return ret;

  SMBusPlatformDelayMs(50);

  ret = SMBusWriteWord(battery->bus, battery->busAddress, SBS_COMMAND_MANUFACTURER_ACCESS, key[1]);
  if (ret != SMBUS_ERR_OK)
    return ret;

  // Takes at least 250ms for the setting to take effect
  SMBusPlatformDelayMs(500);

  uint16_t temp = SBS_BQ_COMMAND_OPERATION_STATUS;
  ret = SBSRunCommand(battery, SBS_SMB_CMD_CODE_MANUFACTURER_ACCESS,
                      &temp, sizeof(temp), dataBuff, sizeof(dataBuff));

  recvCount = *dataBuff;
  if (ret != SMBUS_ERR_OK)
  {
    SBSLogError(SMBUS_ERR_UNEXPECTED_DATA_RECEIVED, &recvCount, sizeof(recvCount));
    return SMBUS_ERR_UNEXPECTED_DATA_RECEIVED;
  }

  for(uint8_t i = 0; i < recvCount; i++)
    dataBuff[i] = dataBuff[i + 1];

  if (accessCmd == SBS_BQ_COMMAND_UNSEAL_DEVICE)
    return ((dataBuff[1] & 1) && !(dataBuff[0] & 0x08)) ? SMBUS_ERR_OK : SMBUS_ERR_FAIL;
  else if (accessCmd == SBS_BQ_COMMAND_FULL_ACCESS_DEVICE)
    return (!(dataBuff[1] & 1) && (dataBuff[0] & 0x08)) ? SMBUS_ERR_OK : SMBUS_ERR_FAIL;

  return SMBUS_ERR_FAIL;
}

int SBSBqBlockAccess2WordKey(sbs_smb_battery_t *battery, uint8_t accessCmd, uint16_t *key)
{
  if (!battery->bus || !key)
    return SMBUS_ERR_INVALID_ARG;

  uint8_t recvCount;
  uint8_t dataBuff[256];

  int ret = SMBusWriteWord(battery->bus, battery->busAddress, SBS_COMMAND_MANUFACTURER_ACCESS, key[0]);
  if (ret != SMBUS_ERR_OK)
    return ret;

  SMBusPlatformDelayMs(50);

  ret = SMBusWriteWord(battery->bus, battery->busAddress, SBS_COMMAND_MANUFACTURER_ACCESS, key[1]);
  if (ret != SMBUS_ERR_OK)
    return ret;

  // Takes at least 250ms for the setting to take effect
  SMBusPlatformDelayMs(500);

  uint16_t temp = SBS_BQ_COMMAND_OPERATION_STATUS;
  ret = SBSRunCommand(battery, SBS_SMB_CMD_CODE_MANUFACTURER_ACCESS,
                      &temp, sizeof(temp), dataBuff, sizeof(dataBuff));

  recvCount = *dataBuff;
  if (ret != SMBUS_ERR_OK)
    return SMBUS_ERR_UNEXPECTED_DATA_RECEIVED;

  for(uint8_t i = 0; i < recvCount; i++)
    dataBuff[i] = dataBuff[i + 1];

  if (accessCmd == SBS_BQ_COMMAND_UNSEAL_DEVICE)
    return ((dataBuff[1] & 1) && !(dataBuff[0] & 0x08)) ? SMBUS_ERR_OK : SMBUS_ERR_FAIL;
  else if (accessCmd == SBS_BQ_COMMAND_FULL_ACCESS_DEVICE)
    return (!(dataBuff[1] & 1) && (dataBuff[0] & 0x08)) ? SMBUS_ERR_OK : SMBUS_ERR_FAIL;

  return SMBUS_ERR_FAIL;
}

int SBSBqSeal(sbs_smb_battery_t *battery)
{
  uint8_t recvCount;
  uint8_t dataBuff[256];

  int ret = SMBusWriteWord(battery->bus, battery->busAddress, SBS_COMMAND_MANUFACTURER_ACCESS, SBS_BQ_COMMAND_SEAL_DEVICE);
  if (ret != SMBUS_ERR_OK)
    return ret;

  // Takes at least 250ms for the setting to take effect
  SMBusPlatformDelayMs(500);

  uint16_t temp = SBS_BQ_COMMAND_OPERATION_STATUS;
  ret = SBSRunCommand(battery, SBS_SMB_CMD_CODE_MANUFACTURER_ACCESS,
                      &temp, sizeof(temp), dataBuff, sizeof(dataBuff));

  recvCount = *dataBuff;
  if (ret != SMBUS_ERR_OK)
    return SMBUS_ERR_UNEXPECTED_DATA_RECEIVED;

  for(uint8_t i = 0; i < recvCount; i++)
    dataBuff[i] = dataBuff[i + 1];

  return ((dataBuff[1] & 1) && (dataBuff[0] & 0x08)) ? SMBUS_ERR_OK : SMBUS_ERR_FAIL;
}