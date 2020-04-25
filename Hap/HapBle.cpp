#define HAP_LOG_REGISTER hapBle
#include "Platform.h"

#include "HapBle.h"

namespace Hap::Ble
{
    const char* TypeToStr(TlvType t)
    {
        #define CASE_TYPE_STR(n) case TlvType::n: return #n
        switch (t)
        {
        CASE_TYPE_STR(Value);
        CASE_TYPE_STR(Aad);
        CASE_TYPE_STR(Origin);
        CASE_TYPE_STR(CharType);
        CASE_TYPE_STR(CharIid);
        CASE_TYPE_STR(SvcType);
        CASE_TYPE_STR(SvcIid);
        CASE_TYPE_STR(Ttl);
        CASE_TYPE_STR(RetResp);
        CASE_TYPE_STR(CharProp);
        CASE_TYPE_STR(Description);
        CASE_TYPE_STR(Format);
        CASE_TYPE_STR(ValidRange);
        CASE_TYPE_STR(Step);
        CASE_TYPE_STR(SvcProp);
        CASE_TYPE_STR(LinkedSvc);
        CASE_TYPE_STR(ValidValues);
        CASE_TYPE_STR(ValidValuesRange);
        }
        return "";
    }

    // generic GATT read
    ssize_t Read(Session* sess, Hap::Property* prop, 
        void *buf, u16_t len, u16_t offset)
    {
        Hap::Characteristic* ch = prop->charOwner;
        if (ch != nullptr && prop->type == Hap::Type::Value)
        {
            // read from Characteristic Value goes to Session/Procedure
            //  (Service properties do not belong to a Characteristic
            //  so they are read directly from property buffer)
            return sess->Read(prop, buf, len, offset);
        }

        // read data directly from property buffer
        //  this includes:
        //  - service IID - always readable (R2: 7.4.4.3)
        //  - characteristic IID - always readable (R2: 7.4.4.5.2)

        auto l = prop->value.l();

        LOG_DBG("Rd %s.%s  ln %d:  len %d  off %d",
            ch ? ch->name : "?", TypeToStr(prop->type), l, len, offset);

        if (offset >= l)
            return 0;

        if (offset + len > l)
            len = l - offset;

        auto p = prop->value.p(offset);
        memcpy(buf, p, len);

        return len;
    }

    // generic GATT write
    ssize_t Write(Session* sess, Hap::Property* prop,
        const void *buf, u16_t len, u16_t offset)
    {
        Hap::Characteristic* ch = prop->charOwner;
        if (ch != nullptr && prop->type == Hap::Type::Value)
        {
            // write to Characteristic Value goes to Session/Procedure
            return sess->Write(prop, buf, len, offset);
        }

        // write data to property other than value
        // this should never happen so emit ERR when it does

        auto s = prop->value.s();
        HAP_LOG_ERR("Wr %s.%s  sz %d:  len %d  off %d",
            ch ? ch->name : "?", TypeToStr(prop->type), s, len, offset);

        len = BleErrorAuthentication;

        return len;
    }

    namespace Characteristic
    {
        // UUIDs
        Uuid uuidIid(0xDC46F0FE, 0x81D2, 0x4616, 0xB5D9, 0x6ABDD796939A);
    }

    namespace Service
    {
        Uuid uuidIid(0xE604E95D, 0xA759, 0x4817, 0x87D3, 0xAA005083A0D1);
        Uuid uuidSign(0x000000A5, 0x0000, 0x1000, 0x8000, 0x0026BB765291);

        void CharSign::init(Hap::Service* svc, iid_t& iid)
        {
            Char::init(iid, "SvcSign", uuidSign, Perm::rd | Perm::wr);
            svc->add(this);
        }
    }

    namespace Adv
    {
        Mem<BEKLen> BEK;         // Broadcast Encryption Key
        Mem<AAILen> AAI;         // Accessory Advertising Identifier

        void Regular::init()
        {
            Flags.data[0] = 0
                | BIT_(1)       // LE General Discoverable Mode
                | BIT_(2)       // BR/EDR not supported
                ;
            
            Manuf.data[ 0] = 0x4C;	                    // CoID - Company ID 0x004C in little endian
	        Manuf.data[ 1] = 0x00;
	        Manuf.data[ 2] = 0x06;	                    // Type
	        Manuf.data[ 3] = 0x31;	                    // Subtype/Length
	        Manuf.data[ 4] = Config::statusFlags;       // SF: 
	        Manuf.data[ 5] = Config::deviceId[0];       //48-bit Device ID
	        Manuf.data[ 6] = Config::deviceId[1];
	        Manuf.data[ 7] = Config::deviceId[2];
	        Manuf.data[ 8] = Config::deviceId[3];
	        Manuf.data[ 9] = Config::deviceId[4];
	        Manuf.data[10] = Config::deviceId[5];
	        Manuf.data[11] = Config::categoryId;	    // ACID - 16 bit Accessory Category Identifier
	        Manuf.data[12] = 0x00;
	        Manuf.data[13] = Config::globalStateNum & 0xFF; // GSN - 16 bit Global State Number
	        Manuf.data[14] = (Config::globalStateNum >> 8) & 0xFF;
	        Manuf.data[15] = Config::configNum & 0xFF;	// CN - 8 bit configuration number
	        Manuf.data[16] = 0x02;	                    // CV - Compatible verion
	        Manuf.data[17] = 0x00;	                    // SH - 4 byte setup hash
	        Manuf.data[18] = 0x00;
	        Manuf.data[19] = 0x00;
	        Manuf.data[20] = 0x00;

            Name.data[0] = Config::name[0];
            Name.data[1] = Config::name[1];
            Name.data[2] = Config::name[2];
        }

        void Notif::init(Hap::Characteristic* ch)
        {
            Flags.data[0] = 0
                | BIT_(1)       // LE General Discoverable Mode
                | BIT_(2)       // BR/EDR not supported
                ;
            
            Manuf.data[ 0] = 0x4C;	                    // CoID - Company ID 0x004C in little endian
	        Manuf.data[ 1] = 0x00;
	        Manuf.data[ 2] = 0x11;	                    // Type
	        Manuf.data[ 3] = 0x36;	                    // Subtype/Length
 	        Manuf.data[ 4] = *AAI.p(0);                 //48-bit AAI
	        Manuf.data[ 5] = *AAI.p(1);
	        Manuf.data[ 6] = *AAI.p(2);
	        Manuf.data[ 7] = *AAI.p(3);
	        Manuf.data[ 8] = *AAI.p(4);
	        Manuf.data[ 9] = *AAI.p(5);

            // encrypted data
            uint8_t data[12];
            memset(data, 0, sizeof(data));
            data[0] = Config::globalStateNum & 0xFF;    // GSN - 16 bit Global State Number
            data[1] = (Config::globalStateNum >> 8) & 0xFF;
            data[2] = ch->iid() & 0xFF;                 // characteristic instance ID
            data[3] = ch->iid() >> 8;
            // characteristic value, up to 8 bytes  // TODO: verify that char is <= 8 bytes, here or when enabling notifications
            memcpy(data+4, ch->ptrVal->value.p(), ch->ptrVal->value.l()); 
            //HAP_LOG_HEX(data, 12, "Notif data");

            // make 96-bit nonce from GSN
            uint8_t nonce[12];
            memset(nonce, 0, sizeof(nonce));
            memcpy(nonce + 4, &Config::globalStateNum, 2);
            //HAP_LOG_HEX(nonce, 12, "Nonce");

            uint8_t tag[16];

            Crypto::Aead aead;
            // encrypt into manuf data [10..21]
            aead.encrypt(
                Manuf.data + 10, tag,	// output data and tag positions
                BEK.p(),	            // encryption key
                nonce,
                data, 12,				// data to encrypt
                AAI.p(), AAI.l()
            );
            //HAP_LOG_HEX(tag, 16, "Tag");

            // copy 4 bytes of tag to manuf data [22..25]
            memcpy(Manuf.data + 22, tag, 4);

            //HAP_LOG_HEX(Manuf.data, sizeof(Manuf.data), "Manuf Data");
       }

        void GenBEK(Hap::Session* sess)      // Generate Broadcast Encryption Key
        {
            hkdf.calc(
                sess->ctlrPaired->ltpk, sess->ctlrPaired->LtpkLen,      // salt
                sess->curve.sharedSecret(), sess->curve.KEY_SIZE_BYTES, // input key
                (const uint8_t*)"Broadcast-Encryption-Key", sizeof("Broadcast-Encryption-Key") - 1,
                BEK.p(), BEKLen
            );

            HAP_LOG_HEX(BEK.p(), BEK.l(), "Broadcast Encryption Key");
        }

        void SetAAI(const Buf& aai)     // Set Accessory Advertising Identifier
        {
            AAI.zero();
            AAI.copy(aai);

            HAP_LOG_HEX(AAI.p(), AAI.l(), "Accessory Advertising Identifier");
        }
    }
}