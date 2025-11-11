typedef enum {
    MOTOR_DIR_FWD,
    MOTOR_DIR_REV
} motor_dir_t;

void run_motor(motor_dir_t dir, uint64_t time_ms);
esp_err_t motor_init();