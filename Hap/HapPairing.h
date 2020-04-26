/*
	Copyright(c) 2020 Gera Kazakov
	SPDX-License-Identifier: Apache-2.0
*/

#ifndef _HAP_PAIRING_H_
#define _HAP_PAIRING_H_

namespace Hap
{
	// pairings DB, persistent across reboots
	class Pairings
	{
	public:
		// count pairing records with matching Permissions
		//	in perm == None, cput all records
		uint8_t Count(Controller::Perm perm = Controller::Perm::None);

		// add pairing record, returns false if failed
		bool Add(const uint8_t* id, uint32_t id_len, const uint8_t* key, Controller::Perm perm);
		bool Add(const Hap::Tlv::Item& id, const Hap::Tlv::Item& key, Controller::Perm perm);

		// update controller permissions
		bool Update(const Hap::Tlv::Item& id, Controller::Perm perm);

		// remove controller
		bool Remove(const Hap::Tlv::Item& id);

		// get pairing record, returns nullptr if not found
		const Controller* Get(const Hap::Tlv::Item& id);

		bool forEach(std::function<bool(const Controller*)> cb);

	protected:
		// Init pairings - destroy all existing records
		void init();

		Controller _db[MaxPairings];
	};
}

#endif /*_HAP_PAIRING_H_*/