
#include <logging/log.h>
LOG_MODULE_REGISTER(appApp, LOG_LEVEL_DBG);

#include "App.h"
#include "Util/LbPwm.h"
#include <drivers/pwm.h>
#include <shell/shell.h>
#include <power/reboot.h>
#include <logging/log_ctrl.h>

#ifdef CONFIG_HAP_CRYPTO_TEST
#include "Crypto/Test/CryptoTest.h"
void main(void)
{
	printk("Crypto test enabled\n");
	cryptoTest();
	
	while (1)
		k_sleep(MSEC_PER_SEC);
}
#else

// HAP accessory on Nordic PCA10059 dongle
//	implements Lightbulb service
//
// Hardware:
//	SW1 Button P1.06
//	SW2 Reset P0.181
//	LD1 Green P0.06		used as blinker indicating the board is alive. TODO: connected/disconnected/paired/adv states
//	LD2 Red P0.08		PWM-controlled RGB LED, used as a light bulb
//	LD2 Green P1.09
//	LD2 Blue P0.12

struct hw_led
{
	const char* ctlr;
	gpio_pin_t pin;
	gpio_flags_t flags;
	struct device *dev = nullptr;
};

static hw_led LD1 = 
{
	DT_ALIAS_LED0_GPIOS_CONTROLLER,
	DT_ALIAS_LED0_GPIOS_PIN,
	GPIO_OUTPUT_ACTIVE | DT_ALIAS_LED0_GPIOS_FLAGS
};

static hw_led LD2[3] = 
{
	{
		DT_ALIAS_RED_PWM_LED_PWMS_CONTROLLER,
		DT_ALIAS_RED_PWM_LED_PWMS_CHANNEL,
	},
	{
		DT_ALIAS_GREEN_PWM_LED_PWMS_CONTROLLER,
		DT_ALIAS_GREEN_PWM_LED_PWMS_CHANNEL,
	},
	{
		DT_ALIAS_BLUE_PWM_LED_PWMS_CONTROLLER,
		DT_ALIAS_BLUE_PWM_LED_PWMS_CHANNEL,
	}
};

void led_init()
{
	int rc;
	hw_led* led;

	led = &LD1;
	led->dev = device_get_binding(led->ctlr);
	if (led->dev != nullptr)
	{
		rc = gpio_pin_configure(led->dev, led->pin, led->flags);
		if (rc < 0)
			led->dev = nullptr;
		else
			gpio_pin_set(led->dev, led->pin, 0);
	}

	for (u32_t i = 0; i < ARRAY_SIZE(LD2); i++)
	{
		hw_led* led = &LD2[i];

		led->dev = device_get_binding(led->ctlr);
		if(led->dev != nullptr)
		{
			uint64_t cycles;
			rc = pwm_get_cycles_per_sec(led->dev, led->pin, &cycles);
			if (rc < 0)
			{
				led->dev = nullptr;
			}
			else
			{
				HAP_LOG_INF("%s:%d: %lld Hz", led->ctlr, led->pin, cycles);
			}
		}
	}
}

// LED1 - blinker
void led_blinker(bool on)
{
	hw_led* blinker = &LD1;

	if (blinker->dev != NULL)
		gpio_pin_set(blinker->dev, blinker->pin, on); 
}

// LED2 - lightbulb

// PWM value 0..255
void led_lb(uint32_t r_pwm, uint32_t g_pwm, uint32_t b_pwm)
{
	hw_led* r = &LD2[0];	
	hw_led* g = &LD2[1];	
	hw_led* b = &LD2[2];	

	pwm_pin_set_usec(r->dev, r->pin, 2550, r_pwm * 10, 0);
	pwm_pin_set_usec(g->dev, g->pin, 2550, g_pwm * 10, 0);
	pwm_pin_set_usec(b->dev, b->pin, 2550, b_pwm * 10, 0);

	HAP_LOG_INF("LD2: R %d  G %d  B %d", r_pwm, g_pwm, b_pwm);
}

void led_lb(bool on)
{
	led_lb(on ? 255 : 0, on ? 255 : 0, on ? 255 : 0);
}

class LbRGB : public LbPWM
{
private:
	void update()
	{
		uint8_t R, G, B;
		rgb(R, G, B);
		led_lb(v2p(R), v2p(G), v2p(B));
	}
public:
	void Init()
	{
		update();
	}
	
	// turn on/off
	void On(bool on)
	{
		_on = on;
		update();
	}

	// set brightness % (0..100)
	void Brightness(uint8_t br)
	{
		_v = br;
		update();
	}

	void Hue(uint16_t h)
	{
		_h = h;
		update();
	}

	void Saturation(uint8_t s)
	{
		_s = s;
		update();
	}
};

// global IID counter
Hap::iid_t iid = 1;

LbRGB lbRGB;

// Lightbulb service
struct LightBulb : public Hap::LightBulb
{
	// max num of characteristics
	static constexpr uint8_t MAX_CHARS = 5;

	// BLE Primary service must include the ServiceSignature characteristic
	Hap::Ble::Service::CharSign sign;

	void init(Hap::iid_t& iid)
	{
		Hap::LightBulb::init(iid, Hap::SvcProp::Primary);

		brightness.init(this, iid);
		hue.init(this, iid);
		saturation.init(this, iid);
        sign.init(this, iid);

		on.onRead = [this]() -> Hap::Status
		{
			//Hap::On* on = static_cast<Hap::On*>(ch);
			LOG_INF("Lb::On Read %d", on.value());

			return Hap::Status::Success;
		};

		on.onWrite = [this]() -> Hap::Status
		{
			LOG_INF("Lb::On Write %d", on.value());

			lbRGB.On(on.value());

			return Hap::Status::Success;
		};

		brightness.onWrite = [this]() -> Hap::Status
		{
			LOG_INF("Lb::Brightness Write %d", brightness.value());

			lbRGB.Brightness(brightness.value());

			return Hap::Status::Success;
		};

		hue.onWrite = [this]() -> Hap::Status
		{
			LOG_INF("Lb::Hue Write %d", (int)hue.value());

			lbRGB.Hue((uint16_t)hue.value());

			return Hap::Status::Success;
		};

		saturation.onWrite = [this]() -> Hap::Status
		{
			LOG_INF("Lb::Saturation Write %d", (int)saturation.value());

			lbRGB.Saturation((uint8_t)saturation.value());

			return Hap::Status::Success;
		};
	}
} lb;

// Define GATT attr array for Zephyr BT API
//	must contain one HAP_BLE_SERVICE record and MAX_CHARS HAP_BLE_CHARACTERISTIC records.
static struct bt_gatt_attr lb_attr[HapBleServiceGattCount(LightBulb::MAX_CHARS)] = 
{
	HAP_BLE_SERVICE,
	HAP_BLE_CHARACTERISTIC,
	HAP_BLE_CHARACTERISTIC,
	HAP_BLE_CHARACTERISTIC,
	HAP_BLE_CHARACTERISTIC,
	HAP_BLE_CHARACTERISTIC,
};
static struct bt_gatt_service lb_svc = BT_GATT_SERVICE(lb_attr);

// init the Lightbulb service
int lb_init(Hap::iid_t& iid)
{
	LOG_DBG("lb_init");

	// init Lightbulb service
	lb.init(iid);
	lb_svc.attr_count = HapBleServiceGattCount(lb.char_count);
	hap_service_init(&lb, lb_attr);
	
//	dump(&lb);
//	dump_gatt(lb_svc.attrs, lb_svc.attr_count);

	int rc = bt_gatt_service_register(&lb_svc);
	if(rc)
	{
		LOG_ERR("bt_gatt_service_register(&lb_svc) failed, rc %d", rc);
		return rc;
	}

	LOG_INF("LightBulb service registered");

	return 0;
}

// bt_enable callback
static void bt_ready_cb(int rc)
{
	if (rc) 
	{
		LOG_ERR("bt initialization failed,  rc %d", rc);
		return;
	}

	k_sleep(100);	

	LOG_INF("BT Initialized");

	bt_set_name(Hap::Config::name);

	hap_accessory_info_init(iid);	// accesory info - mandatory service, must be first so it gets correct IID
	hap_proto_info_init(iid);		// protocol info - mandatory service
	hap_pairing_init(iid);			// pairing service - mandatory service

	lb_init(iid);					// lightbulb service

	hap_ble_start();
}

// reset all hapXXX and appXXX log levels to INF
//	the compiled-in levels are DBG
static void log_levels_init()
{
	for (uint32_t i = 0; i < log_src_cnt_get(CONFIG_LOG_DOMAIN_ID); i++)
	{
		auto name = log_source_name_get(CONFIG_LOG_DOMAIN_ID, i);
		if ((strncmp(name, "hap", 3) == 0) || (strncmp(name, "app", 3) == 0)) 
		{
			u32_t l = log_filter_set(NULL, CONFIG_LOG_DOMAIN_ID, i, LOG_LEVEL_INF);
			LOG_DBG("Set %s (%d) log level to %d", name, i, l);
		}
	}
}

void main(void)
{
	LOG_INF("uHAP project: BLE Lightbulb Accessory");

	Hap::Config::Init();

	//Hap::Config::Reset(true);
	//k_sleep(2000);

	led_init();

	int rc = bt_enable(bt_ready_cb);
	if (rc) 
	{
		LOG_ERR("bt_enable failed,  rc %d", rc);
		return;
	}

	k_sleep(1000);
	log_levels_init();

	uint32_t cnt = 0;
	bool led_is_on = true;
	while (1)
	{
		hap_adv_poll();
		bleSessPoll();

		k_sleep(100);

		if (cnt++ % 10)
			continue;

		led_blinker(led_is_on);
		led_is_on = !led_is_on;
	}
}

// Shell support

static int hap_cmd_config(const struct shell *shell, size_t argc, char **argv)
{
    shell_print(shell, "Configuration:");
    shell_print(shell, "              name: '%s'", Hap::Config::name);
    shell_print(shell, "             model: '%s'", Hap::Config::model);
    shell_print(shell, "      manufacturer: '%s'", Hap::Config::manufacturer);
    shell_print(shell, "      serialNumber: '%s'", Hap::Config::serialNumber);
    shell_print(shell, "  firmwareRevision: '%s'", Hap::Config::firmwareRevision);
    shell_print(shell, "  hardwareRevision: '%s'", Hap::Config::hardwareRevision);
    shell_print(shell, "         setupCode: '%s'", Hap::Config::setupCode);
    shell_print(shell, "          deviceId: %02X:%02X:%02X:%02X:%02X:%02X",
        Hap::Config::deviceId[0], Hap::Config::deviceId[1], Hap::Config::deviceId[2],
        Hap::Config::deviceId[3], Hap::Config::deviceId[4], Hap::Config::deviceId[5]);
    shell_print(shell, "        categoryId: %d", Hap::Config::categoryId);
    shell_print(shell, "       statusFlags: 0x%02X", Hap::Config::statusFlags);
    shell_print(shell, "         configNum: %d", Hap::Config::configNum);
    shell_print(shell, "    globalStateNum: %d", Hap::Config::globalStateNum);
    
	shell_print(shell, "Pairings:");
	for (unsigned i=0; i<ARRAY_SIZE(Hap::Config::pairings.ctl); i++)
	{
    	Hap::Controller* ctlr = &Hap::Config::pairings.ctl[i];

		if (ctlr->perm == Hap::Controller::Perm::None)
			continue;

        shell_print(shell, "  id: '%s'  Perm: %02X", ctlr->ctlr, ctlr->perm);

		if (ctlr->sessValid)
		{
			shell_print(shell, "  sess: %08X_%08X  ssec: %svalid",
				(uint32_t)(ctlr->sess >> 32), (uint32_t)(ctlr->sess), ctlr->ssecValid ? "" : "in");
		}
    };

    return 0;
}

static int hap_cmd_status(const struct shell *shell, size_t argc, char **argv)
{
    shell_print(shell, "Sessions:");

	bleSessForEach([shell](const Hap::Ble::Session* sess)
	{
		struct bt_conn *conn = (struct bt_conn *)sess->conn;
		const bt_addr_le_t * addr = bt_conn_get_dst(conn);
		
		shell_print(shell, "  sess %d  conn %p  %s %02X:%02X:%02X:%02X:%02X:%02X  MTU %d %ssecured",
				sess->id, conn, 
				addr->type == BT_ADDR_LE_PUBLIC ? "Pub" :
				addr->type == BT_ADDR_LE_RANDOM ? "Rnd" : "???",
				addr->a.val[5], addr->a.val[4], addr->a.val[3],  
				addr->a.val[2], addr->a.val[1], addr->a.val[0],  
				bt_gatt_get_mtu(conn),
				sess->secured ? "" : "un");

	});

    shell_print(shell, "Advertising: %s",
		hap_adv_state() == AdvState::None ? "None" :
		hap_adv_state() == AdvState::Regular ? "Regular" :
		hap_adv_state() == AdvState::Notif ? "Notif" :
		hap_adv_state() == AdvState::Update ? "Update" : "?");

	return 0;
}

static int hap_cmd_reset(const struct shell *shell, size_t argc, char **argv)
{
	bool manuf = false;

	if (argc > 1 && strcmp(argv[1], "-m") == 0)
		manuf = true;

	shell_print(shell, "%s Reset, configNum %d", manuf ? "Manufacturing" : "Accessory", Hap::Config::configNum);

	hap_ble_stop();

	Hap::Config::Reset(manuf);

	hap_cmd_config(shell, argc, argv);

	k_sleep(1000);

	sys_reboot(SYS_REBOOT_COLD);

    return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(hap_cmds,
        SHELL_CMD(config, NULL, "Print HAP configuration.", hap_cmd_config),
        SHELL_CMD(status, NULL, "Print HAP status.", hap_cmd_status),
        SHELL_CMD(reset, NULL, "[-m]  (Manufacturing) Reset and reboot.", hap_cmd_reset),
        SHELL_SUBCMD_SET_END
);

SHELL_CMD_REGISTER(hap, &hap_cmds, "HAP status/config", NULL);

static int lb_status(const struct shell *shell, size_t argc, char **argv)
{
	shell_print(shell, "Lb state: %s", lb.on.value() ? "on" : "off");
	shell_print(shell, "  Events: %sabled", lb.on.event ? "en" : "dis");
    return 0;
}

static int lb_on(const struct shell *shell, size_t argc, char **argv)
{
	lb.on.value = true;
	led_lb(true);
	lb.on.Indicate();

	return 0;
}

static int lb_off(const struct shell *shell, size_t argc, char **argv)
{
	lb.on.value = false;
	led_lb(false);
	lb.on.Indicate();

	return 0;
}

static int lb_pwm(const struct shell *shell, size_t argc, char **argv)
{
	long r = strtol(argv[1], NULL, 10);
	long g = strtol(argv[2], NULL, 10);
	long b = strtol(argv[3], NULL, 10);

	HAP_LOG_INF("Set LB: %ld %ld %ld", r, g, b);

	led_lb(r, g, b);
	// TODO: convert to HSV, indicate changes

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(lb_cmds,
        SHELL_CMD(status, NULL, "Print LightBulb status.", lb_status),
        SHELL_CMD(on, NULL, "Turn LightBulb on.", lb_on),
        SHELL_CMD(off, NULL, "Turn LightBulb off.", lb_off),
        SHELL_CMD_ARG(pwm, NULL, "<r> <g> <b> Set LightBulb pwm.", lb_pwm, 4, 0),
        SHELL_SUBCMD_SET_END
);

SHELL_CMD_REGISTER(lb, &lb_cmds, "LightBulb status/control", NULL);

#endif /* CRYPTO_TEST */