#ifndef _UHAP_Z_APP_H_
#define _UHAP_Z_APP_H_

//#include "lib/libc/minimal/include/stdlib.h"
#include <zephyr.h>
#include <device.h>
#include <drivers/gpio.h>

#include <settings/settings.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/conn.h>
#include <bluetooth/uuid.h>
#include <bluetooth/gatt.h>
#include <bluetooth/services/bas.h>
#include <bluetooth/services/hrs.h>

#include "HapBle.h"
#include "Ble.h"
#include "HapApple.h"

void dump(Hap::Service* svc);
void dump_gatt(bt_gatt_attr* attr, uint32_t cnt);

#endif /* _UHAP_Z_APP_H_ */