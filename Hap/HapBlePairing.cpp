#define HAP_LOG_REGISTER hapBlePairing
#include "Platform.h"

#include "HapBle.h"
#include "HapPairing.h"

namespace Hap::Ble
{
    // Hap DB for BLE pairing service 
    namespace Pairing
    {

        // BLE Pairing service UUIDs (R2: 5.13)
        const Uuid uuidService(0x00000055, 0x0000, 0x1000, 0x8000, 0x0026BB765291);
        const Uuid uuidSetup(0x0000004C, 0x0000, 0x1000, 0x8000, 0x0026BB765291);
        const Uuid uuidVerify(0x0000004E, 0x0000, 0x1000, 0x8000, 0x0026BB765291);
        const Uuid uuidFeatures(0x0000004F, 0x0000, 0x1000, 0x8000, 0x0026BB765291);
        const Uuid uuidPairings(0x00000050, 0x0000, 0x1000, 0x8000, 0x0026BB765291);

        struct CharTlv : public Char<Format::Tlv>
        {
            Hap::Pairing::Path path;

            virtual Hap::Status Write(Hap::Operation* op, Buf& req, Buf& rsp) override
            {
                //LOG_HEXDUMP_INF(req.p(), req.l(), "Pairing Write");
                HAP_LOG_DBG("Pairing Write: path %d req %p %d", (uint8_t)path, req.p(), req.l());

                return Hap::Pairing::dispatch(op, path, req, rsp);
            }
        };
     
        struct Service: public Hap::Service
        {
            CharTlv setup;
            CharTlv verify;
            Char<Format::Uint8> features;
            CharTlv pairings;

            void init(Hap::iid_t& iid)
            {
                HAP_LOG_DBG("Hap::Ble::Pairing::Service::init");

                Hap::Service::init(iid, "Pairing", uuidService);

                setup.init(iid, "Setup", uuidSetup, Perm::rd | Perm::wr | Perm::nt);
                setup.path = Hap::Pairing::Path::Setup;
                add(&setup);

                verify.init(iid, "Verify", uuidVerify, Perm::rd | Perm::wr);
                verify.path = Hap::Pairing::Path::Verify;
                add(&verify);

                features.init(iid, "Features", uuidFeatures, Perm::rd);
                features.value = 0;
                add(&features);

                pairings.init(iid, "Pairings", uuidPairings, Perm::pr | Perm::pw);
                pairings.path = Hap::Pairing::Path::Pairings;
                add(&pairings);
            }
        }; 
        
        static Service pairing;

        void init(iid_t& iid)
        {
            Hap::Pairing::init();
            pairing.init(iid);
        }

        Hap::Service* Svc()
        {
            return &pairing;
        }
    }
}
