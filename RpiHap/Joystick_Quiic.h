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

#ifndef _JOYSTICK_QUIIC_H_
#define _JOYSTICK_QUIIC_H_

// Sparkfun Quiic joystic

#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>

#include "Joystick.h"

class Joystick_Quiic : public Joystick
{
private:
	static const uint16_t L1 = 200;	// low theshold 1
	static const uint16_t L2 = 400;	// low theshold 2
	static const uint16_t H1 = 200;	// high theshold 1
	static const uint16_t H2 = 400;	// high theshold 2

	uint16_t Vm = 512;
	uint16_t Hm = 512;
	uint16_t v;
	uint16_t h;

	uint8_t _bus;
	uint8_t _addr;
	char _dev[16];
	int _f = -1;

	// Joystick Registers
	enum class Reg
	{
		Addr = 0,
		Fw = 1,
		Hor = 3,
		Ver = 5,
		BtnPos = 7,
		BtnSts = 8,
	};

	// read 8-bit register
	uint8_t reg8(Reg r)
	{
		if (_f < 0)
		{
			Log::Msg("I2C not opened\n");
			return 0xFF;
		}

		// write pointer
		if (::write(_f, &r, 1) != 1)
		{
			Log::Msg("I2C write error\n");
			return 0xFF;
		}

		// read data
		uint8_t b;
		if (::read(_f, &b, 1) != 1)
		{
			Log::Msg("I2C read data error\n");
			return 0xFF;
		}

		Log::Dbg("Quiic reg %d: %02X\n", r, b);

		return b;
	}

	// read 16-bitregister
	uint16_t reg16(Reg r)
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

		Log::Dbg("Quiic reg %d..%d: %04X\n", r, (int)r+1, d);

		return d;
	}

	uint16_t Fw()
	{
		return reg16(Reg::Fw);
	}

	uint16_t H()
	{
		return reg16(Reg::Hor) >> 6;
	}

	uint16_t V()
	{
		return reg16(Reg::Ver) >> 6;
	}

	Button B()
	{
		return reg8(Reg::BtnPos) ? High : Low;
	}

public:
	bool Init(uint8_t bus, uint8_t addr)
	{
		_bus = bus;
		_addr = addr;

		snprintf(_dev, sizeof(_dev), "/dev/i2c-%d", _bus);

		_f = ::open(_dev, O_RDWR);
		if (_f < 0)
		{
			Log::Msg("Quiic: Device %s open error\n", _dev);
			return false;
		}

		if (::ioctl(_f, I2C_SLAVE, _addr) < 0)
		{
			Log::Msg("Quiic: Device %s set addr 0x%02X error\n", _dev, _addr);
			::close(_f);
			return false;
		}

		if (Fw() == 0xFFFF)
		{
			Log::Msg("Quiic: Device %s addr 0x%02X read error\n", _dev, _addr);
			::close(_f);
			return false;
		}

		return true;
	}
		
	virtual Position Get(Channel ch) override
	{
		// return position of requested channel
		if (ch == Vert)
		{
			v = V();

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
			h = H();

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

	virtual Button Get() override
	{
		return B();
	}

	virtual void Test() override
	{
		while (true)
		{
			printf("H %d  V %d  B %d\n", H(), V(), B());

			std::this_thread::sleep_for(std::chrono::milliseconds(500));
		}
	}
};

#endif /*_JOYSTICK_QUIIC_H_*/
