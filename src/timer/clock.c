#include "driver/gptimer.h"
#include "esp_log.h"
#include "clock.h"

#define TAG_CLOCK "CLOCK"

static custom_clock_t gClock = {0};
static gptimer_handle_t ghTim;

static bool increment_clock(gptimer_handle_t timer, const gptimer_alarm_event_data_t *edata, void *user_ctx) {
    if (gClock.minute == 59) {
        gClock.minute = 0;
        if (gClock.hour == 23) {
            gClock.hour = 0;
        } else {
            gClock.hour++;
        }
    } else {
        gClock.minute++;
    }
    return false;
}

gptimer_event_callbacks_t clock_cbs = {
    .on_alarm = increment_clock // Call the user callback function when the alarm event occurs
};


esp_err_t clock_init() {

    esp_err_t err;

    gptimer_config_t gptim_cfg = {
        .clk_src = GPTIMER_CLK_SRC_DEFAULT,
        .direction = GPTIMER_COUNT_UP,
        .intr_priority = 0,
        .resolution_hz = 10000,
        .flags = {1,0,0}
    };

    gptimer_alarm_config_t alarm_config = {
        .alarm_count = 600000,  // Period of 1 minute
        .flags.auto_reload_on_alarm = true // Enable auto-reload function
    };
    ESP_LOGI(TAG_CLOCK, "Starting clock initialization");
    if ((err = gptimer_new_timer(&gptim_cfg, &ghTim)) != ESP_OK) {
        ESP_LOGI(TAG_CLOCK, "Failed to initialize timer");
        return err;
    }
    if ((err = gptimer_set_alarm_action(ghTim, &alarm_config)) != ESP_OK) {
        ESP_LOGI(TAG_CLOCK, "Failed to set clock alarm action");
        return err;
    }
    if ((err = gptimer_register_event_callbacks(ghTim, &clock_cbs, NULL)) != ESP_OK) {
        ESP_LOGI(TAG_CLOCK, "Failed to register clock alarm callback");
        return err;
    }
    ESP_LOGI(TAG_CLOCK, "Clock timer configured");
    gptimer_set_raw_count(ghTim, 0);
    if ((err = gptimer_enable(ghTim)) != ESP_OK) {
        ESP_LOGI(TAG_CLOCK, "Failed to enable clock timer");
        return err;
    }
    ESP_LOGI(TAG_CLOCK, "Clock timer enabled");
    if ((err = gptimer_start(ghTim)) != ESP_OK) {
        ESP_LOGI(TAG_CLOCK, "Failed to start clock timer");
        return err;
    }
    ESP_LOGI(TAG_CLOCK, "Clock initialized");
    return ESP_OK;

}
void clock_set_time(uint8_t hour, uint8_t minute) {
    gClock.hour = hour;
    gClock.minute = minute;
}

custom_clock_t clock_get_time() {
    return gClock;
}