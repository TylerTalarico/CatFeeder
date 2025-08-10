#include "esp_gatts_api.h"

#define BLE_DEVICE_NAME "Pawto Feeder"

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
