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

#ifndef _HAP_H_
#define _HAP_H_

#include "Platform.h"

#include <stdint.h>
#include <utility>
#include <functional>

#include "Crypto/Crypto.h"
#include "Crypto/MD.h"
#include "Crypto/Srp.h"

namespace Hap
{
	// global constants
	constexpr uint8_t MaxPairings = 16;						// max number of pairings the accessory supports (4.11 Add pairing)
	constexpr uint8_t MaxHttpSessions = 8;					// max HTTP sessions (5.2.3 TCP requirements)
	constexpr uint8_t MaxHttpHeaders = 20;					// max number of HTTP headers in request
	constexpr uint8_t MaxHttpTlv = 10;						// max num of items in incoming TLV
	constexpr uint16_t MaxHttpBlock = 1024;					// max size of encrypted data block (5.5.2 Session securiry)
	constexpr uint16_t MaxHttpFrame = MaxHttpBlock + 2 + 16;// max size of encrypted HTTP frame (size + data + tag)

	constexpr uint16_t DefString = 64;		// default length of a string characteristic
	constexpr uint16_t MaxString = 64;		// max string length


	// Bonjour-related flags
	namespace Bonjour
	{
		enum FeatureFlag
		{
			SupportsHapPairing = 1,
		};

		enum StatusFlag
		{
			NotPaired = 0x01,
			NotConfiguredForWiFi = 0x02,
			ProblemDetected = 0x04
		};
	}

	// iOS device
	struct Controller
	{
		constexpr static unsigned IdLen = 36;	// max size of controller ID
		constexpr static unsigned KeyLen = 32;	// size of controller public key
		using Id = uint8_t[IdLen];
		using Key = uint8_t[KeyLen];

		// permissions
		enum Perm
		{
			None = 0xFF,
			Regular = 0,
			Admin = 1,
		};

		Perm perm;
		uint8_t idLen;
		Id id;
		Key key;
	};

	// HAP Server configuration
	//	this class defines interface to implementation-dependent config info storage
	//	implementation should define methods for saving/restoring/resetting the config
	//	implementatin may save/restore all parameters, or just some of them such as config number and deviceId
	class Config
	{
	public:
		const char* name;				// Accessory name - used as initial Bonjour name and as AIS name of aid=1
		const char* model;				// Model name (Bonjour and AIS)
		const char* manufacturer;		// Manufacturer- used by AIS (Accessory Information Service)
		const char* serialNumber;		// Serial number in arbitrary format
		const char* firmwareRevision;	// Major[.Minor[.Revision]]
		const char* deviceId;			// Device ID (XX:XX:XX:XX:XX:XX, new deviceId generated on each factory reset)
		uint32_t configNum;				// Current configuration number, incremented on db change
		uint8_t categoryId;				// category identifier
		uint8_t statusFlags;			// status flags

		const char* setupCode;			// setupCode code XXX-XX-XXX
		uint16_t port;					// TCP port of HAP service in net byte order
		bool BCT;						// Bonjour Compatibility Test

		std::function<void()> Update;	// config update notification

		void Init(bool reset_ = false)
		{
			// restore saved config	or create new default one	
			if (!_restore())
				_default();
			
			// if reset is requested, reset to defaults
			if (reset_)
				_reset();
			
			// save new config
			_save();
		}

		void Save()
		{
			_save();
		}

	protected:
		enum
		{
			key_name,
			key_model,
			key_manuf,
			key_serial,
			key_firmware,
			key_device,
			key_config,
			key_category,
			key_status,
			key_setup,
			key_port,
			key_keys,
			key_pairings,
			key_db,
			key_max
		};
		static const char* key[key_max];

		virtual void _default() = 0;
		virtual void _reset() = 0;
		virtual bool _restore() = 0;
		virtual bool _save() = 0;
	};

	// Hap::config - global congiguration data
	extern Config* config;	

	// HAP session ID
	//	some DB characteristics and methods depend on HAP session context
	//	example - Event Notification state and pending events
	using sid_t = uint8_t;
	constexpr sid_t sid_invalid = 0xFF;
	constexpr sid_t sid_max = MaxHttpSessions - 1;

	// forward declarations
	class Db;
	class Pairings;
}

#include "Hap/Buffer.h"
#include "Hap/HapJson.h"
#include "Hap/HapTlv.h"
#include "Hap/HapHttp.h"
#include "Hap/HapTcp.h"
#include "Hap/HapMdns.h"
#include "Hap/HapDb.h"
#include "Hap/HapPairing.h"
#include "Hap/HapAppleCharacteristics.h"
#include "Hap/HapAppleServices.h"

#endif /*_HAP_H_*/