#include "driver/gpio.h"
#include "driver/adc.h"
#include "esp_adc/adc_oneshot.h"

typedef struct {
    adc2_channel_t channel;
} sensor_weight_handle_t;

esp_err_t sensor_weight_init(sensor_weight_handle_t *hsw, adc2_channel_t channel);
float sensor_weight_get_reading(sensor_weight_handle_t *hsw);



