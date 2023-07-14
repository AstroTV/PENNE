// Freenove_WS2812B_RGBLED_Controller.h
/**
 * Brief	Apply to Freenove WS2812B RGBLED Controller.
 *			You can use I2C or UART to communicate.
 * Author	SuhaylZhao
 * Company	Freenove
 * Date		2019-08-03
 */

#ifndef _FREENOVE_WS2812B_RGBLED_CONTROLLER_h
#define _FREENOVE_WS2812B_RGBLED_CONTROLLER_h

#if defined(ARDUINO) && ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif

#include <Wire.h>
 //#include <SoftwareSerial.h>


#define REG_LEDS_COUNTS					0
#define REG_SET_LED_COLOR_DATA			1
#define REG_SET_LED_COLOR				2
#define REG_SET_ALL_LEDS_COLOR_DATA		3
#define REG_SET_ALL_LEDS_COLOR			4
#define REG_TRANS_DATA_TO_LED			5

#define REG_LEDS_COUNT_READ				0xfa

#define REG_READ_I2C_ADDRESS			0xfb
#define REG_GET_UART_BAUDRATE			0xfb

#define REG_SET_UART_BAUDRATE			0xfc
#define REG_SET_I2C_ADDRESS				0xfd

#define REG_GET_BRAND					0xfe
#define REG_GET_FIRMWARE_VERSION		0xff

#define I2C_COMMUNICATION_MODE			0
#define UART_COMMUNICATION_MODE			1

#define STRING_BRAND_LENGTH				9
#define STRING_VERSION_LENGTH			16

#define SECTET_KEY_A					0xaa
#define SECTET_KEY_B					0xbb
#define UART_START_BYTE					0xcc
#define UART_END_BYTE					0xdd
#define UART_ACK_BYTE					0x06

//#define TYPE_RGB						0x06	
//#define TYPE_RBG						0x09	
//#define TYPE_GRB						0x12	
//#define TYPE_GBR						0x21	
//#define TYPE_BRG						0x18	
//#define TYPE_BGR						0x24	

enum LED_TYPE
{					  //R  G  B
	TYPE_RGB = 0x06,  //00 01 10
	TYPE_RBG = 0x09,  //00 10 01
	TYPE_GRB = 0x12,  //01 00 10
	TYPE_GBR = 0x21,  //10 00 01
	TYPE_BRG = 0x18,  //01 10 00
	TYPE_BGR = 0x24	  //10 01 00
};

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned long u32;

const u32 _BAUDRATE[] = { 115200, 1200, 2400, 4800, 9600, 14400, 19200, 38400, 57600, 115200, 128000, 230400, 500000 };

class Freenove_WS2812B_Controller
{
protected:
	u8 I2C_Address;
	u8 uartBaudrateIndex;
	u8 commMode;
	u16 ledCounts;
	u32 uartWaitAckTime;	//unit: us
	u8 rOffset;
	u8 gOffset;
	u8 bOffset;

	HardwareSerial *_serial;
	//HardwareSerial *hdSerial;
	//SoftwareSerial *swSerial;
	int writeRegOneByte(uint8_t val);
	int writeReg(uint8_t cmd, u8 *value, u8 size_a);
	int readReg(uint8_t cmd, char *recv, u16 count);
	//bool i2cWriteDataToController(u8 cmd, u8 *value, u8 size_a);
	bool uartWriteDataToControllerWithAck(u8 param[5], bool isShowLed = false);
	
public:
	Freenove_WS2812B_Controller(u8 _address = 0x20, u16 n = 8 , LED_TYPE t = TYPE_GRB);
	Freenove_WS2812B_Controller(HardwareSerial *serial_param, u16 n = 8, LED_TYPE t = TYPE_GRB, u32 _baudrate = 115200);
	//Freenove_WS2812B_Controller(SoftwareSerial *serial_param, u32 _baudrate = 115200, u16 n = 8);

	bool begin();
	bool setLedCount(u16 n);
	void setLedType(LED_TYPE t);

	bool setLedColorData(u8 index, u32 rgb);
	bool setLedColorData(u8 index, u8 r, u8 g, u8 b);

	bool setLedColor(u8 index, u32 rgb);
	bool setLedColor(u8 index, u8 r, u8 g, u8 b);

	bool setAllLedsColorData(u32 rgb);
	bool setAllLedsColorData(u8 r, u8 g, u8 b);

	bool setAllLedsColor(u32 rgb);
	bool setAllLedsColor(u8 r, u8 g, u8 b);

	bool show();

	u8 getLedsCountFromController();
	u8 getI2CAddress();
	u32 getUartBaudrate();
	bool setUartBaudrate(u32 _baudrate);
	bool setI2CNewAddress(u8 addr);
	String getBrand();
	String getFirmwareVersion();

	uint32_t Wheel(byte pos);
};

#endif

