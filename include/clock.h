#include <stdint.h>
#include "esp_system.h"

typedef struct {
    uint8_t hour;
    uint8_t minute;
} custom_clock_t;

esp_err_t clock_init();
void clock_set_time(uint8_t hour, uint8_t minute);
custom_clock_t clock_get_time();


