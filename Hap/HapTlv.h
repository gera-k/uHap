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

#ifndef _HAP_TLV_H_
#define _HAP_TLV_H_

// TLV parser/creator

namespace Hap
{
	namespace Tlv
	{
		enum class Method : uint8_t
		{
			PairSetupNonMfi = 0,
			PairSetup = 1,
			PairVerivy = 2,
			AddPairing = 3,
			RemovePairing = 4,
			ListPairing = 5,
		};

		enum class State : uint8_t
		{
			Unknown = 0,
			M1 = 1,
			M2 = 2,
			M3 = 3,
			M4 = 4,
			M5 = 5,
			M6 = 6,
		};

		enum class Error : uint8_t
		{
			Unknown = 0x01,
			Authentication = 0x02,
			Backoff = 0x03,
			MaxPeers = 0x04,
			MaxTries = 0x05,
			Unavailable = 0x06,
			Busy = 0x07,
		};

		enum class Type : uint8_t
		{							// Format
			Method = 0x00,			//	integer
			Identifier = 0x01,		//	UTF-8
			Salt = 0x02,			//	bytes
			PublicKey = 0x03,		//	bytes
			Proof = 0x04,			//	bytes
			EncryptedData = 0x05,	//	bytes
			State = 0x06,			//	integer
			Error = 0x07,			//	integer
			RetryDelay = 0x08,		//	integer
			Certificate = 0x09,		//	bytes
			Signature = 0x0A,		//	bytes
			Permissions = 0x0B,		//	integer
			FragmentData = 0x0C,	//	bytes
			Fragmentlast = 0x0D,	//	bytes
			Separator = 0xFF,		//	null
			Invalid = 0xFE
		};

		using Item = Buf<const uint8_t>;

		template<int MaxTlv>	// Max number of TLVs expected, parsing stops when MaxTlv is reached
		class Parse
		{
		private:
			const uint8_t* _buf;	// data buffer containing TLVs
			uint32_t _len;			// length of the buffer
			uint32_t _off[MaxTlv];	// offsets
			uint8_t _cnt;			// number of TLVs found

		public:
			Parse() {}

			Parse(const uint8_t* buf, uint16_t len)
			{
				parse(buf, len);
			}

			// parse passed in buffer
			uint8_t parse(const uint8_t* buf, uint32_t len)
			{
				_buf = (const uint8_t*)buf;
				_len = len;
					
				const uint8_t* b = _buf;
				uint32_t l = _len;
				uint8_t i;

				_cnt = 0;
				for (i = 0; i < sizeofarr(_off); i++)
				{
					if (l < 2)
						break;

					_off[_cnt++] = (uint32_t)(b - _buf);

					Type t = Type(b[0]);
					uint32_t s = b[1];

					Log::Msg("Tlv: type %d  length %d\n", int(t), s);

					s += 2;
					if (s > l)	// this Tlv spans beyond the buffer, limit its size
						s = l;
					b += s;
					l -= s;
				}

				return _cnt;
			}

			// return number of items found in this TLV
			uint8_t count()
			{
				return _cnt;
			}

			// return type of item i
			Type type(uint8_t i)
			{
				if (i >= _cnt)
					return Type::Invalid;

				return Type(_buf[_off[i]]);
			}

			// return length of item i
			uint8_t length(uint8_t i)
			{
				if (i >= _cnt)
					return 0;

				return _buf[_off[i] + 1];
			}

			// return pointer to value of item i
			const uint8_t* value(uint8_t i)
			{
				if (i >= _cnt)
					return 0;

				return _buf + _off[i] + 2;
			}

			// get item 
			bool get(Type t, Item& item)
			{
				for (uint8_t i = 0; i < _cnt; i++)
				{
					if (type(i) == t)
					{
						item.p() = value(i);
						item.l() = length(i);
						return true;
					}
				}
				return false;
			}

			// extract low-endian integer from item i
			int getInt(uint8_t i)
			{
				int r = 0;
				uint8_t l = length(i);
				const uint8_t* v = value(i);

				for (int i = 0; i < l; i++)
					r |= (*v++) << (8 * i);

				return r;
			}

			// extract int/enum value from item with type t
			template<typename T>
			bool get(Type t, T& v)
			{
				for (uint8_t i = 0; i < _cnt; i++)
				{
					if (type(i) == t)
					{
						v = T(getInt(i));
						return true;
					}
				}
				return false;
			}

			// extract data bytes from single TLV
			//	the data buffer must be at least length(i) bytes long
			void getBytes(uint8_t i, uint8_t* data)
			{
				memcpy(data, value(i), length(i));
			}

			// extract multi-TLV data starting from TLV i
			//	size contains buffer size
			//	on return contains length of data copied to the buffer
			//	func returns false if data does not fit into the buffer
			bool getData(uint8_t i, uint8_t* data, uint16_t& size)
			{
				uint8_t* d = data;
				uint16_t max = size;
				Type t = type(i);

				while (true)
				{
					if (type(i) != t)	// different type or no more TLVs
					{
						size = (uint16_t)(d - data);
						return true;
					}

					if (length(i) > max)
						return false;

					getBytes(i, d);

					d += length(i);
					max -= length(i);

					i++;
				}
			}

			bool get(Type t, uint8_t* data, uint16_t& size)
			{
				for (uint8_t i = 0; i < _cnt; i++)
				{
					if (type(i) == t)
					{
						return getData(i, data, size);
					}
				}
				return false;
			}

		};

		class Create
		{
		private:
			uint8_t* _buf;		// data buffer for TLV
			uint16_t _size;		// size of the buffer
			uint16_t _len;		// length of valid TLV

		public:
			void create(uint8_t* buf, uint16_t size)
			{
				_buf = buf; 
				_size = size;
				_len = 0;
			}

			uint16_t length()
			{
				return _len;
			}

			// add zero-size item
			bool add(Type t)
			{
				// ensure space for at least 2 bytes
				if (_size - _len < 2)
					return false;

				uint8_t* b = _buf + _len;	// tlv start
				*b++ = uint8_t(t);			// store type
				*b++ = 0;					// store zero length

				_len = (uint32_t)(b - _buf);
				return true;
			}

			// add Integer, use as many bytes as necessary
			bool addInt(Type t, int v)
			{
				// ensure space for at least 3 bytes
				if (_size - _len < 3)
					return false;

				uint8_t* b = _buf + _len;	// tlv start
				*b++ = uint8_t(t);			// store type
				uint8_t* l = b++;			// save length byte pointer

				*l = 0;						// zero length
				do
				{
					*b++ = v & 0xFF;		// store next byte of integer (starting from lsb)
					(*l)++;					// increment length
					
					if (b == _buf + _size)
						return false;		// doesn't fit

					v >>= 8;				// move to next byte

				} while (v != 0);

				_len = (uint32_t)(b - _buf);
				return true;
			}

			template<typename T>
			bool add(Type t, T v)
			{
				return addInt(t, int(v));
			}

			// Add bytes, up to 255
			bool addBytes(Type t, const uint8_t* d, uint8_t l)
			{
				// ensure there is space
				if (_size - _len < (uint16_t)l + 2)
					return false;

				uint8_t* b = _buf + _len;	// tlv start
				*b++ = uint8_t(t);			// store type
				*b++ = l;					// store length
				memcpy(b, d, l);			// store data
				b += l;

				_len = (uint32_t)(b - _buf);
				return true;
			}

			// Add data, span multiple items as necessary
			bool add(Type t, const uint8_t* data, uint16_t len)
			{
				const uint8_t* d = data;

				while (len > 0)
				{
					uint8_t l = len > 255 ? 255 : len;

					if (!addBytes(t, d, l))
						return false;

					d += l;
					len -= l;
				}

				return true;
			}
		};
	}
}

#endif