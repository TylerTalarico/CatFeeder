#include "esp_bt.h"
#include "esp_gap_ble_api.h"
#include "esp_gatts_api.h"
#include "esp_bt_defs.h"
#include "esp_bt_main.h"
#include "esp_gatt_common_api.h"
#include "sdkconfig.h"
#include "nvs_flash.h"

#include "ble.h"
#include "gatt_profiles.h"

esp_err_t start_bt_controller(esp_bt_mode_t bt_mode);

esp_err_t ble_init() {
    
    esp_err_t ret;
    
    if ((ret = start_bt_controller(ESP_BT_MODE_BLE)) != ESP_OK) {
        return ret;
    }

    if ((ret = ble_gatts_init()) != ESP_OK) {
        return ret;
    }
    
    return ESP_OK;
}

esp_err_t start_bt_controller(esp_bt_mode_t bt_mode) {
    esp_err_t ret;
    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();

    esp_bt_controller_mem_release(bt_mode);

    ret = esp_bt_controller_init(&bt_cfg);
    if (ret) {
        return ret;
    }

    ret = esp_bt_controller_enable(bt_mode);
    if (ret) {
        return ret;
    }

    ret = esp_bluedroid_init();
    if (ret) {
        return ret;
    }

    ret = esp_bluedroid_enable();
    if (ret) {
        return ret;
    }
    return ESP_OK;
}
