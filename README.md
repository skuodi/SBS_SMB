# SBS_SMB 
[![MIT License](https://img.shields.io/badge/license-MIT-red.svg)](https://choosealicense.com/licenses/mit/)

This repo contains two libraries:

- **smbus_if** - an implementation of [SMBus v3.1](http://smbus.org/specs/SMBus_3_1_20180319.pdf) using AVR's TWI pripheral.

- **sbs_smb** - implementation of Smart Battery Specification(SBS) v1.1 based on **smbus_if**.

The SMBus implementation was created for use with **sbs_smb** but can be used to interface an ATMEGA with any other SMBus device/sensor as the implemented functions are compliant.

## Table of Contents
- [Features](https://github.com/skuodi/SBS_SMB#features)
- [Installation](https://github.com/skuodi/SBS_SMB#installation)
- [Porting](https://github.com/skuodi/SBS_SMB#porting)
- [References](https://github.com/skuodi/SBS_SMB#References)
- [TODO](https://github.com/skuodi/SBS_SMB#TODO)

## Features

The SMBus implementaiton (**smbus_if**) supports the following SMBus functions
```c

- SMBusSendByte()
- SMBusReceiveByte()
- SMBusWriteByte()
- SMBusWriteWord()
- SMBusReadByte()
- SMBusReadWord()
- SMBusProcessCall()
- SMBusBlockWrite()
- SMBusBlockRead()
- SMBusBlockWriteBlockReadProcessCall()
- SMBusWrite32()
- SMBusRead32()
- SMBusWrite64()
- SMBusRead64()

```

The SBS implementation supports all SBS commands in read mode.

## Installation

- The test code in the examples folder is written for ATMEGA32U4 in Arduino(for convenience of logging over USB) but should work on any ATMEGA.

1. Clone the repo and copy the `sbs_smb` folder into the `Arduino` > `libraries` folder

2. Restart your Arduino IDE, go to open `File` > `Examples` > `sbs_smb` > `ATMEGA32U4_SBS_SMB`

If debug logging is enabled by defining `SBS_PRINT_LOG` in **sbs_smb.cpp** , a lot of overhead is generated so the program doesn't fit if many functions are used. To use multiple functions, disable debug logging and implement the logging yourself using the SMBus return statuses in **smbus_if.h**.

## Porting

The library was created with portability in mind, hence the split into two translation units. To port it to a different microcontroller, you'll have to implement the SMBus functions in **smbus_if.h** according to your MCU phy.
To aid this process, each SMBus function contains an @sequence tag in the function description which outlines the protocol elements that make up the function.
- Elements from the host are in upper case, elements from the peripheral are in lowercase.

| Symbol | Protocol Element |
|--------|------------------|
|	A	 | Host Acknowledge |
|	N	 | Host Not Acknowledge |
|	a	 | Peripheral Acknowledge |
|	S	 | Start Condition |
|	Sr	 | Repeated Start |
|	P	 | Stop Condition |
|	W	 | Write Bit |
|	R	 | Read bit |
| ADDRESS| Peripheral address sent from host |
|DATA BYTE| Byte of data from host |
|data byte| Byte of data from peripheral |

For example:
```c
@sequence:   S,ADDRESS,W,a,P

```
means there's a **Start condition** followed by the **peripheral device address** sent from host with a **write bit**, **acknowledged** by the peripheral and finally a **stop condition**

## TODO
- [ ] Implement Packet Error Checking (PEC)
- [ ] Implement the  Adress Resolution Protocol (ARP)
- [ ] Add an event timeout
	At the moment, an `while()` loop is used to wait for events to occur. For example if no pullup resistor on the bus lines, a start condition cannot occur and the MCU will be stuck in an infinite wait state until said condition occurs.


## References
1. [SMBus v3.1 Data specification](http://smbus.org/specs/SMBus_3_1_20180319.pdf)
2. [Smart Battery Data Specification v1.1](http://www.smartbattery.org/specs/sbdat110.pdf)