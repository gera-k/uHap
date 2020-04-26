/*
	Copyright(c) 2020 Gera Kazakov
	SPDX-License-Identifier: Apache-2.0
*/

#ifndef _HAP_TLV_H_
#define _HAP_TLV_H_

#include "HapBuf.h"

// TLV parser/creator
namespace Hap
{
	namespace Pairing
	{
		// Pairing Tlv type     (R2: 5.15)
		enum class TlvType : uint8_t
		{
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
            SessionID = 0x0E,       //  bytes
			Flags = 0x13,			//  integer
			Separator = 0xFF,		//	null
//			Invalid = 0xFE
		};
	}

    namespace Ble
    {
        // BLE Additional Parameter Types (R2: 7.3.3.4)
        enum class TlvType : uint8_t
        {
            Value               = 0x01,
            Aad                 = 0x02,
            Origin              = 0x03,
            CharType            = 0x04,
            CharIid             = 0x05,
            SvcType             = 0x06,
            SvcIid              = 0x07,
            Ttl                 = 0x08,
            RetResp             = 0x09,
            CharProp            = 0x0A,
            Description         = 0x0B,
            Format              = 0x0C,
            ValidRange          = 0x0D,
            Step                = 0x0E,
            SvcProp             = 0x0F,
            LinkedSvc           = 0x10,
            ValidValues         = 0x11,
            ValidValuesRange    = 0x12
        };

        // BLE Protocol Configuration types
        enum class TlvProtoConfigReqType : uint8_t
        {
            BEK = 0x01,     // Generate Broadcast Encryption Key
            All = 0x02,     // Get All Params
            AAI = 0x03      // Set Accessory Advertising Identifier
        };
        enum class TlvProtoConfigRspType : uint8_t
        {
            SN = 0x01,      // Current State Number (uint16_t)
            CN = 0x02,      // Current Config Number (uint8_t)
            AAI = 0x03,     // Accessory Advertising Identifier
            BEK = 0x04      // Broadcast Encryption Key
        };

        // BLE Characteristic Configuration types
        enum class TlvCharConfigType : uint8_t
        {
            Prop = 0x01,    // Properties, uint16_t
            BcInt = 0x02    // Broadcast interval
        };
    }

    // Tlv - TLV8 with value in external memory
    //  the Tlv object contains type, length, 
    //  and pointer to external buffer with value
    class Tlv
    {
    private:
        // create empty invalid TLV
        Tlv()
        {}

    public:
        using T = Buf::T;     // type of TLV elements (type, length, data)
        using S = Buf::S;     // type of external buffer size

        static constexpr T Invalid = 0xFE;   // FF is taken for pairing separator TLV
        
        // create typed TLV, attach to buffer containig value
        Tlv(T type, const Buf& buf)
        : _t(type), _l(buf.l()), _b(buf)
        {}

        Tlv(Hap::Pairing::TlvType type, const Buf& buf)
        : _t((T)type), _l(buf.l()), _b(buf)
        {}

        Tlv(Hap::Ble::TlvType type, const Buf& buf)
        : _t((T)type), _l(buf.l()), _b(buf)
        {}

        // create TLV from data in buffer containig full TLV - prefix plus value
        Tlv(const Buf& buf)
        : _t(Invalid), _l(0) 
        {
            if (buf.l() < 2)
                return;
            
            _t = *buf.p(0);     // extract TLV type
            _l = *buf.p(1);     // extracl TLV value length
            _b = Buf(buf.p(2), buf.l() - 2, _l);    // point to TLV value
        }

        Tlv(T* b, S l)
        : Tlv(Buf(b, l, l))
        {}

        // true if TLV is valid
        bool valid()
        {
            return _t != Invalid;
        }

        // get/set TLV type field
        T t() const
        {
            return _t;
        }
        void t(T t)
        {
            _t = t;
        }

        // get/set TLV length field
        T l() const
        {
            return _l;
        }
        void l(T l)
        {
            _l = l;
        }

        // get pointer to TLV value field
        const T* p() const
        {
            return _b.p();
        }
        T* p()
        {
            return _b.p();
        }
        const Buf v()
        {
            return _b;
        }

        uint16_t s() const // total size of the TLV including type/length fields
        {
            return _l + 2;
        }

        // parse buffer, count number of TLVs in it
        static uint8_t Parse(Buf& buf);

        // find and return TLV of requested type
        static Tlv Get(Buf& buf, uint8_t type);

        // multi-Tlv processing (R2: 14.1)
        //  - each TLV has the same type
        //  - each TLV is 255 bytes long except the last one
        //  - TLVs are sequential

        // get multi-TLV data and copy into dst_buf
        static S Get(Buf& buf, uint8_t type, T* dst_buf, S dst_size);

        // append src_buf splitting as multi-TLV sequence if necessary
        static S Append(Buf& buf, uint8_t type, const T* src_buf, S src_size);

    private:
        T _t = Invalid;     // data type, first element of the TLV
        T _l = 0;           // data length, second element of the TLV
        Buf _b;             // buffer containing TLV value
    };
}

#endif /*_HAP_TLV_H_*/