#include <string.h>
#include <stdbool.h>
#include "esp_err.h"

#ifndef __BLE_GATT_PROFILES_H__
#define __BLE_GATT_PROFILES_H__

typedef struct {
    uint8_t * data;
    uint16_t max_len;
    uint16_t uuid;
    bool writable;
    bool readable;
    bool notify;
} ble_gatt_char_req_t;

typedef struct {
    uint8_t uuid[16];
    bool is_primary;
    ble_gatt_char_req_t * chars;
    uint16_t num_chars;
} ble_gatt_service_req_t;

typedef struct {
    ble_gatt_service_req_t * services;
    uint16_t num_services;
} ble_gatt_profile_req_t;

esp_err_t ble_profile_req(ble_gatt_profile_req_t ** prof, uint16_t * num_profile);

#endif