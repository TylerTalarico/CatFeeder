#include <string.h>
#include "esp_log.h"
#include "ble_gatt_profiles.h"
#include "gatt_data.h"

#define PROFILES_TAG "GATT_PROFILES"

typedef enum {
    PROF_MAIN,
    NUM_PROFILE
} prof_idx_t;

typedef enum {
    SVC_SCHEDULER,
    MAIN_NUM_SVC
} main_svc_idx_t;

typedef enum {
    CHAR_ALARMS,
    CHAR_WEIGHTS,
    CHAR_EVENTS,
    SCHED_NUM_CHARS
} scheduler_char_idx_t;

esp_err_t ble_profile_req(ble_gatt_profile_req_t ** prof, uint16_t * num_profile) {

    ble_gatt_char_req_t * car;
    ble_gatt_service_req_t * svc;
    ble_gatt_profile_req_t * reqs = (ble_gatt_profile_req_t *) malloc(NUM_PROFILE*sizeof(ble_gatt_profile_req_t));
    ESP_LOGI(PROFILES_TAG, "Requests allocated");
    ble_gatt_profile_req_t * req = reqs;
    if (!req) return ESP_ERR_NO_MEM;

    req = &req[PROF_MAIN];
    req->services = (ble_gatt_service_req_t *) malloc(MAIN_NUM_SVC*sizeof(ble_gatt_service_req_t));
    ESP_LOGI(PROFILES_TAG, "Services allocated");
    if (!req->services) return ESP_ERR_NO_MEM;
    req->num_services = MAIN_NUM_SVC;

    svc = &req->services[SVC_SCHEDULER];
    svc->is_primary = true;
    uint8_t uuid_16[] = {0x40, 0x47, 0x6c, 0xd0, 0x89, 0x95, 0x4d, 0x38, 0xb0, 0xbf, 0x80, 0x87, 0x67, 0xb7, 0xb7, 0x0f};
    memcpy(svc->uuid, uuid_16, sizeof(uuid_16));
    svc->num_chars = SCHED_NUM_CHARS;
    svc->chars = malloc(SCHED_NUM_CHARS*sizeof(ble_gatt_char_req_t));
    if (!svc->chars) return ESP_ERR_NO_MEM;
    ESP_LOGI(PROFILES_TAG, "Characteristics allocated");

    car = &svc->chars[CHAR_ALARMS];
    car->uuid = 0xFF01;
    car->data = (uint8_t *)gattd_dispense_amounts;
    car->max_len = sizeof(gattd_dispense_amounts);
    car->readable = true; car->writable = true; car->notify = false;
    ESP_LOGI(PROFILES_TAG, "Char 1 init");

    car = &svc->chars[CHAR_WEIGHTS];
    car->uuid = 0xFF02;
    car->data = (uint8_t *)gattd_bowl_weight;
    car->max_len = sizeof(gattd_bowl_weight);
    car->readable = true; car->writable = false; car->notify = true;
    ESP_LOGI(PROFILES_TAG, "Char 2 init");

    car = &svc->chars[CHAR_EVENTS];
    car->uuid = 0xFF03;
    car->data = (uint8_t *)gattd_events;
    car->max_len = sizeof(gattd_events);
    car->readable = true; car->writable = false; car->notify = false;
    ESP_LOGI(PROFILES_TAG, "Char 3 init");

    *prof = req;
    *num_profile = NUM_PROFILE;
    ESP_LOGI(PROFILES_TAG, "Params set");

    return ESP_OK;
}