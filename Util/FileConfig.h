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

// Hap configuration file
namespace Hap {
namespace File {

// convert bin to hex, sizeof(s) must be >= size*2 + 1
void bin2hex(uint8_t* buf, size_t size, char* s)
{
	static const char h[] = "0123456789ABCDEF";

	while (size-- > 0)
	{
		uint8_t b = *buf++;

		*s++ = h[b >> 4];
		*s++ = h[b & 0xF];
	}
	*s++ = 0;
}

void hex2bin(const char* s, uint8_t* buf, size_t size)
{
	uint8_t b;

	while (size-- > 0)
	{
		char c = *s++;
		if (c >= '0' && c <= '9')
			b = c - '0';
		else
			b = c - 'A' + 10;
		b <<= 4;

		c = *s++;
		if (c >= '0' && c <= '9')
			b += c - '0';
		else
			b += c - 'A' + 10;

		*buf++ = b;
	}
}

// save/restore Pairing records to/from config file
class Pairings : public Hap::Pairings
{
public:
	void Reset()
	{
		init();
	}

	bool Save(FILE* f)
	{
		if (f == NULL)
			return false;

		char* key = new char[Hap::Controller::KeyLen * 2 + 1];

		bool comma = false;
		for (unsigned i = 0; i < sizeofarr(_db); i++)
		{
			Hap::Controller* ios = &_db[i];

			if (ios->perm == Hap::Controller::Perm::None)
				continue;

			bin2hex(ios->key, ios->KeyLen, key);

			fprintf(f, "\t\t%c[\"%.*s\",\"%s\",\"%d\"]\n", comma ? ',' : ' ', ios->idLen, ios->id, key, (int)ios->perm);
			comma = true;
		}

		delete[] key;

		return true;
	}

	bool Add(const char* id, int id_len, const char* key, int key_len, uint8_t perm)
	{
		if (key_len != Hap::Controller::KeyLen * 2)
			return false;

		uint8_t* key_bin = new uint8_t[Hap::Controller::KeyLen];
		hex2bin(key, key_bin, Hap::Controller::KeyLen);

		bool ret = Hap::Pairings::Add((const uint8_t*)id, id_len, key_bin, Hap::Controller::Perm(perm));

		delete[] key_bin;

		return ret;
	}
};



// implement crypto keys save/restore to/from config file
class Keys : public Crypto::Ed25519
{
public:
	void Reset()
	{
		init();
	}

	bool Restore(const char* pub, int pub_len, const char* prv, int prv_len)
	{
		if (pub_len != PUBKEY_SIZE_BYTES * 2)
			return false;
		if (prv_len != PRVKEY_SIZE_BYTES * 2)
			return false;

		hex2bin(pub, _pubKey, PUBKEY_SIZE_BYTES);
		hex2bin(prv, _prvKey, PRVKEY_SIZE_BYTES);

		return true;
	}

	bool Save(FILE* f)
	{
		if (f == NULL)
			return false;

		char* s = new char[PRVKEY_SIZE_BYTES * 2 + 1];

		bin2hex(_pubKey, PUBKEY_SIZE_BYTES, s);
		fprintf(f, "\t\t \"%s\"\n", s);

		bin2hex(_prvKey, PRVKEY_SIZE_BYTES, s);
		fprintf(f, "\t\t,\"%s\"\n", s);

		delete[] s;

		return true;
	}
};

template< class DbBase >
class Db : public DbBase
{
public:
	virtual bool Restore(Hap::Json::Parser& js, int t) = 0;
	virtual bool Save(FILE* f) = 0;
};

//	implements Hap configuration storage on file
template< class DbBase >
class Config : public Hap::Config
{
public:
	Pairings pairings;
	Keys keys;
	Db<DbBase>* db;


	struct Default
	{
		const char* name;
		const char* model;
		const char* manufacturer;
		const char* serialNumber;
		const char* firmwareRevision;
		const char* setupCode;
		uint16_t tcpPort;
	};

	Config( Db<DbBase>* db_, const Default* def, const char* fileName)
		:  db(db_), _def(def), _fileName(fileName)
	{
		name = _name;
		model = _model;
		manufacturer = _manufacturer;
		serialNumber = _serialNumber;
		firmwareRevision = _firmwareRevision;
		deviceId = _deviceId;
		setupCode = _setupCode;
	}

private:
	const Default* _def;
	const char* _fileName;

	// define storage for string parameters which we save/restore
	char _name[Hap::DefString];				// Accessory name - used as initial Bonjour name and as AIS name of aid=1
	char _model[Hap::DefString];			// Model name (Bonjour and AIS)
	char _manufacturer[Hap::DefString];		// Manufacturer- used by AIS (Accessory Information Service)
	char _serialNumber[Hap::DefString];		// Serial number in arbitrary format
	char _firmwareRevision[Hap::DefString];	// Major[.Minor[.Revision]]
	char _deviceId[Hap::DefString];			// Device ID (XX:XX:XX:XX:XX:XX, new deviceId generated on each factory reset)
	char _setupCode[Hap::DefString];			// setupCode code XXX-XX-XXX

	virtual void _default() override
	{
		Log::Msg("Config: default\n");

		strcpy(_name, _def->name);
		strcpy(_model, _def->model);
		strcpy(_manufacturer, _def->manufacturer);
		strcpy(_serialNumber, _def->serialNumber);
		strcpy(_firmwareRevision, _def->firmwareRevision);

		// generate new random ID
		uint8_t id[6];
		Crypto::rnd_data(id, sizeof(id));
		sprintf(_deviceId, "%02X:%02X:%02X:%02X:%02X:%02X",
			id[0], id[1], id[2], id[3], id[4], id[5]);
		Log::Msg("Config: deviceId '%s'\n", deviceId);

		configNum = 1;
		categoryId = 5;
		statusFlags = 0
			| Hap::Bonjour::NotPaired
//			| Hap::Bonjour::NotConfiguredForWiFi
			;

		strcpy(_setupCode, _def->setupCode);

		port = swap_16(_def->tcpPort);
		BCT = 0;

		pairings.Reset();
		keys.Reset();
	}

	virtual void _reset() override
	{
		Log::Msg("Config: reset\n");

		// generate new random ID
		uint8_t id[6];
		Crypto::rnd_data(id, sizeof(id));
		sprintf(_deviceId, "%02X:%02X:%02X:%02X:%02X:%02X",
			id[0], id[1], id[2], id[3], id[4], id[5]);
		Log::Msg("Config: deviceId '%s'\n", deviceId);

		configNum++;
		statusFlags = Hap::Bonjour::NotPaired;

		pairings.Reset();
		keys.Reset();
	}

	virtual bool _save() override
	{
		FILE* f = fopen(_fileName, "w+b");
		if (f == NULL)
		{
			Log::Msg("Config: cannot open %s for write\n", _fileName);
			return false;
		}

		Log::Msg("Config: save to %s\n", _fileName);

		fprintf(f, "{\n");
		fprintf(f, "\t\"%s\":\"%s\",\n", key[key_name], name);
		fprintf(f, "\t\"%s\":\"%s\",\n", key[key_model], model);
		fprintf(f, "\t\"%s\":\"%s\",\n", key[key_manuf], manufacturer);
		fprintf(f, "\t\"%s\":\"%s\",\n", key[key_serial], serialNumber);
		fprintf(f, "\t\"%s\":\"%s\",\n", key[key_firmware], firmwareRevision);
		fprintf(f, "\t\"%s\":\"%s\",\n", key[key_device], deviceId);
		fprintf(f, "\t\"%s\":\"%d\",\n", key[key_config], configNum);
		fprintf(f, "\t\"%s\":\"%d\",\n", key[key_category], categoryId);
		fprintf(f, "\t\"%s\":\"%d\",\n", key[key_status], statusFlags);
		fprintf(f, "\t\"%s\":\"%s\",\n", key[key_setup], setupCode);
		fprintf(f, "\t\"%s\":\"%d\",\n", key[key_port], swap_16(port));

		fprintf(f, "\t\"%s\":[\n", key[key_keys]);
		keys.Save(f);
		fprintf(f, "\t],\n");

		fprintf(f, "\t\"%s\":[\n", key[key_pairings]);
		pairings.Save(f);
		fprintf(f, "\t],\n");

		fprintf(f, "\t\"%s\":{\n", key[key_db]);
		db->Save(f);
		fprintf(f, "\t}\n");

		fprintf(f, "}\n");

		fclose(f);
		return true;
	}

	virtual bool _restore() override
	{
		Log::Msg("Config: restore from %s\n", _fileName);
		FILE* f = fopen(_fileName, "r+");
		if (f == NULL)
		{
			Log::Msg("Config: cannot open %s for read\n", _fileName);
			return false;
		}

		Hap::Json::member om[] =
		{
			{ key[key_name], Hap::Json::JSMN_STRING | Hap::Json::JSMN_UNDEFINED },
			{ key[key_model], Hap::Json::JSMN_STRING | Hap::Json::JSMN_UNDEFINED },
			{ key[key_manuf], Hap::Json::JSMN_STRING | Hap::Json::JSMN_UNDEFINED },
			{ key[key_serial], Hap::Json::JSMN_STRING | Hap::Json::JSMN_UNDEFINED },
			{ key[key_firmware], Hap::Json::JSMN_STRING | Hap::Json::JSMN_UNDEFINED },
			{ key[key_device], Hap::Json::JSMN_STRING | Hap::Json::JSMN_UNDEFINED },
			{ key[key_config], Hap::Json::JSMN_STRING | Hap::Json::JSMN_UNDEFINED },
			{ key[key_category], Hap::Json::JSMN_STRING | Hap::Json::JSMN_UNDEFINED },
			{ key[key_status], Hap::Json::JSMN_STRING | Hap::Json::JSMN_UNDEFINED },
			{ key[key_setup], Hap::Json::JSMN_STRING | Hap::Json::JSMN_UNDEFINED },
			{ key[key_port], Hap::Json::JSMN_STRING | Hap::Json::JSMN_UNDEFINED },
			{ key[key_keys], Hap::Json::JSMN_ARRAY | Hap::Json::JSMN_UNDEFINED },
			{ key[key_pairings], Hap::Json::JSMN_ARRAY | Hap::Json::JSMN_UNDEFINED },
			{ key[key_db], Hap::Json::JSMN_OBJECT | Hap::Json::JSMN_UNDEFINED },
		};
		bool ret = false;
		char* b = nullptr;
		long size = 0;
		Hap::Json::ParserStatic<100> js;	// allocates parser on stack, TODO? 
		int rc;

		if (fseek(f, 0, SEEK_END) == 0)
			size = ftell(f);

		if (size <= 0)
			goto Ret;

		b = new char[size];
		if (fseek(f, 0, SEEK_SET) < 0)
			goto Ret;

		if (fread(b, 1, size, f) != size_t(size))
			goto Ret;

		if (!js.parse(b, (uint16_t)size))
			goto Ret;

		if (js.tk(0)->type != Hap::Json::JSMN_OBJECT)
			goto Ret;

		rc = js.parse(0, om, sizeofarr(om));
		if (rc >= 0)
		{
			Log::Msg("parameter '%s' is missing or invalid\n", om[rc].key);
			goto Ret;
		}
		// TODO: read partial file, set missing params to defaults

		for (int k = 0; k < key_max; k++)
		{
			int i = om[k].i;
			if (i <= 0)
				continue;

			char s[Hap::DefString + 1];
			js.copy(om[k].i, s, sizeof(s));

			switch (k)
			{
			case key_name:
				js.copy(i, _name, sizeof(_name));
				Log::Msg("Config: restore name '%s'\n", name);
				break;
			case key_model:
				js.copy(i, _model, sizeof(_model));
				Log::Msg("Config: restore model '%s'\n", model);
				break;
			case key_manuf:
				js.copy(i, _manufacturer, sizeof(_manufacturer));
				Log::Msg("Config: restore manufacturer '%s'\n", manufacturer);
				break;
			case key_serial:
				js.copy(i, _serialNumber, sizeof(_serialNumber));
				Log::Msg("Config: restore serialNumber '%s'\n", serialNumber);
				break;
			case key_firmware:
				js.copy(i, _firmwareRevision, sizeof(_firmwareRevision));
				Log::Msg("Config: restore firmwareRevision '%s'\n", firmwareRevision);
				break;
			case key_device:
				js.copy(i, _deviceId, sizeof(_deviceId));
				Log::Msg("Config: restore deviceId '%s'\n", deviceId);
				break;
			case key_config:
				js.set_if(i, configNum);
				Log::Msg("Config: restore configNum '%d'\n", configNum);
				break;
			case key_category:
				js.set_if(i, categoryId);
				Log::Msg("Config: restore categoryId '%d'\n", categoryId);
				break;
			case key_status:
				js.set_if(i, statusFlags);
				Log::Msg("Config: restore statusFlags '%d'\n", statusFlags);
				break;
			case key_setup:
				js.copy(i, _setupCode, sizeof(_setupCode));
				Log::Msg("Config: restore setupCode '%s'\n", setupCode);
				break;
			case key_port:
				js.set_if(i, port);
				Log::Msg("Config: restore port '%d'\n", port);
				port = swap_16(port);
				break;
			case key_keys:
				// keys array must contain exactly two members
				if (js.size(i) == 2)
				{
					int k1 = js.find(i, 0);
					int k2 = js.find(i, 1);
					Log::Msg("Config: restore keys '%.*s' '%.*s'\n", js.length(k1), js.start(k1),
						js.length(k2), js.start(k2));
					keys.Restore(js.start(k1), js.length(k1), js.start(k2), js.length(k2));
				}
				else
					keys.Reset();
				break;
			case key_pairings:
				pairings.Reset();
				for (int k = 0; k < js.size(i); k++)
				{
					int r = js.find(i, k);	// pairing record ["id","key","perm"]
					if (js.type(r) == Hap::Json::JSMN_ARRAY && js.size(r) == 3)
					{
						int id = js.find(r, 0);
						int key = js.find(r, 1);
						uint8_t perm;
						if (id > 0 && key > 0 && js.set_if(js.find(r, 2), perm))
						{
							if (pairings.Add(js.start(id), js.length(id), js.start(key), js.length(key), perm))
								Log::Msg("Config: restore pairing '%.*s' '%.*s' %d\n", js.length(id), js.start(id),
									js.length(key), js.start(key), perm);
						}
					}
				}
				break;
			case key_db:
				db->Restore(js, i);
				break;
			default:
				break;
			}
		}

		ret = true;

	Ret:
		if (b != nullptr)
			delete[] b;
		fclose(f);

		if (!ret)
			Log::Msg("Config: cannot read/parse %s\n", _fileName);
		return ret;
	}
};

}
}
