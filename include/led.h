#include "driver/gpio.h"

typedef struct {
    gpio_num_t pin;
    uint32_t state;
} led_t;

uint32_t led_init(led_t * led);
uint32_t led_set_state(led_t * led, uint32_t level);
uint32_t led_toggle(led_t * led);
