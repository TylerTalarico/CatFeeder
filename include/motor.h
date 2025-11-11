typedef enum {
    MOTOR_DIR_FWD,
    MOTOR_DIR_REV
} motor_dir_t;

typedef struct {
    gpio_num_t sel0;
    gpio_num_t sel1;
    gpio_num_t en;
    bool running;
} motor_handle_t;

void motor_run(motor_handle_t *motor, motor_dir_t dir);
esp_err_t motor_init(motor_handle_t *motor, gpio_num_t sel0, gpio_num_t sel1, gpio_num_t en);