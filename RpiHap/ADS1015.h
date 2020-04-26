/*
	Copyright(c) 2020 Gera Kazakov
	SPDX-License-Identifier: Apache-2.0
*/

#ifndef _ADS1015_H_
#define _ADS1015_H_

// ADS1015 ADC on I2C bus
class ADS1015
{
private:
	uint8_t _bus;
	uint8_t _addr;
	char _dev[16];
	int _f = -1;

protected:
	// Registers
	static const uint8_t Result = 0;
	static const uint8_t Config = 1;
	static const uint8_t LoThresh = 2;
	static const uint8_t HiThresh = 3;

	// Config
	// Bit [15] OS: Operational status/single-shot conversion start
	//	This bit determines the operational status of the device.
	//	This bit can only be written when in power - down mode.
	//		For a write status :
	//			0 : No effect
	//			1 : Begin a single conversion(when in power - down mode)
	//		For a read status :
	//			0 : Device is currently performing a conversion
	//			1 : Device is not currently performing a conversion
	static const uint16_t OS = (1 << 15);
	// Bits [14:12] MUX[2:0]: Input multiplexer configuration (ADS1015 only)
	//	These bits configure the input multiplexer.
	static const uint16_t MUX000 = (0 << 12);	//	000 : AINP = AIN0 and AINN = AIN1 (default)
	static const uint16_t MUX001 = (1 << 12);	//	001 : AINP = AIN0 and AINN = AIN3
	static const uint16_t MUX010 = (2 << 12);	//	010 : AINP = AIN1 and AINN = AIN3
	static const uint16_t MUX011 = (3 << 12);	//	011 : AINP = AIN2 and AINN = AIN3
	static const uint16_t MUX100 = (4 << 12);	//	100 : AINP = AIN0 and AINN = GND
	static const uint16_t MUX101 = (5 << 12);	//	101 : AINP = AIN1 and AINN = GND
	static const uint16_t MUX110 = (6 << 12);	//	110 : AINP = AIN2 and AINN = GND
	static const uint16_t MUX111 = (7 << 12);	//	111 : AINP = AIN3 and AINN = GND
	// Bits[11:9] PGA[2:0]: Programmable gain amplifier configuration(ADS1014 and ADS1015 only)
	//	These bits configure the programmable gain amplifier.
	static const uint16_t PGA000 = (0 << 9);	//	000 : FS = ±6.144V
	static const uint16_t PGA001 = (1 << 9);	//	001 : FS = ±4.096V
	static const uint16_t PGA010 = (2 << 9);	//	010 : FS = ±2.048V (default)
	static const uint16_t PGA011 = (3 << 9);	//	011 : FS = ±1.024V
	static const uint16_t PGA100 = (4 << 9);	//	100 : FS = ±0.512V
	static const uint16_t PGA101 = (5 << 9);	//	101 : FS = ±0.256V
	static const uint16_t PGA110 = (6 << 9);	//	110 : FS = ±0.256V
	static const uint16_t PGA111 = (7 << 9);	//	111 : FS = ±0.256V
	// Bit [8] MODE: Device operating mode
	//	This bit controls the current operational mode of the ADS1013/4/5.
	static const uint16_t MODE_C = (0 << 8);	//	0 : Continuous conversion mode
	static const uint16_t MODE_S = (1 << 8);	//	1 : Power-down single-shot mode
	// Bits [7:5] DR[2:0]: Data rate
	//	These bits control the data rate setting.
	static const uint16_t DR000 = (0 << 5);		//	000 : 128SPS
	static const uint16_t DR001 = (1 << 5);		//	001 : 250SPS
	static const uint16_t DR010 = (2 << 5);		//	010 : 490SPS
	static const uint16_t DR011 = (3 << 5);		//	011 : 920SPS
	static const uint16_t DR100 = (4 << 5);		//	100 : 1600SPS (default)
	static const uint16_t DR101 = (5 << 5);		//	101 : 2400SPS
	static const uint16_t DR110 = (6 << 5);		//	110 : 3300SPS
	static const uint16_t DR111 = (7 << 5);		//	111 : 3300SPS
	//Bit[4] COMP_MODE: Comparator mode(ADS1014 and ADS1015 only)
	//	This bit controls the comparator mode of operation.It changes whether the comparator is implemented
	//	as a traditional comparator(COMP_MODE = '0')
	//	or as a window comparator(COMP_MODE = '1').
	static const uint16_t COMP_HYS = (0 << 4);	//	0 : Traditional comparator with hysteresis(default)
	static const uint16_t COMP_WIN = (1 << 4);	//	1 : Window comparator
	// Bit [3] COMP_POL: Comparator polarity (ADS1014 and ADS1015 only)
	//	This bit controls the polarity of the ALERT/RDY pin.
	//	When COMP_POL = '0' the comparator output is active	low.
	//	When COMP_POL = '1' the ALERT/RDY pin is active high.
	static const uint16_t COMP_LOW = (0 << 3);	//	0 : Active low(default)
	static const uint16_t COMP_HIGH = (1 << 3);	//	1 : Active high
	// Bit[2] COMP_LAT: Latching comparator(ADS1014 and ADS1015 only)
	//	This bit controls whether the ALERT/RDY pin latches once asserted
	//	or clears once conversions are within the margin of the upper and lower threshold values.
	//	When COMP_LAT = '0', the ALERT/RDY pin does not latch when asserted.
	//	When COMP_LAT = '1', the asserted ALERT/RDY pin remains latched until conversion data are read by the master
	static const uint16_t COMP_NLAT = (0 << 2);	//	0 : Non - latching comparator(default)
	static const uint16_t COMP_LAT = (1 << 2);	//	1 : Latching comparator
	// Bits [1:0] COMP_QUE: Comparator queue and disable (ADS1014 and ADS1015 only)
	//	These bits perform two functions.
	//	When set to '11', they disable the comparator function and put the ALERT/RDY pin into a high state.
	//	When set to any other value, they control the number of successive conversions exceeding the upper
	//	or lower thresholds required before asserting the ALERT/RDY pin.
	static const uint16_t CQ00 = (0 << 0);		//	00 : Assert after one conversion
	static const uint16_t CQ01 = (1 << 0);		//	01 : Assert after two conversions
	static const uint16_t CQ10 = (2 << 0);		//	10 : Assert after four conversions
	static const uint16_t CQ11 = (3 << 0);		//	11 : Disable comparator (default)

	// read register
	uint16_t reg(uint8_t r)
	{
		if (_f < 0)
		{
			Log::Msg("I2C not opened\n");
			return 0xFFFF;
		}

		// write pointer
		if (::write(_f, &r, 1) != 1)
		{
			Log::Msg("I2C write error\n");
			return 0xFFFF;
		}

		// read data
		uint8_t b[2];
		if (::read(_f, b, 2) != 2)
		{
			Log::Msg("I2C read data error\n");
			return 0xFFFF;
		}

		uint16_t d = ((uint16_t)b[0] << 8) | b[1];

		Log::Dbg("ADS reg%d: %04X\n", r, d);

		return d;
	}

	// write register
	void reg(uint8_t r, uint16_t d)
	{
		if (_f < 0)
		{
			Log::Msg("I2C not opened\n");
			return;
		}

		Log::Dbg("ADS reg%d= %04X\n", r, d);

		uint8_t b[3] = { r, uint8_t(d >> 8), uint8_t(d & 0xFF) };
		if (::write(_f, b, 3) != 3)
		{
			Log::Msg("I2C write data error\n");
		}
	}

	void start(uint16_t conf)
	{
		reg(Config, conf | OS);
		reg(Config);
	}

	bool ready()
	{
		return reg(Config)& OS;
	}

	uint16_t result()
	{
		return reg(Result);
	}

public:
	ADS1015()
	{
	}

	~ADS1015()
	{
		if (_f > 0)
			::close(_f);
	}

	bool Init(uint8_t bus, uint8_t addr)
	{
		_bus = bus;
		_addr = addr;

		snprintf(_dev, sizeof(_dev), "/dev/i2c-%d", _bus);

		_f = ::open(_dev, O_RDWR);
		if (_f < 0)
		{
			Log::Msg("ADS1015: Device %s open error\n", _dev);
			return false;
		}

		if (::ioctl(_f, I2C_SLAVE, _addr) < 0)
		{
			Log::Msg("ADS1015: Device %s set addr 0x%02X error\n", _dev, _addr);
			::close(_f);
			return false;
		}

		if (reg(Result) == 0xFFFF)
		{
			Log::Msg("ADS1015: Device %s addr 0x%02X read error\n", _dev, _addr);
			::close(_f);
			return false;
		}

		return true;
	}
};


#endif /*ADS1015*/