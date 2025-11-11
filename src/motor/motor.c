#include "driver/gpio.h"
#include "driver/gptimer.h"
#include "time.h"
#include "led.h"
#include "motor.h"

#define MS_TO_CNTS(ms) (ms*10000)

extern led_t blue_led;
gptimer_handle_t on_timer;
volatile bool running = false;

static bool alarm_cb_stop_motor(gptimer_handle_t timer, const gptimer_alarm_event_data_t *edata, void *user_ctx) {
    
    gptimer_stop(timer);
    gptimer_disable(on_timer);
    
    led_set_state(&blue_led, 0);
    gpio_set_level(GPIO_NUM_16, 0);
    gpio_set_level(GPIO_NUM_17, 0);
    running = false;

    return false;
}

gptimer_event_callbacks_t cbs = {
    .on_alarm = alarm_cb_stop_motor // Call the user callback function when the alarm event occurs
};

esp_err_t motor_timer_init() {

    gptimer_config_t gptim_cfg;
    gptim_cfg.clk_src = GPTIMER_CLK_SRC_DEFAULT;
    gptim_cfg.direction = GPTIMER_COUNT_UP;
    gptim_cfg.intr_priority = 0;
    gptim_cfg.resolution_hz = 1000000;

    return gptimer_new_timer(&gptim_cfg, &on_timer);
}

esp_err_t motor_init() {
    
    if (motor_timer_init() != ESP_OK) {
        return ESP_ERR_INVALID_STATE;
    }
    if (gpio_set_direction(GPIO_NUM_16, GPIO_MODE_OUTPUT) != ESP_OK) {
        return ESP_ERR_INVALID_STATE;
    }
    if (gpio_set_direction(GPIO_NUM_17, GPIO_MODE_OUTPUT) != ESP_OK) {
        return ESP_ERR_INVALID_STATE;
    }
    
    return 0;
}


void run_motor(motor_dir_t dir, uint64_t time_ms) {
    
    uint64_t cnt;
    gptimer_alarm_config_t alarm_config = {
        .alarm_count = time_ms*1000, // Set the actual alarm period, since the resolution is 1us, 1000000 represents 1s
        .flags.auto_reload_on_alarm = false // Disable auto-reload function
    };
    gptimer_set_alarm_action(on_timer, &alarm_config);
    gptimer_register_event_callbacks(on_timer, &cbs, NULL);

    if (dir == MOTOR_DIR_FWD) {
        gpio_set_level(GPIO_NUM_16, 0);
        gpio_set_level(GPIO_NUM_17, 1);
    } else {
        gpio_set_level(GPIO_NUM_16, 1);
        gpio_set_level(GPIO_NUM_17, 0);
    }

    gptimer_set_raw_count(on_timer, 0);
    gptimer_enable(on_timer);
    gptimer_start(on_timer);
    led_set_state(&blue_led, 1);
    running = true;
    while (running);

}