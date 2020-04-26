/*
	Copyright(c) 2020 Gera Kazakov
	SPDX-License-Identifier: Apache-2.0
*/

#ifndef _JOYSTICK_ADS1015_H_
#define _JOYSTICK_ADS1015_H_

#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>

#include <wiringPi.h>

#include "Joystick.h"
#include "ADS1015.h"

class Joystick_ADS1015 : public Joystick, public ADS1015
{
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

	Channel channel = Vert;		// last channel measured
	int button = -1;					// button GPIO

public:
	bool Init(uint8_t bus, uint8_t addr, int btn = -1)
	{
		if (!ADS1015::Init(bus, addr))
			return false;

		// init button GPIO
		button = btn;
		if (button > 0)
		{
			pinMode(button, INPUT);
			pullUpDnControl(button, PUD_UP);
		}

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

	virtual Position Get(Channel ch) override
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

	virtual Button Get() override
	{
		if (button < 0)
			return High;

		int b = digitalRead(button);

		return b == LOW ? Low : High;
	}

	virtual void Test() override
	{

	}

};

#endif /*JOYSTICK_ADS1015_H*/