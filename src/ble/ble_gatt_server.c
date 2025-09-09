/*
 * SPDX-FileCopyrightText: 2021-2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */

/****************************************************************************
*
* This demo showcases creating a GATT database using a predefined attribute table.
* It acts as a GATT server and can send adv data, be connected by client.
* Run the gatt_client demo, the client demo will automatically connect to the gatt_server_service_table demo.
* Client demo will enable GATT server's notify after connection. The two devices will then exchange
* data.
*
****************************************************************************/


#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_bt.h"

#include "esp_gap_ble_api.h"
#include "esp_gatts_api.h"
#include "esp_bt_main.h"
#include "esp_bt_device.h"
#include "gatts_table_creat_demo.h"
#include "esp_gatt_common_api.h"

#include "ble_gatt_profiles.h"
#include "ble_gatt_server.h"
#include "gatt_data.h"

#define GATTS_TABLE_TAG "GATTS_TABLE_DEMO"

#define PROFILE_NUM                 1
#define PROFILE_APP_IDX             0
#define ESP_APP_ID                  0x55
#define SAMPLE_DEVICE_NAME          "Pawto Feeder"
#define SVC_INST_ID                 0

/* The max length of characteristic value. When the GATT client performs a write or prepare write operation,
*  the data length must be less than GATTS_DEMO_CHAR_VAL_LEN_MAX.
*/
#define GATTS_DEMO_CHAR_VAL_LEN_MAX 500
#define PREPARE_BUF_MAX_SIZE        1024
#define CHAR_DECLARATION_SIZE       (sizeof(uint8_t))

#define ADV_CONFIG_FLAG             (1 << 0)
#define SCAN_RSP_CONFIG_FLAG        (1 << 1)

static uint8_t adv_config_done       = 0;

typedef struct {
    uint8_t                 *prepare_buf;
    int                     prepare_len;
} prepare_type_env_t;

static prepare_type_env_t prepare_write_env;


#define MAX_PROFILES 3
#define MAX_SERVICES 4
#define MAX_ATTR 20

typedef struct {
    uint8_t *data;
    uint16_t len;
    esp_bt_uuid_t uuid;
    uint16_t attr_handle;
    uint16_t ccc_handle;
    uint16_t ccc;
    esp_gatt_perm_t perms;
    esp_gatt_char_prop_t props;

} gatts_char_inst;

typedef struct {
    uint16_t handle;
    esp_gatt_srvc_id_t uuid;
    gatts_char_inst * chars;
    uint16_t num_chars;
    uint16_t attr_idx;
} gatts_svc_inst;

typedef struct {
    esp_gatts_cb_t gatts_cb;
    uint16_t gatts_if;
    gatts_svc_inst *svcs;
    uint16_t num_svcs;
    uint16_t conn_id;
} gatts_profile_inst;

static esp_gatts_attr_db_t * prof_init = NULL;
static uint16_t * num_handles = NULL;

static gatts_profile_inst profile_tab[MAX_PROFILES] = {0};
static uint16_t num_profiles = 0;
static uint16_t num_services_initialized = 0;


//#define CONFIG_SET_RAW_ADV_DATA
#ifdef CONFIG_SET_RAW_ADV_DATA
static uint8_t raw_adv_data[] = {
    /* Flags */
    0x02, ESP_BLE_AD_TYPE_FLAG, 0x06,
    /* TX Power Level */
    0x02, ESP_BLE_AD_TYPE_TX_PWR, 0xEB,
    /* Complete 16-bit Service UUIDs */
    0x03, ESP_BLE_AD_TYPE_16SRV_CMPL, 0xFF, 0x00,
    /* Complete Local Name */
    0x0F, ESP_BLE_AD_TYPE_NAME_CMPL,
    'E', 'S', 'P', '_', 'G', 'A', 'T', 'T', 'S', '_', 'D', 'E', 'M', 'O'
};

static uint8_t raw_scan_rsp_data[] = {
    /* Flags */
    0x02, ESP_BLE_AD_TYPE_FLAG, 0x06,
    /* TX Power Level */
    0x02, ESP_BLE_AD_TYPE_TX_PWR, 0xEB,
    /* Complete 16-bit Service UUIDs */
    0x03, ESP_BLE_AD_TYPE_16SRV_CMPL, 0xFF, 0x00
};

#else
static uint8_t service_uuid[] = {
    /* LSB <--------------------------------------------------------------------------------> MSB */
    //first uuid, 16bit, [12],[13] is the value
    0x40, 0x47, 0x6c, 0xd0, 0x89, 0x95, 0x4d, 0x38, 0xb0, 0xbf, 0x80, 0x87, 0x67, 0xb7, 0xb7, 0x0f
};

/* The length of adv data must be less than 31 bytes */
static esp_ble_adv_data_t adv_data = {
    .set_scan_rsp        = false,
    .include_name        = true,
    .include_txpower     = true,
    .min_interval        = 0x0006, //slave connection min interval, Time = min_interval * 1.25 msec
    .max_interval        = 0x0010, //slave connection max interval, Time = max_interval * 1.25 msec
    .appearance          = 0x00,
    .manufacturer_len    = 0,    //TEST_MANUFACTURER_DATA_LEN,
    .p_manufacturer_data = NULL, //test_manufacturer,
    .service_data_len    = 0,
    .p_service_data      = NULL,
    .service_uuid_len    = sizeof(service_uuid),
    .p_service_uuid      = service_uuid,
    .flag = (ESP_BLE_ADV_FLAG_GEN_DISC | ESP_BLE_ADV_FLAG_BREDR_NOT_SPT),
};

// scan response data
static esp_ble_adv_data_t scan_rsp_data = {
    .set_scan_rsp        = true,
    .include_name        = true,
    .include_txpower     = true,
    .min_interval        = 0x0006,
    .max_interval        = 0x0010,
    .appearance          = 0x00,
    .manufacturer_len    = 0, //TEST_MANUFACTURER_DATA_LEN,
    .p_manufacturer_data = NULL, //&test_manufacturer[0],
    .service_data_len    = 0,
    .p_service_data      = NULL,
    .service_uuid_len    = sizeof(service_uuid),
    .p_service_uuid      = service_uuid,
    .flag = (ESP_BLE_ADV_FLAG_GEN_DISC | ESP_BLE_ADV_FLAG_BREDR_NOT_SPT),
};
#endif /* CONFIG_SET_RAW_ADV_DATA */

static esp_ble_adv_params_t adv_params = {
    .adv_int_min         = 0x20,
    .adv_int_max         = 0x40,
    .adv_type            = ADV_TYPE_IND,
    .own_addr_type       = BLE_ADDR_TYPE_PUBLIC,
    .channel_map         = ADV_CHNL_ALL,
    .adv_filter_policy   = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
};


static void gatts_default_event_handler(uint16_t app_id, esp_gatts_cb_event_t event,
					esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param);


static const uint16_t primary_service_uuid         = ESP_GATT_UUID_PRI_SERVICE;
static const uint16_t secondary_service_uuid       = ESP_GATT_UUID_SEC_SERVICE;
static const uint16_t character_declaration_uuid   = ESP_GATT_UUID_CHAR_DECLARE;
static const uint16_t character_client_config_uuid = ESP_GATT_UUID_CHAR_CLIENT_CONFIG;
// static const uint8_t char_prop_read                =  ESP_GATT_CHAR_PROP_BIT_READ;
// static const uint8_t char_prop_write               = ESP_GATT_CHAR_PROP_BIT_WRITE;
// static const uint8_t char_prop_read_write_notify   = ESP_GATT_CHAR_PROP_BIT_WRITE | ESP_GATT_CHAR_PROP_BIT_READ | ESP_GATT_CHAR_PROP_BIT_NOTIFY;


static void gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param)
{
    
    switch (event) {
    #ifdef CONFIG_SET_RAW_ADV_DATA
        case ESP_GAP_BLE_ADV_DATA_RAW_SET_COMPLETE_EVT:
            adv_config_done &= (~ADV_CONFIG_FLAG);
            if (adv_config_done == 0){
                esp_ble_gap_start_advertising(&adv_params);
            }
            break;
        case ESP_GAP_BLE_SCAN_RSP_DATA_RAW_SET_COMPLETE_EVT:
            adv_config_done &= (~SCAN_RSP_CONFIG_FLAG);
            if (adv_config_done == 0){
                esp_ble_gap_start_advertising(&adv_params);
            }
            break;
    #else
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
    #endif
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

void prepare_write_event_env(esp_gatt_if_t gatts_if, prepare_type_env_t *prepare_write_env, esp_ble_gatts_cb_param_t *param)
{
    ESP_LOGI(GATTS_TABLE_TAG, "prepare write, handle = %d, value len = %d", param->write.handle, param->write.len);
    esp_gatt_status_t status = ESP_GATT_OK;
    if (param->write.offset > PREPARE_BUF_MAX_SIZE) {
        status = ESP_GATT_INVALID_OFFSET;
    } else if ((param->write.offset + param->write.len) > PREPARE_BUF_MAX_SIZE) {
        status = ESP_GATT_INVALID_ATTR_LEN;
    }
    if (status == ESP_GATT_OK && prepare_write_env->prepare_buf == NULL) {
        prepare_write_env->prepare_buf = (uint8_t *)malloc(PREPARE_BUF_MAX_SIZE * sizeof(uint8_t));
        prepare_write_env->prepare_len = 0;
        if (prepare_write_env->prepare_buf == NULL) {
            ESP_LOGE(GATTS_TABLE_TAG, "%s, Gatt_server prep no mem", __func__);
            status = ESP_GATT_NO_RESOURCES;
        }
    }

    /*send response when param->write.need_rsp is true */
    if (param->write.need_rsp){
        esp_gatt_rsp_t *gatt_rsp = (esp_gatt_rsp_t *)malloc(sizeof(esp_gatt_rsp_t));
        if (gatt_rsp != NULL){
            gatt_rsp->attr_value.len = param->write.len;
            gatt_rsp->attr_value.handle = param->write.handle;
            gatt_rsp->attr_value.offset = param->write.offset;
            gatt_rsp->attr_value.auth_req = ESP_GATT_AUTH_REQ_NONE;
            memcpy(gatt_rsp->attr_value.value, param->write.value, param->write.len);
            esp_err_t response_err = esp_ble_gatts_send_response(gatts_if, param->write.conn_id, param->write.trans_id, status, gatt_rsp);
            if (response_err != ESP_OK) {
               ESP_LOGE(GATTS_TABLE_TAG, "Send response error");
            }
            free(gatt_rsp);
        }else{
            ESP_LOGE(GATTS_TABLE_TAG, "%s, malloc failed", __func__);
            status = ESP_GATT_NO_RESOURCES;
        }
    }
    if (status != ESP_GATT_OK){
        return;
    }
    memcpy(prepare_write_env->prepare_buf + param->write.offset,
           param->write.value,
           param->write.len);
    prepare_write_env->prepare_len += param->write.len;

}

void exec_write_event_env(prepare_type_env_t *prepare_write_env, esp_ble_gatts_cb_param_t *param){
    if (param->exec_write.exec_write_flag == ESP_GATT_PREP_WRITE_EXEC && prepare_write_env->prepare_buf){
        ESP_LOG_BUFFER_HEX(GATTS_TABLE_TAG, prepare_write_env->prepare_buf, prepare_write_env->prepare_len);
    }else{
        ESP_LOGI(GATTS_TABLE_TAG,"ESP_GATT_PREP_WRITE_CANCEL");
    }
    if (prepare_write_env->prepare_buf) {
        free(prepare_write_env->prepare_buf);
        prepare_write_env->prepare_buf = NULL;
    }
    prepare_write_env->prepare_len = 0;
}

static void gatts_default_event_handler(uint16_t app_id, esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param)
{
    gatts_profile_inst * prof = &profile_tab[app_id];
    switch (event) {
        case ESP_GATTS_REG_EVT:{

            esp_err_t set_dev_name_ret = esp_ble_gap_set_device_name(SAMPLE_DEVICE_NAME);
            if (set_dev_name_ret){
                ESP_LOGE(GATTS_TABLE_TAG, "set device name failed, error code = %x", set_dev_name_ret);
            }

            prof->gatts_if = gatts_if;

    #ifdef CONFIG_SET_RAW_ADV_DATA
            esp_err_t raw_adv_ret = esp_ble_gap_config_adv_data_raw(raw_adv_data, sizeof(raw_adv_data));
            if (raw_adv_ret){
                ESP_LOGE(GATTS_TABLE_TAG, "config raw adv data failed, error code = %x ", raw_adv_ret);
            }
            adv_config_done |= ADV_CONFIG_FLAG;
            esp_err_t raw_scan_ret = esp_ble_gap_config_scan_rsp_data_raw(raw_scan_rsp_data, sizeof(raw_scan_rsp_data));
            if (raw_scan_ret){
                ESP_LOGE(GATTS_TABLE_TAG, "config raw scan rsp data failed, error code = %x", raw_scan_ret);
            }
            adv_config_done |= SCAN_RSP_CONFIG_FLAG;
    #else
            //config adv data
            esp_err_t ret = esp_ble_gap_config_adv_data(&adv_data);
            if (ret){
                ESP_LOGE(GATTS_TABLE_TAG, "config adv data failed, error code = %x", ret);
            }
            adv_config_done |= ADV_CONFIG_FLAG;
            //config scan response data
            ret = esp_ble_gap_config_adv_data(&scan_rsp_data);
            if (ret){
                ESP_LOGE(GATTS_TABLE_TAG, "config scan response data failed, error code = %x", ret);
            }
            adv_config_done |= SCAN_RSP_CONFIG_FLAG;
    #endif

            uint16_t handle_cnt = 0;
            for (uint8_t i = 0; i < prof->num_svcs; i++)
            {   
                ret = esp_ble_gatts_create_attr_tab(&prof_init[handle_cnt], gatts_if, num_handles[i], 0);
                handle_cnt += num_handles[i];
            }
        }
       	    break;

        case ESP_GATTS_CREATE_EVT:
            break;
        case ESP_GATTS_ADD_CHAR_EVT:
            break;
        case ESP_GATTS_ADD_CHAR_DESCR_EVT:
            break;
        case ESP_GATTS_READ_EVT:
            ESP_LOGI(GATTS_TABLE_TAG, "ESP_GATTS_READ_EVT");
       	    break;
        case ESP_GATTS_WRITE_EVT:
            if (!param->write.is_prep){
                // the data length of gattc write  must be less than GATTS_DEMO_CHAR_VAL_LEN_MAX.
                ESP_LOGI(GATTS_TABLE_TAG, "GATT_WRITE_EVT, handle = %d, value len = %d, value :", param->write.handle, param->write.len);
                ESP_LOG_BUFFER_HEX(GATTS_TABLE_TAG, param->write.value, param->write.len);

                bool found = false;
                int svc_idx = 0;
                int car_idx = 0;
                for (int i = 0; i < prof->num_svcs; i++)
                {
                    for (int j = 0; j < prof->svcs[i].num_chars; j++)
                    {
                        if (prof->svcs[i].chars[j].attr_handle == param->write.handle || prof->svcs[i].chars[j].ccc_handle == param->write.handle)
                        {
                            svc_idx = i;
                            car_idx = j;
                            found = true;
                            break;
                        }
                        if (found) break;
                    }
                    if (found) break;
                }
                if (!found) break;

                if (prof->svcs[svc_idx].chars[car_idx].ccc_handle == param->write.handle){
                    if (param->write.len != 2)
                    {
                        if (param->write.need_rsp){
                            esp_ble_gatts_send_response(gatts_if, param->write.conn_id, param->write.trans_id, ESP_GATT_INVALID_ATTR_LEN, NULL);
                            break;
                        }
                    }
                    uint16_t descr_value = param->write.value[1]<<8 | param->write.value[0];
                    if (descr_value > 0x0002){
                        if (param->write.need_rsp){
                            esp_ble_gatts_send_response(gatts_if, param->write.conn_id, param->write.trans_id, ESP_GATT_OUT_OF_RANGE, NULL);
                            break;
                        }
                    }
                    prof->svcs[svc_idx].chars[car_idx].ccc = descr_value;

                } else {
                    if (param->write.len > prof->svcs[svc_idx].chars[car_idx].len) {
                        esp_ble_gatts_send_response(gatts_if, param->write.conn_id, param->write.trans_id, ESP_GATT_INVALID_ATTR_LEN, NULL);
                        break;
                    }
                }

                memcpy(prof->svcs[svc_idx].chars[car_idx].data, param->write.value, param->write.len);
                ESP_LOGI(GATTS_TABLE_TAG, "data stored");
                /* send response when param->write.need_rsp is true*/
                if (param->write.need_rsp){
                    esp_ble_gatts_send_response(gatts_if, param->write.conn_id, param->write.trans_id, ESP_GATT_OK, NULL);
                }

            }else{
                /* handle prepare write */
                prepare_write_event_env(gatts_if, &prepare_write_env, param);
            }
      	    break;
        case ESP_GATTS_EXEC_WRITE_EVT:
            // the length of gattc prepare write data must be less than GATTS_DEMO_CHAR_VAL_LEN_MAX.
            ESP_LOGI(GATTS_TABLE_TAG, "ESP_GATTS_EXEC_WRITE_EVT");
            exec_write_event_env(&prepare_write_env, param);
            break;
        case ESP_GATTS_MTU_EVT:
            ESP_LOGI(GATTS_TABLE_TAG, "ESP_GATTS_MTU_EVT, MTU %d", param->mtu.mtu);
            break;
        case ESP_GATTS_CONF_EVT:
            ESP_LOGI(GATTS_TABLE_TAG, "ESP_GATTS_CONF_EVT, status = %d, attr_handle %d", param->conf.status, param->conf.handle);
            break;
        case ESP_GATTS_START_EVT:
            ESP_LOGI(GATTS_TABLE_TAG, "SERVICE_START_EVT, status %d, service_handle %d", param->start.status, param->start.service_handle);
            break;

        case ESP_GATTS_CONNECT_EVT:
            ESP_LOGI(GATTS_TABLE_TAG, "ESP_GATTS_CONNECT_EVT, conn_id = %d", param->connect.conn_id);
            prof->conn_id = param->connect.conn_id;
            ESP_LOG_BUFFER_HEX(GATTS_TABLE_TAG, param->connect.remote_bda, 6);
            esp_ble_conn_update_params_t conn_params = {0};
            memcpy(conn_params.bda, param->connect.remote_bda, sizeof(esp_bd_addr_t));
            /* For the iOS system, please refer to Apple official documents about the BLE connection parameters restrictions. */
            conn_params.latency = 0;
            conn_params.max_int = 0x20;    // max_int = 0x20*1.25ms = 40ms
            conn_params.min_int = 0x10;    // min_int = 0x10*1.25ms = 20ms
            conn_params.timeout = 400;    // timeout = 400*10ms = 4000ms
            //start sent the update connection parameters to the peer device.
            esp_ble_gap_update_conn_params(&conn_params);
            break;

        case ESP_GATTS_DISCONNECT_EVT:
            ESP_LOGI(GATTS_TABLE_TAG, "ESP_GATTS_DISCONNECT_EVT, reason = 0x%x", param->disconnect.reason);
            esp_ble_gap_start_advertising(&adv_params);
            break;
        case ESP_GATTS_SET_ATTR_VAL_EVT:
            break;
        case ESP_GATTS_CREAT_ATTR_TAB_EVT:
         {
            uint16_t handle_count = 0;
            if (param->add_attr_tab.status != ESP_GATT_OK){
                ESP_LOGE(GATTS_TABLE_TAG, "create attribute table failed, error code=0x%x", param->add_attr_tab.status);
            } else {
                ESP_LOGI(GATTS_TABLE_TAG, "create attribute table successfully, the number handle = %d\n",param->add_attr_tab.num_handle);
                for (uint16_t i = 0; i < prof->num_svcs; i++) {
                    if (memcmp(prof->svcs[i].uuid.id.uuid.uuid.uuid128, param->add_attr_tab.svc_uuid.uuid.uuid128, ESP_UUID_LEN_128))
                        continue;
                    prof->svcs[i].handle = param->add_attr_tab.handles[handle_count];
                    handle_count++;
                    for (uint16_t j = 0; j < prof->svcs[i].num_chars; j++) {
                        prof->svcs[i].chars[j].attr_handle = param->add_attr_tab.handles[handle_count+1];
                        handle_count += 2;
                        if (prof->svcs[i].chars[j].props & ESP_GATT_CHAR_PROP_BIT_NOTIFY) {
                            prof->svcs[i].chars[j].ccc_handle = param->add_attr_tab.handles[handle_count];
                            handle_count++;
                        }
                    }
                    num_services_initialized++;
                    esp_ble_gatts_start_service(prof->svcs[i].handle);
                    break;
                }
                
            }
            break;
        }
        case ESP_GATTS_STOP_EVT:
        case ESP_GATTS_OPEN_EVT:
        case ESP_GATTS_CANCEL_OPEN_EVT:
        case ESP_GATTS_CLOSE_EVT:
        case ESP_GATTS_LISTEN_EVT:
        case ESP_GATTS_CONGEST_EVT:
        case ESP_GATTS_UNREG_EVT:
        case ESP_GATTS_DELETE_EVT:
        default:
            break;
    }
}


static void gatts_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param)
{

    /* If event is register event, store the gatts_if for each profile */
    if (event == ESP_GATTS_REG_EVT) {
        if (param->reg.status == ESP_GATT_OK) {
            profile_tab[param->reg.app_id].gatts_if = gatts_if;
        } else {
            ESP_LOGE(GATTS_TABLE_TAG, "reg app failed, app_id %04x, status %d",
                    param->reg.app_id,
                    param->reg.status);
            return;
        }
    }
    do {
        uint16_t idx;
        for (idx = 0; idx < num_profiles; idx++) {
            /* ESP_GATT_IF_NONE, not specify a certain gatt_if, need to call every profile cb function */
            if (gatts_if == ESP_GATT_IF_NONE || gatts_if == profile_tab[idx].gatts_if) {
                if (profile_tab[idx].gatts_cb) {
                    profile_tab[idx].gatts_cb(event, gatts_if, param);
                } else {
                    gatts_default_event_handler(idx, event, gatts_if, param);
                }
            }
        }
    } while (0);
}

esp_err_t ble_gatts_init(void)
{
    
    esp_err_t ret = esp_ble_gatts_register_callback(gatts_event_handler);
    if (ret){
        ESP_LOGE(GATTS_TABLE_TAG, "gatts register error, error code = %x", ret);
        return ret;
    }

    ret = esp_ble_gap_register_callback(gap_event_handler);
    if (ret){
        ESP_LOGE(GATTS_TABLE_TAG, "gap register error, error code = %x", ret);
        return ret;
    }

    ret = esp_ble_gatt_set_local_mtu(500);
    if (ret){
        ESP_LOGE(GATTS_TABLE_TAG, "set local  MTU failed, error code = %x", ret);
    }
    return ret;
}

esp_err_t gatts_update_attrs(uint16_t app_id) {

    gatts_profile_inst * prof = &profile_tab[app_id];
    gatts_char_inst * car;
    //uint8_t * uuid_ptr;
    //uint16_t uuid;
    gatts_svc_inst svc;
    for (int i = 0; i < prof->num_svcs; i++) {
        svc = prof->svcs[i];
        car = svc.chars;
        for (int j = 0; j < svc.num_chars; j++) {
            ESP_LOGI(GATTS_TABLE_TAG, "char #%d, len=%d, data_ptr=%d", (int)j, (int)car[j].len, (int)car[j].data);
            esp_ble_gatts_set_attr_value(
                car[j].attr_handle, 
                car[j].len, 
                car[j].data
            );

            bool conf_req = car[j].ccc == 0x0002 ? true : false;
            if ((car[j].props & ESP_GATT_CHAR_PROP_BIT_NOTIFY) && car[j].ccc) {
                esp_ble_gatts_send_indicate(
                    prof->gatts_if, 
                    prof->conn_id,
                    svc.handle,
                    car[j].len,
                    car[j].data,
                    conf_req
                );
            }

        }
    }

    return ESP_OK;
}

#define TEMP_BUF_LEN 30
esp_err_t ble_gatts_profile_init(ble_gatt_profile_req_t * reqs, uint16_t num_profs)
{
    esp_err_t ret;

    ble_gatt_profile_req_t * prof_req = reqs;
    ble_gatt_service_req_t * svc_req;
    ble_gatt_char_req_t * car_req;

    //gatts_profile_inst * prof;
    gatts_svc_inst * svc;
    gatts_char_inst * car;

    //esp_gatts_attr_db_t * attr;
    uint16_t attr_cnt;

    ble_gatts_init();

    uint16_t handle_cnt;
    for (uint16_t i = 0; i < num_profs; i++) {
        handle_cnt = 0;
        prof_req = &reqs[i];
        for (uint16_t j = 0; j < prof_req->num_services; j++) {
            svc_req = &prof_req->services[j];
            handle_cnt++;
            for (uint16_t k = 0; k < svc_req->num_chars; k++) {
                car_req = &svc_req->chars[k];
                handle_cnt += 2;
                if (car_req->notify) handle_cnt++;
            }
        }

        prof_init = (esp_gatts_attr_db_t *)malloc(handle_cnt*sizeof(esp_gatts_attr_db_t));
        num_handles = (uint16_t *)malloc(prof_req->num_services*sizeof(uint16_t));

        attr_cnt = 0;

        profile_tab[i].num_svcs = prof_req->num_services;
        profile_tab[i].svcs = (gatts_svc_inst *) malloc(sizeof(gatts_svc_inst) * prof_req->num_services);
        svc = profile_tab[i].svcs;
        if (svc == NULL)  {
            ESP_LOGE(GATTS_TABLE_TAG, "No memory for service");
            return ESP_ERR_NO_MEM;
        }
        for (uint8_t j = 0; j < prof_req->num_services; j++) {
            handle_cnt = 0;
            svc = &profile_tab[i].svcs[j];
            svc->uuid.is_primary = prof_req->services;
            svc->uuid.id.inst_id = 0;
            svc->uuid.id.uuid.len = ESP_UUID_LEN_128;
            svc->num_chars = prof_req->services[j].num_chars;
            memcpy (svc->uuid.id.uuid.uuid.uuid128, prof_req->services[j].uuid, ESP_UUID_LEN_128);
            svc->chars = (gatts_char_inst *) malloc(svc->num_chars*sizeof(gatts_char_inst));
            car = svc->chars;
            if (car == NULL) {
                ESP_LOGE(GATTS_TABLE_TAG, "No memory for %d characteristics", (int)svc[j].num_chars);
                return ESP_ERR_NO_MEM;
            }

            prof_init[attr_cnt].attr_control.auto_rsp = ESP_GATT_AUTO_RSP;
            prof_init[attr_cnt].att_desc.length = ESP_UUID_LEN_128;
            prof_init[attr_cnt].att_desc.max_length = ESP_UUID_LEN_128;
            prof_init[attr_cnt].att_desc.perm = ESP_GATT_PERM_READ;
            prof_init[attr_cnt].att_desc.uuid_p = prof_req->services[j].is_primary ? (uint8_t*)&primary_service_uuid : (uint8_t*)&secondary_service_uuid;
            prof_init[attr_cnt].att_desc.uuid_length = ESP_UUID_LEN_16;
            prof_init[attr_cnt].att_desc.value = prof_req->services[j].uuid;
            handle_cnt++; attr_cnt++;

            for (uint8_t k = 0; k < svc[j].num_chars; k++) {
                car_req = &prof_req->services[j].chars[k];
                car = &svc->chars[k];
                car->ccc = 0xFFFF;
                car->data = car_req->data;
                car->len = car_req->max_len;
                car->uuid.uuid.uuid16 = car_req->uuid;
                car->uuid.len = ESP_UUID_LEN_16;
                car->perms = 0;
                car->props = 0;
                if (car_req->readable) {
                    car->perms |= ESP_GATT_PERM_READ;
                    car->props |= ESP_GATT_CHAR_PROP_BIT_READ;
                }
                if (car_req->writable) {
                    car->perms |= ESP_GATT_PERM_WRITE;
                    car->props |= ESP_GATT_CHAR_PROP_BIT_WRITE;
                }
                if (car_req->notify) {
                    car->props |= ESP_GATT_CHAR_PROP_BIT_NOTIFY;
                }

                // Characteristic Declaration
                prof_init[attr_cnt].attr_control.auto_rsp = ESP_GATT_AUTO_RSP;
                prof_init[attr_cnt].att_desc.value = &car->props;
                prof_init[attr_cnt].att_desc.length = sizeof(uint8_t);
                prof_init[attr_cnt].att_desc.max_length = sizeof(uint8_t);
                prof_init[attr_cnt].att_desc.perm = ESP_GATT_PERM_READ;
                prof_init[attr_cnt].att_desc.uuid_p = (uint8_t*)&character_declaration_uuid;
                prof_init[attr_cnt].att_desc.uuid_length = ESP_UUID_LEN_16;
                handle_cnt++; attr_cnt++;

                // Characteristic Value
                prof_init[attr_cnt].attr_control.auto_rsp = ESP_GATT_AUTO_RSP;
                prof_init[attr_cnt].att_desc.value = car_req->data;
                prof_init[attr_cnt].att_desc.length = car_req->max_len;
                prof_init[attr_cnt].att_desc.max_length = car_req->max_len;
                prof_init[attr_cnt].att_desc.perm = car->perms;
                prof_init[attr_cnt].att_desc.uuid_p = (uint8_t*)&car->uuid.uuid.uuid16;
                prof_init[attr_cnt].att_desc.uuid_length = ESP_UUID_LEN_16;
                handle_cnt++; attr_cnt++;

                // Client Characteristic Configuration 
                if (car_req->notify) {
                    prof_init[attr_cnt].attr_control.auto_rsp = ESP_GATT_AUTO_RSP;
                    prof_init[attr_cnt].att_desc.value = (uint8_t*)&car->ccc;
                    prof_init[attr_cnt].att_desc.length = sizeof(uint16_t);
                    prof_init[attr_cnt].att_desc.max_length = sizeof(uint16_t);
                    prof_init[attr_cnt].att_desc.perm = ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE;
                    prof_init[attr_cnt].att_desc.uuid_p = (uint8_t*)&character_client_config_uuid;
                    prof_init[attr_cnt].att_desc.uuid_length = ESP_UUID_LEN_16;
                    handle_cnt++; attr_cnt++;
                }
            }
            num_handles[j] = handle_cnt;
        }

        num_profiles++;
        num_services_initialized = 0;
        ret = esp_ble_gatts_app_register(i);
        if (ret) {
            return ret;
        }
        while(num_services_initialized < profile_tab[i].num_svcs);
        free(num_handles);
        free(prof_init);
    }

    return ESP_OK;
}
