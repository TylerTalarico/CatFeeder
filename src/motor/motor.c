#include "driver/gpio.h"
#include "driver/gptimer.h"
#include "time.h"
#include "led.h"
#include "motor.h"

#define MS_TO_CNTS(ms) (ms*10000)

extern led_t blue_led;
gptimer_handle_t on_timer;
volatile bool running = false;

esp_err_t motor_init(motor_handle_t *motor, gpio_num_t sel0, gpio_num_t sel1, gpio_num_t en) {
    
    esp_err_t err;

    if ((err = gpio_set_direction(sel0, GPIO_MODE_OUTPUT)) != ESP_OK) return err;
    if ((err = gpio_set_direction(sel1, GPIO_MODE_OUTPUT)) != ESP_OK) return err;
    if ((err = gpio_set_direction(en, GPIO_MODE_OUTPUT)) != ESP_OK) return err;
    if ((err = gpio_set_level(sel0, 0)) != ESP_OK) return err;
    if ((err = gpio_set_level(sel1, 0)) != ESP_OK) return err;
    if ((err = gpio_set_level(sel1, 0)) != ESP_OK) return err;

    motor->sel0 = sel0;
    motor->sel1 = sel1;
    motor->en = en;
    
    return ESP_OK;
}


void motor_run(motor_handle_t *motor, motor_dir_t dir) {

    if (dir == MOTOR_DIR_FWD) {
        gpio_set_level(motor->sel0, 1);
        gpio_set_level(motor->sel1, 0);
    } else {
        gpio_set_level(motor->sel0, 0);
        gpio_set_level(motor->sel1, 1);
    }
    gpio_set_level(motor->en, 1);
    motor->running = true;

}

void motor_stop(motor_handle_t *motor) {
    gpio_set_level(motor->en, 0);
    gpio_set_level(motor->sel0, 0);
    gpio_set_level(motor->sel1, 0);
}