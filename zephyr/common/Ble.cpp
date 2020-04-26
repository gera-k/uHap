/*
	Copyright(c) 2020 Gera Kazakov
	SPDX-License-Identifier: Apache-2.0
*/

#include <logging/log.h>
LOG_MODULE_REGISTER(appBle, LOG_LEVEL_DBG);

#include "App.h"

static bool hap_ble_enabled = false;		// global BLE enable/disable
static bool hap_ble_connected = false;		// BLE connected/disconnected
static bool hap_ble_gsn_updated = false;	// GSN updated during current connected/disconnected state

//=== connection/session list
static K_MUTEX_DEFINE(bleSessMtx);
static Hap::Ble::Session* bleSess[CONFIG_BT_MAX_CONN] = {nullptr};
static uint8_t bleSessCount = 0;

static Hap::Ble::Session* bleSessCreate(struct bt_conn *conn)
{
	Hap::Ble::Session* sess = nullptr;

	k_mutex_lock(&bleSessMtx, K_FOREVER);

	for (uint8_t i = 0; i < ARRAY_SIZE(bleSess); i++)
	{
		auto _sess = bleSess[i];
		if (_sess == nullptr)
		{
			sess = Hap::Ble::SessAlloc(conn);
			if (sess != nullptr)
			{
				bleSess[i] = sess;
				sess->id = i + 1;

				bleSessCount++;
			}
			break;
		}
		else
		{
			// TODO: verify that existing session is still active
		}
	}

	k_mutex_unlock(&bleSessMtx);

	return sess;
}

static void bleSessDestroy(struct bt_conn *conn)
{
	k_mutex_lock(&bleSessMtx, K_FOREVER);

	for (uint32_t i = 0; i < ARRAY_SIZE(bleSess); i++)
	{
		auto sess = bleSess[i];
		
		if (sess != nullptr && sess->conn == conn)
		{
			bleSess[i] = NULL;
			sess->~Session();
			Hap::Ble::SessFree(sess);

			bleSessCount--;

			break;
		}
	}

	k_mutex_unlock(&bleSessMtx);
}

static Hap::Ble::Session* bleSessGet(struct bt_conn *conn)
{
	Hap::Ble::Session* sess = nullptr;

	k_mutex_lock(&bleSessMtx, K_FOREVER);

	for (uint32_t i = 0; i < ARRAY_SIZE(bleSess); i++)
	{
		auto _sess = bleSess[i];
		
		if (_sess != nullptr && _sess->conn == conn)
		{
			sess = _sess;
			break;
		}
	}

	k_mutex_unlock(&bleSessMtx);

	return sess;
}

void bleSessForEach(std::function<void(const Hap::Ble::Session* sess)> cb)
{
	for (uint32_t i = 0; i < ARRAY_SIZE(bleSess); i++)
	{
		auto sess = bleSess[i];
		
		if (sess != nullptr)
		{
			cb(sess);
		}
	}
}

void bleSessPoll()
{
	for (uint32_t i = 0; i < ARRAY_SIZE(bleSess); i++)
	{
		auto sess = bleSess[i];
		if (sess == nullptr)
			continue;

		if (sess->lastActivity + 30000 < Hap::Time::stampMs())
		{
			Hap::Disconnect(sess, 10);
		}
	}
}

struct disconnectWork
{
	struct k_delayed_work work;
	const Hap::Controller* ctlr;
	const Hap::Session* sess;
};
K_MEM_SLAB_DEFINE(hap_dw_slab, sizeof(disconnectWork), 4, 4);

static void disconnectWorkHandler(struct k_work *work)
{
	disconnectWork* dw = CONTAINER_OF(work, disconnectWork, work);

	k_mutex_lock(&bleSessMtx, K_FOREVER);

	if (dw->sess == nullptr)
	{
		for (uint32_t i = 0; i < ARRAY_SIZE(bleSess); i++)
		{
			auto sess = bleSess[i];
			if (sess == nullptr)
				continue;

			if (dw->ctlr == NULL ||	// if null, disconnect all 
				(sess->ctlrPaired != NULL &&
					memcmp(sess->ctlrPaired->ctlr, dw->ctlr->ctlr, sess->ctlrPaired->CtlrLen) == 0)
			)
			{
				struct bt_conn *conn = (struct bt_conn *)sess->conn;
				bt_conn_disconnect(conn, BT_HCI_ERR_LOCALHOST_TERM_CONN);
			}
		}
	}
	else
	{
		const Hap::Ble::Session* bleSess = static_cast<const Hap::Ble::Session*>(dw->sess);
		struct bt_conn *conn = (struct bt_conn *)bleSess->conn;
		bt_conn_disconnect(conn, BT_HCI_ERR_CONN_TIMEOUT);
	}

	k_mutex_unlock(&bleSessMtx);

	k_mem_slab_free(&hap_dw_slab, (void**)(&dw));
}

void hap_ble_disconnect(const Hap::Session* sess, uint32_t delay)
{
	disconnectWork* dw;
	
	k_mem_slab_alloc(&hap_dw_slab, (void**)&dw, K_NO_WAIT);
	if (dw != NULL)
	{
		k_delayed_work_init(&dw->work, disconnectWorkHandler);
		dw->ctlr = nullptr;
		dw->sess = sess;

		k_delayed_work_submit(&dw->work, delay);
	}
}

// request disconnect of all sessions to this controller
void hap_ble_disconnect(const Hap::Controller* ctlr, uint32_t delay)
{
	disconnectWork* dw;
	
	k_mem_slab_alloc(&hap_dw_slab, (void**)&dw, K_NO_WAIT);
	if (dw != NULL)
	{
		k_delayed_work_init(&dw->work, disconnectWorkHandler);
		dw->ctlr = ctlr;
		dw->sess = nullptr;

		k_delayed_work_submit(&dw->work, delay);
	}
}

bool hap_ble_is_connected()
{
	return hap_ble_connected;
}

void hap_ble_gsn_update(bool each_update)
{
	if (!hap_ble_gsn_updated || each_update)
	{
		hap_ble_gsn_updated = true;
		Hap::Config::globalStateNum++;
		Hap::Config::Update();
	}
}

static void connected(struct bt_conn *conn, u8_t err)
{
	if (err)
	{
		LOG_INF("Connection failed (err 0x%02x)", err);
	}
	else
	{
		Hap::Ble::Session* sess = NULL;
		
		if (hap_ble_enabled)
			sess = bleSessCreate(conn);

		if (sess == NULL)
		{
			LOG_WRN("Session creation error");
			bt_conn_disconnect(conn, BT_HCI_ERR_CONN_LIMIT_EXCEEDED);
		}
		else
		{
			const bt_addr_le_t * addr = bt_conn_get_dst(conn);
			LOG_INF("%d:Connected: %p  %02X-%02X:%02X:%02X:%02X:%02X:%02X  MTU %d",
				sess->id,
				conn, addr->type,
				addr->a.val[5], addr->a.val[4], addr->a.val[3],  
				addr->a.val[2], addr->a.val[1], addr->a.val[0],  
				bt_gatt_get_mtu(conn));

			hap_ble_connected = true;
			hap_ble_gsn_updated = false;
			if (addr->type == BT_ADDR_LE_RANDOM)
				hap_adv_stop();
		}
	}
}

static void disconnected(struct bt_conn *conn, u8_t reason)
{
	const bt_addr_le_t * addr = bt_conn_get_dst(conn);
	Hap::Ble::Session* sess = bleSessGet(conn);
	LOG_INF("%d:Disconnected: %p reason %02x", sess->id, conn, reason);
	bleSessDestroy(conn);

	if (addr->type == BT_ADDR_LE_RANDOM)
		hap_adv_start();

	if (bleSessCount == 0)
	{
		hap_ble_connected = false;
		hap_ble_gsn_updated = false;
	}
}

static struct bt_conn_cb conn_callbacks = {
	.connected = connected,
	.disconnected = disconnected,
};

void hap_ble_start()
{
	bt_conn_cb_register(&conn_callbacks);
	hap_ble_enabled = true;

	Hap::Ble::Adv::SetAAI(Hap::Buf(Hap::Config::deviceId, sizeof(Hap::Config::deviceId)));
	
	hap_adv_start();
}

void hap_ble_stop()
{
	hap_ble_enabled = false;
	hap_adv_stop();
	Hap::Disconnect((Hap::Controller*)nullptr, 1);

	k_sleep(100);
}

ssize_t hap_prop_read(
	struct bt_conn *conn, const struct bt_gatt_attr *attr,
	void *buf, u16_t len, u16_t offset)
{
	Hap::Ble::Session* sess = bleSessGet(conn);
	if (sess == NULL)
	{
		LOG_ERR("Read: unknown connection");
		return BT_GATT_ERR(BT_ATT_ERR_UNLIKELY);
	}

	Hap::Property* prop = (Hap::Property*)(attr->user_data);
	return Hap::Ble::Read(sess, prop, buf, len, offset);
}

ssize_t hap_prop_write(
	struct bt_conn *conn, const struct bt_gatt_attr *attr,
	const void *buf, u16_t len, u16_t offset, u8_t flags)
{
	Hap::Ble::Session* sess = bleSessGet(conn);
	if (sess == NULL)
	{
		LOG_ERR("Read: unknown connection");
		return BT_GATT_ERR(BT_ATT_ERR_UNLIKELY);
	}

	Hap::Property* prop = (Hap::Property*)(attr->user_data);
	return Hap::Ble::Write(sess, prop, buf, len, offset);
}

static void hap_indicate_cb(
	struct bt_conn *conn,
	const struct bt_gatt_attr *attr,
	u8_t err)
{
	LOG_DBG("hap_indicate_cb: err %d", err);
}

// init GATT attrs for service record
// It needs four updates:
//	[0].user_data = service UUID
//	(struct bt_gatt_chrc*)([1].user_data)->uuid = Service::uuidIid
//	[2].uuid = Service::uuidIid
//	[2].user_data = aHap::Property* pointer to service iid property
void hap_svc_gatt_init(Hap::Service* svc, bt_gatt_attr* attr)
{
	bt_uuid* uuid = &Hap::Ble::Service::uuidIid.uuid;
	attr[0].user_data = CONTAINER_OF(svc->uuid(), bt_uuid_128, val);
	((struct bt_gatt_chrc*)(attr[1].user_data))->uuid = uuid;
	attr[2].uuid = uuid;
	attr[2].user_data = &svc->iid;
}

// init GATT attrs for characteristic record
// Tt needs five updates:
//	(struct bt_gatt_chrc*)([0].user_data)->uuid = characteristic UUID
//	(struct bt_gatt_chrc*)([0].user_data)->prop |= Perm::ev ? BT_GATT_CHRC_INDICATE : 0
//	[1].uuid = characteristic UUID
//	[1].user_data = Hap::Property* pointer to Value property
//  [2].user_data = eventCtx._ccc
//	[3].uuid = Characteristic::uuidIid
//  [3].user_data = Hap::Property* pointer to Iid property
void hap_ch_gatt_init(Hap::Characteristic* ch, bt_gatt_attr* attr)
{
	bt_uuid* uuid = &CONTAINER_OF(ch->uuid(), bt_uuid_128, val)->uuid;
	((struct bt_gatt_chrc*)(attr[0].user_data))->uuid = uuid;
	if (ch->perm() & Hap::Perm::ev)
		((struct bt_gatt_chrc*)(attr[0].user_data))->properties |= BT_GATT_CHRC_INDICATE; // Set Indicate property (R2: 7.4.6)
	attr[1].uuid = uuid;
	attr[1].user_data = ch->get(Hap::Type::Value);
	attr[2].user_data = &ch->eventCtx._ccc;
	attr[3].uuid = &Hap::Ble::Characteristic::uuidIid.uuid;
	attr[3].user_data = ch->get(Hap::Type::CharIid);

	// init event context

	memset(&ch->eventCtx._ccc, 0, sizeof(ch->eventCtx._ccc));
	ch->eventCtx._ccc.cfg_changed = [](const bt_gatt_attr *attr, u16_t value)
	{
		// send notification to characteristic this attr belongs to
		//	the attr is CCCD one so take user_data of next attr which points to iid property
		Hap::Property* prop = static_cast<Hap::Property*>(attr[1].user_data);
		Hap::Characteristic* ch = prop->charOwner;
		ch->ConnectedEvent(nullptr, value & BIT_(1));	// TODO: figure session
	};

	ch->eventCtx.params.uuid = NULL; //uuid;
	ch->eventCtx.params.attr = &attr[0];
	ch->eventCtx.params.func = hap_indicate_cb;
	ch->eventCtx.params.data = NULL;
	ch->eventCtx.params.len = 0;
}

// update HAP service GATT attrs
void hap_service_init(Hap::Service* svc, bt_gatt_attr* attr)
{
	hap_svc_gatt_init(svc, attr);

	attr += HAP_SVC_GATT_COUNT;
	svc->forEachChar([&attr](Hap::Characteristic* ch)
	{
		hap_ch_gatt_init(ch, attr);
		attr += HAP_CHAR_GATT_COUNT;
	});
}

// Accessory Info Service
static Hap::AccessoryInfo accessory_info;
static struct bt_gatt_attr hap_accessory_info_attr[] = 
{
	HAP_BLE_SERVICE,
	HAP_BLE_CHARACTERISTIC,
	HAP_BLE_CHARACTERISTIC,
	HAP_BLE_CHARACTERISTIC,
	HAP_BLE_CHARACTERISTIC,
	HAP_BLE_CHARACTERISTIC,
	HAP_BLE_CHARACTERISTIC,
	HAP_BLE_CHARACTERISTIC,
};
static struct bt_gatt_service hap_accessory_info_svc = BT_GATT_SERVICE(hap_accessory_info_attr);

int hap_accessory_info_init(Hap::iid_t& iid)
{
	LOG_DBG("hap_accessory_info_init");

	// init accessory_info service
	accessory_info.init(iid);
	hap_service_init(&accessory_info, hap_accessory_info_attr);
	
	int rc = bt_gatt_service_register(&hap_accessory_info_svc);
	if(rc)
	{
		LOG_ERR("bt_gatt_service_register(&hap_accessory_info_svc) failed, rc %d", rc);
		return rc;
	}

	LOG_INF("HAP accessory_info service registered");

	return 0;
}

// Proto info service
static struct bt_gatt_attr hap_proto_info_attr[] = 
{
	HAP_BLE_SERVICE,
	HAP_BLE_CHARACTERISTIC,
	HAP_BLE_CHARACTERISTIC,
};
static struct bt_gatt_service hap_proto_info_svc = BT_GATT_SERVICE(hap_proto_info_attr);

int hap_proto_info_init(Hap::iid_t& iid)
{
	LOG_DBG("hap_proto_info_init");

	Hap::Ble::ProtoInfo::init(iid);
	Hap::Service* svc = Hap::Ble::ProtoInfo::Svc();
	hap_service_init(svc, hap_proto_info_attr);
	
	//dump(svc);
	//dump_gatt(hap_proto_info_attr, ARRAY_SIZE(hap_proto_info_attr));

	int rc = bt_gatt_service_register(&hap_proto_info_svc);
	if(rc)
	{
		LOG_ERR("bt_gatt_service_register(&hap_proto_info_svc) failed, rc %d", rc);
		return rc;
	}

	LOG_INF("HAP proto_info service registered");

	return 0;
}

// Pairing service
static struct bt_gatt_attr hap_pairing_attr[] = 
{
	HAP_BLE_SERVICE,
	HAP_BLE_CHARACTERISTIC,
	HAP_BLE_CHARACTERISTIC,
	HAP_BLE_CHARACTERISTIC,
	HAP_BLE_CHARACTERISTIC,
};
static struct bt_gatt_service hap_pairing_svc = BT_GATT_SERVICE(hap_pairing_attr);

int hap_pairing_init(Hap::iid_t& iid)
{
	LOG_DBG("hap_pairing_init");

	// init Pairing service
	Hap::Ble::Pairing::init(iid);
	Hap::Service* svc = Hap::Ble::Pairing::Svc();
	hap_service_init(svc, hap_pairing_attr);
	
	//dump(svc);
	//dump_gatt(hap_pairing_attr, ARRAY_SIZE(hap_pairing_attr));

	int rc = bt_gatt_service_register(&hap_pairing_svc);
	if(rc)
	{
		LOG_ERR("bt_gatt_service_register(&hap_pairing_svc) failed, rc %d", rc);
		return rc;
	}

	LOG_INF("HAP pairing service registered");

	return 0;
}

