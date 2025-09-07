#include <string.h>

typedef struct {
    uint8_t hour;
    uint8_t minute;
    uint16_t max_weight;
} feed_alarm_t;

extern feed_alarm_t gattd_dispense_amounts[6];
extern uint16_t gattd_bowl_weight[2];
extern uint8_t gattd_events[256];