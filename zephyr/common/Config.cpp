#include <logging/log.h>
LOG_MODULE_REGISTER(appConfig, LOG_LEVEL_DBG);

#include "App.h"
#include "Crypto/MD.h"
#include "Crypto/Srp.h"

// zephyr-specific implementation of Hap::Config

namespace Hap::Config
{
    char name[Hap::DefString];				// nm Accessory name - used as initial Bonjour name and as AIS name of aid=1
    char model[Hap::DefString];				// md Model name (Bonjour and AIS)
    char manufacturer[Hap::DefString];		// mf Manufacturer- used by AIS (Accessory Information Service)
    char serialNumber[Hap::DefString];		// sn Serial number in arbitrary format
    char firmwareRevision[Hap::DefString];	// fw Major[.Minor[.Revision]]
    char hardwareRevision[Hap::DefString];	// hw hardware revision string x[.y[.z]]
    char setupCode[Hap::DefString];			// sc setupCode code XXX-XX-XXX
    uint8_t setupVerifier[Srp::SRP_VERIFIER_BYTES];	// sv pre-generated setup verifier and salt
	uint8_t setupSalt[Srp::SRP_SALT_BYTES];	// ss		
    uint8_t deviceId[6];			        // id Device ID (new deviceId generated on each factory reset)
    uint8_t categoryId;				        // ci category identifier
    uint8_t statusFlags;				    // sf status flags
    uint32_t configNum;				        // cn Current configuration number, incremented on db change
	uint16_t globalStateNum;		        // gs Global State Number (GSN - R2: 7.4.1.8)
    uint16_t port;					        // pn TCP port of HAP service in net byte order
    
    Pairings pairings;				        // pX pairing record, where X=0..F
    Crypto::Ed25519 keys;		            // pk, sk long term keys (private/shared)

    static struct _conf
    {
        const char* key;
        int32_t len;        // negative - max size of var size string value
        void* val;
    } conf[] = 
    {
        { "nm", -Hap::DefString, name }, 
        { "md", -Hap::DefString, model },
        { "mf", -Hap::DefString, manufacturer },
        { "sn", -Hap::DefString, serialNumber },
        { "fw", -Hap::DefString, firmwareRevision },
        { "hw", -Hap::DefString, hardwareRevision },
        { "sc", -Hap::DefString, setupCode },
        { "sv", sizeof(setupVerifier), setupVerifier },
        { "ss", sizeof(setupSalt), setupSalt },
        { "id", sizeof(deviceId), deviceId },
        { "ci", sizeof(categoryId), &categoryId },
        { "sf", sizeof(statusFlags), &statusFlags },
        { "cn", sizeof(configNum), &configNum },
        { "gs", sizeof(globalStateNum), &globalStateNum },
        { "pn", sizeof(port), &port },
        { "sk", sizeof(keys._pubKey), keys._pubKey},
        { "pk", sizeof(keys._prvKey), keys._prvKey},

        { "p0", sizeof(Controller), &pairings.ctl[0] },
        { "p1", sizeof(Controller), &pairings.ctl[1] },
        { "p2", sizeof(Controller), &pairings.ctl[2] },
        { "p3", sizeof(Controller), &pairings.ctl[3] },
        { "p4", sizeof(Controller), &pairings.ctl[4] },
        { "p5", sizeof(Controller), &pairings.ctl[5] },
        { "p6", sizeof(Controller), &pairings.ctl[6] },
        { "p7", sizeof(Controller), &pairings.ctl[7] },
        { "p8", sizeof(Controller), &pairings.ctl[8] },
        { "p9", sizeof(Controller), &pairings.ctl[9] },
        { "pA", sizeof(Controller), &pairings.ctl[10] },
        { "pB", sizeof(Controller), &pairings.ctl[11] },
        { "pC", sizeof(Controller), &pairings.ctl[12] },
        { "pD", sizeof(Controller), &pairings.ctl[13] },
        { "pE", sizeof(Controller), &pairings.ctl[14] },
        { "pF", sizeof(Controller), &pairings.ctl[15] },
    };


    static void __unused log_conf(const char* pr, _conf* cf)
    {
        if (cf->len < 0)
            LOG_INF("%s: %s= '%s'", pr, cf->key, log_strdup((char*)cf->val));
        else if (cf->len == 1)
            LOG_INF("%s: %s= %d", pr, cf->key, *(uint8_t*)cf->val);
        else if (cf->len == 2)
            LOG_INF("%s: %s= %d", pr, cf->key, *(uint16_t*)cf->val);
        else if (cf->len == 4)
            LOG_INF("%s: %s= %d", pr, cf->key, *(uint32_t*)cf->val);
        else if (cf->len == 6)
            LOG_INF("%s: %s= %02X:%02X:%02X:%02X:%02X:%02X:", pr, cf->key, 
                ((uint8_t*)cf->val)[0], ((uint8_t*)cf->val)[1], ((uint8_t*)cf->val)[2], 
                ((uint8_t*)cf->val)[3], ((uint8_t*)cf->val)[4], ((uint8_t*)cf->val)[5]);
        else
        {
            char s[10];
            snprintf(s, sizeof(s), "%s: %s=", pr, cf->key);
            LOG_HEXDUMP_INF(cf->val, cf->len, log_strdup(s));
        }
    }

    // Restore configuration
    //	Return: 0 on success, non-zero on failure.
    static int cf_get(
        const char *key,			// key[in] the name with skipped part that was used as name in handler registration
        size_t len,					// len[in] the size of the data found in the backend
        settings_read_cb read_cb,	// read_cb[in] function provided to read the data from the backend
        void *cb_arg				// cb_arg[in] arguments for the read function provided by the backend
    )
    {
        const char *next;
        int rc = -ENOENT;

        for (uint32_t i = 0; i < ARRAY_SIZE(conf); i++)
        {
            _conf* cf = &conf[i];

            if (settings_name_steq(key, cf->key, &next) && !next)
            {
                if (cf->len > 0 && (size_t)cf->len != len) 
                {
                    LOG_ERR("Get: Invalid length of '%s' value: %d  expected %d", cf->key, len, cf->len);
                    return -EINVAL;
                }
                else if (cf->len < 0 && len > (size_t)(-cf->len))
                {
                    LOG_ERR("Get: Invalid length of '%s' value: %d  max %d", cf->key, len, -cf->len);
                    return -EINVAL;
                }

                if (cf->len < 0)
                    memset(cf->val, 0, -cf->len);
                rc = read_cb(cb_arg, cf->val, len);
                if (rc < 0) 
                {
                    LOG_ERR("Get: %s read error %d", cf->key, rc);
                }

                //log_conf("Get", cf);

                rc = 0;
            }
        }

        if (rc == -ENOENT)
        {
            char name[8];
            snprintf(name, sizeof(name), "cf/%s", key);
            settings_delete(name);
            //rc = 0;
        }

        return rc;
    }

    // save configuration
    static int cf_set(
        int (*storage_func)(
            const char *name,
            const void *val,
            size_t val_len)
    )
    {
        int rc = 0;
        
        // save config items
        for (uint32_t i = 0; i < ARRAY_SIZE(conf); i++)
        {
            _conf* cf = &conf[i];
            char name[8];

            //log_conf("Set", cf);

            snprintf(name, sizeof(name), "cf/%s", cf->key);

            size_t len = cf->len < 0 ? strlen((const char*)cf->val) : cf->len;

            rc = storage_func(name, cf->val, len);
            if (rc < 0)
            {
                LOG_ERR("Set: %s write error %d", cf->key, rc);
            }
        }
        return 0;
    }
    struct settings_handler hap_cf_settings = 
    {
        .name = (char*)"cf",
        .h_set = cf_get,
        .h_export = cf_set,
    };

    void Init()
    {
        bool manuf = false;

        int rc;
        rc = settings_subsys_init();
        if (rc) 
        {
            LOG_ERR("Init: Settings init failed");
        }
        else
        {
            rc = settings_register(&hap_cf_settings);
            if (rc) 
            {
                LOG_ERR("Init: Settings register failed");
            }
        }

        if (rc == 0)
        {
            rc = settings_load();
            if (rc) 
            {
                LOG_ERR("init: Settings load failed");
            }
        }
        else
        {
            manuf = true;
        }

        if (configNum == 0) // stored settings are invalid, re-init
        {
            manuf = true;   
        }

        if (manuf)
            Reset(manuf);
    }

	void Reset(bool manuf)
    {
        if (manuf)
        {
            LOG_DBG("Manufacturing reset");

            // default settings
            strcpy(name, "uHapTest");
            strcpy(model, "uHap Test Model");
            strcpy(manufacturer, "uHap Manufacturing");
            strcpy(serialNumber, "0000-0001");
            strcpy(firmwareRevision, "0.1");
            strcpy(hardwareRevision, "0.0.1");
            strcpy(setupCode, "000-11-000");

            configNum = 0;
            categoryId = 5;
            port = 0;           // not used with BLE

            keys.init();    // generate new keys
        }
        
        LOG_DBG("Accessory reset, configNum %d", configNum);
        
        // generate new random ID
        Crypto::rnd_data(deviceId, sizeof(deviceId));

        // generate Srp setup verifier
        Srp::Verifier ver;
		ver.init("Pair-Setup", setupCode);
        memcpy(setupVerifier, ver.v, sizeof(setupVerifier));
        memcpy(setupSalt, ver.s, sizeof(setupSalt));

        // increment config number
        configNum++;
        
        // reset GSN to 1
        globalStateNum = 1;

        // erase pairings
        statusFlags = Hap::StatusFlag::NotPaired;

        pairings.Init();
        //resumable.Init();

        Erase();

        Save();
    }

    void Erase()
    {
        for (uint32_t i = 0; i < ARRAY_SIZE(conf); i++)
        {
            _conf* cf = &conf[i];
            char name[8];

            snprintf(name, sizeof(name), "cf/%s", cf->key);
            settings_delete(name);
        }            
    }

    void Save()
    {
        int rc = settings_save();
        if (rc) 
        {
            LOG_ERR("Settings save failed");
        }
    }

    // update settings (paired/non-paired state)
    void Update()
    {
        Config::statusFlags = pairings.Count() ? Hap::StatusFlag::Paired : Hap::StatusFlag::NotPaired;

        Save();

        hap_adv_update();
    }

}
