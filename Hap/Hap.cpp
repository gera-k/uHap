/*
	Copyright(c) 2020 Gera Kazakov
	SPDX-License-Identifier: Apache-2.0
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

			if (ios->perm == Controller::Perm::None)
				continue;

			if (perm == Controller::Perm::None || perm == ios->perm)
				cnt++;
		}

		return cnt;
	}

	bool Pairings::Add(const uint8_t* id, uint32_t id_len, const uint8_t* key, Controller::Perm perm)
	{
		for (unsigned i = 0; i < sizeofarr(_db); i++)
		{
			Controller* rec = &_db[i];
			if (rec->perm == Controller::Perm::None)
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

			if (ios->perm == Controller::Perm::None)	// empty record
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
				ios->perm = Controller::Perm::None;	// mark record empty
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

			if (ios->perm == Controller::Perm::None)	// empty record
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

			if (ios->perm == Controller::Perm::None)	// empty record
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

			ios->perm = Controller::Perm::None;
		}
	}
}
