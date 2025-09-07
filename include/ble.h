#include "esp_gatts_api.h"
#include "ble_gatt_profiles.h"

#define BLE_DEVICE_NAME "Pawto Feeder"

esp_err_t ble_init(ble_gatt_profile_req_t * profs, uint16_t num_profs);
