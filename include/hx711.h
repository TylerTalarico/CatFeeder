#include "driver/gpio.h"

#define HX711_CLK_DELAY 500

typedef struct  {
    gpio_num_t clk;
    gpio_num_t data;
} hx711_t;

esp_err_t hx711_get_value(hx711_t * handle, int32_t * val);
esp_err_t hx711_init(hx711_t * handle, gpio_num_t clk, gpio_num_t data);