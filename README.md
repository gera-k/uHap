# uHap - Homekit Accessory Protocol server, BLE port

The HAP server implements HomeKit Accessory Protocol for BLE Accessories as defined by the HomeKit Accessory Protocol Specification (Non-Commertial version) Release R2.

Source code structure:
* __Hap__ - core module
* __Crypto__ - Cryptography
* __Crypto/Test__ - Cryptography test
* __Util__ - various utilities
* __zephyr__ - Zephyr RTOS platform support and test app

## Building for Zephyr

The test app supports [NRF52840 dongle](https://docs.zephyrproject.org/latest/boards/arm/nrf52840dongle_nrf52840/doc/index.html) and implements Lightbulb service using three-color LED as a lightbulb.

* Install [Zephyr build tools](https://docs.zephyrproject.org/latest/getting_started/index.html) and configure environment.
* Clone this repo into directory of your choice.
* Build the test app:
```
  > cd uhap\zephyr
  > west build -b nrf52840dongle_nrf52840 test
```
* Flash the app to the dongle using your favorite way. For example, to flash through J-Link debugger, do:
```
  > west flash --snr <serial num of your J-LINK tool> 
```

### Zephyr shell

* UART0 configuration is following:
  * TX - P0.10
  * RX - P0.09
* shell commands:
  * __hap config__  Print HAP configuration.
  * __hap status__  Print HAP status.
  * __hap reset [-m]__  (Manufacturing) Reset and reboot.
  * __lb status__ Print LightBulb status.
  * __lb on__ Turn LightBulb on.
  * __lb off__  Turn LightBulb off.
  * __lb pwm r g b__  Set LightBulb pwm (r, g, b: 0..255).

### Zephyr updates
* To build standalone app (to use with external debugger), edit nrf52840dongle_nrf52840.dts:
```
diff --git a/boards/arm/nrf52840dongle_nrf52840/nrf52840dongle_nrf52840.dts b/boards/arm/nrf52840dongle_nrf52840/nrf52840dongle_nrf52840.dts
index 52d74a7392..2d33fa27b6 100644
--- a/boards/arm/nrf52840dongle_nrf52840/nrf52840dongle_nrf52840.dts
+++ b/boards/arm/nrf52840dongle_nrf52840/nrf52840dongle_nrf52840.dts
@@ -162,7 +162,8 @@
  * fstab-stock         -compatible with Nordic nRF5 bootloader, default
  * fstab-debugger      -to use an external debugger, w/o the nRF5 bootloader
  */
-#include "fstab-stock.dts"
+/*#include "fstab-stock.dts"*/
+#include "fstab-debugger.dts"

```
* Comment out couple of deprecated functions that fail to compile with C++ compiler:
```
diff --git a/include/bluetooth/conn.h b/include/bluetooth/conn.h
index c3c166c83c..eb207bb5cb 100644
--- a/include/bluetooth/conn.h
+++ b/include/bluetooth/conn.h
@@ -392,7 +392,7 @@ int bt_conn_le_create(const bt_addr_le_t *peer,
                      const struct bt_conn_le_create_param *create_param,
                      const struct bt_le_conn_param *conn_param,
                      struct bt_conn **conn);
-
+#if 0
 __deprecated static inline
 struct bt_conn *bt_conn_create_le(const bt_addr_le_t *peer,
                                  const struct bt_le_conn_param *conn_param)
@@ -406,7 +406,7 @@ struct bt_conn *bt_conn_create_le(const bt_addr_le_t *peer,

        return conn;
 }
-
+#endif
 /** @brief Automatically connect to remote devices in whitelist.
  *
  *  This uses the Auto Connection Establishment procedure.
@@ -424,13 +424,13 @@ struct bt_conn *bt_conn_create_le(const bt_addr_le_t *peer,
  */
 int bt_conn_le_create_auto(const struct bt_conn_le_create_param *create_param,
                           const struct bt_le_conn_param *conn_param);
-
+#if 0
 __deprecated static inline
 int bt_conn_create_auto_le(const struct bt_le_conn_param *conn_param)
 {
        return bt_conn_le_create_auto(BT_CONN_LE_CREATE_CONN_AUTO, conn_param);
 }
-
+#endif
 /** @brief Stop automatic connect creation.
  *
  *  @return Zero on success or (negative) error code on failure.
```
