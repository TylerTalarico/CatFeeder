#include "esp_gatts_api.h"

#define NUM_SERVICES 3
#define NUM_CHARACTERISTICS 2

typedef struct {
    uint16_t char_handle;
    esp_bt_uuid_t char_uuid;
    uint16_t descr_handle;
    esp_bt_uuid_t descr_uuid;
    esp_gatt_perm_t perm;
    esp_gatt_char_prop_t property;
} ble_gatt_charac_t;

typedef struct {
    uint16_t service_handle;
    esp_gatt_srvc_id_t service_id;
    ble_gatt_charac_t chars[NUM_CHARACTERISTICS];
} ble_gatt_service_t;

typedef struct {
    esp_gatts_cb_t gatts_cb;
    uint16_t gatts_if;
    uint16_t app_id;
    uint16_t conn_id;
    ble_gatt_service_t services[NUM_SERVICES];

} ble_gatt_profile_t;

typedef struct {
    bool set_scan_rsp;            /*!< Set this advertising data as scan response or not*/
    bool include_name;            /*!< Advertising data include device name or not */
    bool include_txpower;         /*!< Advertising data include TX power */
    int min_interval;             /*!< Advertising data show slave preferred connection min interval */
    int max_interval;             /*!< Advertising data show slave preferred connection max interval */
    int appearance;               /*!< External appearance of device */
    uint16_t manufacturer_len;    /*!< Manufacturer data length */
    uint8_t *p_manufacturer_data; /*!< Manufacturer data point */
    uint16_t service_data_len;    /*!< Service data length */
    uint8_t *p_service_data;      /*!< Service data point */
    uint16_t service_uuid_len;    /*!< Service uuid length */
    uint8_t *p_service_uuid;      /*!< Service uuid array point */
    uint8_t flag;                 /*!< Advertising flag of discovery mode, see BLE_ADV_DATA_FLAG detail */
} esp_ble_adv_data_t;