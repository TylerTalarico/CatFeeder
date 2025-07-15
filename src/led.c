#include "led.h"

uint32_t led_init(led_t * led) {
    if (gpio_set_direction(led->pin, GPIO_MODE_OUTPUT) != ESP_OK) {
        return 1;
    }
    if (gpio_set_drive_capability(led->pin, GPIO_DRIVE_CAP_MAX) != ESP_OK) {
        return 1;
    }
    if (led_set_state(led, 0)) {
        return 1;
    }
    return 0;
}

uint32_t led_set_state(led_t * led, uint32_t level) {
    if (gpio_set_level(led->pin, level) != ESP_OK) {
        return 1;
    }
    led->state = level;
    return 0;
}

uint32_t led_toggle(led_t * led) {
    if (gpio_set_level(led->pin, !led->state) != ESP_OK) {
        return 1;
    }
    led->state = !led->state;
    return 0;
}

