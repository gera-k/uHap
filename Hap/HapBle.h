/*
	Copyright(c) 2020 Gera Kazakov
	SPDX-License-Identifier: Apache-2.0
*/

#ifndef _HAP_BLE_H_
#define _HAP_BLE_H_

#include "Platform.h"

#include "Hap.h"
#include "HapTlv.h"
#include "HapBlePdu.h"
#include "HapBleSession.h"

namespace Hap::Ble
{
    //  Max length of Data characteristic.
    //  When PDU exeeds this size it gets fragmented
    static constexpr uint16_t MaxDataLength = BLE_MTU;

    const char* TypeToStr(TlvType t);

    // Additional parameter data types.
    //  When no data format conversion (from internal HapDb representaion to
    //  BLE Parameter type) is required, the Tlv just points to data in the Db.
    //  Otherwise data buffer is internal and conversion occurs on
    //  construction or assign
    
    // 01 Value - var length data
    struct TlvValue : public Tlv
    {
        TlvValue(const Buf& val)
        : Tlv(TlvType::Value, val)
        {}
    };

    // 04 CharType - 128-bit UUID
    struct TlvCharType : public Tlv
    {
        TlvCharType(uint8_t* uuid)
        : Tlv(TlvType::CharType, Buf(uuid, 128/8))
        {}
    };

    // 06 SvcType - UUID
    struct TlvSvcType : public Tlv
    {
        TlvSvcType(uint8_t* uuid)
        : Tlv(TlvType::SvcType, Buf(uuid, 128/8))
        {}
    };

    // 07 SvcIid - IID
    //  BLE IID is 16 bit while internal IID may be wider
    struct TlvSvcIid : public Tlv
    {
        uint16_t iid;

        TlvSvcIid(Hap::iid_t iid_)
        : Tlv(TlvType::SvcIid, Buf(&iid, sizeof(iid))),
            iid(uint16_t(iid_))
        {}
    };

    // 0A CharProp
    //  BLE - 16 bit bitmask, currently matches the internal Perm
    //  declare locally anyway in case internal representation change
    struct TlvCharProp : public Tlv
    {
        uint16_t prop;

        TlvCharProp(Perm::T perm)
        : Tlv(TlvType::CharProp, Buf(&prop, sizeof(prop))),
            prop(uint16_t(perm))
        {}
    };

    // 0B Description
    struct TlvDescription : public Tlv
    {
        TlvDescription(const char* desc, uint8_t len)
        : Tlv(TlvType::Description, Buf((Buf::T*)desc, len))
        {}
    };

    // 0C Format -  BT Core Spec 5.2: Vol3-G 3.3.3.5
    //              HAP R2: 7.4.4.6.3
    //  Format      1 octet     Format of the value of this characteristic.
    //  Exponent    1 octet     Exponent field to determine how the value of this
    //                              characteristic is further formatted.
    //  Unit        2 octets    The unit of this characteristic as defined in [1]
    //  NameSpace   1 octet     The name space of the description as defined in [1]
    //  Description 2 octets    The description of this characteristic as defined in a
    //                              higher layer profile
    struct TlvFormat : public Tlv
    {
        struct
        {
            uint8_t format = 0;
            uint8_t exponent = 0;
            uint8_t unit[2] = {0, 0x27};
            uint8_t ns = 1;
            uint8_t description[2] = {0,0};
        } __packed desc;

        TlvFormat(Format f, Unit u)
        : Tlv(TlvType::Format, Buf(&desc, sizeof(desc)))
        {
            switch (f)
            {
		    case Format::Bool:     desc.format = 0x01; break;
		    case Format::Uint8:    desc.format = 0x04; break;
		    case Format::Uint16:   desc.format = 0x06; break;
		    case Format::Uint32:   desc.format = 0x08; break;
		    case Format::Uint64:   desc.format = 0x0A; break;
		    case Format::Int:      desc.format = 0x10; break;
		    case Format::Float:    desc.format = 0x14; break;
		    case Format::String:   desc.format = 0x19; break;
            case Format::Tlv:
		    case Format::Data:     desc.format = 0x1B; break;
            default: break;
            }

            switch (u)
            {
		    case Unit::Unitless:   desc.unit[0] = 0x00; break;
		    case Unit::Celsius:    desc.unit[0] = 0x2F; break;
		    case Unit::Arcdegrees: desc.unit[0] = 0x63; break;
		    case Unit::Percentage: desc.unit[0] = 0xAD; break;
		    case Unit::Lux:        desc.unit[0] = 0x31; break;
		    case Unit::Seconds:    desc.unit[0] = 0x03; break;
            default: break;
            }
        }
    };

    // 0D ValidRange - R2: 7.4.4.6.4
    struct TlvValidRange : public Tlv
    {
        uint8_t desc[sizeof(uint64_t) * 2];

        TlvValidRange(const Buf& min, const Buf& max)
        : Tlv(TlvType::ValidRange, Buf(&desc, sizeof(desc)))
        {
            uint8_t* p = desc;
            memcpy(p, min.p(), min.l());
            p += min.l();
            memcpy(p, max.p(), max.l());
            p += max.l();
            l(p - desc);
        }
    };

    // 0E Step - R2: 7.4.4.6.5
    struct TlvStep : public Tlv
    {
        uint8_t desc[sizeof(uint64_t)];
        
        TlvStep(const Buf& step)
        : Tlv(TlvType::Step, Buf(&desc, sizeof(desc)))
        {
            memcpy(desc, step.p(), step.l());
            l(step.l());
        }
    };

    // 0F SvcProp - R2: 7.4.4.4
    struct TlvSvcProp : public Tlv
    {
        TlvSvcProp(const Buf& prop)
        : Tlv(TlvType::SvcProp, prop)
        {}
    };
/*
    // 10 LinkedSvc - R2: 7.4.4.4.1
    //  Note: link array is initialized to all zero
    //      assign linked svc iids as nesessary by direct assignment to desc.link
    template<uint8_t N>
    struct TlvLinkedSvc : public Tlv
    {
        struct
        {
            uint16_t link[N] = {0};
        } __packed desc;

        TlvLinkedSvc()
        : Tlv(TlvType::LinkedSvc, Buf(&desc, sizeof(desc)))
        {
        }
    };

    // 11 ValidValues - R2: 7.4.5.3 (shall only be used for uint8_t characteristics)
    template<uint8_t... Val>
    struct TlvValidValues: public Tlv
    {
        struct
        {
            uint8_t val[sizeof...(Val)] = {Val...,};
        } __packed desc;

        TlvValidValues()
        : Tlv(TlvType::ValidValues, Buf(&desc, sizeof(desc)))
        {
        }
    };

    // 12 ValidValuesRange - R2: 7.4.5.4 (shall only be used for uint8_t characteristics)
    using Range = struct {uint8_t start; uint8_t end;};
    template<Range... Rng>
    struct TlvValidValuesRange: public Tlv
    {
        struct
        {
            Range rng[sizeof...(Rng)] = {Rng...,};
        } __packed desc;

        TlvValidValuesRange()
        : Tlv(TlvType::ValidValuesRange, Buf(&desc, sizeof(desc)))
        {
        }
    };
*/
    // BLE characteristic
    //  HAP BLE characteristic contains
    //  - Characteristic attribute (BT Core Spec 5.2: Vol3-G 3.3.1)
    //      0x2803  UUID for 'Characteristic'
    //      Properties (1 byte)  HAP: Read/Write/Indicate
    //      UUID (2 or 16 bytes)
    //  - Characteristic value (BT Core Spec 5.2: Vol3-G 3.3.2)
    //      UUID  characteristic UUID
    //      Value
    //  - CCCD (Client Characteristic Configuration, BT Core Spec 5.2: Vol3-G 3.3.3.3)
    //          optional - only when HAP characteristic supports events
    //      0x2902  UUID for 'Client Characteristic Configuration'
    //       Config (1 byte)  Characteristic Configuration  HAP: Indication
    //  - Charactristic Instance ID (HAP R2: 7.4.4.5.2)
    //      UUID - CharIidUuid
    //      iid   2 bytes
    namespace Characteristic
    {
        extern Uuid uuidIid;

        enum Prop : uint8_t
        {
            PropBroadcast = 0x01,
            PropRead = 0x02,
            PropWriteWithoutResponse = 0x04,
            PropWrite = 0x08,
            PropNotify = 0x10,
            PropIndicate = 0x20,
            PropAuthenticatedSignedWrites = 0x40,
            PropExtendedProperties = 0x80
        };

        enum Config : uint8_t
        {
            Notification = 0x01,
            Indication = 0x02
        };
    }

    // BLE Service
    //  - Service attribure
    //      0x2800 – UUID for 'Primary Service' OR 0x2801 for 'Secondary Service'
    //      16-bit Bluetooth UUID or 128-bit UUID for Service
    //  - Service Instance ID characteristic (R2: 7.4.4.3)
    //      - service attribute
    //          0x2803  UUID for 'Characteristic'
    //          Properties (1 byte)  Read
    //          UUID  SvcIidUuid
    //      - value
    //          UUID  SvcIidUuid
    //          Value 2 byte iid
    //      - must not have its own char instance ID
    //  - Service Signature characteristic (optional) (R2: 7.4.4.4)
    //      - characteristic attribute
    //          0x2803  UUID for 'Characteristic'
    //          Properties (1 byte)  Read
    //          UUID  SvcSignUuid
    //      - value
    //          UUID  SvcIidUuid
    //          Value  ?
    //      - characteristic Instance ID
    //          UUID - CharIidUuid
    //          iid   2 bytes
    namespace Service
    {
        extern Uuid uuidIid;
        extern Uuid uuidSign;

        struct CharSign : public Char<Format::Tlv>
        {
            void init(Hap::Service* svc, iid_t& iid_);
        };
    };

    // BLE Advertisement data
    namespace Adv
    {
        template<uint8_t T, uint8_t L>
        struct Item
        {
            uint8_t length = L + 1;
            uint8_t type = T;
            uint8_t data[L];
        } __packed;

        // Regular format  (R2: 7.4.2.1)
        struct Regular
        {
            static constexpr int Count = 3;

            Item<0x01, 1> Flags;       // Flags - 1 byte, AD type 0x01
            Item<0xFF, 21> Manuf;      // manuf data - 21 bytes, AD type 0xFF)
            Item<0x08, 3> Name;        // shortened name (AD type 0x08)

            void init();
        } __packed;

        // Encrypted Notification format (R2: 7.4.2.2)
        struct Notif
        {
            static constexpr int Count = 2;

            Item<0x01, 1> Flags;       // Flags - 1 byte, AD type 0x01
            Item<0xFF, 26> Manuf;      // manuf data - 26 bytes, AD type 0xFF)

            void init(Hap::Characteristic* ch);
        } __packed;

        static constexpr uint8_t BEKLen = 32;
        static constexpr uint8_t AAILen = 6;

        extern Mem<BEKLen> BEK;         // Broadcast Encryption Key
        extern Mem<AAILen> AAI;         // Accessory Advertising Identifier

        void GenBEK(Hap::Session* sess);    // Generate Broadcast Encryption Key
        void SetAAI(const Buf& aai);        // Set Accessory Advertising Identifier
    }

    // BLE Protocol Info service
    namespace ProtoInfo
    {
        void init(iid_t& iid);
        Hap::Service* Svc();
    }

    // BLE Pairing service
    namespace Pairing
    {   
        extern const Uuid uuidVerify;

        void init(iid_t& iid);
        Hap::Service* Svc();
    }

    // Platform-callalble property read/write methods
    
    // read property
    ssize_t Read(
        Session* sess,
        Hap::Property* prop,
        void *buf,              //  buf    Buffer to place the read result in
        u16_t len,              //  len    Length of data to read
        u16_t offset            //  offset Offset to start reading from
    );

    // write property
    ssize_t Write(
        Session* sess,
        Hap::Property* prop,
        const void *buf,        //  buf    Buffer with the data to write
        u16_t len,              //  len    Number of bytes in the buffer
        u16_t offset            //  offset Offset to start writing from
    );

}

#endif /*_HAP_BLE_H_*/
