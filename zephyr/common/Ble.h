/*
	Copyright(c) 2020 Gera Kazakov
	SPDX-License-Identifier: Apache-2.0
*/

#ifndef _UHAP_Z_BLE_H_
#define _UHAP_Z_BLE_H_

// platform-dependent BLE stuff


// GATT read/write callbacks
ssize_t hap_prop_read(
	struct bt_conn *conn, const struct bt_gatt_attr *attr,
	void *buf, u16_t len, u16_t offset);
ssize_t hap_prop_write(
	struct bt_conn *conn, const struct bt_gatt_attr *attr,
	const void *buf, u16_t len, u16_t offset, u8_t flags);

// fix for C++
#undef BT_GATT_ATTRIBUTE
#define BT_GATT_ATTRIBUTE(_uuid, _perm, _read, _write, _value) { \
	.uuid = _uuid, .read = _read, .write = _write, \
	.user_data = _value, .perm = _perm }

// define HAP service record - 3 GATT attributes
//	- service attr
//	- service iid char attr (R2: 7.4.4.3)
//	- service iid char value (RO)
#define HAP_BLE_SERVICE \
	BT_GATT_PRIMARY_SERVICE(NULL), \
	BT_GATT_CHARACTERISTIC(NULL, \
		BT_GATT_CHRC_READ, BT_GATT_PERM_READ, \
		hap_prop_read, NULL, NULL)
#define HAP_SVC_GATT_COUNT 3

static constexpr uint8_t HapBleSvcGattCount = 3;

// define HAP characteristic record - 3-4 GATT attributes
//	- char attr
//	- char value (RW)
//	- CCC (RW)
//	- char iid descriptor (RO)
#define HAP_BLE_CHARACTERISTIC \
	BT_GATT_CHARACTERISTIC(NULL, \
		BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE, \
		BT_GATT_PERM_READ | BT_GATT_PERM_WRITE, \
		hap_prop_read, hap_prop_write, NULL), \
	BT_GATT_ATTRIBUTE(BT_UUID_GATT_CCC,	\
		BT_GATT_PERM_READ | BT_GATT_PERM_WRITE, \
		bt_gatt_attr_read_ccc, bt_gatt_attr_write_ccc, NULL), \
	BT_GATT_DESCRIPTOR(NULL, \
		BT_GATT_PERM_READ, \
		hap_prop_read, NULL, NULL)
#define HAP_CHAR_GATT_COUNT 4

static constexpr uint8_t HapBleCharGattCount = 4;

static constexpr uint8_t HapBleServiceGattCount(uint8_t charCount)
{
	return HapBleSvcGattCount + HapBleCharGattCount * charCount;
}

int hap_accessory_info_init(Hap::iid_t& iid);
int hap_proto_info_init(Hap::iid_t& iid);
int hap_pairing_init(Hap::iid_t& iid);

void hap_svc_gatt_init(Hap::Service* svc, bt_gatt_attr* attr);
void hap_ch_gatt_init(Hap::Characteristic* ch, bt_gatt_attr* attr);
void hap_service_init(Hap::Service* svc, bt_gatt_attr* attr);

void hap_ble_start();
void hap_ble_stop();

void bleSessForEach(std::function<void(const Hap::Ble::Session* sess)> cb);
void bleSessPoll();

void hap_ble_disconnect(const Hap::Session* sess, uint32_t delay);
void hap_ble_disconnect(const Hap::Controller* ctlr, uint32_t delay);

// BLE connection state
bool hap_ble_is_connected();

// increment GSN
void hap_ble_gsn_update(bool each_update = false);

// BLE advertisement state
enum class AdvState : uint8_t
{
	None = 0,
	Regular,
	Notif,
	Update
};
AdvState hap_adv_state();

void hap_adv_start();
void hap_adv_stop();
void hap_adv_poll();
void hap_adv_update();
void hap_adv_broadcast(Hap::Characteristic* ch);

#endif /* _UHAP_Z_BLE_H_ */