#ifndef _HAP_DB_H_
#define _HAP_DB_H_

#include "Platform.h"

#include "HapTlv.h"
#include "HapBuf.h"
#include "Crypto/Crypto.h"

// Hap elements

namespace Hap
{
	constexpr uint16_t DefString = 64;			// default length of a string characteristic
	constexpr uint16_t MaxString = 64;			// max string length
	constexpr uint8_t MaxPairings = 16;			// max number of pairings the accessory supports (5.10 Add pairing)

	// global hkdf object
	extern Crypto::HkdfSha512 hkdf;

	// iOS device descriptor
	struct Controller
	{
		constexpr static unsigned CtlrLen = 36;					// max size of controller ID
		constexpr static unsigned LtpkLen = 32;					// size of controller public key
		constexpr static unsigned SessLen = sizeof(uint64_t);	// size of session ID
		constexpr static unsigned SsecLen = Crypto::Curve25519::KEY_SIZE_BYTES;	// session shared secret length
		
		// permissions
		enum class Perm : uint8_t
		{
			None = 0xFF,
			Regular = 0,
			Admin = 1,
		};

		Perm perm;
		bool sessValid;				// session ID is valid
		bool ssecValid;				// session shared secret is valid
		uint8_t ctlr[CtlrLen + 1];	// ios controller ID
		uint8_t ltpk[LtpkLen];		// ios Long term Public Key
		uint8_t ssec[SsecLen];		// Session shared secret
		uint64_t sess;				// Session ID
	};

	// HAP pairings
	struct Pairings
	{
		// Init pairings - destroy all existing records
		void Init();

		// count pairing records with matching Permissions
		//	in perm == None, count all records
		uint8_t Count(Controller::Perm perm = Controller::Perm::None);

		// add pairing record, returns nullptr if failed
		Controller* Add(const uint8_t* ctlr, uint32_t ctlr_len, const uint8_t* ltpk, Controller::Perm perm);
		Controller* Add(const Hap::Tlv& ctlr, const Hap::Tlv& ltpk, Controller::Perm perm);

		// update controller permissions
		bool Update(const Hap::Tlv& ctlr, Controller::Perm perm);
	
		// remove controller
		const Controller* Remove(const Hap::Tlv& id);

		// get pairing record, returns nullptr if not found
		Controller* Get(const Hap::Tlv& ctlr);

		bool forEach(std::function<bool(const Controller*)>);

		// Add resumable session ID
		void AddSess(Controller* ctlr, const Hap::Buf& sess, const Hap::Buf& ssec);
		
		// Find controller by resumable session ID
		Controller* GetSess(const Hap::Tlv& sess);

		Controller ctl[MaxPairings];
	};	

	namespace StatusFlag	// R2: 6.4
	{
		enum _flag : uint8_t
		{
			Paired = 0x00,
			NotPaired = 0x01,
			NotConfiguredForWiFi = 0x02,
			ProblemDetected = 0x04
		};
	}

	// HAP Server configuration
	//	platform should alloc storage and define Init/Erase/Save/Update
	namespace Config
	{
		extern char name[];					// Accessory name - used as initial Bonjour name and as AIS name of aid=1
		extern char model[];				// Model name (Bonjour and AIS)
		extern char manufacturer[];			// Manufacturer- used by AIS (Accessory Information Service)
		extern char serialNumber[];			// Serial number in arbitrary format
		extern char firmwareRevision[];		// Major[.Minor[.Revision]]
		extern char hardwareRevision[];		// hardware revision string x[.y[.z]]
		extern char setupCode[];			// setupCode code XXX-XX-XXX
		extern uint8_t setupVerifier[];		// pre-generated setup verifier and salt
		extern uint8_t setupSalt[];			
		extern uint8_t deviceId[6];			// Device ID (new deviceId generated on each factory reset)
		extern uint8_t categoryId;			// category identifier
		extern uint8_t statusFlags;			// status flags
		extern uint32_t configNum;			// Current configuration number, incremented on db change
		extern uint16_t globalStateNum;		// Global State Number (GSN - R2: 7.4.1.8)
		extern uint16_t port;				// TCP port of HAP service in net byte order
		extern Pairings pairings;			// pairings
		extern Crypto::Ed25519 keys;		// accessory long term keys

		void Init();
		void Reset(
			bool manuf		// manufacturing defaults
			);
		void Erase();
		void Save();
		void Update();

		const char* deviceIdStr();
		uint8_t deviceIdLen();
	};

	// IP: service/characteristic properties (R2: 6.3).
	// BLE: Additional parameter types (R2: 7.3.3.4).
	enum class Type : uint8_t
	{					//									IP				BLE Addl Param type
		Null = 0,
		AccIid,			// Accessory Instance ID			*
		AccSvc,			// Array of Service Objects			*
		
		SvcType,		// Service Type						short UUID		06
		SvcIid,			// Service Instance ID				*				07
		SvcChar,		// Array of Characteristic objects	*
		SvcProp,		// Service is hidden, prim, conf	*				0F
		SvcLinked,		// Array of IIDs of linked svcs		*				10
		
		CharType, 		// Char Type						short UUID		04
		CharIid, 		// Char Instance ID					*				05
		Value,			// Char Value						*				01
		Permissions,	// Char Permissions					*				0A
		Event,			// Event notifications enabled		*				CCCD
		Description,	// User description					*				0B
		Format,			// Format of the Value				*				0C
		Unit,			// Unit of the Value				*				0C
		MinValue,		// Minimum Value					*				0D
		MaxValue,		// Maximum Value					*				0D
		StepValue,		// Step Value						*				0E
		MaxLength,		// Max Length of string Value		*
		MaxDataLength,	// Max Length of data Value			*
		ValidValues,	// Array of valid values			*				11
		ValidRange,		// Range of valid values			*				12
		Ttl, 			// Timed write length				*				08 
	};
	const char* TypeToStr(Type t);

	// IP: Characteristic permissions (R2: 6.3.3).
	// BLE: Characteristic property (R2: 7.4.4.6.1).
	struct Perm 
	{
		using T = uint16_t;
		enum	// non-scoped enum to avoid casting when building bitmasks
		{
			rd = 0x0001,	// Read without secure session					*
			wr = 0x0002,	// Write without secure session					*
			aa = 0x0004,	// Additional Authorization data*				02
			tw = 0x0008,	// Timed Write					*				*
			pr = 0x0010,	// Secure (Paired) Reads		*				*
			pw = 0x0020,	// Secure (Paired) Writes		*				*
			hd = 0x0040,	// Hidden						*				*
			ev = 0x0080,	// Events in connected state	*				*
			de = 0x0100,	// Events in disconnected state					*
			bn = 0x0200,	// Broadcast notify								*

			nt = 0x8000,	// no timeout indicator (BLE pair setup)
		};
	};

	// BLE: characteristic format.
	//	BT_SIG Char Presentation Format Descriptor (R2: 7.4.4.6.3).
	// IP: Conversion to JSON strings (R2: tables 6-5, 6-6) is done in Hap::Ip
	enum class Format : uint8_t
	{
		Null,
		Bool,
		Uint8,
		Uint16,
		Uint32,
		Uint64,
		Int,
		Float,
		String,
		Data,
		Tlv,

		Uuid,
		Format,
		Unit,

		Iid,		// Instance ID property
	};
	const char* FormatToStr(Format f);

	enum class Unit : uint8_t
	{
		Unitless,
		Celsius, 
		Arcdegrees,
		Percentage,
		Lux,
		Seconds,
	};

	namespace SvcProp
	{
		using T = uint16_t;
		enum _prop : T
		{
			Primary = 0x0001,
			Hidden = 0x0002,
			Configurable = 0x0004
		};
	}

	// HAP Status - defines BLE PDU status codes (R2: 7.3.6)
	//				TODO: corresponding HTTP status (R2: 6.7.1.4)
	enum class Status : uint8_t
	{
		Success = 0x00,
		UnsupportedPDU = 0x01,
		MaxProcedures = 0x02,
		InsufficientAuthorization = 0x03,
		InvalidInstanceID = 0x04,
		InsufficientAuthentication = 0x05,
		InvalidRequest = 0x06,

		Busy = 0x80,		// internal use
	};


	// Format -> C type conversions
	struct prop_simple { static constexpr bool is_simple = true; };
	struct prop_not_simple { static constexpr bool is_simple = false; };
	template <Format> struct prop
	{
	};
	template<> struct prop<Format::Null> : prop_simple
	{
		using T = uint8_t;
	};
	template<> struct prop<Format::Bool> : prop_simple
	{
		using T = bool;
	};
	template<> struct prop<Format::Uint8> : prop_simple
	{
		using T = uint8_t;
	};
	template<> struct prop<Format::Uint16> : prop_simple
	{
		using T = uint16_t;
	};
	template<> struct prop<Format::Uint32> : prop_simple
	{
		using T = uint32_t;
	};
	template<> struct prop<Format::Uint64> : prop_simple
	{
		using T = uint64_t;
	};
	template<> struct prop<Format::Int> : prop_simple
	{
		using T = int32_t;
	};
	template<> struct prop<Format::Float> : prop_simple
	{
		using T = float;
	};
	template<> struct prop<Format::String> : prop_not_simple
	{
		using T = Buf;
	};
	template<> struct prop<Format::Data> : prop_not_simple
	{
		using T = Buf;
	};
	template<> struct prop<Format::Tlv> : prop_not_simple
	{
		using T = Buf;
	};
	template<> struct prop<Format::Uuid> : prop_not_simple
	{
		using T = Uuid;
	};
	template<> struct prop<Format::Format> : prop_simple
	{
		using T = Format;
	};
	template<> struct prop<Format::Unit> : prop_simple
	{
		using T = Unit;
	};
	template<> struct prop<Format::Iid> : prop_simple
	{
		using T = uint16_t;
	};

	// Instance ID
	//	IP: 64bit
	//	BLE: 16 bit
   	using iid_t = prop<Format::Iid>::T;
	constexpr iid_t null_id = 0;

	// forwards
	class Characteristic;
	class Service;
	class Session;
	class Operation;

	// Property base class
	class Property
	{
	public:		
		STAILQ_ENTRY(Property) entry = {nullptr};
		
		Characteristic* charOwner = nullptr;

		Type type;			// property type
		Format fmt;			// property format
		Buf value;			// property value - points to either internal _value or external buffer

		Property(Type type_, Format fmt_)
		: type(type_), fmt(fmt_)
		{
		}

		uint8_t* data()
		{
			return value.p();
		}

		auto size()
		{
			return value.s();
		}

		auto len()
		{
			return value.l();
		}
	};

	template<Type TYPE, Format F, typename Enable = void>
	struct Prop;

	// simple property, data is stored in internal memory of property itself
	template<Type TYPE, Format F>
	struct Prop <TYPE, F, std::enable_if_t< prop<F>::is_simple > > : public Property
	{
	public:
		using T = typename prop<F>::T;

		Prop()
		: Property(TYPE, F)
		{
			// point property value to internal memory
			value = Buf(_mem, sizeof(_mem), sizeof(T));	
		}

		// assign - copy value into internal memory
		void operator=(T v)
		{
			*reinterpret_cast<T*>(_mem) = v;
			value.l(sizeof(T));
		}

		// extract value from property
		T operator()()
		{
			return *reinterpret_cast<T*>(_mem);
		}
	protected:
		uint8_t _mem[sizeof(T)];
	};

	// property with external data (String, Data, Uuid)
	template<Type TYPE, Format F>
	struct Prop <TYPE, F, std::enable_if_t< !prop<F>::is_simple > > : public Property
	{
	public:
		using T = typename prop<F>::T;

		Prop()
			: Property(TYPE, F)
		{
		}

		// assign - copy external data ptr to internal Buf
		void operator=(const T& v)
		{
			if constexpr (F == Format::Uuid)	
			{	
				// special case for Uuid which is not Buf
				auto len = v.size();
				value = Buf((Buf::T*)(v.data()), len);
			}
			else
			{
				value = v;
			}
		}

		T& operator()()
		{
			return value;
		}
	};

	// array of simple values
	template<Type TYPE, Format F, uint8_t S, typename Enable = void>
	struct PropArray;
	template<Type TYPE, Format F, uint8_t S>
	struct PropArray<TYPE, F, S, std::enable_if_t< prop<F>::is_simple > > : public Property
	{
	public:
		using T = typename prop<F>::T;

		PropArray()
		: Property(TYPE, F) 
		{
			value = Buf(_value, sizeof(_value));	
		}

		T& operator[](int i)
		{
			return _value[i];
		}
	private:
		T _value[S];
	};

	using PropAccIid = Prop<Type::AccIid, Format::Iid>;
	using PropSvcType = Prop<Type::SvcType, Format::Uuid>;
	using PropSvcIid = Prop<Type::SvcIid, Format::Iid>;
	using PropSvcProp = Prop<Type::SvcProp, Format::Uint16>;
	template<uint8_t S> using PropSvcLinked = PropArray<Type::SvcLinked, Format::Iid, S>;
	using PropCharType = Prop<Type::CharType, Format::Uuid>;
	using PropCharIid = Prop<Type::CharIid, Format::Iid>;
	template<Format F> using PropValue = Prop<Type::Value, F>;
	using PropPermissions = Prop<Type::Permissions, Format::Uint16>;
	using PropDescription = Prop<Type::Description, Format::String>;
	using PropFormat = Prop<Type::Format, Format::Format>;
	using PropUnit = Prop<Type::Unit, Format::Unit>;
	template<Format F> using PropMinValue = Prop<Type::MinValue, F>;
	template<Format F> using PropMaxValue = Prop<Type::MaxValue, F>;
	template<Format F> using PropStepValue = Prop<Type::StepValue, F>;
	using PropMaxLength = Prop<Type::MaxLength, Format::Uint8>;
	using PropMaxDataLength = Prop<Type::MaxDataLength, Format::Uint32>;
	template<Format F, uint8_t S> using PropValidValues = PropArray<Type::ValidValues, F, S>;
	template<Format F> using PropValidRange = PropArray<Type::ValidValues, F, 2>;
	using PropTtl = Prop<Type::Permissions, Format::Uint16>;

	// characteristic base class
	class Characteristic
	{
	public:
		STAILQ_ENTRY(Characteristic) entry;				// entry in Service.char_list
		STAILQ_HEAD(_prop_list, Property) prop_list;	// list of properties this char contains

		Service* svcOwner = nullptr;		// service this char belongs to

		const char* name;

		// event notifications
		bool event;			// event notifications enabled status 
							// 	This is OR of enabled status of all open sessions
							//	Protocol level maintains per-session enable
		bool bc_event;		// broadcast notifications enabled status 
		uint8_t bc_int;		// broadcast interval, (1 - 20ms, 2 - 1280 ms, 3 - 2560 ms)

		// timed write support
		Buf* twValue;		// buffer holding timed write value
		bool twRetResp;		// write response requested
		Time::T twExpire;	// timed write expiration time

		// common propertis included in each characteristic
		PropCharType type;		// type/uuid
		PropCharIid iid;		// instance id
		PropPermissions perm;	// HAP permissions
		PropFormat format;		// data format
		
		Property* ptrVal;		// pointer to value property (allows access to value buffer without casting to Char<Format>)

		// read/write event handlers
		//	called from default Read/Write on R/W requests from controller
		//Hap::Status (*onRead)(Characteristic* ch);
		//Hap::Status (*onWrite)(Characteristic* ch);
		std::function<Hap::Status()> onRead;
		std::function<Hap::Status()> onWrite;

		// Event indication context - platform-dependent
		EventCtx eventCtx;

		void init(const char* name_)
		{
			name = name_;
			STAILQ_INIT(&prop_list);
			add(&type);
			add(&iid);
			add(&perm);
			add(&format);

			ptrVal = nullptr;

			onRead = nullptr;
			onWrite = nullptr;
		}

		// Add property to property list
		void add (Property* prop)
		{
			STAILQ_INSERT_TAIL(&prop_list, prop, entry);

			prop->charOwner = this;
		}

		// Find property by type
		Property* get(Type type)
		{
			Property* prop;
			STAILQ_FOREACH(prop, &prop_list, entry)
			{
				if (prop->type == type)
					return prop;
			}
			return NULL;
		}

        void forEachProp(std::function<void(Hap::Property*)> cb)
        {
            Hap::Property* pr;
            STAILQ_FOREACH(pr, &prop_list, entry)
            {
                cb(pr);
            }
        }

		// return Characteristic Type (128-bit UUID)
        uint8_t* uuid()
        {
            return type.data();
        }

		// Default Read/Write read from and write to value prop.
		// Override methods when non-standard handling of r/w is required.
		virtual Hap::Status Read(Operation* op, Buf& req, Buf& rsp);
		virtual Hap::Status Write(Operation* op, Buf& req, Buf& rsp);

		// Default Connected Event indication enable/disable
		virtual void ConnectedEvent(Hap::Session* sess, bool enable);

		// Default Broadcast Event indication enable/disable
		virtual void BroadcastEvent(Hap::Session* sess, bool enable, uint8_t interval = 0);

		// indicate event if enabled
		void Indicate();
	};

	// characteristic with Value
	template<Format F>
	class Char : public Characteristic
	{
	public:
		using T = typename PropValue<F>::T;
		PropValue<F> value;

		void init(iid_t& iid_, const char* name_, const Uuid& uuid_, Perm::T P)
		{
			Characteristic::init(name_);
			format = F;
			add(&value);
			ptrVal = &value;
			type = uuid_;
			iid = iid_++;
			perm = P;
		}
	};

	// generic String characteristic
	template<uint16_t L>
	struct CharString : public Char<Format::String>
	{
		char v[L];     // characteristic value

		void init(iid_t& iid, const char* name, const Uuid& uuid, Perm::T perm, const char* v_ = nullptr)
		{
			if (v != nullptr)
				strncpy(v, v_, sizeof(v));
			else
				memset(v, 0, sizeof(v));
			Char::init(iid, name, uuid, perm);
			value = Buf{v, sizeof(v)};
		}
	};


	class Service
	{
	public:
		STAILQ_ENTRY(Service) entry;	// entry in Accessory.svc_list
		STAILQ_HEAD(_char_list, Characteristic) char_list;	// characteristics
		uint8_t char_count;

		const char* name;

		// Service propertis
		PropSvcType type;		// type/uuid
		PropSvcIid iid;			// instance id
		PropSvcProp prop;

		// TODO: linked services

		void init(iid_t& iid_, const char* name_, const Uuid& uuid_, PropSvcProp::T prop_ = 0)
		{
			STAILQ_INIT(&char_list);
			char_count = 0;
			iid = iid_++;
			name = name_;
            type = uuid_;
			prop = prop_;
		}

		void add (Characteristic* ch)
		{
			STAILQ_INSERT_TAIL(&char_list, ch, entry);
			char_count++;
			ch->svcOwner = this;
		}

        void forEachChar(std::function<void(Hap::Characteristic*)> cb)
        {
            Hap::Characteristic* ch;
            STAILQ_FOREACH(ch, &char_list, entry)
            {
                cb(ch);
            }
        }

        uint8_t* uuid()
        {
            return type.data();
        }
	};

	class Session
	{
	public:
		Session()
		{
			reset();
		}

		uint8_t id;			// sequential ID

		Crypto::Curve25519 curve;				// Session securiry keys (used on Pair Verivication phase)
		const Controller* ctlrPaired;			// paired iOS device
		bool secured;							// session is secured
		uint8_t AccessoryToControllerKey[32];
		uint8_t ControllerToAccessoryKey[32];
		uint64_t recvSeq;
		uint64_t sendSeq;
		uint8_t key[32];	// session key
		const Controller* ctlrRemoved;			// controller removed by remove request
												// after sending pending response, disconnect all sessions
												// to this controller

		void reset()
		{
			curve.erase();
			ctlrPaired = nullptr;
			secured = false;
			memset(AccessoryToControllerKey, 0, sizeof(AccessoryToControllerKey));
			memset(ControllerToAccessoryKey, 0, sizeof(ControllerToAccessoryKey));
			recvSeq = 0;
			sendSeq = 0;
			memset(key, 0, sizeof(key));
			ctlrRemoved = nullptr;
		}
	};

	// generic operation context passed to DB operations (char read/write/...)
	struct Operation
	{
		Async async;	// async operation context
		Session* sess;	// session context

		Operation()
		{
			async.init(this);
		}
	};
};

#endif /*_HAP_DB_H_*/