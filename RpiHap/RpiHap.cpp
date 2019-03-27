/*
MIT License

Copyright (c) 2019 Gera Kazakov

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include <stdio.h>
#include <string>
#include <iostream>
#include <thread>
#include <cmath>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>

#include "Platform.h"

#include "Hap/Hap.h"
#include "Crypto/Crypto.h"
#include "Crypto/MD.h"
#include "CryptoTest/CryptoTest.h"
#include "Util/FileConfig.h"
#include "Util/CLI11.hpp"

// TODO: get rid of..
#include <wiringPi.h>
#include <wiringPiSPI.h>

#define ACCESSORY_NAME "CuteLight"
#define CONFIG_NAME "/etc/hap.cfg"
#define LOG_NAME "/var/log/hap.log"

static bool Restart = false;
static void do_restart()
{
	Log::Msg("===RESTART===\n");

	Restart = true;
	kill(getpid(), SIGINT);
}

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
			Log::Msg("I2C not opened");
			return 0xFFFF;
		}

		// write pointer
		if (::write(_f, &r, 1) != 1)
		{
			Log::Msg("I2C write error");
			return 0xFFFF;
		}

		// read data
		uint8_t b[2];
		if (::read(_f, b, 2) != 2)
		{
			Log::Msg("I2C read data error");
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
			Log::Msg("I2C not opened");
			return;
		}

		Log::Dbg("ADS reg%d= %04X\n", r, d);

		uint8_t b[3] = { r, uint8_t(d >> 8), uint8_t(d & 0xFF) };
		if (::write(_f, b, 3) != 3)
		{
			Log::Msg("I2C write data error");
		}
	}

	void start(uint16_t conf)
	{
		reg(Config, conf | OS);
		reg(Config);
	}

	bool ready()
	{
		return reg(Config) & OS;
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

		if (_f > 0)
		{
			if (::ioctl(_f, I2C_SLAVE, _addr) < 0)
			{
				::close(_f);
				_f = -1;
			}
		}
		else
		{
			Log::Msg("Device %s open error", _dev);
		}

		return _f > 0;
	}
};

// 4-way joystic on ADS1015
class Joystick : public ADS1015
{
public:
	enum Channel
	{
		Vert = 0,
		Horiz = 1
	};

	enum Position
	{
		Middle = 0,
		Low1,
		Low2,
		High1,
		High2,
	};

private:
	static const uint16_t ConfigV = MUX100 | PGA000 | MODE_S | DR100;	// V channel
	static const uint16_t ConfigH = MUX101 | PGA000 | MODE_S | DR100;	// H channel

	static const uint16_t L1 = 1000;	// low theshold 1
	static const uint16_t L2 = 4000;	// low theshold 2
	static const uint16_t H1 = 3000;	// high theshold 1
	static const uint16_t H2 = 8000;	// high theshold 2

	uint16_t Vm = 14350;
	uint16_t Hm = 14350;
	uint16_t v;
	uint16_t h;

	Channel channel = Vert;	// last channel measured

public:
	Joystick()
	{
	}

	bool Init(uint8_t bus, uint8_t addr)
	{
		if (!ADS1015::Init(bus, addr))
			return false;

		// read both V and H channels
		start(ConfigV);
		std::this_thread::sleep_for(std::chrono::milliseconds(50));
		if (ready())
		{
			Vm = result();
		}
		else
		{
			Log::Msg("Joystic V: not ready\n");
		}

		start(ConfigH);
		std::this_thread::sleep_for(std::chrono::milliseconds(50));
		if (ready())
		{
			Hm = result();
		}
		else
		{
			Log::Msg("Joystic H: not ready\n");
		}

		Log::Msg("Joystic Vm: %d  Hm: %d\n", Vm, Hm);

		// start next measurement cycle
		if (channel == Vert)
			start(ConfigV);
		else
			start(ConfigH);

		return true;
	}

	Position Get(Channel ch)
	{
		// if ADS is ready take measurement
		//	and start next conversion
		if (ready())
		{
			if (channel == Vert)
			{
				v = result();
				channel = Horiz;
				start(ConfigH);
			}
			else
			{
				h = result();
				channel = Vert;
				start(ConfigV);
			}
		}

		//Log::Msg("Joystick Get(%d) v: %d  h: %d\n", ch, v, h);

		// return position of requested channel
		if (ch == Vert)
		{
			if (Vm > v)
			{
				if (Vm - v > L2)
					return Low2;

				if (Vm - v > L1)
					return Low1;
			}
			else if (v > Vm)
			{
				if (v - Vm > H2)
					return High2;

				if (v - Vm > H1)
					return High1;
			}
		}
		else if (ch == Horiz)
		{
			if (Hm > h)
			{
				if (Hm - h > L2)
					return Low2;

				if (Hm - h > L1)
					return Low1;
			}
			else if (h > Hm)
			{
				if (h - Hm > H2)
					return High2;

				if (h - Hm > H1)
					return High1;
			}
		}

		return Middle;
	}
};

// PWM-controlled light bulb
class LbPWM
{
protected:
	static const uint8_t _b2p[];	// 0..100% brightness to 0.255 pwm
	static const uint8_t _v2p[];	// 0..255 value to 0.255 pwm

	uint8_t _v = 50;	// value (brightness) 0..100
	uint8_t _s = 0;		// saturation 0..100
	uint16_t _h = 0;	// hue 0..359
public:
	// convert brightness (0..100%) to pwm (0..255)
	//	pwm = 256^(br/100)-1
	uint8_t b2p(uint8_t br)
	{
		return _b2p[br % 101];
	}

	// convert rgb value (0..255) to pwm (0..255)
	//	pwm = 256^(v/256)-1
	uint8_t v2p(uint8_t v)
	{
		return _v2p[v];
	}

	void rgb(uint8_t& R, uint8_t& G, uint8_t& B)
	{
		float V = (float)_v / 100.;
		float S = (float)_s / 100.;
		float H = (float)_h;
		
		float C = V * S;
		float X = C * (1. - fabs(fmod(H/60, 2) - 1));
		float m = V - C;
		float r, g, b;
		if (H < 60)
		{
			r = C; g = X, b = 0;
		}
		else if (H < 120)
		{
			r = X; g = C, b = 0;
		}
		else if (H < 180)
		{
			r = 0; g = C, b = X;
		}
		else if (H < 240)
		{
			r = 0; g = X, b = C;
		}
		else if (H < 300)
		{
			r = X; g = 0, b = C;
		}
		else
		{
			r = C; g = 0, b = X;
		}

		R = (uint8_t)((r + m) * 255);
		G = (uint8_t)((g + m) * 255);
		B = (uint8_t)((b + m) * 255);

		Log::Msg("h s v %d %d %d  RGB %02X %02X %02X\n", _h, _s, _v, R, G, B);
	}
};
const uint8_t LbPWM::_b2p[] =
{
	 0,  0,  0,  0,   0,   0,   0,   0,   1,   1,   1,   1,   1,   1,   1,   1,   1,   2,   2,   2,
	 2,  2,  2,  3,   3,   3,   3,   3,   4,   4,   4,   5,   5,   5,   6,   6,   6,   7,   7,   8,
	 8,  9,  9, 10,  10,  11,  12,  13,  13,  14,  15,  16,  17,  18,  19,  20,  21,  23,  24,  25,
	27, 28, 30, 32,  34,  36,  38,  40,  42,  45,  48,  50,  53,  56,  60,  63,  67,  71,  75,  79,
	83, 88, 93, 99, 104, 110, 117, 123, 131, 138, 146, 154, 163, 173, 183, 193, 204, 216, 228, 241,
	255
};
const uint8_t LbPWM::_v2p[] =
{
	  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
	  0,   0,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,
	  1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   2,   2,   2,   2,   2,   2,
	  2,   2,   2,   2,   2,   2,   2,   2,   2,   3,   3,   3,   3,   3,   3,   3,
	  3,   3,   3,   3,   3,   4,   4,   4,   4,   4,   4,   4,   4,   4,   5,   5,
	  5,   5,   5,   5,   5,   5,   6,   6,   6,   6,   6,   6,   6,   7,   7,   7,
	  7,   7,   8,   8,   8,   8,   8,   9,   9,   9,   9,   9,  10,  10,  10,  10,
	 11,  11,  11,  11,  12,  12,  12,  12,  13,  13,  13,  14,  14,  14,  15,  15,
	 15,  16,  16,  16,  17,  17,  18,  18,  18,  19,  19,  20,  20,  21,  21,  22,
	 22,  23,  23,  24,  24,  25,  25,  26,  26,  27,  28,  28,  29,  30,  30,  31,
	 32,  32,  33,  34,  35,  35,  36,  37,  38,  39,  40,  40,  41,  42,  43,  44,
	 45,  46,  47,  48,  49,  51,  52,  53,  54,  55,  56,  58,  59,  60,  62,  63,
	 64,  66,  67,  69,  70,  72,  73,  75,  77,  78,  80,  82,  84,  86,  88,  90,
	 91,  94,  96,  98, 100, 102, 104, 107, 109, 111, 114, 116, 119, 122, 124, 127,
	130, 133, 136, 139, 142, 145, 148, 151, 155, 158, 161, 165, 169, 172, 176, 180,
	184, 188, 192, 196, 201, 205, 210, 214, 219, 224, 229, 234, 239, 244, 250, 255
};


// light bulb made of APA102C LEDs
//	connected to SPI channel spi
//	CNT - number of LEDs
template<uint8_t CNT>
class LbAPA102C : public LbPWM
{
private:
	uint32_t _b[CNT + 2];	// SPI data buffer
	uint8_t _spi;
	uint8_t _curr;

	void update()
	{
		uint8_t R, G, B;
		rgb(R, G, B);

		//uint8_t pwm = br2pwm(_v);

		_b[0] = 0;
		for (int i = 1; i < CNT + 1; i++)
			_b[i] = 0xE0
			| ((uint32_t)_curr << 0)	// global
			| ((uint32_t)v2p(B) << 8)	// blue
			| ((uint32_t)v2p(G) << 16)	// green
			| ((uint32_t)v2p(R) << 24)	// red
			;
		_b[CNT + 1] = 0xFFFFFFFF;

		int rc = wiringPiSPIDataRW(0, (uint8_t*)_b, sizeof(_b));
		if (rc < 0)
		{
			Log::Msg("SPIDataRW error\n");
		}
	}

public:

	bool Init(uint8_t spi = 0)
	{
		_spi = spi;
			
		int rc = wiringPiSPISetup(_spi, 2000000);
		if (rc < 0)
		{
			Log::Msg("SPISetup error\n");
			return false;
		}
		return true;
	}
	
	// turn on/off
	void On(bool on)
	{
		_curr = on ? 31 : 0;
		update();
	}

	// set brightness % (0..100)
	void Brightness(uint8_t br)
	{
		_v = br;
		update();
	}

	void Hue(uint16_t h)
	{
		_h = h;
		update();
	}

	void Saturation(uint8_t s)
	{
		_s = s;
		update();
	}
};

// HAP database

class MyAccessoryInformation : public Hap::AccessoryInformation
{
public:
	MyAccessoryInformation()
	{
		_identify.onWrite([this](Hap::Obj::wr_prm& p, Hap::Characteristic::Identify::V v) -> void {
			Log::Msg("MyAccessoryInformation: write Identify\n");
		});
	}

	void config()
	{
		_manufacturer.Value(Hap::config->manufacturer);
		_model.Value(Hap::config->model);
		_name.Value(Hap::config->name);
		_serialNumber.Value(Hap::config->serialNumber);
		_firmwareRevision.Value(Hap::config->firmwareRevision);
	}

} myAis;

class MyLb : public Hap::Lightbulb
{
private:
	Hap::Characteristic::Name _name;
	Hap::Characteristic::Brightness _brightness;
	Hap::Characteristic::Hue _hue;
	Hap::Characteristic::Saturation _saturation;

	LbAPA102C<16> _lb;	// 2 x Sparkfun Lumenati-8
	Joystick js;

	std::thread _poll;
	bool _run = false;

public:
	MyLb(Hap::Characteristic::Name::V name)
	{
		AddName(_name);
		AddBrightness(_brightness);
		AddHue(_hue);
		AddSaturation(_saturation);

		_name.Value(name);

		_on.onRead([this](Hap::Obj::rd_prm& p) -> void {
			Log::Msg("%s: read On: %d\n", _name.Value(), _on.Value());
		});

		_on.onWrite([this](Hap::Obj::wr_prm& p, Hap::Characteristic::On::V v) -> void {
			Log::Msg("%s: write On: %d -> %d\n", _name.Value(), _on.Value(), v);

			_lb.On(v);
		});

		_brightness.onRead([this](Hap::Obj::rd_prm& p) -> void {
			Log::Msg("%s: read Brightness: %d\n", _name.Value(), _brightness.Value());
		});

		_brightness.onWrite([this](Hap::Obj::wr_prm& p, Hap::Characteristic::Brightness::V v) -> void {
			Log::Msg("%s: write Brightness: %d -> %d\n", _name.Value(), _brightness.Value(), v);

			_lb.Brightness(v % 101);
		});

		_hue.onRead([this](Hap::Obj::rd_prm& p) -> void {
			Log::Msg("%s: read Hue: %f\n", _name.Value(), _hue.Value());
		});

		_hue.onWrite([this](Hap::Obj::wr_prm& p, Hap::Characteristic::Hue::V v) -> void {
			Log::Msg("%s: write Hue: %f -> %f\n", _name.Value(), _hue.Value(), v);

			_lb.Hue((uint16_t)fmod(v, 360));
		});

		_saturation.onRead([this](Hap::Obj::rd_prm& p) -> void {
			Log::Msg("%s: read Saturation: %f\n", _name.Value(), _saturation.Value());
		});

		_saturation.onWrite([this](Hap::Obj::wr_prm& p, Hap::Characteristic::Saturation::V v) -> void {
			Log::Msg("%s: write Saturation: %f -> %f\n", _name.Value(), _saturation.Value(), v);

			_lb.Saturation(v);
		});

		_lb.Init();

		_run = true;

		// TODO: sync this thread with onWrites which are in TCP thread context
		_poll = std::thread([this]() -> void {

			static constexpr uint32_t ResetTime = 60;	// 6 sec
			static constexpr int Button = 6;

			Log::Msg("%s: Enter thread\n", _name.Value());

			js.Init(1, 0x48);

			wiringPiSetup();
			pinMode(Button, INPUT);
			pullUpDnControl(Button, PUD_UP);

			int btn = HIGH;
			uint32_t cnt = 0;
			uint32_t btn_cnt = 0;

			while (_run)
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(100));
				cnt++;

				// read state of on/off pin
				if (btn == HIGH && digitalRead(Button) == LOW)
				{
					btn = LOW;
					auto v = !_on.Value();
					_on.Value(v);
					_lb.On(v);
				}
				else if (btn == LOW && digitalRead(Button) == HIGH)
				{
					btn = HIGH;
				}

				if (!Restart && btn == LOW)
				{
					btn_cnt++;
					if (btn_cnt >= ResetTime)
					{
						do_restart();
						btn_cnt = 0;
					}
				}

				// state of Joystick
				Joystick::Position p;

				// vertical channel controls brightness and saturation
				p = js.Get(Joystick::Channel::Vert);
				int32_t upd = 0;
				if (((p == Joystick::Position::Low1) && (cnt % 10 == 0))
				 || ((p == Joystick::Position::Low2) && (cnt % 2 == 0)))
					upd = -1;
				if (((p == Joystick::Position::High1) && (cnt % 10 == 0))
				 || ((p == Joystick::Position::High2) && (cnt % 2 == 0)))
					upd = 1;
				if (upd)
				{
					auto v = btn == HIGH ? _brightness.Value() : _saturation.Value();
					if (upd < 0 && v > 0)
						v--;
					else if (upd > 0 && v < 100)
						v++;
					if (btn == HIGH)
					{
						_brightness.Value(v);
						_lb.Brightness(v);
					}
					else
					{
						_saturation.Value(v);
						_lb.Saturation(v);
					}
				}

				// horizontal channel controls Hue
				p = js.Get(Joystick::Channel::Horiz);
				upd = 0;
				if (((p == Joystick::Position::Low1) && (cnt % 10 == 0))
				 || ((p == Joystick::Position::Low2) && (cnt % 2 == 0)))
					upd = -1;
				if (((p == Joystick::Position::High1) && (cnt % 10 == 0))
				 || ((p == Joystick::Position::High2) && (cnt % 2 == 0)))
					upd = 1;
				if (upd)
				{
					auto v = _hue.Value();
					if (upd < 0)
					{
						v-=2;
						if (v < 0)
							v += 360;
					}
					else
					{
						v += 2;
						if (v >= 360)
							v -= 360;
					}
					_hue.Value(v);
					_lb.Hue((uint16_t)v);
				}
			}

			Log::Msg("%s: Exit thread\n", _name.Value());
		});
	}

	virtual ~MyLb()
	{
		_run = false;
		if (_poll.joinable())
			_poll.join();
	}

	Hap::Characteristic::On::V On()
	{
		return _on.Value();
	}
	void On(Hap::Characteristic::On::V v)
	{
		_on.Value(v);
		_lb.On(v);
	}

	Hap::Characteristic::Brightness::V Brightness()
	{
		return _brightness.Value();
	}
	void Brightness(Hap::Characteristic::Brightness::V v)
	{
		uint8_t b = v % 101;
		_brightness.Value(b);
		_lb.Brightness(b);
	}

	Hap::Characteristic::Hue::V Hue()
	{
		return _hue.Value();
	}
	void Hue(Hap::Characteristic::Hue::V v)
	{
		uint16_t h = (uint16_t)fmod(v, 360);
		_hue.Value(h);
		_lb.Hue(h);
	}

	Hap::Characteristic::Saturation::V Saturation()
	{
		return _saturation.Value();
	}
	void Saturation(Hap::Characteristic::Saturation::V v)
	{
		uint8_t s = v;
		_saturation.Value(s);
		_lb.Saturation(s);
	}

} myLb(ACCESSORY_NAME);

class MyAcc : public Hap::Accessory<2>
{
public:
	MyAcc() : Hap::Accessory<2>()
	{
		AddService(&myAis);
		AddService(&myLb);
	}

} myAcc;

using MyDbType = Hap::DbStatic<1>;
class MyDb : public Hap::File::Db<MyDbType>
{
public:
	MyDb()
	{
		AddAcc(&myAcc);
	}

	// db initialization:
	void Init(Hap::iid_t aid)
	{
		// assign instance IDs
		myAcc.setId(aid);

		// config AIS
		myAis.config();
	}

	// restore Db from config file
	virtual bool Restore(Hap::Json::ParserDef& js, int t) override
	{
		if (js.type(t) != Hap::Json::JSMN_OBJECT)
		{
			Log::Msg("Db::Restore: 'db' is not an object");
			return false;
		}

		struct Lb {
			const char* nm;
			MyLb& lb;
		} _lb[] =
		{
			{ "Lb1", myLb },
//			{ "Lb2", myLb2 },
		};

		for (unsigned i = 0; i < sizeofarr(_lb); i++)
		{
			Lb& lb = _lb[i];
			int a;	// accessory
			int c;	// characteristic

			a = js.find(t, lb.nm);
			if (a > 0)
			{
				a++;
				if (js.type(a) != Hap::Json::JSMN_OBJECT)
				{
					Log::Msg("Db::Restore: %s is not an object\n", lb.nm);
					continue;
				}

				c = js.find(a, "On");
				if (c > 0)
				{
					c++;
					Hap::Characteristic::On::V v;
					if (!js.is_bool(c, v))
					{
						Log::Msg("Db::Restore: %s.On: invalid\n", lb.nm);
						continue;
					}
					Log::Msg("Db::Restore: %s.On: %d\n", lb.nm, v);
					lb.lb.On(v);
				}

				c = js.find(a, "Brightness");
				if (c > 0)
				{
					c++;
					Hap::Characteristic::Brightness::V v;
					if (!js.is_number<Hap::Characteristic::Brightness::V>(c, v))
					{
						Log::Msg("Db::Restore: %s.Brightness: invalid\n", lb.nm);
						continue;
					}
					Log::Msg("Db::Restore: %s.Brightness: %d\n", lb.nm, v);
					lb.lb.Brightness(v);
				}

				c = js.find(a, "Hue");
				if (c > 0)
				{
					c++;
					Hap::Characteristic::Hue::V v;
					if (!js.is_number<Hap::Characteristic::Hue::V>(c, v))
					{
						Log::Msg("Db::Restore: %s.Hue: invalid\n", lb.nm);
						continue;
					}
					Log::Msg("Db::Restore: %s.Hue: %d\n", lb.nm, v);
					lb.lb.Hue(v);
				}

				c = js.find(a, "Saturation");
				if (c > 0)
				{
					c++;
					Hap::Characteristic::Saturation::V v;
					if (!js.is_number<Hap::Characteristic::Saturation::V>(c, v))
					{
						Log::Msg("Db::Restore: %s.Saturation: invalid\n", lb.nm);
						continue;
					}
					Log::Msg("Db::Restore: %s.Saturation: %d\n", lb.nm, v);
					lb.lb.Saturation(v);
				}

			}
		}

		return true;
	}

	// save Db to config file
	virtual bool Save(FILE* f) override
	{
		fprintf(f, "\t\t\"Lb1\":{\n");
		fprintf(f, "\t\t\t\"On\":%d,\n", myLb.On());
		fprintf(f, "\t\t\t\"Brightness\":%d,\n", myLb.Brightness());
		fprintf(f, "\t\t\t\"Hue\":%f,\n", myLb.Hue());
		fprintf(f, "\t\t\t\"Saturation\":%f\n", myLb.Saturation());
		fprintf(f, "\t\t},\n");

		return true;
	}

} db;

// configuration data of this accessory server
//	uses persistent storage on a file
const Hap::File::Config<MyDbType>::Default configDef =
{
	ACCESSORY_NAME,		// const char* name;
	"TestModel",		// const char* model;
	"TestMaker",		// const char* manufacturer;
	"0001",				// const char* serialNumber;
	"0.1",				// const char* firmwareRevision;
	"000-11-000",		// const char* setupCode;
	7889				// uint16_t tcpPort;
};
Hap::File::Config<MyDbType> myConfig(&db, &configDef, CONFIG_NAME);
Hap::Config* Hap::config = &myConfig;

// statically allocated storage for HTTP processing
//	Our implementation is single-threaded so only one set of buffers.
//	The http server uses this buffers only during processing a request.
//	All session-persistent data is kept in Session objects.
Hap::BufStatic<char, Hap::MaxHttpFrame * 2> http_req;
Hap::BufStatic<char, Hap::MaxHttpFrame * 4> http_rsp;
Hap::BufStatic<char, Hap::MaxHttpFrame * 1> http_tmp;
Hap::Http::Server::Buf buf = { http_req, http_rsp, http_tmp };
Hap::Http::Server http(buf, db, myConfig.pairings, myConfig.keys);

int main(int argc, char* argv[])
{
	CLI::App app{"LinuxTest HAP server"};

	if (geteuid() != 0)
	{
		printf("Please run as root\n");
		exit(1);
	}

	Crypto::rnd_init();

	Log::Info = true;

	app.add_flag("-D,--debug", Log::Debug, "Turn on debugging messages");
	app.add_flag("-C,--console", Log::Console, "Print log to console");

	bool reset = false;
	app.add_flag("-R,--reset", reset, "Reset configuration");

	CLI11_PARSE(app, argc, argv);

	Log::Init(LOG_NAME);

	// Joystick test
	if (0)
	{
		Joystick js;

		js.Init(1, 0x48);

		return 0;
	}
	// crypto functions test
	if (0)
	{
		return cryptoTest();
	}

	while (true)
	{
		// create servers
		Hap::Mdns* mdns = Hap::Mdns::Create();
		Hap::Tcp* tcp = Hap::Tcp::Create(&http);

		// restore configuration
		myConfig.Init(reset);

		// set config update callback
		myConfig.Update = [mdns]() -> void {

			bool mdnsUpdate = false;

			// see if status flag must change
			bool paired = myConfig.pairings.Count() != 0;
			if (paired && (Hap::config->statusFlags & Hap::Bonjour::NotPaired))
			{
				Hap::config->statusFlags &= ~Hap::Bonjour::NotPaired;
				mdnsUpdate = true;
			}
			else if (!paired && !(Hap::config->statusFlags & Hap::Bonjour::NotPaired))
			{
				Hap::config->statusFlags |= Hap::Bonjour::NotPaired;
				mdnsUpdate = true;
			}

			myConfig.Save();

			if (mdnsUpdate)
				mdns->Update();
		};

		// init static objects
		db.Init(1);

		// start servers
		mdns->Start();
		tcp->Start();

		// wait for signal, gracefully handle SIGINT and SIGTERM
		signal(SIGINT, [](int s)->void {
			printf("===Interrupted===\n");
			});
		signal(SIGTERM, [](int s)->void {
			printf("===Terminated===\n");
			});
		pause();

		myConfig.Save();

		// stop servers
		tcp->Stop();
		mdns->Stop();

		if (Restart)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(1000));
			reset = true;	// reset config after restart
			Restart = false;
			continue;
		}

		break;
	}
	
	return 0;
}
