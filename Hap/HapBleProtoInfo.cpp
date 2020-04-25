#define HAP_LOG_REGISTER hapBleProtoInfo
#include "Platform.h"

#include "HapBle.h"
#include "HapPairing.h"

namespace Hap::Ble
{
    // Hap DB for BLE Protocol Information service (R2: 7.4.3, 8.17)
    namespace ProtoInfo
    {
        static const Uuid uuidService(0x000000A2, 0x0000, 0x1000, 0x8000, 0x0026BB765291);
        static const Uuid uuidVersion(0x00000037, 0x0000, 0x1000, 0x8000, 0x0026BB765291);

        const char version[] = "2.2.0";
        struct CharVersion : public Char<Format::String>
        {
            void init(Hap::Service*svc, iid_t& iid)
            {
                Char::init(iid, "Version", uuidVersion, Perm::rd);
                value = Buf{version, 5};
                svc->add(this);
            }
        };
        
        struct Service: public Hap::Service
        {
            CharVersion version;
            Hap::Ble::Service::CharSign sign;

            void init(Hap::iid_t& iid)
            {
                HAP_LOG_DBG("Hap::Ble::ProtoInfo::Service::init");

                Hap::Service::init(iid, "ProtoInfo", uuidService, Hap::SvcProp::Configurable);

                version.init(this, iid);
                sign.init(this, iid);
            }
        };

        static Service protoInfo;

        void init(iid_t& iid)
        {
            protoInfo.init(iid);
        }

        Hap::Service* Svc()
        {
            return &protoInfo;
        }
    }
}