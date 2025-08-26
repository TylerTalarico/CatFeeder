#include "driver/gpio.h"
#include "led.h"
#include "ble.h"
#include "gatt_data.h"

led_t blue_led;

void app_main() {
    blue_led.pin = GPIO_NUM_2;
    led_init(&blue_led);
    ble_init();

    while (1) 
    {
        if (gattd_dispense_amounts[0] > 10)
        {
            led_set_state(&blue_led, 1);
        } else {
            led_set_state(&blue_led, 0);
        }

        for (int i = 0; i < 100000000; i++);
        led_toggle(&blue_led);
    }
}