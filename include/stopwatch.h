#include "driver/gptimer.h"
typedef struct {
    gptimer_handle_t htim;
    bool done;
} stopwatch_handle_t;

esp_err_t stopwatch_init(stopwatch_handle_t * sw);
esp_err_t stopwatch_reset_start(stopwatch_handle_t * sw, uint64_t time_ms);