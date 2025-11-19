#include "sensor_weight.h"
#include "driver/adc.h"
#include "esp_adc_cal.h"

#define DEFAULT_VREF 1100

esp_err_t sensor_weight_init(sensor_weight_handle_t *hsw, adc2_channel_t channel) {
    
    esp_err_t err;

    err = adc2_config_channel_atten(channel, ADC_ATTEN_DB_12);
    if (err) return err;

    esp_adc_cal_characteristics_t *adc_chars = calloc(1, sizeof(esp_adc_cal_characteristics_t));
    esp_adc_cal_value_t val_type = esp_adc_cal_characterize(ADC_UNIT_2, ADC_ATTEN_DB_12, ADC_WIDTH_BIT_12, DEFAULT_VREF, adc_chars);

    hsw->channel = channel;

    return ESP_OK;
}

float sensor_weight_get_reading(sensor_weight_handle_t *hsw) {

    int raw;
    adc2_get_raw(hsw->channel, ADC_WIDTH_BIT_12, &raw);
    return (float)raw;

}