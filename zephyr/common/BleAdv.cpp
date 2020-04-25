#include <logging/log.h>
LOG_MODULE_REGISTER(appBleAdv, LOG_LEVEL_DBG);

#include "App.h"

// Platform-dependent BLE Adv

static AdvState advCurrent = AdvState::None;		// current adv state
static AdvState advRequested = AdvState::None;		// requested adv state

static Hap::Characteristic* bc_char = nullptr;		// char for broadcast notifications TODO: multiple chars
static Hap::Time::T bc_expire = 0;

void hap_adv_start()
{
	advRequested = AdvState::Regular;
}

void hap_adv_stop()
{
	advRequested = AdvState::None;
	hap_adv_poll();
}

AdvState hap_adv_state()
{
	return advCurrent;
}

static Hap::Ble::Adv::Regular advRegular;
static struct bt_data advRegularData[Hap::Ble::Adv::Regular::Count];
static bool adv_regular_start()
{
	advRegular.init();

	advRegularData[0].type = advRegular.Flags.type;
	advRegularData[0].data_len = advRegular.Flags.length - 1;
	advRegularData[0].data = advRegular.Flags.data;

	advRegularData[1].type = advRegular.Manuf.type;
	advRegularData[1].data_len = advRegular.Manuf.length - 1;
	advRegularData[1].data = advRegular.Manuf.data;

	advRegularData[2].type = advRegular.Name.type;
	advRegularData[2].data_len = advRegular.Name.length - 1;
	advRegularData[2].data = advRegular.Name.data;

	struct bt_le_adv_param param =
	{
		.options = BT_LE_ADV_OPT_CONNECTABLE | BT_LE_ADV_OPT_USE_NAME,
		.interval_min = BT_GAP_ADV_FAST_INT_MIN_2,
		.interval_max = BT_GAP_ADV_FAST_INT_MAX_2
	};
	int rc = bt_le_adv_start(&param, advRegularData, ARRAY_SIZE(advRegularData), NULL, 0);
	if (rc)
	{
		LOG_ERR("bt_le_adv_start failed, rc %d", rc);
		return false;
	}

	LOG_INF("Regular advertising started");
	return true;
}

static Hap::Ble::Adv::Notif advNotif;
static struct bt_data advNotifData[Hap::Ble::Adv::Notif::Count];
static bool adv_notif_start()
{
	advNotif.init(bc_char);

	advNotifData[0].type = advNotif.Flags.type;
	advNotifData[0].data_len = advNotif.Flags.length - 1;
	advNotifData[0].data = advNotif.Flags.data;

	advNotifData[1].type = advNotif.Manuf.type;
	advNotifData[1].data_len = advNotif.Manuf.length - 1;
	advNotifData[1].data = advNotif.Manuf.data;

	struct bt_le_adv_param param =
	{
		.options = BT_LE_ADV_OPT_CONNECTABLE,
		.interval_min = BT_GAP_ADV_FAST_INT_MIN_2,	// TODO: configured BC interval
		.interval_max = BT_GAP_ADV_FAST_INT_MAX_2
	};
	int rc = bt_le_adv_start(&param, advNotifData, ARRAY_SIZE(advNotifData), NULL, 0);
	if (rc)
	{
		LOG_ERR("bt_le_adv_start failed, rc %d", rc);
		return false;
	}

	LOG_INF("Notification advertising started");
	return true;
}

void hap_adv_poll()
{
	// if bc timer expired go back to regurar adv
	if ((advCurrent == AdvState::Notif) && (Hap::Time::stampMs() > bc_expire))
		advRequested = AdvState::Regular;

	// check if state switch requested
	if (advCurrent == advRequested)
		return;

	// stop adv if active
	if (advCurrent != AdvState::None)
	{
		bt_le_adv_stop();
		LOG_INF("Advertising stopped");
	}

	// if update requested restart previous state
	if (advRequested == AdvState::Update)
		advRequested = advCurrent;
	advCurrent = AdvState::None;

	// start adv
	if (advRequested == AdvState::Regular)
	{
		if (adv_regular_start())
			advCurrent = AdvState::Regular;
	}
	else if (advRequested == AdvState::Notif)
	{
		if (adv_notif_start())
			advCurrent = AdvState::Notif;
	}
}

void hap_adv_update()
{
	// re-calculate current adv data to see if it's changed
	if (advCurrent == AdvState::Regular)
	{
		Hap::Ble::Adv::Regular adv;
		adv.init();

		if (memcmp(&adv, &advRegular, sizeof(advRegular)) != 0)
			advRequested = AdvState::Update; 
	}
	else if (advCurrent == AdvState::Notif)
	{
		Hap::Ble::Adv::Notif adv;
		adv.init(bc_char);

		if (memcmp(&adv, &advNotif, sizeof(advNotif)) != 0)
			advRequested = AdvState::Update; 
	}
}

// start/extend broadcast notifications
void hap_adv_broadcast(Hap::Characteristic* ch)
{
	bc_char = ch;
	bc_expire = Hap::Time::stampMs() + 30000;
	advRequested = AdvState::Notif;
}