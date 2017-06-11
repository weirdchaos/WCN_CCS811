/******************************************






******************************************/

//#define DEBUG

#ifndef _WCN_CCS811_H
#define _WCN_CCS811_H

#define CCS811_ADDR 0x5A //7-bit unshifted default I2C Address

//Register addresses
#define CCS811_STATUS           0x00
#define CCS811_MEAS_MODE        0x01
#define CCS811_ALG_RESULT_DATA  0x02
#define CCS811_RAW_DATA         0x03
#define CCS811_ENV_DATA         0x05
#define CCS811_NTC              0x06
#define CCS811_THRESHOLDS       0x10
#define CCS811_BASELINE         0x11
#define CCS811_HW_ID            0x20
#define CCS811_HW_VERSION       0x21
#define CCS811_FW_BOOT_VERSION  0x23
#define CCS811_FW_APP_VERSION   0x24
#define CCS811_ERROR_ID         0xE0
#define CCS811_APP_START        0xF4
#define CCS811_SW_RESET         0xFF

// Drive modes
#define CCS811_DRIVE_MODE0      0
#define CCS811_DRIVE_MODE1      1
#define CCS811_DRIVE_MODE2      2
#define CCS811_DRIVE_MODE3      3
#define CCS811_DRIVE_MODE4      4

// Error codes
#define CCS811_ERR_MSG_INVALID   B00000001
#define CCS811_READ_REG_INVALID  B00000010
#define CCS811_MEASMODE_INVALID  B00000100
#define CCS811_MAX_RESISTANCE    B00001000
#define CCS811_HEATER_FAULT      B00010000
#define CCS811_HEATER_SUPPLY     B00100000

class WCN_CCS811 {
public:
	bool begin(byte addr, int wake_pin = -1);
	bool begin(void);
	byte readData();
	unsigned int getCO2(void);
	unsigned int getTVOC(void);
	byte checkForError(void);
	byte	getError(void);
	unsigned int getBaseline(void);
	void enableInterrupts(void);
	void disableInterrupts(void);
	void setDriveMode(byte mode);
	void setEnvironmentalData(float relativeHumidity, float temperature);
	bool dataAvailable(void);
	void sensorInfo(void);
	void reset(void);

private:
	byte _addr;
	byte _wake_pin;
	bool _handle_wake;
	//These are the air quality values obtained from the sensor
	unsigned int tVOC;
	unsigned int CO2;

	bool appValid(void);

	byte readRegister(byte addr);
	void writeRegister(byte addr, byte val);

	void wakeAssert(void);
	void wakeRelease(void);
};

#endif

