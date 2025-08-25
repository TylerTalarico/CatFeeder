#include <string.h>
#include "nvs_flash.h"
#include "ble.h"
#include "esp_bt.h"
#include "esp_gap_ble_api.h"
#include "esp_gatts_api.h"
#include "esp_bt_defs.h"
#include "esp_bt_main.h"
#include "esp_gatt_common_api.h"

#include "gatt_services.h"
#include "gatt_data.h"

#define GATTS_TAG "GATTS_HANDLER"
#define GATTS_TABLE_TAG "GATTS_HANDLER"

typedef enum {
    PROFILE_DEFAULT,
    PROFILE_NUM
} gatt_profile_name_t;

static uint8_t adv_config_done = 0;
#define ADV_CONFIG_FLAG      (1 << 0)
#define SCAN_RSP_CONFIG_FLAG (1 << 1)

#define CHAR_DECLARATION_SIZE (sizeof(uint8_t))


static void gatts_default_profile_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param);

static ble_gatt_profile_t gl_profile_tab[PROFILE_NUM] = {
    [PROFILE_DEFAULT] = {
        .gatts_cb = gatts_default_profile_event_handler,
        .gatts_if = ESP_GATT_IF_NONE,
    },
};

static uint16_t schedule_handle_table[NUM_ATTR];

static const uint16_t GATTS_SERVICE_SCHEDULE_UUID  = 0x00FF;
static const uint16_t GATTS_CHAR_PORTIONS_UUID              = 0xFF01;
static const uint16_t GATTS_CHAR_MEASUREMENT_UUID       = 0x2A9D;
static const uint16_t GATTS_CHAR_EVENTS_UUID       = 0xFF03;

static const uint8_t primary_service_uuid[ESP_UUID_LEN_128] = { 0x61,0x03,0xce,0xb5,0xbb,0x86,0x48,0x7f,0x9c,0x9a,0x28,0x56,0xec,0xc4,0x54,0xe1 };
static const uint16_t char_declaration_uuid   = ESP_GATT_UUID_CHAR_DECLARE;
static const uint16_t char_client_config_uuid = ESP_GATT_UUID_CHAR_CLIENT_CONFIG;
static const uint8_t char_prop_read                =  ESP_GATT_CHAR_PROP_BIT_READ;
static const uint8_t char_prop_write               = ESP_GATT_CHAR_PROP_BIT_WRITE;
static const uint8_t char_prop_read_write_notify   = ESP_GATT_CHAR_PROP_BIT_WRITE | ESP_GATT_CHAR_PROP_BIT_READ | ESP_GATT_CHAR_PROP_BIT_NOTIFY;
static const uint8_t bowl_weight_ccc[2]      = {0x00, 0x00};


static const esp_gatts_attr_db_t gatt_db[NUM_ATTR] = {
    [IDX_SVC_SCHEDULE] = {
        {ESP_GATT_AUTO_RSP},
        {ESP_UUID_LEN_128, (uint8_t *)&primary_service_uuid, ESP_GATT_PERM_READ,
        sizeof(uint16_t), sizeof(GATTS_SERVICE_SCHEDULE_UUID), (uint8_t *)&GATTS_SERVICE_SCHEDULE_UUID}
    },


    [IDX_CHAR_PORTIONS] = {
        {ESP_GATT_AUTO_RSP},
        {ESP_UUID_LEN_16, (uint8_t *)&char_declaration_uuid, ESP_GATT_PERM_READ,
        CHAR_DECLARATION_SIZE, sizeof(char_prop_read_write_notify), (uint8_t *)&char_prop_read_write_notify}
    },

    [IDX_CHAR_PORTIONS_VAL] = {
        {ESP_GATT_RSP_BY_APP},
        {ESP_UUID_LEN_16, (uint8_t *)&GATTS_CHAR_PORTIONS_UUID, ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
        sizeof(gattd_dispense_amounts), sizeof(gattd_dispense_amounts), (uint8_t *)&gattd_dispense_amounts}
    },


    [IDX_CHAR_MEASUREMENT] = {
        {ESP_GATT_AUTO_RSP},
        {ESP_UUID_LEN_16, (uint8_t *)&char_declaration_uuid, ESP_GATT_PERM_READ,
        CHAR_DECLARATION_SIZE, sizeof(char_prop_read), (uint8_t *)&char_prop_read}
    },

    [IDX_CHAR_MEASUREMENT_VAL] = {
        {ESP_GATT_AUTO_RSP},
        {ESP_UUID_LEN_16, (uint8_t *)&GATTS_CHAR_MEASUREMENT_UUID, ESP_GATT_PERM_READ,
        sizeof(gattd_bowl_weight), sizeof(gattd_bowl_weight), (uint8_t *)&gattd_bowl_weight}
    },

    [IDX_CHAR_MEASUREMENT_CFG]  = {
        {ESP_GATT_AUTO_RSP}, 
        {ESP_UUID_LEN_16, (uint8_t *)&char_client_config_uuid, ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
        sizeof(uint16_t), sizeof(bowl_weight_ccc), (uint8_t *)bowl_weight_ccc}
    },

    
    [IDX_CHAR_EVENTS] = {
        {ESP_GATT_AUTO_RSP},
        {ESP_UUID_LEN_16, (uint8_t *)&char_declaration_uuid, ESP_GATT_PERM_READ,
        CHAR_DECLARATION_SIZE, sizeof(char_prop_read_write_notify), (uint8_t *)&char_prop_read_write_notify}
    },

    [IDX_CHAR_EVENTS_VAL] = {
        {ESP_GATT_AUTO_RSP},
        {ESP_UUID_LEN_16, (uint8_t *)&GATTS_CHAR_EVENTS_UUID, ESP_GATT_PERM_READ,
        sizeof(gattd_events), sizeof(gattd_events), (uint8_t *)&gattd_events}
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
    .p_service_uuid = &primary_service_uuid[0],
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
    .service_uuid_len = sizeof(primary_service_uuid),
    .p_service_uuid = &primary_service_uuid[0],
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
            esp_ble_gap_set_device_name(BLE_DEVICE_NAME);

            //config adv data
            esp_err_t ret = esp_ble_gap_config_adv_data(&adv_data);
            if (ret){
                ESP_LOGE(GATTS_TAG, "config adv data failed, error code = %x", ret);
            }
            adv_config_done |= ADV_CONFIG_FLAG;
            //config scan response data
            ret = esp_ble_gap_config_adv_data(&scan_rsp_data);
            if (ret){
                ESP_LOGE(GATTS_TAG, "config scan response data failed, error code = %x", ret);
            }
            adv_config_done |= SCAN_RSP_CONFIG_FLAG;

            ret = esp_ble_gatts_create_attr_tab(gatt_db, gatts_if, NUM_ATTR, 0);
            
            break;

        }

        case ESP_GATTS_CREAT_ATTR_TAB_EVT:
        {
            if (param->add_attr_tab.status != ESP_GATT_OK){
                ESP_LOGE(GATTS_TAG, "create attribute table failed, error code=0x%x", param->add_attr_tab.status);
            }
            else if (param->add_attr_tab.num_handle != NUM_ATTR){
                ESP_LOGE(GATTS_TAG, "create attribute table abnormally, num_handle (%d) \
                        doesn't equal to HRS_IDX_NB(%d)", param->add_attr_tab.num_handle, NUM_ATTR);
            }
            else {
                ESP_LOGI(GATTS_TAG, "create attribute table successfully, the number handle = %d\n",param->add_attr_tab.num_handle);
                memcpy(schedule_handle_table, param->add_attr_tab.handles, sizeof(schedule_handle_table));
                esp_ble_gatts_start_service(schedule_handle_table[IDX_SVC_SCHEDULE]);
            }
            break;
        }

        case ESP_GATTS_READ_EVT: 
        {
            ESP_LOGI(GATTS_TAG, "GATT_READ_EVT, conn_id %d, trans_id %d, handle %d\n", param->read.conn_id, param->read.trans_id, param->read.handle);
            esp_gatt_rsp_t rsp;
            memset(&rsp, 0, sizeof(esp_gatt_rsp_t));
            


            break;
        }

        default:
    }
}

static void gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param)
{
    switch (event) {
        case ESP_GAP_BLE_ADV_DATA_SET_COMPLETE_EVT:
            adv_config_done &= (~ADV_CONFIG_FLAG);
            if (adv_config_done == 0){
                esp_ble_gap_start_advertising(&adv_params);
            }
            break;
        case ESP_GAP_BLE_SCAN_RSP_DATA_SET_COMPLETE_EVT:
            adv_config_done &= (~SCAN_RSP_CONFIG_FLAG);
            if (adv_config_done == 0){
                esp_ble_gap_start_advertising(&adv_params);
            }
            break;
        case ESP_GAP_BLE_ADV_START_COMPLETE_EVT:
            /* advertising start complete event to indicate advertising start successfully or failed */
            if (param->adv_start_cmpl.status != ESP_BT_STATUS_SUCCESS) {
                ESP_LOGE(GATTS_TABLE_TAG, "advertising start failed");
            }else{
                ESP_LOGI(GATTS_TABLE_TAG, "advertising start successfully");
            }
            break;
        case ESP_GAP_BLE_ADV_STOP_COMPLETE_EVT:
            if (param->adv_stop_cmpl.status != ESP_BT_STATUS_SUCCESS) {
                ESP_LOGE(GATTS_TABLE_TAG, "Advertising stop failed");
            }
            else {
                ESP_LOGI(GATTS_TABLE_TAG, "Stop adv successfully");
            }
            break;
        case ESP_GAP_BLE_UPDATE_CONN_PARAMS_EVT:
            ESP_LOGI(GATTS_TABLE_TAG, "update connection params status = %d, conn_int = %d, latency = %d, timeout = %d",
                  param->update_conn_params.status,
                  param->update_conn_params.conn_int,
                  param->update_conn_params.latency,
                  param->update_conn_params.timeout);
            break;
        default:
            break;
    }
}

static void gatts_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param)
{

    /* If event is register event, store the gatts_if for each profile */
    if (event == ESP_GATTS_REG_EVT) {
        if (param->reg.status == ESP_GATT_OK) {
            gl_profile_tab[param->reg.app_id].gatts_if = gatts_if;
        } else {
            ESP_LOGE(GATTS_TAG, "reg app failed, app_id %04x, status %d",
                    param->reg.app_id,
                    param->reg.status);
            return;
        }
    }
    do {
        int idx;
        for (idx = 0; idx < PROFILE_NUM; idx++) {
            /* ESP_GATT_IF_NONE, not specify a certain gatt_if, need to call every profile cb function */
            if (gatts_if == ESP_GATT_IF_NONE || gatts_if == gl_profile_tab[idx].gatts_if) {
                if (gl_profile_tab[idx].gatts_cb) {
                    gl_profile_tab[idx].gatts_cb(event, gatts_if, param);
                }
            }
        }
    } while (0);
}

