// Freenove_WS2812B_RGBLED_Controller.cpp
/**
 * Brief	Apply to Freenove WS2812B RGBLED Controller.
 *			You can use I2C or UART to communicate.
 * Author	SuhaylZhao
 * Company	Freenove
 * Date		2021-02-04
 */

#include "Freenove_WS2812B_RGBLED_Controller.h"

int Freenove_WS2812B_Controller::writeRegOneByte(uint8_t val)
{
	Wire.beginTransmission(I2C_Address);
	Wire.write(val);
	int error = Wire.endTransmission();
	return error;
}

int Freenove_WS2812B_Controller::writeReg(uint8_t cmd, u8 * value, u8 size_a)
{
	Wire.beginTransmission(I2C_Address);
	Wire.write(cmd);
	Wire.write(value, size_a);
	int error = Wire.endTransmission();
	return error;
}

int Freenove_WS2812B_Controller::readReg(uint8_t cmd, char * recv, u16 count)
{
	Wire.beginTransmission(I2C_Address);
	Wire.write(cmd);
	Wire.endTransmission(false);
	Wire.requestFrom(I2C_Address, count);
	//Wire.requestFrom(I2C_Address, count, cmd, 1, true);
	int i = 0;
	while (Wire.available()) {
		recv[i++] = Wire.read();
		if (i == count)
		{
			break;
		}
	}
	int error = Wire.endTransmission();
	return error;
}

bool Freenove_WS2812B_Controller::uartWriteDataToControllerWithAck(u8 param[5], bool isShowLed)
{
	_serial->flush();
	while (_serial->available() > 0)
	{
		_serial->read();
	}
	u8 arr[8] = { UART_START_BYTE,param[0],param[1],param[2],param[3],param[4],UART_END_BYTE,0 };
	arr[7] = (u8)(arr[0] + arr[1] + arr[2] + arr[3] + arr[4] + arr[5] + arr[6]);
	_serial->write(arr, sizeof(arr));
	if (isShowLed)
	{
		if (uartWaitAckTime > 16383)        //delayMicroseconds max:16383
		{
			delay(uartWaitAckTime / 1000 + 1);
		}
		else
		{
			delayMicroseconds(uartWaitAckTime);
		}
	}
	else
	{
		delayMicroseconds(9000);
	}

	u8 ack;
	while (_serial->available() > 0)
	{
		ack = _serial->read();
	}
	if (ack == UART_ACK_BYTE)
	{
		return true;
	}
	else
	{
		return false;
	}
}

Freenove_WS2812B_Controller::Freenove_WS2812B_Controller(u8 _address, u16 n, LED_TYPE t)
{
	commMode = I2C_COMMUNICATION_MODE;
	I2C_Address = (u8)_address;
	ledCounts = n;
	uartWaitAckTime = (280 + ledCounts * 40) + 10000;    //
	setLedType(t);
}

Freenove_WS2812B_Controller::Freenove_WS2812B_Controller(HardwareSerial *serial_param, u16 n, LED_TYPE t, u32 _baudrate)
{
	uartBaudrateIndex = 0;
	for (int i = 0; i < 13; i++)
	{
		if (_BAUDRATE[i] == _baudrate)
		{
			uartBaudrateIndex = i;
			break;
		}
	}
	commMode = UART_COMMUNICATION_MODE;
	ledCounts = n;
	uartWaitAckTime = (280 + ledCounts * 40) + 10000;   //
	_serial = serial_param;
	setLedType(t);
}

bool Freenove_WS2812B_Controller::begin()
{
	if (commMode == I2C_COMMUNICATION_MODE)
	{
		Wire.begin();
	}
	else
	{
		_serial->begin(_BAUDRATE[uartBaudrateIndex]);
	}

	return setLedCount(ledCounts);
}

bool Freenove_WS2812B_Controller::setLedCount(u16 n)
{
	if (commMode == I2C_COMMUNICATION_MODE)
	{
		u8 arr[] = { n };
		return !writeReg(REG_LEDS_COUNTS, arr, sizeof(arr));
	}
	else
	{
		u8 arr[] = { REG_LEDS_COUNTS,n,0,0,0 };
		return uartWriteDataToControllerWithAck(arr);
	}
}

void Freenove_WS2812B_Controller::setLedType(LED_TYPE t)
{
	rOffset = (t >> 4) & 0x03;
	gOffset = (t >> 2) & 0x03;
	bOffset = t & 0x03;
}

bool Freenove_WS2812B_Controller::setLedColorData(u8 index, u32 rgb)
{
	return setLedColorData(index, rgb >> 16, rgb >> 8, rgb);
}

bool Freenove_WS2812B_Controller::setLedColorData(u8 index, u8 r, u8 g, u8 b)
{
	u8 p[3];
	p[rOffset] = r;
	p[gOffset] = g;
	p[bOffset] = b;
	if (commMode == I2C_COMMUNICATION_MODE)
	{
		u8 arr[] = { index, p[0], p[1], p[2] };
		return !writeReg(REG_SET_LED_COLOR_DATA, arr, sizeof(arr));
	}
	else
	{
		u8 arr[] = { REG_SET_LED_COLOR_DATA,index,p[0], p[1], p[2] };
		return uartWriteDataToControllerWithAck(arr);
	}
}

bool Freenove_WS2812B_Controller::setLedColor(u8 index, u32 rgb)
{
	return setLedColor(index, rgb >> 16, rgb >> 8, rgb);;
}

bool Freenove_WS2812B_Controller::setLedColor(u8 index, u8 r, u8 g, u8 b)
{
	u8 p[3];
	p[rOffset] = r;
	p[gOffset] = g;
	p[bOffset] = b;
	if (commMode == I2C_COMMUNICATION_MODE)
	{
		u8 arr[] = { index, p[0], p[1], p[2] };
		return !writeReg(REG_SET_LED_COLOR, arr, sizeof(arr));
	}
	else
	{
		u8 arr[] = { REG_SET_LED_COLOR,index,p[0], p[1], p[2] };
		return uartWriteDataToControllerWithAck(arr, true);
	}
}

bool Freenove_WS2812B_Controller::setAllLedsColorData(u32 rgb)
{
	setAllLedsColorData(rgb >> 16, rgb >> 8, rgb);
}

bool Freenove_WS2812B_Controller::setAllLedsColorData(u8 r, u8 g, u8 b)
{
	u8 p[3];
	p[rOffset] = r;
	p[gOffset] = g;
	p[bOffset] = b;
	if (commMode == I2C_COMMUNICATION_MODE)
	{
		u8 arr[] = { p[0], p[1], p[2] };
		return !writeReg(REG_SET_ALL_LEDS_COLOR_DATA, arr, sizeof(arr));
	}
	else
	{
		u8 arr[] = { REG_SET_ALL_LEDS_COLOR_DATA,p[0], p[1], p[2],0 };
		return uartWriteDataToControllerWithAck(arr);
	}
}

bool Freenove_WS2812B_Controller::setAllLedsColor(u32 rgb)
{
	setAllLedsColor(rgb >> 16, rgb >> 8, rgb);
}

bool Freenove_WS2812B_Controller::setAllLedsColor(u8 r, u8 g, u8 b)
{
	u8 p[3];
	p[rOffset] = r;
	p[gOffset] = g;
	p[bOffset] = b;
	if (commMode == I2C_COMMUNICATION_MODE)
	{
		u8 arr[] = { p[0], p[1], p[2] };
		return !writeReg(REG_SET_ALL_LEDS_COLOR, arr, sizeof(arr));
	}
	else
	{
		u8 arr[] = { REG_SET_ALL_LEDS_COLOR,p[0], p[1], p[2],0 };
		return uartWriteDataToControllerWithAck(arr, true);
	}
}

bool Freenove_WS2812B_Controller::show()
{
	if (commMode == I2C_COMMUNICATION_MODE)
	{
		return !writeRegOneByte(REG_TRANS_DATA_TO_LED);
	}
	else
	{
		u8 arr[] = { REG_TRANS_DATA_TO_LED,0,0,0,0 };
		return uartWriteDataToControllerWithAck(arr, true);
	}
}

u8 Freenove_WS2812B_Controller::getLedsCountFromController()
{
	if (commMode == I2C_COMMUNICATION_MODE)
	{
		char recv[1];
		readReg(REG_LEDS_COUNT_READ, recv, 1);
		return recv[0];
	}
	else
	{
		_serial->flush();
		while (_serial->available() > 0)
		{
			_serial->read();
		}
		u8 arr[8] = { UART_START_BYTE,REG_LEDS_COUNT_READ,0,0,0,0,UART_END_BYTE,0 };
		arr[7] = (u8)(arr[0] + arr[1] + arr[2] + arr[3] + arr[4] + arr[5] + arr[6]);
		_serial->write(arr, sizeof(arr));
		delay(10);
		u8 recv = 0;
		while (_serial->available() > 0)
		{
			recv = _serial->read();
		}
		return recv;
	}
}

u8 Freenove_WS2812B_Controller::getI2CAddress()
{
	if (commMode == I2C_COMMUNICATION_MODE)
	{
		return I2C_Address;
	}
	else
	{
		_serial->flush();
		while (_serial->available() > 0)
		{
			_serial->read();
		}
		u8 arr[8] = { UART_START_BYTE,REG_READ_I2C_ADDRESS,0,0,0,0,UART_END_BYTE,0 };
		arr[7] = (u8)(arr[0] + arr[1] + arr[2] + arr[3] + arr[4] + arr[5] + arr[6]);
		_serial->write(arr, sizeof(arr));
		delay(10);
		u8 i2c_Address;
		while (_serial->available() > 0)
		{
			i2c_Address = _serial->read();
		}
		return i2c_Address;		//7bit Address
	}
}

u32 Freenove_WS2812B_Controller::getUartBaudrate()
{
	if (commMode == I2C_COMMUNICATION_MODE)
	{
		char recv[1];
		readReg(REG_GET_UART_BAUDRATE, recv, 1);
		return _BAUDRATE[recv[0]];
	}
	else
	{
		return _BAUDRATE[uartBaudrateIndex];
	}
}

bool Freenove_WS2812B_Controller::setUartBaudrate(u32 _baudrate)
{
	for (int i = 0; i < 13; i++)
	{
		if (_BAUDRATE[i] == _baudrate)
		{
			if (commMode == I2C_COMMUNICATION_MODE)
			{
				u8 arr[] = { SECTET_KEY_A, SECTET_KEY_B, i };
				return !writeReg(REG_SET_UART_BAUDRATE, arr, sizeof(arr));
			}
			else
			{
				u8 arr[] = { REG_SET_UART_BAUDRATE,SECTET_KEY_A,SECTET_KEY_B,i,0 };
				return uartWriteDataToControllerWithAck(arr);
			}
			break;
		}
	}
}

bool Freenove_WS2812B_Controller::setI2CNewAddress(u8 addr)
{
	if (commMode == I2C_COMMUNICATION_MODE)
	{
		u8 arr[] = { SECTET_KEY_A, SECTET_KEY_B, addr };
		if (writeReg(REG_SET_I2C_ADDRESS, arr, sizeof(arr)) == 0)
		{
			I2C_Address = addr;
			return true;
		}
		else
		{
			return false;
		}
	}
	else
	{
		u8 arr[] = { REG_SET_I2C_ADDRESS,SECTET_KEY_A,SECTET_KEY_B,addr,0 };
		if (uartWriteDataToControllerWithAck(arr))
		{
			I2C_Address = addr;
			return true;
		}
		else
		{
			return false;
		}
	}
}

String Freenove_WS2812B_Controller::getBrand()
{
	if (commMode == I2C_COMMUNICATION_MODE)
	{
		char recv[STRING_BRAND_LENGTH];
		readReg(REG_GET_BRAND, recv, STRING_BRAND_LENGTH);
		return String(recv);
	}
	else
	{
		_serial->flush();
		while (_serial->available() > 0)
		{
			_serial->read();
		}
		u8 arr[8] = { UART_START_BYTE,REG_GET_BRAND,0,0,0,0,UART_END_BYTE,0 };
		arr[7] = (u8)(arr[0] + arr[1] + arr[2] + arr[3] + arr[4] + arr[5] + arr[6]);
		_serial->write(arr, sizeof(arr));
		delay(10);
		char brandChar[STRING_BRAND_LENGTH] = { 0 };
		u8 i = 0;
		while (_serial->available() > 0)
		{
			brandChar[i++] = _serial->read();
			if (i == STRING_BRAND_LENGTH)
			{
				break;
			}
		}
		return String(brandChar);
	}
}

String Freenove_WS2812B_Controller::getFirmwareVersion()
{
	if (commMode == I2C_COMMUNICATION_MODE)
	{
		char recv[STRING_VERSION_LENGTH];
		readReg(REG_GET_FIRMWARE_VERSION, recv, STRING_VERSION_LENGTH);
		return String(recv);
	}
	else
	{
		_serial->flush();
		while (_serial->available() > 0)
		{
			_serial->read();
		}
		u8 arr[8] = { UART_START_BYTE,REG_GET_FIRMWARE_VERSION,0,0,0,0,UART_END_BYTE,0 };
		arr[7] = (u8)(arr[0] + arr[1] + arr[2] + arr[3] + arr[4] + arr[5] + arr[6]);
		_serial->write(arr, sizeof(arr));
		delay(10);
		char brandChar[STRING_VERSION_LENGTH] = { 0 };
		u8 i = 0;
		while (_serial->available() > 0)
		{
			brandChar[i++] = _serial->read();
			if (i == STRING_VERSION_LENGTH)
			{
				break;
			}
		}
		return String(brandChar);
	}
}

uint32_t Freenove_WS2812B_Controller::Wheel(byte pos)
{
	u32 WheelPos = pos % 0xff;
	if (WheelPos < 85) {
		return ((255 - WheelPos * 3) << 16) | ((WheelPos * 3) << 8);
	}
	if (WheelPos < 170) {
		WheelPos -= 85;
		return (((255 - WheelPos * 3) << 8) | (WheelPos * 3));
	}
	WheelPos -= 170;
	return ((WheelPos * 3) << 16 | (255 - WheelPos * 3));
}
