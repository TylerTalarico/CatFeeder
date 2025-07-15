#include "driver/gpio.h"
#include "led.h"

led_t blue_led;

void app_main() {
    blue_led.pin = GPIO_NUM_2;
    led_init(&blue_led);

    while (1) {
        for (int i = 0; i < 10000000; i++);
        led_toggle(&blue_led);
    }
    
}