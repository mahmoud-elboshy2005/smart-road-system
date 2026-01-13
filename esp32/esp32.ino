
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "socket_io_manager.h"
#include "traffic_light.h"
#include "motor.h"
#include "fsm.h"


// Server (laptop) IP and Port number
// IPAddress serverIP(192, 168, 137, 1);
// IPAddress serverIP(192, 168, 1, 4);
IPAddress serverIP(10, 42, 0, 1);
uint16_t serverPort = 5000;
uint16_t updPort    = 3000;

uint32_t cars_count = 0;
bool has_ambulance = false;

void setup() {
  Serial.begin(115200);


  fsm_init(STATE_IDLE);

  fsm_register_transition(STATE_IDLE, STATE_PROCESSING, EVENT_START, NULL);

  xTaskCreatePinnedToCore(motor_task, "Motor Task", 2048, NULL, 5, NULL, 1);
  xTaskCreatePinnedToCore(traffic_light_task, "Traffic 1 Task", 2048, NULL, 5, NULL, 1);
  xTaskCreatePinnedToCore(socket_io_task, "Socket IO Task", 4096, NULL, 20, NULL, 1);
}

void loop() {
  return;
}

void emergency_stop(void) {
  // Stop motor
  motor_stop();

  // Turn off all traffic lights
  traffic_light_turn_off_all();
}