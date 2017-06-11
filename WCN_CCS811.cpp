/*


*/

#ifdef __AVR
  #include <avr/pgmspace.h>
#elif defined(ESP8266)
  #include <pgmspace.h>
#endif

#include "Arduino.h"

#include "WCN_CCS811.h"
#include <Wire.h>

bool WCN_CCS811::begin(byte addr, int wake_pin) {
  bool result = true;

  if (addr != 0x5A && addr != 0x5B) addr = 0x5A;
  else _addr = addr;

  if (wake_pin = -1) {
    _wake_pin = 0;
    _handle_wake = false;
  } else {
    _wake_pin = wake_pin;
    _handle_wake = true;
  }

  Wire.begin();

  if (_handle_wake) pinMode(_wake_pin, OUTPUT);   // set WAKE pin as OUTPUT

  wakeAssert();

  //Verify the hardware ID is what we expect
  byte hwID = readRegister(0x20); //Hardware ID should be 0x81
  if (hwID == 0x81)
  {
    byte error = WCN_CCS811::checkForError();
    if ( error == 0)
    {
      if (WCN_CCS811::appValid())
      {
        //Write to this register to start app
        Wire.beginTransmission(CCS811_ADDR);
        Wire.write(CCS811_APP_START);
        Wire.endTransmission();

        //Set Drive Mode
        WCN_CCS811::setDriveMode(CCS811_DRIVE_MODE1); //Read every second

        error = WCN_CCS811::checkForError();
        if ( error != 0)
        {
#ifdef DEBUG
          Serial.print("Error Second error check: ");
          Serial.println(error, HEX);
          result = false;
#endif
        }
      }
      else
      {
#ifdef DEBUG
        Serial.print("Error App Invalid");
        result = false;
#endif
      }
    }
    else
    {
#ifdef DEBUG
      Serial.print("Error First error check: ");
      Serial.println(error, HEX);
      result = false;
#endif
    }
  }
  else
  {
#ifdef DEBUG
    Serial.print("Error Hardware ID: ");
    Serial.println(hwID, HEX);
    result = false;
#endif
  }

  wakeRelease();

  return result;
}

byte WCN_CCS811::readData()
{
  wakeAssert();

  byte value = readRegister(CCS811_STATUS);
  bool dataready = value & 1 << 3;

  if (dataready)
  {
    Wire.beginTransmission(CCS811_ADDR);
    Wire.write(CCS811_ALG_RESULT_DATA);
    Wire.endTransmission();

    Wire.requestFrom(CCS811_ADDR, 4); //Get four bytes

    byte co2MSB = Wire.read();
    byte co2LSB = Wire.read();
    byte tvocMSB = Wire.read();
    byte tvocLSB = Wire.read();

    CO2 = ((unsigned int)co2MSB << 8) | co2LSB;
    tVOC = ((unsigned int)tvocMSB << 8) | tvocLSB;
  }

  wakeRelease();

  return WCN_CCS811::checkForError();
}

unsigned int WCN_CCS811::getCO2()
{
  return CO2;
}

unsigned int WCN_CCS811::getTVOC()
{
  return tVOC;
}

byte WCN_CCS811::checkForError(void) {
  wakeAssert();

  byte value = readRegister(CCS811_STATUS);
  bool error = value & 1 << 0;
  byte result = 0;

  if (error) result = readRegister(CCS811_ERROR_ID);

  wakeRelease();

  return result;
}

bool WCN_CCS811::appValid()
{
  wakeAssert();

  byte value = readRegister(CCS811_STATUS);

  wakeRelease();

  return (value & 1 << 4);
}

//Returns the baseline value
//Used for telling sensor what 'clean' air is
//You must put the sensor in clean air and record this value
unsigned int WCN_CCS811::getBaseline()
{
  wakeAssert();

  Wire.beginTransmission(CCS811_ADDR);
  Wire.write(CCS811_BASELINE);
  Wire.endTransmission();

  Wire.requestFrom(CCS811_ADDR, 2); //Get two bytes

  byte baselineMSB = Wire.read();
  byte baselineLSB = Wire.read();

  wakeRelease();

  unsigned int baseline = ((unsigned int)baselineMSB << 8) | baselineLSB;

  return (baseline);
}

//Checks to see if DATA_READY flag is set in the status register
bool WCN_CCS811::dataAvailable()
{
  wakeAssert();

  byte value = readRegister(CCS811_STATUS);

  wakeRelease();

  return (value & 1 << 3);
}

//Enable the nINT signal
void WCN_CCS811::enableInterrupts(void)
{
  wakeAssert();

  byte setting = readRegister(CCS811_MEAS_MODE); //Read what's currently there
  setting |= (1 << 3); //Set INTERRUPT bit
  writeRegister(CCS811_MEAS_MODE, setting);

  wakeRelease();
}

//Disable the nINT signal
void WCN_CCS811::disableInterrupts(void)
{
  wakeAssert();

  byte setting = readRegister(CCS811_MEAS_MODE); //Read what's currently there
  setting &= ~(1 << 3); //Clear INTERRUPT bit
  writeRegister(CCS811_MEAS_MODE, setting);

  wakeRelease();
}

//Mode 0 = Idle
//Mode 1 = read every 1s
//Mode 2 = every 10s
//Mode 3 = every 60s
//Mode 4 = RAW mode
void WCN_CCS811::setDriveMode(byte mode)
{
  if (mode > 4) mode = 4; //Error correction

  wakeAssert();

  byte setting = readRegister(CCS811_MEAS_MODE); //Read what's currently there

  setting &= ~(0b00000111 << 4); //Clear DRIVE_MODE bits
  setting |= (mode << 4); //Mask in mode
  writeRegister(CCS811_MEAS_MODE, setting);

  wakeRelease();
}

//Given a temp and humidity, write this data to the CSS811 for better compensation
//This function expects the humidity and temp to come in as floats
void WCN_CCS811::setEnvironmentalData(float relativeHumidity, float temperature)
{
  int rH = relativeHumidity * 1000; //42.348 becomes 42348
  int temp = temperature * 1000; //23.2 becomes 23200

  byte envData[4];

  //Split value into 7-bit integer and 9-bit fractional
  envData[0] = ((rH % 1000) / 100) > 7 ? (rH / 1000 + 1) << 1 : (rH / 1000) << 1;
  envData[1] = 0; //CCS811 only supports increments of 0.5 so bits 7-0 will always be zero
  if (((rH % 1000) / 100) > 2 && (((rH % 1000) / 100) < 8))
  {
    envData[0] |= 1; //Set 9th bit of fractional to indicate 0.5%
  }

  temp += 25000; //Add the 25C offset
  //Split value into 7-bit integer and 9-bit fractional
  envData[2] = ((temp % 1000) / 100) > 7 ? (temp / 1000 + 1) << 1 : (temp / 1000) << 1;
  envData[3] = 0;
  if (((temp % 1000) / 100) > 2 && (((temp % 1000) / 100) < 8))
  {
    envData[2] |= 1;  //Set 9th bit of fractional to indicate 0.5C
  }

  wakeAssert();

  Wire.beginTransmission(CCS811_ADDR);
  Wire.write(CCS811_ENV_DATA); //We want to write our RH and temp data to the ENV register
  Wire.write(envData[0]);
  Wire.write(envData[1]);
  Wire.write(envData[2]);
  Wire.write(envData[3]);
  Wire.endTransmission();

  wakeRelease();
}

void WCN_CCS811::reset(void) {
  wakeAssert();

  Wire.beginTransmission(CCS811_ADDR);
  Wire.write(CCS811_SW_RESET);
  Wire.write(0x11);
  Wire.write(0xE5);
  Wire.write(0x72);
  Wire.write(0x8A);
  Wire.endTransmission();

  wakeRelease();
}

void WCN_CCS811::sensorInfo(void) {
  byte versiondata[4];

  wakeAssert();

  byte hwversion = readRegister(CCS811_HW_VERSION);

  // Read Firmware Boot Version
  Wire.beginTransmission(CCS811_ADDR);
  Wire.write(CCS811_FW_BOOT_VERSION);
  Wire.endTransmission();
  Wire.requestFrom(CCS811_ADDR, 2);
  versiondata[0] = Wire.read();
  versiondata[1] = Wire.read();

  // Read Firmware App Version
  Wire.beginTransmission(CCS811_ADDR);
  Wire.write(CCS811_FW_APP_VERSION);
  Wire.endTransmission();
  Wire.requestFrom(CCS811_ADDR, 2);
  versiondata[2] = Wire.read();
  versiondata[3] = Wire.read();

  wakeRelease();

  Serial.print("Hardware version: 0x");
  Serial.println(hwversion, HEX);

  byte bmajor = versiondata[0] >> 4;
  byte bminor = versiondata[0] & B00001111;

  Serial.print("Firmware Boot version: ");
  Serial.print(bmajor);
  Serial.print(".");
  Serial.print(bminor);
  Serial.print(".");
  Serial.println(versiondata[1]);

  byte amajor = versiondata[2] >> 4;
  byte aminor = versiondata[2] & B00001111;

  Serial.print("Firmware App version: ");
  Serial.print(amajor);
  Serial.print(".");
  Serial.print(aminor);
  Serial.print(".");
  Serial.println(versiondata[3]);
}

//Reads from a give location from the CSS811
byte WCN_CCS811::readRegister(byte addr)
{
  Wire.beginTransmission(CCS811_ADDR);
  Wire.write(addr);
  Wire.endTransmission();

  Wire.requestFrom(CCS811_ADDR, 1);

  return (Wire.read());
}

//Write a value to a spot in the CCS811
void WCN_CCS811::writeRegister(byte addr, byte val)
{
  Wire.beginTransmission(CCS811_ADDR);
  Wire.write(addr);
  Wire.write(val);
  Wire.endTransmission();
}

void WCN_CCS811::wakeAssert(void) {
  if (!_handle_wake) return;

  digitalWrite(_wake_pin, LOW);
  delayMicroseconds(500);
}

void WCN_CCS811::wakeRelease(void) {
  if (!_handle_wake) return;

  digitalWrite(_wake_pin, HIGH);
  delayMicroseconds(40);
}

