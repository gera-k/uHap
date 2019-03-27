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

#ifndef _HAP_DB_H_
#define _HAP_DB_H_

namespace Hap
{
	using iid_t = uint32_t;		// limit instance IDs range
	constexpr iid_t null_id = 0;

	// Hap operation status
	enum class Status : uint8_t
	{
		Success = 0,
		RequestDenied,
		UnableToCommunicate,
		ResourceIsBusy,
		CannotWrite,
		CannotRead,
		NotificationNotSupported,
		OutOfResources,
		OperationTimedOut,
		ResourceNotExist,
		InvalidValue,
		InsufficientAuthorization
	};
	const char* StatusStr(Status c);

	// 'unit' enumerator values and string representations
	enum class UnitId : uint8_t
	{
		celsius,
		percentage,
		arcdegrees,
		lux,
		seconds
	};
	const char* UnitStr(UnitId c);

	// Property 'key' enum and string representation
	enum class KeyId : uint8_t
	{
		aid,
		services,

		type,
		iid,
		characteristics,
		hidden,
		primary,
		linked,

		value,
		perms,
		ev,
		description,
		format,
		unit,
		minValue,
		maxValue,
		minStep,
		maxLen,
		maxDataLen,
		valid_values,
		valid_values_range
	};
	const char* KeyStr(KeyId c);

	// Property 'format' enum and string representation
	//	also used to define format of all other properties
	enum class FormatId : uint8_t
	{
		Null = 0,

		// base properties
		Bool,			// bool
		Uint8,			// uint8_t
		Uint16,			// uint16_t
		Uint32,			// uint32_t
		Uint64,			// uint64_t
		Int,			// int32_t
		Float,			// double
		ConstStr,		// const char *

		// enumerated properties (internal representation - uint8_t, external - string)
		Format,			// FormatId
		Unit,			// UnitId	

		// variable-size properties
		String,			// char[S]
		Tlv8,			// uint8_t[S]
		Data,			// uint8_t[S]

		Id,				// iid_t
		IdArray,		// uint64_t[S]
		PtrArray,		// void*[S]
	};
	const char* FormatStr(FormatId c);

	// this set of templates maps FormatId to:
	//	- C type used for internal representation
	//	- Read function to format the property to string
	//	- Write function to convert JSON token to internal representation
	template <FormatId> struct hap_type;
	template<> struct hap_type<FormatId::Null>
	{
		using type = uint8_t;
		static inline int Read(char* s, uint32_t max, type v)
		{
			return snprintf(s, max, "null");
		}
		static inline bool Write(const Hap::Json::Obj& js, int t, type& v)
		{
			return js.is_null(t);
		}
	};
	template<> struct hap_type<FormatId::Bool>
	{
		using type = bool;
		static inline int Read(char* s, uint32_t max, type v)
		{
			return snprintf(s, max, "%s", v ? "true" : "false");
		}
		static inline bool Write(const Hap::Json::Obj& js, int t, type& v)
		{
			return js.is_bool(t, v);
		}
	};
	template<> struct hap_type<FormatId::Uint8>
	{
		using type = uint8_t;
		static inline int Read(char* s, uint32_t max, type v)
		{
			return snprintf(s, max, "%u", v);
		}
		static inline bool Write(const Hap::Json::Obj& js, int t, type& v)
		{
			return js.is_number<type>(t, v);
		}
	};
	template<> struct hap_type<FormatId::Uint16>
	{
		using type = uint16_t;
		static inline int Read(char* s, uint32_t max, type v)
		{
			return snprintf(s, max, "%u", v);
		}
		static inline bool Write(const Hap::Json::Obj& js, int t, type& v)
		{
			return js.is_number<type>(t, v);
		}
	};
	template<> struct hap_type<FormatId::Uint32>
	{
		using type = uint32_t;
		static inline int Read(char* s, uint32_t max, type v)
		{
			return snprintf(s, max, "%u", v);
		}
		static inline bool Write(const Hap::Json::Obj& js, int t, type& v)
		{
			return js.is_number<type>(t, v);
		}
	};
	template<> struct hap_type<FormatId::Uint64>
	{
		using type = uint64_t;
		static inline int Read(char* s, uint32_t max, type v)
		{
			return snprintf(s, max, "%llu", v);
		}
		static inline bool Write(const Hap::Json::Obj& js, int t, type& v)
		{
			return js.is_number<type>(t, v);
		}
	};
	template<> struct hap_type<FormatId::Int>
	{
		using type = int32_t;
		static inline int Read(char* s, uint32_t max, type v)
		{
			return snprintf(s, max, "%d", v);
		}
		static inline bool Write(const Hap::Json::Obj& js, int t, type& v)
		{
			return js.is_number<type>(t, v);
		}
	};
	template<> struct hap_type<FormatId::Float>
	{
		using type = double;
		static inline int Read(char* s, uint32_t max, type v)
		{
			return snprintf(s, max, "%lg", v);
		}
		static inline bool Write(const Hap::Json::Obj& js, int t, type& v)
		{
			return js.is_number<type>(t, v);
		}
	};
	template<> struct hap_type<FormatId::ConstStr>
	{
		using type = const char *;
		static inline int Read(char* s, uint32_t max, type v)
		{
			return snprintf(s, max, "\"%s\"", v);
		}
		static inline bool Write(const Hap::Json::Obj& js, int t, type& v)
		{
			return false;
		}
	};
	template<> struct hap_type<FormatId::Format>
	{
		using type = FormatId;
		static inline int Read(char* s, uint32_t max, type v)
		{
			return snprintf(s, max, "\"%s\"", FormatStr(v));
		}
	};
	template<> struct hap_type<FormatId::Unit>
	{
		using type = UnitId;
		static inline int Read(char* s, uint32_t max, type v)
		{
			return snprintf(s, max, "\"%s\"", UnitStr(v));
		}
	};
	template<> struct hap_type<FormatId::String>
	{
		using type = char;
		static inline int Read(char* str, uint32_t max, type v[], int _length)
		{
			char* s = str;
			int l = snprintf(s, max, "\"%.*s\"", _length, v);
			s += l;

			return (int)(s - str);
		}
	};
	template<> struct hap_type<FormatId::Data>
	{
		using type = uint8_t;
	};
	template<> struct hap_type<FormatId::Id>
	{
		using type = iid_t;
		static inline int Read(char* s, uint32_t max, type v)
		{
			return snprintf(s, max, "%u", v);
		}
	};
	template<> struct hap_type<FormatId::IdArray>
	{
		using type = uint64_t;
		static int Read(char* str, int max, type v[], int _length)
		{
			char* s = str;
			bool comma;

			*s++ = '[';
			max--;
			if (max <= 0) goto Ret;

			comma = false;
			for (int i = 0; i < _length; i++)
			{
				if (comma)
				{
					*s++ = ',';
					max--;
					if (max <= 0) goto Ret;
				}
				int l = snprintf(s, max, "\"%lld\"", v[i]);
				s += l;
				max -= l;
				if (max <= 0) goto Ret;

				comma = true;
			}

			*s++ = ']';
		Ret:
			return (int)(s - str);
		}
	};

	// Obj - base class for most of DB objects
	//	defines set of virtual functions
	//		getId - returns object id (aid or iid), or null_id
	//		setId - sequentially sets object id, and all child ids; returns next available id
	//		isType - return true if object has property Type and its value matches t
	//		getDb - return JSON representation of Db object for GET/accessories request
	//		Write - write single characteristic
	//				returns true when it completes write to characteristic, 
	//					status of the operation is indicated in p.status
	//				returns false when characteristic not found
	//		Read - Read single characteristic
	//				returns true when it completes read from characteristic, 
	//					status of the operation is indicated in p.status
	//				returns false when characteristic not found
	class Obj
	{
	public:
		virtual iid_t getId() { return null_id; }
		virtual iid_t setId(iid_t iid) { return iid; }
		virtual bool isType(const char* t) { return false; }
		virtual void Open(sid_t sid) {}
		virtual void Close(sid_t sid) {}
		virtual int getDb(char* str, int max, sid_t sid) = 0;
		virtual int getEvents(char* str, int max, sid_t sid, iid_t aid, iid_t iid) { return 0; }

		// parsed parameters of PUT/characteristics request
		struct wr_prm
		{
			wr_prm(Hap::Json::Obj& rq_) : rq(rq_) {}

			Hap::Json::Obj& rq;			// request object

			iid_t aid = null_id;
			iid_t iid = null_id;

			bool val_present = false;	// value member is present
			bool ev_present = false;	// event member is present
			bool auth_present = false;	// authData member is present 
			bool remote_present = false;// remote member is present
			bool ev_value = false;		// event member value
			bool remote_value = false;	// remote member value
			uint8_t val_ind = 0;		// value token index in rq
			uint8_t auth_ind = 0;		// authData token index in rq

			Hap::Status status = Hap::Status::Success;
		};
		virtual bool Write(wr_prm& p, sid_t sid) { return false; };

		struct rd_prm
		{
			iid_t aid = null_id;
			iid_t iid = null_id;

			bool meta : 1;
			bool perms : 1;
			bool type : 1;
			bool ev : 1;

			char* s;
			int max;

			Hap::Status status = Hap::Status::Success;
		};
		virtual bool Read(rd_prm& p, sid_t sid) { return false; };
	};

	// ObjArray - array of Obj
	//	object storage is defined by derived classes
	class ObjArrayBase
	{
	protected:
		Obj** _obj = NULL;	// points to array storage
		uint16_t _max = 0;	// max number of elements
		uint16_t _sz = 0;	// current array size

		ObjArrayBase(Obj** obj, uint8_t max) : _obj(obj), _max(max) {}
	
	public:
		ObjArrayBase() = delete;

		// return current size of the array
		uint16_t size() const
		{
			return _sz;
		}

		// add object to the end of the array
		void set(Obj* obj)
		{
			if (_sz < _max)
				_obj[_sz++] = obj;
		}

		// add object to position i
		void set(Obj* obj, int i)
		{
			if (i < _max)
			{
				_obj[i] = obj;
				if (_sz <= i)
					_sz = i + 1;
			}
		}

		// get object at position i
		Obj* get(int i) const
		{
			if (i >= _sz)
				return nullptr;
			return _obj[i];
		}

		// get(iid_t) - seeks object by object ID
		Obj* GetObj(iid_t id)
		{
			for (int i = 0; i < _sz; i++)
			{
				Obj* obj = _obj[i];
				if (obj == nullptr)
					continue;
				if (obj->getId() == id)
					return obj;
			}
			return nullptr;
		}

		// find object using matching function
		Obj* GetObj(std::function<bool(Obj*)> match)
		{
			for (int i = 0; i < _sz; i++)
			{
				Obj* obj = _obj[i];
				if (obj == nullptr)
					continue;
				if (match(obj))
					return obj;
			}
			return nullptr;
		}

		// getDb - create JSON representation of the array
		int getDb(char* str, int max, sid_t sid, const char* name = nullptr) const
		{
			char* s = str;
			int l;
			bool comma;

			if (max <= 0) goto Ret;

			if (name != nullptr)
			{
				l = snprintf(s, max, "\"%s\":[", name);
				s += l;
				max -= l;
				if (max <= 0) goto Ret;
			}

			comma = false;
			for (int i = 0; i < _sz; i++)
			{
				Obj* obj = _obj[i];
				if (obj != nullptr)
				{
					if (comma)
					{
						*s++ = ',';
						max--;
						if (max <= 0) goto Ret;
					}

					l = obj->getDb(s, max, sid);
					s += l;
					max -= l;
					if (max <= 0) goto Ret;
					comma = true;
				}
			}

			if (name != nullptr)
			{
				*s++ = ']';
				max--;
				if (max <= 0) goto Ret;
			}
		Ret:
			return (int)(s - str);
		}

		// getEvents
		int getEvents(char* str, int max, sid_t sid, iid_t aid = 0, iid_t iid = 0, const char* name = nullptr) const
		{
			char* s = str;
			int l;
			bool comma;

			if (max <= 0) goto Ret;

			if (name != nullptr)
			{
				l = snprintf(s, max, "\"%s\":[", name);
				s += l;
				max -= l;
				if (max <= 0) goto Ret;
			}

			comma = false;
			for (int i = 0; i < _sz; i++)
			{
				Obj* obj = _obj[i];
				if (obj != nullptr)
				{
					if (comma)
					{
						*s++ = ',';
						max--;
						if (max <= 0) goto Ret;
					}

					if (aid == 0)		// accessories array
						l = obj->getEvents(s, max, sid, obj->getId(), 0);
					else if (iid == 0)	// services array
						l = obj->getEvents(s, max, sid, aid, obj->getId());
					else				// characteristics array
						l = obj->getEvents(s, max, sid, aid, iid);
					if (l > 0)
					{
						comma = true;
						s += l;
						max -= l;
						if (max <= 0) goto Ret;
					}
					else if (comma)
					{
						s--;
						max++;
					}
				}
			}

			if (name != nullptr)
			{
				*s++ = ']';
				max--;
				if (max <= 0) goto Ret;
			}
		Ret:
			return (int)(s - str);
		}
	};
	
	// static array of DB objects
	//	max size is set on compile time through Count parameter
	template<int Count>
	class ObjArrayStatic : public ObjArrayBase
	{
	private:
		Obj* _obj[Count];
	public:
		ObjArrayStatic() : ObjArrayBase(_obj, Count) {}
	};

	namespace Property
	{
		// Hap::Property::Obj - base class of Properties
		class Obj : public Hap::Obj
		{
		protected:
			KeyId _keyId;
			Obj(KeyId keyId) : _keyId(keyId) {}
		public:
			KeyId keyId() const
			{
				return _keyId;
			}
			const char* key() const
			{
				return KeyStr(_keyId);
			}
		};

		// Hap::Property::Simple - base class for simple properties
		//	bool, int, uint, float, const string
		template<KeyId Key, FormatId Format>
		class Simple : public Obj
		{
		public:
			static constexpr KeyId K = Key;
			using T = typename hap_type<Format>::type;
		protected:
			T _v;
		public:
			Simple() : Obj(Key) {}
			Simple(T v) : Obj(Key), _v(v) {}

			T get() const { return _v; }
			void set(T v) { _v = v; }

			// get JSON-formatted characteristic descriptor
			virtual int getDb(char* str, int max, sid_t sid) override
			{
				char* s = str;
				int l;

				if (max <= 0) goto Ret;

				l = snprintf(s, max, "\"%s\":", key());
				s += l;
				max -= l;
				if (max <= 0) goto Ret;

				l = hap_type<Format>::Read(s, max, _v);
				s += l;
				max -= l;
			Ret:
				return (int)(s - str);
			}
		};

		// Hap::Property::Array - base class for array properties
		//	string, tlv8, data, linked services, valid values
		template<KeyId Key, FormatId Format, int Size>
		class Array : public Obj
		{
		public:
			using T = typename hap_type<Format>::type;
		protected:
			uint16_t _size = Size;	// max size of the array in elements
			uint16_t _length;		// current length of _v in elements
			T _v[Size];
		public:
			Array(const T* v = nullptr, uint16_t size = 0) : Obj(Key)
			{
				if (v != nullptr)
					set(v, size);
				else
					_length = Size;
			}

			const T* get() const
			{
				return _v;
			}

			void set(const T* v, uint16_t length)
			{
				if (length > _size)
					length = _size;
				memcpy(_v, v, length * sizeof(T));
				_length = length;
			}

			T get(int i) const
			{
				if (i < _length)
					return _v[i];
				return T(-1);
			}

			void set(int i, T v)
			{
				if (i < _length)
					_v[i] = v;
			}

			// get JSON-formatted characteristic descriptor
			virtual int getDb(char* str, int max) override
			{
				char*s = str;
				int l;
					
				if (max <= 0) goto Ret;

				l = snprintf(s, max, "\"%s\":", key());
				s += l;
				max -= l;
				if (max <= 0) goto Ret;

				l = hap_type<Format>::getDb(s, max, _v, _length);
				s += l;
				max -= l;

			Ret:
				return s - str;
			}
		};

		// Properties
		using aid = Simple<KeyId::aid, FormatId::Id>;
		using Type = Simple<KeyId::type, FormatId::ConstStr>;
		using InstanceId = Simple<KeyId::iid, FormatId::Id>;
		using HiddenService = Simple<KeyId::hidden, FormatId::Bool>;
		using PrimaryService = Simple<KeyId::primary, FormatId::Bool>;
		template <uint8_t Size> using LinkedServices = Array<KeyId::linked, FormatId::IdArray, Size>;

		// Permissions - bitmask, special processing required
		class Permissions : public Simple<KeyId::perms, FormatId::Uint8>
		{
		public:
			enum Perm
			{
				PairedRead = 1 << 0,
				PairedWrite = 1 << 1,
				Events = 1 << 2,
				AdditionalAuthorization = 1 << 3,
				TimedWrite = 1 << 4,
				Hidden = 1 << 5
			};

			using Simple::Simple;

			bool isEnabled(Perm p)
			{
				return (get() & p) != 0;
			}

			virtual int getDb(char* str, int max, sid_t sid) override
			{
				static const char* PermStr[] =
				{
					"pr", "pw", "ev", "aa", "tw", "hd"
				};
				char* s = str;
				bool comma;
				int l;

				if (max <= 0) goto Ret;

				l = snprintf(s, max, "\"%s\":[", key());
				s += l;
				max -= l;
				if (max <= 0) goto Ret;

				comma = false;
				for (int i = 0; i < 5; i++)
				{
					if (isEnabled(Perm(1 << i)))
					{
						if (comma)
						{
							*s++ = ',';
							max--;
							if (max <= 0) goto Ret;
						}

						l = snprintf(s, max, "\"%s\"", PermStr[i]);
						s += l;
						max -= l;
						if (max <= 0) goto Ret;

						comma = true;
					}
				}

				*s++ = ']';
			Ret:
				return (int)(s - str);
			}
		};

		class EventNotifications : public Simple<KeyId::ev, FormatId::Bool>
		{
		protected:
			bool _v[sid_max + 1];	// event notification is enabled
			bool _e[sid_max + 1];	// event notification is pending
		public:
			EventNotifications()
			{}

			T get(sid_t sid) const { return _v[sid]; }
			void set(T v, sid_t sid) { _v[sid] = v; }

			void SetEvent(bool e = true, sid_t sid = sid_invalid)
			{
				Log::Msg("SetEvent e=%d  sid=%d\n", e, sid);
				for (unsigned i = 0; i < sizeofarr(_e); i++)
					if ((i != sid) && _v[i])
						_e[i] = true;
			}
			
			bool GetAndClearEvent(sid_t sid)
			{
				bool ret = _e[sid];
				_e[sid] = false; 
				Log::Dbg("GetAndClearEvent sid=%d e=%d\n", sid, ret);
				return ret;
			}

			virtual void Open(sid_t sid) override
			{
				_v[sid] = false;
				_e[sid] = false;
			}

			virtual void Close(sid_t sid) override
			{
				_v[sid] = false;
				_e[sid] = false;
			}

			// get JSON-formatted characteristic descriptor
			virtual int getDb(char* str, int max, sid_t sid) override
			{
				char* s = str;
				int l;

				if (max <= 0) goto Ret;

				l = snprintf(s, max, "\"%s\":", key());
				s += l;
				max -= l;
				if (max <= 0) goto Ret;

				l = hap_type<FormatId::Bool>::Read(s, max, _v[sid]);
				s += l;
				max -= l;
			Ret:
				return (int)(s - str);
			}
		};

		using Description = Simple<KeyId::description, FormatId::ConstStr>;
		using Format = Simple<KeyId::format, FormatId::Format>;
		using Unit = Simple<KeyId::unit, FormatId::Unit>;
		template<FormatId F> using MinValue = Simple<KeyId::minValue, F>;
		template<FormatId F> using MaxValue = Simple<KeyId::maxValue, F>;
		template<FormatId F> using MinStep = Simple<KeyId::minStep, F>;
		using MaxLen = Simple<KeyId::maxLen, FormatId::Int>;
		using MaxDataLen = Simple<KeyId::maxDataLen, FormatId::Int>;
		template<FormatId F, int S> using ValidValues = Array<KeyId::valid_values, F, S>;
		template<FormatId F> using ValidValuesRange = Array<KeyId::valid_values_range, F, 2>;
	}

	namespace Characteristic
	{
		// Hap::Characteristic::Base
		template<int PropertyCount>	// number of optional properties
		class Base : public Obj
		{
		private:
			ObjArrayStatic<PropertyCount + 5> _prop;	// first five slots are for common properties:
			Property::Type _type;
			Property::InstanceId _iid;
			Property::Permissions _perms;
			Property::Format _format;
			Property::EventNotifications _ev;	// only valid when _perms contains Events

		protected:
			void AddProperty(Obj* pr) { _prop.set(pr); }

			void SetEvent(bool e, sid_t sid = sid_invalid) { _ev.SetEvent(e, sid); }
			bool GetAndClearEvent(sid_t sid) { return _ev.GetAndClearEvent(sid); }

		public:
			Base(
				Property::Type::T type,
				Property::Permissions::T perms,
				Property::Format::T format
			) :
				_type(type),
				_perms(perms),
				_format(format)
			{
				_prop.set(&_type, 0);
				_prop.set(&_iid, 1);
				_prop.set(&_perms, 2);
				_prop.set(&_format, 3);
				_prop.set(&_ev, 4);
			}

			virtual iid_t getId() override
			{
				return _iid.get();
			}

			virtual iid_t setId(iid_t iid) override
			{
				_iid.set(iid++);
				return iid;
			}

			virtual bool isType(const char* t) override
			{
				return strcmp(t, _type.get()) == 0;
			}

			// open session context
			virtual void Open(sid_t sid) override
			{
				// _ev is the only characteristic with per-session value
				_ev.Open(sid);
			}

			// close session context
			virtual void Close(sid_t sid) override
			{
				// _ev is the only characteristic with per-session value
				_ev.Close(sid);
			}

			// get JSON-formatted characteristic descriptor
			virtual int getDb(char* str, int max, sid_t sid) override
			{
				char* s = str;
				int l;

				if (max <= 0) goto Ret;

				*s++ = '{';
				max--;
				if (max <= 0) goto Ret;

				l = _prop.getDb(s, max, sid);
				s += l;
				max -= l;
				if (max <= 0) goto Ret;

				*s++ = '}';
			Ret:
				return (int)(s - str);
			}

			// access to common properties
			Property::Type& Type() { return _type; }
			Property::InstanceId& Iid() { return _iid; }
			Property::Permissions& Perms() { return _perms; }
			Property::Format& Format() { return _format; }
			Property::EventNotifications& EventNotifications() { return _ev; }

			// access to all properties
			template<typename Prop> Prop* GetProperty()
			{
				return static_cast<Prop*>(_prop.GetObj([](Obj* obj) -> bool {
					return static_cast<Property::Obj*>(obj)->keyId() == Prop::K;
				}));
			}
			Obj* GetProperty(KeyId keyId)
			{
				return _prop.GetObj([keyId](Obj* obj) -> bool {
					return static_cast<Property::Obj*>(obj)->keyId() == keyId;
				});
			}
		};

		using OnRead = std::function<void(Obj::rd_prm&)>;
		template<typename V> using OnWrite = std::function<void(Obj::wr_prm&, V)>;

		// Hap::Characteristic::Simple
		template<
			int PropertyCount,				// number of optional properties
			FormatId F = FormatId::Null,	// format of the Value property
			bool PerSession = false			// true if per-session value is required
		>
		class Simple : public Base<PropertyCount + 1>	// add one slot for the Value property 
		{
		public:
			using T = Property::Simple<KeyId::value, F>;	// type of Value property
			using V = typename T::T;						// C type associated with T
		
		protected:
			T _value;

			OnRead _onRead;
			OnWrite<V> _onWrite;

			using B = Base<PropertyCount + 1>;

		public:
			Simple(Hap::Property::Type::T type, Property::Permissions::T perms)
				: B(type, perms, F)
			{
				if( perms & Property::Permissions::PairedRead)
					B::AddProperty(&_value);
			}

			// get/set the value
			V Value() { return _value.get(); }
			void Value(const V& value, sid_t sid = sid_invalid)
			{ 
				V v = _value.get();

				_value.set(value);
			
				if (B::Perms().isEnabled(Property::Permissions::Events) && v != value)
					B::SetEvent(true, sid);
			}

			// set Read/Write handlers
			void onRead(OnRead h) { _onRead = h; }
			void onWrite(OnWrite<V> h) { _onWrite = h; }

			virtual int getEvents(char* str, int max, sid_t sid, iid_t aid, iid_t iid) override
			{
				char* s = str;
				int l;

				if (max <= 0) goto Ret;

				if (B::GetAndClearEvent(sid))
				{
					*s++ = '{';
					max--;
					if (max <= 0) goto Ret;

					l = snprintf(s, max, "\"aid\":%d,\"iid\":%d,", aid, B::Iid().get());
					s += l;
					max -= l;
					if (max <= 0) goto Ret;

					l = _value.getDb(s, max, sid);
					s += l;
					max -= l;

					*s++ = '}';
				}
			Ret:
				return (int)(s - str);
			}

			virtual bool Write(Obj::wr_prm& p, sid_t sid) override
			{ 
				if (p.iid != B::Iid().get())
				{
					p.status = Hap::Status::ResourceNotExist;
					return false;
				}

				// if ev present, set it first
				if (p.ev_present)
				{
					if (!B::Perms().isEnabled(Property::Permissions::Events))
					{
						p.status = Hap::Status::NotificationNotSupported;
					}
					else
					{
						B::EventNotifications().set(p.ev_value, sid);
						if (!p.ev_value)
							B::SetEvent(false);
					}
				}

				// if value is present, set it
				if (p.val_present)
				{
					if (!B::Perms().isEnabled(Property::Permissions::PairedWrite))
					{
						p.status = Hap::Status::CannotWrite;
					}
					else
					{
						// convert JSON token to internal value
						V v;
						bool rc = hap_type<F>::Write(p.rq, p.val_ind, v);

						if (rc)
						{
							//V old = Value();

							// call write handler
							if (_onWrite)
							{
								_onWrite(p, v);

								if (p.status != Hap::Status::Success)
									return true;
							}

							Value(v, sid);
						}
						else
						{
							p.status = Hap::Status::InvalidValue;
						}
					}

				}

				return true;	// true indicates that characteristic was found
			}

			virtual bool Read(Obj::rd_prm& p, sid_t sid) override
			{
				if (p.iid != B::Iid().get())
				{
					p.status = Hap::Status::ResourceNotExist;
					return false;
				}

				int l;

				// add value
				if (!B::Perms().isEnabled(Property::Permissions::PairedRead))
				{
					p.status = Hap::Status::CannotRead;
				}
				else
				{
					// call read handler, abort read if non-success status is set
					if (_onRead)
					{
						_onRead(p);

						if (p.status != Hap::Status::Success)
							return true;
					}

					*p.s++ = ',';
					p.max--;
					if (p.max <= 0) return true;

					l = _value.getDb(p.s, p.max, sid);
					p.s += l;
					p.max -= l;
					if (p.max <= 0)	return true;
				}

				Obj* prop;

				// add meta
				if (p.meta)
				{
					*p.s++ = ',';
					p.max--;
					if (p.max <= 0) return true;

					l = B::Format().getDb(p.s, p.max, sid);
					p.s += l;
					p.max -= l;
					if (p.max <= 0)	return true;

					prop = B::GetProperty(KeyId::unit);
					if (prop != nullptr)
					{
						*p.s++ = ',';
						p.max--;
						if (p.max <= 0) return true;

						l = prop->getDb(p.s, p.max, sid);
						p.s += l;
						p.max -= l;
						if (p.max <= 0)	return true;
					}

					prop = B::GetProperty(KeyId::minValue);
					if (prop != nullptr)
					{
						*p.s++ = ',';
						p.max--;
						if (p.max <= 0) return true;

						l = prop->getDb(p.s, p.max, sid);
						p.s += l;
						p.max -= l;
						if (p.max <= 0)	return true;
					}

					prop = B::GetProperty(KeyId::maxValue);
					if (prop != nullptr)
					{
						*p.s++ = ',';
						p.max--;
						if (p.max <= 0) return true;

						l = prop->getDb(p.s, p.max, sid);
						p.s += l;
						p.max -= l;
						if (p.max <= 0)	return true;
					}

					prop = B::GetProperty(KeyId::minStep);
					if (prop != nullptr)
					{
						*p.s++ = ',';
						p.max--;
						if (p.max <= 0) return true;

						l = prop->getDb(p.s, p.max, sid);
						p.s += l;
						p.max -= l;
						if (p.max <= 0)	return true;
					}

					prop = B::GetProperty(KeyId::maxLen);
					if (prop != nullptr)
					{
						*p.s++ = ',';
						p.max--;
						if (p.max <= 0) return true;

						l = prop->getDb(p.s, p.max, sid);
						p.s += l;
						p.max -= l;
						if (p.max <= 0)	return true;
					}
				}

				// add perms
				if (p.perms)
				{
					*p.s++ = ',';
					p.max--;
					if (p.max <= 0) return true;

					l = B::Perms().getDb(p.s, p.max, sid);
					p.s += l;
					p.max -= l;
					if (p.max <= 0)	return true;
				}

				// add type
				if (p.type)
				{
					*p.s++ = ',';
					p.max--;
					if (p.max <= 0) return true;

					l = B::Type().getDb(p.s, p.max, sid);
					p.s += l;
					p.max -= l;
					if (p.max <= 0)	return true;
				}

				// add ev
				if (p.ev)
				{
					*p.s++ = ',';
					p.max--;
					if (p.max <= 0) return true;

					l = B::EventNotifications().getDb(p.s, p.max, sid);
					p.s += l;
					p.max -= l;
					if (p.max <= 0)	return true;
				}

				return true;	// true indicates that characteristic was found
			}
		};
#if 0
		// Hap::Characteristic::Array
		template<
			int PropertyCount,			// number of optional properties
			FormatId F,				// format of the Value property
			int Size = 64			// max size of the array (64 - default size for strings)
		>
		class Array : public Base<PropertyCount + 1>
		{
		public:
			using T = Property::Array<KeyId::value, F, Size>;	// type of Value property
			using V = typename T::T;							// C type associated with T (array base type)
		protected:
			T _value;
		public:
			Array(Hap::Property::Type::T type, Hap::Property::InstanceId::T iid, Property::Permissions::T perms)
				: Base<PropertyCount + 1>(type, iid, perms, F)
			{
				AddProperty(&_value);
			}

			// get/set the value
			const V* Value() const { return _value.get(); }
			void Value(const V* v, uint16_t length) { _value.set(v, length); }
			V Value(int i) const { return _value.get(i); }
			void Value(int i, V v) { _value.set(i, v); }
		};
#endif
	}

	// Hap::Service
	template<int CharCount>	// max number of characteristics
	class Service : public Obj
	{
	private:
		// internal properties, slot [4] is for optional Linked property
		ObjArrayStatic<5> _prop;						

		Property::Type _type;
		Property::InstanceId _iid;
		Property::PrimaryService _primary;
		Property::HiddenService _hidden;

		ObjArrayStatic<CharCount> _char;	// characteristics

	protected:
		void AddLinked(Property::Obj& linked) { _prop.set(&linked, 4); }

		void AddCharacteristic(Obj* ch, int i) { _char.set(ch, i); }
		Obj* GetCharacteristic(int i) { return _char.get(i); }

	public:
		Service(Property::Type::T type) 
		:	_type(type)
		{
			_prop.set(&_type, 0);
			_prop.set(&_iid, 1);
			_prop.set(&_primary, 2);
			_prop.set(&_hidden, 3);
		}

		// access to properties
		void primary(Property::PrimaryService::T v) { _primary.set(v); }
		void hidden(Property::HiddenService::T v) { _hidden.set(v); }

		// access to characteristics
		template<typename Char> Char* GetCharacteristic()
		{
			return static_cast<Char*>(_char.GetObj([](Obj* obj) -> bool {
				return obj->isType(Char::Type);
			}));
		}

		// Obj virtual overrides
		virtual void Open(sid_t sid) override
		{
			for (int i = 0; i < _char.size(); i++)
			{
				Obj* ch = _char.get(i);
				if (ch != nullptr)
					ch->Open(sid);
			}
		}

		virtual void Close(sid_t sid) override
		{
			for (int i = 0; i < _char.size(); i++)
			{
				Obj* ch = _char.get(i);
				if (ch != nullptr)
					ch->Close(sid);
			}
		}

		virtual iid_t getId() override
		{
			return _iid.get();
		}

		virtual iid_t setId(iid_t iid) override
		{ 
			_iid.set(iid++); 

			for (int i = 0; i < _char.size(); i++)
			{
				auto ch = GetCharacteristic(i);
				if (ch == nullptr)
					continue;

				iid = ch->setId(iid);
			}

			return iid;
		}

		virtual bool isType(const char* t) override
		{
			return strcmp(t, _type.get()) == 0;
		}

		virtual int getDb(char* str, int max, sid_t sid) override
		{
			char* s = str;
			int l;

			if (max <= 0) goto Ret;

			*s++ = '{';
			max--;
			if (max <= 0) goto Ret;

			l = _prop.getDb(s, max, sid);
			s += l;
			max -= l;
			if (max <= 0) goto Ret;

			*s++ = ',';
			max--;
			if (max <= 0) goto Ret;

			l = _char.getDb(s, max, sid, "characteristics");
			s += l;
			max -= l;
			if (max <= 0) goto Ret;

			*s++ = '}';
		Ret:
			return (int)(s - str);
		}

		virtual int getEvents(char* str, int max, sid_t sid, iid_t aid, iid_t iid) override
		{
			char* s = str;
			int l;

			if (max <= 0) goto Ret;

			l = _char.getEvents(s, max, sid, aid, iid);
			s += l;
			max -= l;
		Ret:
			return (int)(s - str);
		}

		virtual bool Write(wr_prm& p, sid_t sid) override
		{
			for (int i = 0; i < _char.size(); i++)
			{
				auto ch = GetCharacteristic(i);
				if (ch == nullptr)
					continue;

				if (ch->getId() == p.iid)
				{
					return ch->Write(p, sid);
				}
			}

			return false;
		}

		virtual bool Read(rd_prm& p, sid_t sid) override
		{
			for (int i = 0; i < _char.size(); i++)
			{
				auto ch = GetCharacteristic(i);
				if (ch == nullptr)
					continue;

				if (ch->getId() == p.iid)
				{
					return ch->Read(p, sid);
				}
			}

			return false;
		}

	};

	// Hap::Accesory
	template<int ServiceCount>	// max number of services
	class Accessory : public Obj
	{
	private:
		// internal properties
		ObjArrayStatic<1> _prop;
		Property::aid _aid;

		// and array of services
		ObjArrayStatic<ServiceCount> _serv;	

	protected:
		void AddService(Obj* serv) { _serv.set(serv); }
		Obj* GetService(int i) { return _serv.get(i); }

	public:
		Accessory() 
		{
			_prop.set(&_aid,0);
		}

		// Init accessory:
		//	- set aid
		//	- set service/characteristic iids for all services/characteristics
		//	- return next iid 
		iid_t setId(iid_t aid, iid_t iid = 1)
		{
			_aid.set(aid);

			for (int i = 0; i < _serv.size(); i++)
			{
				auto serv = GetService(i);
				if (serv == nullptr)
					continue;

				iid = serv->setId(iid);
			}

			return iid;
		}

		// Obj virtual overrides
		virtual iid_t getId() override
		{
			return _aid.get();
		}

		virtual void Open(sid_t sid) override
		{
			for (int i = 0; i < _serv.size(); i++)
			{
				Obj* serv = _serv.get(i);
				if (serv != nullptr)
					serv->Open(sid);
			}
		}
		
		virtual void Close(sid_t sid) override
		{
			for (int i = 0; i < _serv.size(); i++)
			{
				Obj* serv = _serv.get(i);
				if (serv != nullptr)
					serv->Close(sid);
			}
		}

		virtual int getDb(char* str, int max, sid_t sid) override
		{
			char* s = str;
			int l;

			if (max <= 0) goto Ret;

			*s++ = '{';
			max--;
			if (max <= 0) goto Ret;

			l = _prop.getDb(s, max, sid);
			s += l;
			max -= l;
			if (max <= 0) goto Ret;

			*s++ = ',';
			max--;
			if (max <= 0) goto Ret;

			l = _serv.getDb(s, max, sid, "services");
			s += l;
			max -= l;
			if (max <= 0) goto Ret;

			*s++ = '}';
		Ret:
			return (uint32_t)(s - str);
		}

		virtual int getEvents(char* str, int max, sid_t sid, iid_t aid, iid_t iid) override
		{
			char* s = str;
			int l;

			if (max <= 0) goto Ret;

			l = _serv.getEvents(s, max, sid, aid, iid);
			s += l;
			max -= l;
		Ret:
			return (uint32_t)(s - str);
		}

		virtual bool Write(wr_prm& p, sid_t sid) override
		{
			if (p.aid != _aid.get())
			{
				p.status = Hap::Status::ResourceNotExist;
				return false;
			}

			// pass Write to each Service until	it returns true
			for (int i = 0; i < _serv.size(); i++)
			{
				auto serv = GetService(i);
				if (serv == nullptr)
					continue;

				if (serv->Write(p, sid))
					return true;
			}

			return false;
		}

		virtual bool Read(rd_prm& p, sid_t sid) override
		{ 
			if (p.aid != _aid.get())
			{
				p.status = Hap::Status::ResourceNotExist;
				return false;
			}

			// pass Write to each Service until	it returns true
			for (int i = 0; i < _serv.size(); i++)
			{
				auto serv = GetService(i);
				if (serv == nullptr)
					continue;

				if (serv->Read(p, sid))
					return true;
			}

			return false;
		};
	};

	// Hap::Db - top database object, not inherited from Obj
	//	- does not allocate storage for accessories, the storage must be passed into
	//		constructor; use DbStatic for statically allocate the accessory storage
	//	- all access to Db object must be externally serialized
	class Db
	{
	private:
		ObjArrayBase& _acc;		// array of accessories

	protected:
		void AddAcc(Obj* acc) {	_acc.set(acc); }
		Obj* GetAcc(iid_t id) { return _acc.GetObj(id); }

	public:
		Db(ObjArrayBase& acc)
			: _acc(acc)
		{}

		void Open(sid_t sid)
		{
			// propagate Open down to accessories
			for (int i = 0; i < _acc.size(); i++)
			{
				Obj* acc = _acc.get(i);
				if (acc != nullptr)
					acc->Open(sid);
			}
		}

		// Close
		//	returns true if opened session was closed
		void Close(sid_t sid)
		{
			// propagate Close down to accessories
			for (int i = 0; i < _acc.size(); i++)
			{
				Obj* acc = _acc.get(i);
				if (acc != nullptr)
					acc->Close(sid);
			}
		}

		// get JSON-formatted database
		//	returns num of charactes written to str (up to max)
		int getDb(sid_t sid, char* str, int max)
		{
			char* s = str;
			int l;

			if (max <= 0) goto Ret;

			*s++ = '{';
			max--;
			if (max <= 0) goto Ret;

			l = _acc.getDb(s, max, sid, "accessories");
			s += l;
			max -= l;

			*s++ = '}';
		Ret:
			return (int)(s - str);
		}

		// collect events
		//	returns HTTP status and JSON-formatted body for HTTP EVENT
		//	the rsp_size must be initially set to size of the rsp buffer;
		//	on return in contains size of the response object, if any 
		Http::Status getEvents(sid_t sid, char* rsp, int& rsp_size)
		{
			char* s = rsp;
			int l, max = rsp_size;

			rsp_size = 0;

			l = snprintf(s, max, "{\"characteristics\":[");
			s += l;
			max -= l;
			if (max <= 0)
				return Http::HTTP_500;	// Internal error

			l = _acc.getEvents(s, max, sid);
			if (l == 0)
			{
				rsp_size = 0;
				return Http::HTTP_200;
			}
			s += l;
			max -= l;
			if (max <= 0)
				return Http::HTTP_500;	// Internal error

			l = snprintf(s, max, "]}");
			s += l;
			max -= l;
			if (max <= 0)
				return Http::HTTP_500;	// Internal error

			rsp_size = (int)(s - rsp);

			return Http::HTTP_200;
		}

		// exec PUT/characteristics request
		//	accepts JSON-formatted message body of parsed HTTP request
		//	returns HTTP status and JSON-formatted body for HTTP response
		//	the rsp_size must be initially set to size of the rsp buffer;
		//	on return in contains size of the response object, if any 
		Http::Status Write(sid_t sid, const char* req, int req_length, char* rsp, int& rsp_size)
		{
			int l, max = rsp_size;
			Hap::Json::Parser<> wr;
			int rc = wr.parse(req, req_length);

			Log::Msg("parse = %d\n", rc);

			rsp_size = 0;

			// expect root object
			if (!rc || wr.tk(0)->type != Hap::Json::JSMN_OBJECT)
			{
				Log::Err("JSON parse error\n");
				return Http::HTTP_400;	// Bad request
			}

			wr.dump();

			// parse root object
			Hap::Json::member om[] =
			{
				{ "characteristics", Hap::Json::JSMN_ARRAY }
			};
			rc = wr.parse(0, om, sizeofarr(om));
			if (rc >= 0)
			{
				Log::Err("parameter '%s' is missing or invalid", om[rc].key);
				return Http::HTTP_400;
			}

			int cnt = wr.tk(om[0].i)->size;
			Log::Msg("Request contains %d characteristics\n", cnt);

			// prepare response
			char* s = rsp;
			bool comma = false;
			int errcnt = 0;

			l = snprintf(s, max, "{\"characteristics\":[");
			s += l;
			max -= l;
			if (max <= 0)
				return Http::HTTP_500;	// Internal error

			// parse and execute individual writes
			for (int i = 0; i < cnt; i++)
			{
				int c = wr.find(om[0].i, i);

				if (c < 0)
				{
					Log::Err("Characteristic %d not found\n", i);
					return Http::HTTP_400;
				}

				if (wr.tk(c)->type != Hap::Json::JSMN_OBJECT)
				{
					Log::Err("Characteristic %d: Object expected\n", i);
					return Http::HTTP_400;
				}

				// parse characteristic object
				Hap::Json::member om[] =
				{
					{ "aid", Hap::Json::JSMN_PRIMITIVE },
					{ "iid", Hap::Json::JSMN_PRIMITIVE },
					{ "value", Hap::Json::JSMN_ANY | Hap::Json::JSMN_UNDEFINED },
					{ "ev", Hap::Json::JSMN_PRIMITIVE | Hap::Json::JSMN_UNDEFINED },
					{ "authData", Hap::Json::JSMN_STRING | Hap::Json::JSMN_UNDEFINED },
					{ "remote", Hap::Json::JSMN_PRIMITIVE | Hap::Json::JSMN_UNDEFINED },
				};

				rc = wr.parse(c, om, sizeofarr(om));
				if (rc >= 0)
				{
					Log::Err("Characteristic %d: parameter '%s' is missing or invalid", i, om[rc].key);
					return Http::HTTP_400;
				}

				// fill write request parameters and status
				Obj::wr_prm p = { wr };

				// aid
				if (!wr.is_number<Hap::iid_t>(om[0].i, p.aid))
				{
					Log::Err("Characteristic %d: invalid aid\n", i);
					return Http::HTTP_400;
				}

				// iid
				if (!wr.is_number<Hap::iid_t>(om[1].i, p.iid))
				{
					Log::Err("Characteristic %d: invalid iid\n", i);
					return Http::HTTP_400;
				}

				// value
				if (om[2].i > 0)
				{
					p.val_present = true;
					p.val_ind = om[2].i;
				}

				// ev
				if (om[3].i > 0)
				{
					p.ev_present = wr.is_bool(om[3].i, p.ev_value);
				}

				// authData
				if (om[4].i > 0)
				{
					p.auth_present = true;
					p.auth_ind = om[4].i;
				}

				// remote
				if (om[5].i > 0)
				{
					p.remote_present = wr.is_bool(om[5].i, p.remote_value);
				}

				Log::Msg("Characteristic %d:  aid %u  iid %u\n", i, p.aid, p.iid);
				if (p.val_present)
					Log::Msg("      value: '%.*s'\n", wr.length(p.val_ind), wr.start(p.val_ind));
				if (p.ev_present)
					Log::Msg("         ev: %s\n", p.ev_value ? "true" : "false");
				if (p.auth_present)
					Log::Msg("   authData: '%.*s'\n", wr.length(p.auth_ind), wr.start(p.auth_ind));
				if (p.remote_present)
					Log::Msg("         ev: %s\n", p.remote_value ? "true" : "false");

				// find accessory by aid
				auto acc = GetAcc(p.aid);
				if (acc == nullptr)
				{
					p.status = Hap::Status::ResourceNotExist;
				}
				else
				{
					if (!acc->Write(p, sid))
						p.status = Hap::Status::ResourceNotExist;
				}

				if (p.status != Hap::Status::Success)
					errcnt++;

				if (comma)
				{
					*s++ = ',';
					max--;
					if (max <= 0)
						return Http::HTTP_500;	// Internal error
				}

				l = snprintf(s, max, "{\"aid\":%d,\"iid\":%d,\"status\":%s}",
					p.aid, p.iid, StatusStr(p.status));
				s += l;
				max -= l;
				if (max <= 0)
					return Http::HTTP_500;	// Internal error
				comma = true;

			}

			l = snprintf(s, max, "]}");
			s += l;
			max -= l;
			if (max <= 0)
				return Http::HTTP_500;	// Internal error

			if (errcnt == 0)
			{
				rsp_size = 0;
				return Http::HTTP_204;	// No content
			}

			rsp_size = (int)(s - rsp);

			if (cnt == errcnt)		// all writes completed with error
				return Http::HTTP_400;	// bad request

			return Http::HTTP_207;	// Multi-status
		}
		
		// exec GET/characteristics request
		//	accepts query string of parsed HTTP request (excluding '?' char)
		//	returns HTTP status and JSON-formatted body for HTTP response
		//	the rsp_size must be initially set to size of the rsp buffer;
		//	on return in contains size of the response object, if any 
		Http::Status Read(sid_t sid, const char* req, int req_length, char* rsp, int& rsp_size)
		{
			Obj::rd_prm p;
			const char* r = req;
			int l = req_length;
			int max = rsp_size;
			const char* id = nullptr;
			int id_length = 0;
			
			rsp_size = 0;

			while (l > 0)
			{
				if (strncmp(r, "id=", 3) == 0)
				{
					r += 3;
					l -= 3;
					if (l <= 0)
						return Http::HTTP_400;	// bad request

					id = r;
					bool loop = true;
					while (loop)
					{
						switch (*r)
						{
						case '0':
						case '1':
						case '2':
						case '3':
						case '4':
						case '5':
						case '6':
						case '7':
						case '8':
						case '9':
						case '.':
						case ',':
							r++;
							l--;
							id_length++;
							if (l <= 0)
								loop = false;	// end of URL
							break;

						case '&':
						case ';':
						case '#':
							loop = false;		// end of query parameter
							break;

						default:
							return Http::HTTP_400;	// unexpected character - bad request
						}
					}

				}
				else if (strncmp(r, "meta=", 5) == 0)
				{
					r += 5;
					l -= 5;
					if (l <= 0)
						return Http::HTTP_400;
					if (*r == '1')
						p.meta = true;
					else if (*r != '0')
						return Http::HTTP_400;
					r++;
					l--;
				}
				else if (strncmp(r, "perms=", 6) == 0)
				{
					r += 6;
					l -= 6;
					if (l <= 0)
						return Http::HTTP_400;
					if (*r == '1')
						p.perms = true;
					else if (*r != '0')
						return Http::HTTP_400;
					r++;
					l--;
				}
				else if (strncmp(r, "type=", 5) == 0)
				{
					r += 5;
					l -= 5;
					if (l <= 0)
						return Http::HTTP_400;
					if (*r == '1')
						p.type = true;
					else if (*r != '0')
						return Http::HTTP_400;
					r++;
					l--;
				}
				else if (strncmp(r, "ev=", 3) == 0)
				{
					r += 3;
					l -= 3;
					if (l <= 0)
						return Http::HTTP_400;
					if (*r == '1')
						p.ev = true;
					else if (*r != '0')
						return Http::HTTP_400;
					r++;
					l--;
				}
				else
					return Http::HTTP_400;

				if (l > 0 && *r != '&' && *r != ';' && *r != '#')
					return Http::HTTP_400;

				r++;
				l--;
			}

			Log::Msg("Read: '%.*s' meta %d  perms %d  type %d  ev %d\n", id_length, id, p.meta, p.perms, p.type, p.ev);

			if (id_length == 0)
				return Http::HTTP_400;	// id must present

			// prepare response
			char* s = rsp;
			int acccnt = 0;
			int errcnt = 0;

			l = snprintf(s, max, "{\"characteristics\":[");
			s += l;
			max -= l;
			if (max <= 0) return Http::HTTP_500;


			// parse id list and call read on each characteristic
			bool read_aid = true;
			while (id_length > 0)
			{
				if (read_aid)
				{
					// read aid
					if (*id >= '0' && *id <= '9')
					{
						p.aid = p.aid * 10 + (*id++ - '0');
						id_length--;
						continue;
					}
					else if (*id++ != '.')
						return Http::HTTP_400;

					id_length--;
					read_aid = false;
				}
				else
				{
					// read iid
					if (*id >= '0' && *id <= '9')
					{
						p.iid = p.iid * 10 + (*id++ - '0');
						id_length--;
						if (id_length > 0)
							continue;
					}
					else if (id_length > 0 && *id++ != ',')
						return Http::HTTP_400;

					id_length--;
					read_aid = true;

					Log::Msg("Read: aid %d iid %d\n", p.aid, p.iid);

					if (acccnt > 0)
					{
						*s++ = ',';
						max--;
						if (max <= 0) return Http::HTTP_500;
					}

					l = snprintf(s, max, "{\"aid\":%d,\"iid\":%d", p.aid, p.iid);
					s += l;
					max -= l;
					if (max <= 0) return Http::HTTP_500;

					p.s = s;
					p.max = max;

					// find accessory by aid
					auto acc = GetAcc(p.aid);
					if (acc == nullptr)
					{
						p.status = Hap::Status::ResourceNotExist;
					}
					else
					{
						if (!acc->Read(p, sid))
							p.status = Hap::Status::ResourceNotExist;
					}

					s = p.s;
					max = p.max;
					if (max <= 0) return Http::HTTP_500;

					if (p.status != Hap::Status::Success)
					{
						errcnt++;

						*s++ = ',';
						max--;
						if (max <= 0) return Http::HTTP_500;

						l = snprintf(s, max, "\"status\":%s}", StatusStr(p.status));
						s += l;
						max -= l;
						if (max <= 0) return Http::HTTP_500;
					}
					else
					{
						*s++ = '}';
						max--;
						if (max <= 0) return Http::HTTP_500;
					}

					acccnt++;

					p.aid = 0; 
					p.iid = 0;
				}
			}

			l = snprintf(s, max, "]}");
			s += l;
			max -= l;
			if (max <= 0)
				return Http::HTTP_500;	// Internal error

			rsp_size = (int)(s - rsp);

			if (errcnt == 0)
				return Http::HTTP_200;	// OK

			if (acccnt == errcnt)	// all reads completed with error
				return Http::HTTP_400;	// bad request

			return Http::HTTP_207;	// Multi-status
		}
	};

	template<int AccCount>		// max number of Accessories
	class DbStatic : public Db
	{
	private:
		ObjArrayStatic<AccCount> _acc;
	public:
		DbStatic() 
			: Db(_acc) 
		{}
	};
}

#endif
