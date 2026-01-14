#ifndef _PIN_CONFIG_H_
#define _PIN_CONFIG_H_

#define MOTOR_PIN_1  18
#define MOTOR_PIN_2  19

#define TRAFFIC_1_RED     21
#define TRAFFIC_1_YELLOW  22
#define TRAFFIC_1_GREEN   23

#define TRAFFIC_2_RED     25
#define TRAFFIC_2_YELLOW  26
#define TRAFFIC_2_GREEN   27

#define PUMP_DURATION_MS        2700 // 2.7 seconds

#define STATUS_UPDATE_INTERVAL_MS   2000

#define GREEN_DURATION_MS         30000 // 30 seconds
#define YELLOW_DURATION_MS        5000  // 5 seconds
#define RED_DURATION_MS          30000 // 30 seconds


#define MIN_GREEN_DURATION_MS   30000 // 30 seconds
#define EXTRA_TIME_PER_CAR_MS   3000  // 3 seconds per detected car
#define CAR_COUNT_THRESHOLD     9    // Max number of cars to consider for extra time

#define SPEED_THRESHOLD         80   // (~80 km/h)

#define TOTAL_GREEN_TIME_MS(_CAR_COUNT) (MIN_GREEN_DURATION_MS + (EXTRA_TIME_PER_CAR_MS * (_CAR_COUNT - CAR_COUNT_THRESHOLD))) 

#endif //_PIN_CONFIG_H_