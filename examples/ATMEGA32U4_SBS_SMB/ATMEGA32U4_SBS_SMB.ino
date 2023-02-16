#include <sbs_smb.h>
#include <smbus_if.h>

char dataBuf[100] = { 0 };

void EchoPrint(char *buf) {
  Serial.print(buf);
}

//scan for devices present on the bus. If you've connected a smart battery to the AVR I2C pins
//it should appear at address 0x0B
void I2CTest() {
  Serial.println("Scanning...");
  for (int i = 0; i < 128; i++) {
    if (SMBusQuickCommand_NonCompliant(i * 2)) {
      Serial.print(".");
    } else {
      Serial.print("Found 0x");
      Serial.println(i, HEX);
      return;
    }
  }
}

void setup() {
  SMBusInit();
  Serial.begin(9600);
  delay(5000);

  //scan for I2C/SMBus devices. Smart batteries show up at address 0x0B.
  I2CTest();
}

void loop() {
  //uncomment a function to view its output

  ReadVoltage();
  ReadRelativeStateOfCharge();
  ReadRemainingCapacity();
  ReadCycleCount();
  ReadManufactureDate();


  delay(2000);
}
