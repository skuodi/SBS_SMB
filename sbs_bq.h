#ifndef _SBS_BQ_H_
#define _SBS_BQ_H_

/**
 *
 * The default UNSEAL key is 0x0414 and 0x3672. The default FULL ACCESS key is 0xFFFF and 0xFFFF.
 * The default Manual PF key is 0x2857 and 0x2A98. The default Lifetimes Reset key is 0x2B14 and
 * 0x2C8A.
 * It is highly recommended to change the UNSEAL, FULL ACCESS, Manual PF, and Lifetimes Reset keys
 * from default.
 * The keys can only be changed through the ManufacturerBlockAccess().
 * Example: Change UNSEAL key to 0x1234, 0x5678, and leave the other security keys at their default
 * values.
 * Send an SMBus block write with Command = 0x0035.
 * Data = MAC command + UNSEAL key + FULL ACCESS KEY + PF key + Lifetimes Reset key
 * = 35 00 34 12 78 56 FF FF FF FF 57 28 98 2A 14 2B 8A 2C
 * NOTE: The first word of the keys cannot be the same. That means an UNSEAL key with 0xABCD
 * 0x1234 and FULL ACCESS key with 0xABCD 0x5678 are not valid because the first word is
 * the same.
 * This is because the first word is used as a “detection” for the right command. This also
 * means the first word cannot be the same as any existing MAC command.

*/

#define SBS_BQ_COMMAND_MANUFACTURER_BLOCK_ACCESS                        0x44 

#define SBS_BQ_COMMAND_MANUFACTURER_DATA                                0x0000 
#define SBS_BQ_COMMAND_DEVICE_TYPE                                      0x0001 
#define SBS_BQ_COMMAND_FIRMWARE_VERSION                                 0x0002 
#define SBS_BQ_COMMAND_HARDWARE_VERSION                                 0x0003 
#define SBS_BQ_COMMAND_INSTRUCTION_FLASH_CHECKSUM                       0x0004
#define SBS_BQ_COMMAND_DATA_FLASH_CHECKSUM                              0x0005
#define SBS_BQ_COMMAND_CHEMICAL_ID                                      0x0006 
#define SBS_BQ_COMMAND_SHUTDOWN_MODE                                    0x0010 
#define SBS_BQ_COMMAND_SLEEP_MODE                                       0x0011 
#define SBS_BQ_COMMAND_DEVICE_RESET                                     0x0012 
#define SBS_BQ_COMMAND_FUSE_TOGGLE                                      0x001D 
#define SBS_BQ_COMMAND_PRECHG_FET                                       0x001E 
#define SBS_BQ_COMMAND_CHG_FET                                          0x001F 
#define SBS_BQ_COMMAND_DSG_FET                                          0x0020 
#define SBS_BQ_COMMAND_GAUGING                                          0x0021 
#define SBS_BQ_COMMAND_FET_CONTROL                                      0x0022 
#define SBS_BQ_COMMAND_LIFETIME_DATA_COLLECTION                         0x0023 
#define SBS_BQ_COMMAND_PERMANENT_FAILURE                                0x0024 
#define SBS_BQ_COMMAND_BLACK_BOX_RECORDER                               0x0025 
#define SBS_BQ_COMMAND_FUSE                                             0x0026 
#define SBS_BQ_COMMAND_LIFETIME_DATA_RESET                              0x0028 
#define SBS_BQ_COMMAND_PERMANENT_FAIL_DATA_RESET                        0x0029 
#define SBS_BQ_COMMAND_BLACK_BOX_RECORDER_RESET                         0x002A 
#define SBS_BQ_COMMAND_CAL_MODE                                         0x002D 
#define SBS_BQ_COMMAND_SEAL_DEVICE                                      0x0030 
#define SBS_BQ_COMMAND_UNSEAL_DEVICE                                    0x0031 
#define SBS_BQ_COMMAND_FULL_ACCESS_DEVICE                               0x0032 
#define SBS_BQ_COMMAND_ROM_MODE                                         0x0033 
#define SBS_BQ_COMMAND_UNSEAL_KEY                                       0x0035 
#define SBS_BQ_COMMAND_FULL_ACCESS_KEY                                  0x0036 
#define SBS_BQ_COMMAND_AUTHENTICATION_KEY                               0x0037 
#define SBS_BQ_COMMAND_SAFETY_ALERT                                     0x0050 
#define SBS_BQ_COMMAND_SAFETY_STATUS                                    0x0051 
#define SBS_BQ_COMMAND_PF_ALERT                                         0x0052 
#define SBS_BQ_COMMAND_PF_STATUS                                        0x0053 
#define SBS_BQ_COMMAND_OPERATION_STATUS                                 0x0054 
#define SBS_BQ_COMMAND_CHARGING_STATUS                                  0x0055 
#define SBS_BQ_COMMAND_GAUGING_STATUS                                   0x0056 
#define SBS_BQ_COMMAND_MANUFACTURING_STATUS                             0x0057 
#define SBS_BQ_COMMAND_AFE_REGISTER                                     0x0058 
#define SBS_BQ_COMMAND_TURBO_POWER                                      0x0059 
#define SBS_BQ_COMMAND_TURBO_FINAL                                      0x005A 
#define SBS_BQ_COMMAND_TURBO_PACK_R                                     0x005B 
#define SBS_BQ_COMMAND_TURBO_SYS_R                                      0x005C 
#define SBS_BQ_COMMAND_MIN_SYS_V                                        0x005D 
#define SBS_BQ_COMMAND_TURBO_CURRENT                                    0x005E 
#define SBS_BQ_COMMAND_LIFETIME_DATA_BLOCK_1                            0x0060 
#define SBS_BQ_COMMAND_LIFETIME_DATA_BLOCK_2                            0x0061 
#define SBS_BQ_COMMAND_LIFETIME_DATA_BLOCK_3                            0x0062 
#define SBS_BQ_COMMAND_MANUFACTURER_INFO                                0x0070 
#define SBS_BQ_COMMAND_VOLTAGES                                         0x0071 
#define SBS_BQ_COMMAND_TEMPERATURES                                     0x0072 
#define SBS_BQ_COMMAND_IT_STATUS1                                       0x0073 
#define SBS_BQ_COMMAND_IT_STATUS2                                       0x0074 
#define SBS_BQ_COMMAND_MANUAL FET CONTROL                               0x270C 
#define SBS_BQ_COMMAND_EXIT_CALIBRATION_OUTPUT_MODE                     0xF080 
#define SBS_BQ_COMMAND_OUTPUT_CC_AND_ADC_FOR_CALIBRATION                0xF081 
#define SBS_BQ_COMMAND_OUTPUT_SHORTED_CC_AND_ADC_OFFSET_FOR_CALIBRATION 0xF082 
#define SBS_BQ_COMMAND_DF_ACCESS_ROW_ADDRESS(r)                         (0x0100 | (r & 0b11))

#include <stdint.h>
#include <string.h>

#include "libs/WjCryptLib/lib/WjCryptLib_Sha1.h"

#include "sbs_smb.h"
#include "sbs_bq.h"

/// @brief Perform a SHA1 authentication handshake using a 128-bit key
/// @param battery 
/// @param accessCmd 
/// @param key 
/// @return 
int SBSBqAccessSha1Hmac(sbs_smb_battery_t *battery, uint8_t accessCmd, uint8_t *key);

int SBSBqBlockAccessSha1Hmac(sbs_smb_battery_t *battery, uint8_t accessCmd, uint8_t *key);

int SBSBqAccess2WordKey(sbs_smb_battery_t *battery, uint8_t accessCmd, uint16_t *key);

int SBSBqBlockAccess2WordKey(sbs_smb_battery_t *battery, uint8_t accessCmd, uint16_t *key);

int SBSBqSeal(sbs_smb_battery_t *battery);

#endif