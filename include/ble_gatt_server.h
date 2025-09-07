#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"
#include "ble_gatt_profiles.h"


esp_err_t ble_gatts_profile_init(ble_gatt_profile_req_t * reqs, uint16_t num_profs);
esp_err_t gatts_update_attrs(uint16_t app_id);