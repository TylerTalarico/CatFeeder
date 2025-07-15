#include "ble.h"
typedef enum {
    PROFILE_DEFAULT,
    PROFILE_NUM
} gatt_profile_name_t;

static ble_gatt_profile_t gl_profile_tab[PROFILE_NUM] = {
    [PROFILE_DEFAULT] = {
        .gatts_cb = gatts_default_profile_event_handler,
        .gatts_if = ESP_GATT_IF_NONE,
    },
};

static void gatts_default_profile_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param) {
    switch (event) {
        case ESP_GATTS_REG_EVT:
        {
            gl_profile_tab[PROFILE_DEFAULT].services[0].service_id.is_primary = true;
            gl_profile_tab[PROFILE_DEFAULT].services[0].service_id.is_primary = true;

        }
        default:
    }
}

