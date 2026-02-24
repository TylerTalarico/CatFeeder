#include "hx711.h"

esp_err_t hx711_init(hx711_t * handle, gpio_num_t clk, gpio_num_t data) {
    
    esp_err_t err = ESP_OK;
    
    if ((err = gpio_set_direction(clk, GPIO_MODE_OUTPUT)) != ESP_OK) return err;
    if ((err = gpio_set_direction(data, GPIO_MODE_INPUT)) != ESP_OK) return err;
    if ((err = gpio_set_level(clk, 0)) != ESP_OK) return err;

    handle->clk = clk;
    handle->data = data;

    return err;
}

esp_err_t hx711_get_value(hx711_t * handle, int32_t * val) {
    if (gpio_get_level(handle->data) != 0) {
        return ESP_ERR_NOT_ALLOWED;
    }

    int32_t data = 0;
    uint16_t delay = 0;

    for (uint8_t i = 0; i < 24; i++) {
        gpio_set_level(handle->clk, 1);
        for (delay = 0; delay < HX711_CLK_DELAY; delay++); // wait
        
        data <<= 1;
        data |= gpio_get_level(handle->data) & 0x01;

        gpio_set_level(handle->clk, 0);
        for (delay = 0; delay < HX711_CLK_DELAY; delay++); // wait
    }

    // Re-init serial comms
    gpio_set_level(handle->clk, 1);
    for (delay = 0; delay < HX711_CLK_DELAY; delay++); // wait
    gpio_set_level(handle->clk, 0);

    // get int with proper sign
    data <<= 8;
    data >>= 8;
    *val = data;
    return ESP_OK;
}