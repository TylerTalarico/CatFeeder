#include "esp_bt.h"
#include "esp_gap_ble_api.h"
#include "esp_gatts_api.h"
#include "esp_bt_defs.h"
#include "esp_bt_main.h"
#include "esp_gatt_common_api.h"
#include "sdkconfig.h"

#include "ble.h"

esp_err_t start_bt_controller(esp_bt_mode_t bt_mode);
esp_err_t gatt_profile_init();


esp_err_t ble_gatt_server_init(ble_gatt_profile_t * profiles, uint8_t num_profiles) {
    
    esp_err_t ret;
    
    if ((ret = start_bt_controller(ESP_BT_MODE_BLE)) != ESP_OK) {
        return ret;
    }

    for (uint8_t i = 0; i < num_profiles; i++) {
        if ((ret = esp_ble_gatts_app_register(profiles[i].app_id)) != ESP_OK) {
            return ret;
        }
    }

    

    



    

}

esp_err_t start_bt_controller(esp_bt_mode_t bt_mode) {
    esp_err_t ret;

    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
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

esp_err_t gatt_profile_init() {
    return ESP_OK;
}




