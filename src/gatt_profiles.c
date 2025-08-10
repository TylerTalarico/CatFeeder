#include <string.h>
#include "nvs_flash.h"
#include "ble.h"
#include "esp_bt.h"
#include "esp_gap_ble_api.h"
#include "esp_gatts_api.h"
#include "esp_bt_defs.h"
#include "esp_bt_main.h"
#include "esp_gatt_common_api.h"

#define GATTS_TAG "GATTS_HANDLER"

typedef enum {
    PROFILE_DEFAULT,
    PROFILE_NUM
} gatt_profile_name_t;

static uint8_t main_services_uuids[NUM_SERVICES][ESP_UUID_LEN_128] = {
    {0x61,0x03,0xce,0xb5,0xbb,0x86,0x48,0x7f,0x9c,0x9a,0x28,0x56,0xec,0xc4,0x54,0xe1},
    {0x61,0x03,0xce,0xb5,0xbb,0x86,0x48,0x7f,0x9c,0x9a,0x28,0x56,0xec,0xc4,0x54,0xe2},
    {0x61,0x03,0xce,0xb5,0xbb,0x86,0x48,0x7f,0x9c,0x9a,0x28,0x56,0xec,0xc4,0x54,0xe3}
};

static void gatts_default_profile_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param);

static ble_gatt_profile_t gl_profile_tab[PROFILE_NUM] = {
    [PROFILE_DEFAULT] = {
        .gatts_cb = gatts_default_profile_event_handler,
        .gatts_if = ESP_GATT_IF_NONE,
    },
};

static esp_ble_adv_data_t adv_data = {
    .set_scan_rsp = false,
    .include_name = true,
    .include_txpower = true,
    .min_interval = 0x0006,
    .max_interval = 0x0010,
    .appearance = 0x00,
    .manufacturer_len = 0, //TEST_MANUFACTURER_DATA_LEN,
    .p_manufacturer_data =  NULL, //&test_manufacturer[0],
    .service_data_len = 0,
    .p_service_data = NULL,
    .service_uuid_len = NUM_SERVICES*ESP_UUID_LEN_128,
    .p_service_uuid = &main_services_uuids[0][0],
    .flag = (ESP_BLE_ADV_FLAG_GEN_DISC | ESP_BLE_ADV_FLAG_BREDR_NOT_SPT)
};

static esp_ble_adv_data_t scan_rsp_data = {
    .set_scan_rsp = true,
    .include_name = true,
    .include_txpower = true,
    //.min_interval = 0x0006,
    //.max_interval = 0x0010,
    .appearance = 0x00,
    .manufacturer_len = 0, //TEST_MANUFACTURER_DATA_LEN,
    .p_manufacturer_data =  NULL, //&test_manufacturer[0],
    .service_data_len = 0,
    .p_service_data = NULL,
    .service_uuid_len = NUM_SERVICES*ESP_UUID_LEN_128,
    .p_service_uuid = &main_services_uuids[0][0],
    .flag = (ESP_BLE_ADV_FLAG_GEN_DISC | ESP_BLE_ADV_FLAG_BREDR_NOT_SPT),
};

static esp_ble_adv_params_t adv_params = {
    .adv_int_min        = 0x20,
    .adv_int_max        = 0x40,
    .adv_type           = ADV_TYPE_IND,
    .own_addr_type      = BLE_ADDR_TYPE_PUBLIC,
    //.peer_addr            =
    //.peer_addr_type       =
    .channel_map        = ADV_CHNL_ALL,
    .adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
};



static void gatts_default_profile_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param) {
    switch (event) {
        case ESP_GATTS_REG_EVT:
        {
            for (uint8_t i = 0; i < NUM_SERVICES; i++) {
                gl_profile_tab[PROFILE_DEFAULT].services[i].service_id.is_primary = (i == 0) ? true : false;
                gl_profile_tab[PROFILE_DEFAULT].services[i].service_id.id.inst_id = 0x00;
                gl_profile_tab[PROFILE_DEFAULT].services[i].service_id.id.uuid.len = ESP_UUID_LEN_128;
                memcpy(
                    gl_profile_tab[PROFILE_DEFAULT].services[i].service_id.id.uuid.uuid.uuid128,
                    &main_services_uuids[i],
                    ESP_UUID_LEN_128*sizeof(uint8_t)
                );
            }
            esp_ble_gap_set_device_name(BLE_DEVICE_NAME);

            //config adv data
            esp_err_t ret = esp_ble_gap_config_adv_data(&adv_data);
            if (ret){
                ESP_LOGE(GATTS_TAG, "config adv data failed, error code = %x", ret);
            }
            adv_config_done |= adv_config_flag;
            //config scan response data
            ret = esp_ble_gap_config_adv_data(&scan_rsp_data);
        }

        default:
    }
}

