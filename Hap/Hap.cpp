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

#include "Hap.h"

namespace Hap
{
	const char* Config::key[] =
	{
		"name",
		"model",
		"manufacturer",
		"serialNumber",
		"firmwareRevision",
		"deviceId",
		"configNum",
		"categoryId",
		"statusFlags",
		"setupCode",
		"port",
		"keys",
		"pairings",
		"db"
	};

	uint8_t Pairings::Count(Controller::Perm perm)
	{
		uint8_t cnt = 0;
		for (unsigned i = 0; i < sizeofarr(_db); i++)
		{
			Controller* ios = &_db[i];

			if (ios->perm == Controller::None)
				continue;

			if (perm == Controller::None || perm == ios->perm)
				cnt++;
		}

		return cnt;
	}

	bool Pairings::Add(const uint8_t* id, uint32_t id_len, const uint8_t* key, Controller::Perm perm)
	{
		for (unsigned i = 0; i < sizeofarr(_db); i++)
		{
			Controller* rec = &_db[i];
			if (rec->perm == Controller::None)
			{
				// add new record
				memset(rec->id, 0, Controller::IdLen);
				rec->idLen = uint8_t(id_len > Controller::IdLen ? Controller::IdLen : id_len);
				memcpy(rec->id, id, rec->idLen);
				memcpy(rec->key, key, Controller::KeyLen);
				rec->perm = perm;

				return true;
			}
			else if (memcmp(rec->id, id, id_len) == 0)
			{
				// Id matches, TODO

				return true;
			}
		}

		return false;
	}

	bool Pairings::Add(const Hap::Tlv::Item& id, const Hap::Tlv::Item& key, Controller::Perm perm)
	{
		if (id.l() > Controller::IdLen)
		{
			Log::Err("Pairings: Invalid ID length %d\n", id.l());
			return false;
		}

		if (key.l() != Controller::KeyLen)
		{
			Log::Err("Pairings: Invalid key length %d\n", key.l());
			return false;
		}

		return Add(id.p(), id.l(), key.p(), perm);
	}

	bool Pairings::Update(const Hap::Tlv::Item& id, Controller::Perm perm)
	{
		if (id.l() > Controller::IdLen)
			return false;

		for (unsigned i = 0; i < sizeofarr(_db); i++)
		{
			Controller* ios = &_db[i];

			if (ios->perm == Controller::None)	// empty record
				continue;

			if (memcmp(ios->id, id.p(), id.l()) == 0)
			{
				ios->perm = perm;
				return true;
			}
		}

		return false;
	}

	bool Pairings::Remove(const Hap::Tlv::Item& id)
	{
		if (id.l() > Controller::IdLen)
			return false;

		for (unsigned i = 0; i < sizeofarr(_db); i++)
		{
			Controller* ios = &_db[i];

			if (memcmp(ios->id, id.p(), id.l()) == 0)
			{
				ios->perm = Controller::None;	// mark record empty
				return true;
			}
		}

		return true;
	}

	const Controller* Pairings::Get(const Hap::Tlv::Item& id)
	{
		if (id.l() > Controller::IdLen)
			return nullptr;

		for (unsigned i = 0; i < sizeofarr(_db); i++)
		{
			Controller* ios = &_db[i];

			if (ios->perm == Controller::None)	// empty record
				continue;

			if (memcmp(ios->id, id.p(), id.l()) == 0)
				return ios;
		}

		return nullptr;
	}

	bool Pairings::forEach(std::function<bool(const Controller*)> cb)
	{
		for (unsigned i = 0; i < sizeofarr(_db); i++)
		{
			Controller* ios = &_db[i];

			if (ios->perm == Controller::None)	// empty record
				continue;

			if (!cb(ios))
				return false;
		}

		return true;
	}

	void Pairings::init()		// Init pairings - destroy all existing records
	{
		for (unsigned i = 0; i < sizeofarr(_db); i++)
		{
			Controller* ios = &_db[i];

			ios->perm = Controller::None;
		}
	}
}
