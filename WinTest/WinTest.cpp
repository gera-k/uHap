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

#include "Platform.h"

#include "Hap/Hap.h"
#include "Crypto/Crypto.h"
#include "Crypto/MD.h"
#include "CryptoTest/CryptoTest.h"
#include "Srp.h"

#include "Util/FileConfig.h"

#define ACCESSORY_NAME "WinTest"
#define CONFIG_NAME "WinTest.cfg"
#define LOG_NAME "WinTest.log"

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
	int _n;
public:
	MyLb(Hap::Characteristic::Name::V name, int n) : _n(n)
	{
		AddBrightness(_brightness);
		AddName(_name);
		AddHue(_hue);
		AddSaturation(_saturation);

		_name.Value(name);

		_on.onRead([this](Hap::Obj::rd_prm& p) -> void {
			Log::Msg("MyLb%d: read On: %d\n", _n, _on.Value());
		});

		_on.onWrite([this](Hap::Obj::wr_prm& p, Hap::Characteristic::On::V v) -> void {
			Log::Msg("MyLb%d: write On: %d -> %d\n", _n, _on.Value(), v);
		});

		_brightness.onRead([this](Hap::Obj::rd_prm& p) -> void {
			Log::Msg("MyLb%d: read Brightness: %d\n", _n, _brightness.Value());
		});

		_brightness.onWrite([this](Hap::Obj::wr_prm& p, Hap::Characteristic::Brightness::V v) -> void {
			Log::Msg("MyLb%d: write Brightness: %d -> %d\n", _n, _brightness.Value(), v);
		});

		_hue.onRead([this](Hap::Obj::rd_prm& p) -> void {
			Log::Msg("MyLb%d: read Hue: %f\n", _n, _hue.Value());
		});

		_hue.onWrite([this](Hap::Obj::wr_prm& p, Hap::Characteristic::Hue::V v) -> void {
			Log::Msg("MyLb%d: write Hue: %f -> %f\n", _n, _hue.Value(), v);
		});

		_saturation.onRead([this](Hap::Obj::rd_prm& p) -> void {
			Log::Msg("MyLb%d: read Saturation: %d\n", _n, _saturation.Value());
		});

		_saturation.onWrite([this](Hap::Obj::wr_prm& p, Hap::Characteristic::Saturation::V v) -> void {
			Log::Msg("MyLb%d: write Saturation: %d -> %d\n", _n, _saturation.Value(), v);
		});

	}

	Hap::Characteristic::On::V On()
	{
		return _on.Value();
	}
	void On(Hap::Characteristic::On::V v)
	{
		_on.Value(v);
	}

	Hap::Characteristic::Brightness::V Brightness()
	{
		return _brightness.Value();
	}
	void Brightness(Hap::Characteristic::Brightness::V v)
	{
		_brightness.Value(v);
	}

	Hap::Characteristic::Hue::V Hue()
	{
		return _hue.Value();
	}
	void Hue(Hap::Characteristic::Hue::V v)
	{
		_hue.Value(v);
	}

	Hap::Characteristic::Saturation::V Saturation()
	{
		return _saturation.Value();
	}
	void Saturation(Hap::Characteristic::Saturation::V v)
	{
		_saturation.Value(v);
	}

} myLb1("Light-1", 1), myLb2("Light-2", 2);

class MyAcc : public Hap::Accessory<3>
{
public:
	MyAcc() : Hap::Accessory<3>()
	{
		AddService(&myAis);
		AddService(&myLb1);
		AddService(&myLb2);
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
			{ "Lb1", myLb1 },
			{ "Lb2", myLb2 },
		};

		for (int i = 0; i < sizeofarr(_lb); i++)
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
		fprintf(f, "\t\t\t\"On\":%d,\n", myLb1.On());
		fprintf(f, "\t\t\t\"Brightness\":%d,\n", myLb1.Brightness());
		fprintf(f, "\t\t\t\"Hue\":%f,\n", myLb1.Hue());
		fprintf(f, "\t\t\t\"Saturation\":%f\n", myLb1.Saturation());
		fprintf(f, "\t\t},\n");

		fprintf(f, "\t\t\"Lb2\":{\n");
		fprintf(f, "\t\t\t\"On\":%d,\n", myLb2.On());
		fprintf(f, "\t\t\t\"Brightness\":%d,\n", myLb2.Brightness());
		fprintf(f, "\t\t\t\"Hue\":%f,\n", myLb2.Hue());
		fprintf(f, "\t\t\t\"Saturation\":%f\n", myLb2.Saturation());
		fprintf(f, "\t\t}\n");

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
//	Our implementation is single-threaded hence the only one set of buffers.
//	The http server uses this buffers only during processing a request.
//	All session-persistent data is kept in Session objects.
char http_req_buf[Hap::MaxHttpFrame * 2];
char http_rsp_buf[Hap::MaxHttpFrame * 4];
char http_tmp_buf[Hap::MaxHttpFrame * 1];
static Hap::Buf<char> http_req(http_req_buf, sizeof(http_req_buf));
static Hap::Buf<char> http_rsp(http_rsp_buf, sizeof(http_rsp_buf));
static Hap::Buf<char> http_tmp(http_tmp_buf, sizeof(http_tmp_buf));
static Hap::Http::Server::Buf http_buf{ http_req, http_rsp, http_tmp };

Hap::Http::Server http(http_buf, db, myConfig.pairings, myConfig.keys);

static int hapTest()
{
	int r = 0;

	Log::Console = true;
	Log::Info = true;
	Log::Debug = true;

	Log::Msg("Server start\n");

	// create servers
	Hap::Mdns* mdns = Hap::Mdns::Create();
	Hap::Tcp* tcp = Hap::Tcp::Create(&http);

	// restore configuration
	myConfig.Init();

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

	// wait for interrupt
	char c;
	std::cin >> c;

	// stop servers
	tcp->Stop();
	mdns->Stop();

	myConfig.Save();

	Log::Exit();

	return r;
}

static void ktest()
{
	int64_t x = 0x6789ab;
	int64_t y = 0xfcba98;

	int64_t a0 = (x >> 0) & 0xFF;
	int64_t a1 = (x >> 8) & 0xFF;
	int64_t a2 = (x >> 16) & 0xFF;
	int64_t b0 = (y >> 0) & 0xFF;
	int64_t b1 = (y >> 8) & 0xFF;
	int64_t b2 = (y >> 16) & 0xFF;

	int64_t p012 = (a0 + a1 + a2)*(b0 + b1 + b2);
	int64_t p01 = (a0 + a1)*(b0 + b1);
	int64_t p02 = (a0 + a2)*(b0 + b2);
	int64_t p12 = (a1 + a2)*(b1 + b2);
	int64_t p0 = a0 * b0;
	int64_t p1 = a1 * b1;
	int64_t p2 = a2 * b2;

	int64_t r1 = (p2 << 32) + ((p012 + p0 - p01 - p02) << 24) + ((p012 + p1 + p1 - p01 - p12) << 16) + ((p012 + p2 - p02 - p12) << 8) + p0;
	int64_t r2 = x * y;

	printf("r1 %llX  r2 %llX   r1-r2 %llX %llX\n", r1, r2, r1-r2, r2-r1);
}


int main()
{
	int r = 0;

	Crypto::rnd_init();

	Log::Init(LOG_NAME);
	Log::Console = true;
	Log::Debug = true;

//	r += cryptoTest();

	r += hapTest();

//	ktest();

	return r;
}
