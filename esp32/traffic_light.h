#ifndef _TRAFFIC_LIGHT_H_
#define _TRAFFIC_LIGHT_H_

#include <stdint.h>

// Traffic light colors
typedef enum {
  RED = 0,
  YELLOW = 1,
  GREEN = 2
} traffic_light_color_t;

// Traffic light identifiers
typedef enum {
  TRAFFIC_LIGHT_1 = 0,
  TRAFFIC_LIGHT_2 = 1
} traffic_light_id_t;

// Main task function
void traffic_light_task(void *pvParams);

// Manual control functions
void traffic_light_set(traffic_light_id_t id, traffic_light_color_t color);
void traffic_light_set_duration(traffic_light_id_t id, uint32_t duration_ms);
void traffic_light_turn_off_all();

#endif //_TRAFFIC_LIGHT_H_