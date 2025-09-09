#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_bt.h"
#include "esp_gap_ble_api.h"
#include "esp_gatts_api.h"
#include "esp_bt_defs.h"
#include "esp_bt_main.h"
#include "esp_gatt_common_api.h"
#include "sdkconfig.h"
#include "nvs_flash.h"
#include "esp_log.h"

#include "ble.h"
#include "ble_gatt_server.h"

#define BLE_TAG "BLE"

esp_err_t start_bt_controller(esp_bt_mode_t bt_mode);

esp_err_t ble_init(ble_gatt_profile_req_t * profs, uint16_t num_profs) {
    
    esp_err_t ret;
    
    if ((ret = start_bt_controller(ESP_BT_MODE_BLE)) != ESP_OK) {
        return ret;
    }

    if ((ret = ble_gatts_profile_init(profs, num_profs)) != ESP_OK) {
        ESP_ERROR_CHECK(ret);
        return ret;
    }
    
    return ESP_OK;
}

esp_err_t start_bt_controller(esp_bt_mode_t bt_mode) {
    esp_err_t ret;

    ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT));

    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    ret = esp_bt_controller_init(&bt_cfg);
    if (ret) {
        ESP_LOGE(BLE_TAG, "%s enable controller failed: %s", __func__, esp_err_to_name(ret));
        return ret;
    }

    ret = esp_bt_controller_enable(ESP_BT_MODE_BLE);
    if (ret) {
        ESP_LOGE(BLE_TAG, "%s enable controller failed: %s", __func__, esp_err_to_name(ret));
        return ret;
    }

    esp_bluedroid_config_t cfg = BT_BLUEDROID_INIT_CONFIG_DEFAULT();
    ret = esp_bluedroid_init_with_cfg(&cfg);
    if (ret) {
        ESP_LOGE(BLE_TAG, "%s init bluetooth failed: %s", __func__, esp_err_to_name(ret));
        return ret;
    }

    ret = esp_bluedroid_enable();
    if (ret) {
        ESP_LOGE(BLE_TAG, "%s enable bluetooth failed: %s", __func__, esp_err_to_name(ret));
        return ret;
    }
    return ESP_OK;
}
