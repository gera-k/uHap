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

#include <wiringPiSPI.h>

#define HAP_SERVER
//#define JOY_TEST

#include "Platform.h"
#include "Util/CLI11.hpp"

#include "Joystick_ADS1015.h"
#include "Joystick_Quiic.h"

#include "Hap/Hap.h"
#include "Crypto/Crypto.h"
#include "Crypto/MD.h"
#include "CryptoTest/CryptoTest.h"
#include "Util/FileConfig.h"

#ifdef HAP_SERVER

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
	Joystick* _js = nullptr;

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
	}

	virtual ~MyLb()
	{
		Stop();
	}

	void Init(Joystick* js)
	{
		Log::Msg("%s: Init\n", _name.Value());
		_js = js;
		_lb.Init();
	}

	void Start()
	{
		Log::Msg("%s: Start\n", _name.Value());

		_run = true;

		// TODO: sync this thread with onWrites which are in TCP thread context
		_poll = std::thread([this]() -> void
		{

			static constexpr uint32_t ResetTime = 60;	// 6 sec

			Log::Msg("%s: Enter thread\n", _name.Value());

			Joystick::Button btn = Joystick::High;
			uint32_t cnt = 0;
			uint32_t btn_cnt = 0;

			while (_run)
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(100));
				cnt++;

				// process on/off button
				if (btn == Joystick::High && _js->Get() == Joystick::Low)
				{
					btn = Joystick::Low;
					auto v = !_on.Value();
					_on.Value(v);
					_lb.On(v);
				}
				else if (btn == Joystick::Low && _js->Get() == Joystick::High)
				{
					btn = Joystick::High;
				}

				if (!Restart && btn == Joystick::Low)
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

				static constexpr uint32_t lowPer = 2;
				static constexpr uint32_t highPer = 1;

				// vertical channel controls brightness and saturation
				p = _js->Get(Joystick::Channel::Vert);
				int32_t upd = 0;
				if (((p == Joystick::Position::Low1) && (cnt % lowPer == 0))
				 || ((p == Joystick::Position::Low2) && (cnt % highPer == 0)))
					upd = -1;
				if (((p == Joystick::Position::High1) && (cnt % lowPer == 0))
				 || ((p == Joystick::Position::High2) && (cnt % highPer == 0)))
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
				p = _js->Get(Joystick::Channel::Horiz);
				upd = 0;
				if (((p == Joystick::Position::Low1) && (cnt % lowPer == 0))
				 || ((p == Joystick::Position::Low2) && (cnt % highPer == 0)))
					upd = -1;
				if (((p == Joystick::Position::High1) && (cnt % lowPer == 0))
				 || ((p == Joystick::Position::High2) && (cnt % highPer == 0)))
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

	void Stop()
	{
		Log::Msg("%s: Stop\n", _name.Value());
		if (_run)
		{
			_run = false;
			if (_poll.joinable())
				_poll.join();
		}
	}

	void Name(Hap::Characteristic::Name::V v)
	{
		_name.Value(v);
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

int hapServer(int argc, char* argv[])
{
	CLI::App app{ "LinuxTest HAP server" };

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

	// setup wiring pi lib
	wiringPiSetup();

	Joystick_ADS1015 js_1;
	Joystick_Quiic js_2;
	static constexpr int Button = 6;
	Joystick* js = nullptr;

	if (js_1.Init(1, 0x48, Button))
	{
		Log::Msg("Joystick_ADS1015 selected\n");
		js = &js_1;
	}
	else if (js_2.Init(1, 0x20))
	{
		Log::Msg("Joystick_Quiic selected\n");
		js = &js_2;
	}
	else
	{
		Log::Err("Joystick initialization failure\n");
		return 1;
	}

	myLb.Init(js);

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

		// set LB name to Accessory name from config
		myLb.Name(myConfig.name);

		// start Lb
		myLb.Start();

		// init HK db
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

		// stop LB
		myLb.Stop();

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

#else
Hap::Config* Hap::config = nullptr;
#endif

int main(int argc, char* argv[])
{
#ifdef HAP_SERVER
	return hapServer(argc, argv);
#endif

#ifdef CRYPTO_TEST
	return cryptoTest();
#endif

#ifdef JOY_TEST
	Joystick* js = nullptr;

	Log::Info = true;
	Log::Debug = true;
	Log::Console = true;

	static Joystick_ADS1015 js_1;
	static Joystick_Quiic js_2;
	static constexpr int Button = 6;

	if (js_1.Init(1, 0x48, Button))
	{
		Log::Msg("Joystick_ADS1015 selected\n");
		js = &js_1;
	}
	else if (js_2.Init(1, 0x20))
	{
		Log::Msg("Joystick_Quiic selected\n");
		js = &js_2;
	}
	else
	{
		Log::Msg("Joystick initialization failure\n");
	}

	if (js != nullptr)
	{
		js->Test();
	}

	return 0;
#endif
}
