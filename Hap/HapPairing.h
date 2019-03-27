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
		uint8_t Count(Controller::Perm perm = Controller::None);

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