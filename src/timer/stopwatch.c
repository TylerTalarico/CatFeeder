#include "stopwatch.h"
#include "led.h"

const gptimer_config_t gptim_cfg = {
    .clk_src = GPTIMER_CLK_SRC_DEFAULT,
    .direction = GPTIMER_COUNT_DOWN,
    .intr_priority = 0,
    .resolution_hz = 1000000,
    .flags = {1,0,0}
};

static bool alarm_cb_stop_timer(gptimer_handle_t timer, const gptimer_alarm_event_data_t *edata, void *user_ctx) {
    
    gptimer_stop(timer);
    gptimer_disable(timer);
    
    bool *done_flag = (bool *)user_ctx;
    *done_flag = true;

    return false;
}

gptimer_event_callbacks_t cbs = {
    .on_alarm = alarm_cb_stop_timer // Call the user callback function when the alarm event occurs
};

esp_err_t stopwatch_init(stopwatch_handle_t * sw) {
    return gptimer_new_timer(&gptim_cfg, &sw->htim);
}
esp_err_t stopwatch_reset_start(stopwatch_handle_t * sw, uint64_t time_ms) {
    
    gptimer_alarm_config_t alarm_config = {
        .alarm_count = 0, // Set the actual alarm period, since the resolution is 1us, 1000000 represents 1s
        .flags.auto_reload_on_alarm = false // Disable auto-reload function
    };
    gptimer_set_alarm_action(sw->htim, &alarm_config);
    gptimer_register_event_callbacks(sw->htim, &cbs, &sw->done);

    gptimer_set_raw_count(sw->htim, time_ms*1000);
    gptimer_enable(sw->htim);
    gptimer_start(sw->htim);
    sw->done = false;

    return ESP_OK;
}

esp_err_t stopwatch_stop(stopwatch_handle_t * sw) {
    
    esp_err_t err;
    if ((err = gptimer_stop(sw->htim))) return err;
    if ((err = gptimer_disable(sw->htim))) return err;
    sw->done = true;
    return ESP_OK;
}